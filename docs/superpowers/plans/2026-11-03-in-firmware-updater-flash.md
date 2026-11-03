# Flash over the in-firmware updater: implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `qmk-flash` installs both Svalboard halves over the one plugged-in
cable through upstream's in-firmware updater, with no reset button, no cable
swap and no chord. BOOTSEL + picotool stays as the fallback.

**Architecture:** Every build of this userspace becomes a no-chord updater
build. Each build also writes one `.svup` per half next to the `.uf2` files,
plus copies of the upstream `sval_update.py`/`make_update.py` from the same
checkout. CI attaches all of these to releases. `qmk-flash` (in chezmoi)
works out where those files come from (this machine, `--host`, or
`--release`). It checks that both halves come from one build, pauses the
overlay through Hammerspoon, runs `sval_update.py pair`, and reads the
version back.

**Tech Stack:** QMK userspace (`qmk userspace-compile`), Python 3.9 stdlib
(unittest), GitHub Actions, zsh + shellspec, Hammerspoon Lua, `uv`
(+ `hidapi`).

**Spec:** `docs/superpowers/specs/2026-11-03-in-firmware-updater-flash-design.md`

**Repos:**
- **QC** = this repo, `~/Projects/platforms/keyboards/qmk-config`. Commits
  use a gitmoji subject and a prose body, with no trailers.
- **QH** = `qmk_home`, `~/Projects/platforms/keyboards/qmk`, on branch
  `svalboard`. Never commit here.
- **CZ** = chezmoi source, `~/.local/share/chezmoi`. Commits use
  `type(scope): summary`, with no trailers. Edit the source only, then
  `chezmoi apply <target>`, per `~/AGENTS.md`.

## Global Constraints

- Build flags on every build, local and CI: `SVAL_UPDATER=yes SVAL_UPDATE_NO_CHORD=yes`.
- Never set `SVAL_DEV_KEEP_SETTINGS` or `SVAL_UPDATE_RELEASE`. The first-boot settings reset must stay.
- `SVAL_FW_VERSION_STRING`: at most 16 characters from `A-Z a-z 0-9 . _ + -`. Locally it's `<sha7>-<HHMMSS>` (local time, so it changes every build, dirty trees included). In CI it's `v<VERSION>` (e.g. `v1.0.35`).
- Leave `SVAL_FW_VERSION` (the number) to upstream's `fw_version.txt`.
- `.svup` files are made with `make_update.py … --diagnostic --yes`.
- `sval_update.py` and `make_update.py` always sit in the same directory as each other. They come from the same upstream checkout the firmware was built from.
- Python on the Mac runs as `uv run --no-project --quiet --with hidapi python -I`. Nothing gets installed by hand.
- Keyboard targets stay `svalboard/trackball/pmw3389/{left,right}`, keymap `townk`.
- CI keeps building against the tip of `svalboard/qmk@svalboard`, with no pinning. The release notes record the upstream SHA.
- No polling loops left running. Every wait is one-shot and bounded.
- Hardware tests never touch a real board. picotool, diskutil, uv, gh and hs are always stubbed.

## Review Focus

1. **Board on non-updater firmware, unplugged, or already in BOOTSEL.** The `info` probe fails, and `qmk-flash` takes the BOOTSEL path with today's reset prompt. It must not report "updater failed". (Task 5, `falls back to BOOTSEL when the probe fails`.)
2. **A stale `.svup` for one half.** For example, only the right half was rebuilt. `qmk-flash` refuses before it probes or pauses anything. (Task 5, `refuses halves from different builds`.)
3. **`pair` fails partway through** (cable pulled, hash error). `qmk-flash` says the halves may now differ, how to finish (rerun; BOOTSEL if a half shows `RPI-RP2`), and exits non-zero. (Task 5, `reports a failed pair loudly`.)
4. **Ctrl-C or a failure while the overlay is paused.** The overlay is always resumed. Hammerspoon's auto-resume backs that up even if `qmk-flash` is killed. (Task 4, `resumes on its own after maxSecs`; Task 5, `resumes the overlay when pair fails`.)
5. **Hammerspoon not running, or `hs` missing.** The flash still goes ahead, and the pause and resume calls can't hang. (Task 5, `flashes when hs is unavailable`.)

---

### Task 1: Bring the userspace up to current upstream (QH + QC)

The local QH sits at `eddc56ca`, 111 commits behind `svalboard/svalboard`
and older than the updater. CI already builds the tip.

**Files:**
- Modify (QC), as the bump needs: `keyboards/svalboard/keymaps/townk/sval.json` (a verbatim copy of QH `keyboards/svalboard/keymaps/sval/sval.json`), and anything under `users/townk/` or the keymap that no longer compiles.

- [ ] **Step 1:** In QH: `git fetch svalboard && git checkout svalboard && git merge --ff-only svalboard/svalboard && git submodule update --init --recursive`. Expected: HEAD equals `svalboard/svalboard` (currently `3fc078a618`).
- [ ] **Step 2:** In QC: `cp <QH>/keyboards/svalboard/keymaps/sval/sval.json keyboards/svalboard/keymaps/townk/sval.json`, then `make`. Expected: both `svalboard_trackball_pmw3389_{left,right}_townk.uf2` build with no warnings. Fix compile errors in the userspace, not in QH.
- [ ] **Step 3:** Recheck the known collisions against the new upstream: `SV_SAFE_RANGE` versus `SV_RIGHT_DPI_INC` (userspace keys must stay at `QK_KB_20+`), and whether `MAC_DIVISOR` is still overridden to 120. The upstream RGB idle dim stays inert on rev-A boards. Write down what changed in the commit body.
- [ ] **Step 4:** `python3 tests/run_tests.py`. Expected: all tests OK.
- [ ] **Step 5:** Commit in QC: `⬆️ Build against current Svalboard upstream` (a body listing the upstream SHA and any fixes).

### Task 2: Every build is a no-chord updater build with `.svup` files (QC)

**Files:**
- Modify: `qmk.json`: each `build_targets` entry becomes `{"keyboard": "...", "keymap": "townk", "env": {"SVAL_UPDATER": "yes", "SVAL_UPDATE_NO_CHORD": "yes"}}`.
- Modify: `Makefile` `all:` target.
- Create: `tools/updater/package_updates.py`.
- Create: `tests/test_package_updates.py`.
- Modify: `.gitignore`: add `*.svup`, `/sval_update.py`, `/make_update.py`.

**Interfaces:**
- Produces, for Tasks 3 and 5, in the repo root after `make`: `svalboard_trackball_pmw3389_left_townk.svup`, `…_right_townk.svup`, `sval_update.py`, `make_update.py`.
- Produces: `make FW_VERSION_STRING=<s>` overrides the computed local string (CI uses this).
- `package_updates.py` CLI:
  - `package_updates.py version`: prints `<sha7>-<HHMMSS>`.
  - `package_updates.py package [--qmk-home DIR] --dir DIR`: for each `*_{left,right}_townk.uf2` in `--dir`, runs `DIR/keyboards/svalboard/tools/make_update.py UF2 -o <stem>.svup --kb svalboard/trackball/pmw3389/<half> --keymap townk --diagnostic --yes`, then `make_update.py verify <stem>.svup`. It copies both tools into `--dir`. It exits non-zero if either half is missing or any step fails.
- Python functions: `version_string(sha: str, now: datetime.datetime) -> str`, `qmk_home_from_config(output: str) -> pathlib.Path` (parses `qmk config user.qmk_home`, which prints `user.qmk_home=/path (config)`), `uf2_targets(dir: Path) -> dict[str, Path]` (`{"left": …, "right": …}`).

- [ ] **Step 1: Write the failing tests** in `tests/test_package_updates.py` (unittest, importing the module through `sys.path` from `tools/updater`):
  - `test_version_string_format`: `version_string("abcdef1234", datetime(2026,11,3,15,4,5)) == "abcdef1-150405"`, and `len(...) <= 16`.
  - `test_version_string_charset`: the result matches `^[A-Za-z0-9._+-]{1,16}$`.
  - `test_qmk_home_strips_config_suffix`: `qmk_home_from_config("user.qmk_home=/x/qmk (config)\n") == Path("/x/qmk")`.
  - `test_uf2_targets_finds_both_halves`: a temporary directory with both `.uf2` files maps `left` and `right` to them.
  - `test_uf2_targets_refuses_missing_half`: only the left file present raises `SystemExit` with a message naming `right`.
- [ ] **Step 2:** `python3 tests/run_tests.py`. Expected: the 5 new tests FAIL (module not found).
- [ ] **Step 3:** Implement `tools/updater/package_updates.py` (Python 3.9, stdlib only, `subprocess.run(check=True)` with `sys.executable -I`).
- [ ] **Step 4:** `python3 tests/run_tests.py`. Expected: all tests pass.
- [ ] **Step 5:** Wire up the `Makefile` `all:` target: `FW_VERSION_STRING ?= $(shell python3 -I tools/updater/package_updates.py version)`. Then `qmk userspace-compile -e SVAL_FW_VERSION_STRING=$(FW_VERSION_STRING)`, then `python3 -I tools/updater/package_updates.py package --dir .`. `--qmk-home` is optional and defaults to `qmk_home_from_config(<output of qmk config user.qmk_home>)`, so the Makefile doesn't parse it. Update `qmk.json` as above. Update the comment at the top of the Makefile to say `make` (not a bare `qmk userspace-compile`) is the build, because it also packages the updates.
- [ ] **Step 6: Verify:** `make`. Then, from the repo root, `python3 -I make_update.py verify svalboard_trackball_pmw3389_left_townk.svup`, and the same for right. Expected: both pass. Then `python3 -I -c 'import sys; sys.path.insert(0,"."); import make_update as m; d=open("svalboard_trackball_pmw3389_left_townk.svup","rb").read(); print(m.manifest_version(m.parse_manifest(m.parse_svup(d)[0])))'`. Expected: the version string from `make`.
- [ ] **Step 7:** Commit in QC: `✨ Build every firmware as a no-chord updater build with .svup files`.

### Task 3: CI attaches the updates to releases (QC)

**Files:**
- Modify: `.github/workflows/build_binaries.yaml`.

**Interfaces:**
- Produces, for Task 6, release assets: `svalboard-colemak-dh-townk-<V>.{left,right}.uf2`, `svalboard-colemak-dh-townk-<V>.{left,right}.svup`, `sval_update.py`, `make_update.py`. The release body gets a line `**Svalboard QMK:** <sha>`.

- [ ] **Step 1:** In the `build` job, replace `qmk userspace-compile` with `make FW_VERSION_STRING="v${{ steps.version.outputs.version }}"`. Record the upstream SHA with `git -C svalboard-qmk rev-parse HEAD` and expose it as a job output, `qmk_sha`.
- [ ] **Step 2:** Rename the `.svup` files the same way the `.uf2` files are renamed (`….left.svup`, `….right.svup`). Add `*.svup`, `sval_update.py` and `make_update.py` to the uploaded artifact paths.
- [ ] **Step 3:** In `publish`, add `**Svalboard QMK:** ${{ needs.build.outputs.qmk_sha }}` to the release body. Add one line saying the `.svup` files and the two scripts are what `qmk-flash` uses to update over USB.
- [ ] **Step 4: Verify:** push the feature branch (branches never publish) and check the run with `gh run watch`. Expected: green, and `gh run download <id> -n svalboard-colemak-townk` lists 2 `.uf2`, 2 `.svup`, `sval_update.py` and `make_update.py`.
- [ ] **Step 5:** Commit in QC: `👷 Attach the updater files to releases`.

### Task 4: The overlay can pause and resume (CZ)

**Files:**
- Modify: `home/dot_config/hammerspoon/modules/svalboard/init.lua`.
- Create: `tests/hammerspoon_svalboard_spec.sh`.

**Interfaces:**
- Produces, for Task 5: `M.pause(maxSecs: number)` stops the overlay task without changing the saved mode, and schedules an automatic `M.resume()` after `maxSecs`. Pausing again while paused only resets the timer. `M.resume()` does nothing unless paused. Otherwise it clears the pause, cancels the timer, resets `quickExits`, and calls `start()` (which is itself a no-op when mode is `"off"`). `M.setMode` clears any pause. `start()` does nothing while paused, so the crash-restart path can't restart it during a pause.

- [ ] **Step 1: Write the failing spec**, headless under `nvim --headless -u NONE -l` like `tests/hammerspoon_jobs_classify_spec.sh`. Use a stub `hs` table: `hs.task.new` returns an object that records `start` and `terminate`; `hs.timer.doAfter` records `(secs, fn)` and returns an object with `stop`; and stubs for `hs.settings`, `hs.fs.attributes` (always truthy), `hs.logger.new` and `hs.timer.secondsSinceEpoch`. Examples:
  - `pause stops a running overlay`: after `setup()`, `pause(300)` terminates the task, and a doAfter of 300 is pending.
  - `resume restarts it`: after `pause(300)`, `resume()` starts a new task and stops the pending timer.
  - `resumes on its own after maxSecs`: after `pause(300)`, firing the recorded doAfter function starts a new task.
  - `resume does nothing when not paused`: `resume()` after `setup()` starts no second task.
  - `stays off when the mode is off`: with mode `"off"`, `pause(300)` followed by `resume()` starts no task.
  - `a crash during pause does not restart it`: the task's exit callback firing while paused schedules no restart.
- [ ] **Step 2:** `make test-one SPEC=tests/hammerspoon_svalboard_spec.sh`. Expected: FAIL (`pause` is nil).
- [ ] **Step 3:** Implement `M.pause` and `M.resume` as specified. Add one comment-header paragraph saying they exist for `qmk-flash`.
- [ ] **Step 4:** Run the same command. Expected: PASS.
- [ ] **Step 5:** `chezmoi apply ~/.config/hammerspoon`. Then `hs -c 'hs.reload()'`, `hs -c 'require("svalboard").pause(30)'` (the overlay vanishes), and `hs -c 'require("svalboard").resume()'` (it comes back).
- [ ] **Step 6:** Commit in CZ: `feat(hammerspoon): let qmk-flash pause the svalboard overlay`.

### Task 5: `qmk-flash` flashes over the updater (CZ)

**Files:**
- Modify: `home/dot_local/bin/executable_qmk-flash`.
- Modify: `tests/qmk-flash_spec.sh`.
- Modify: `home/dot_config/qmk-flash/config.toml.example` (wording only: the timeout also bounds the updater wait).

**Interfaces:**
- Consumes: Task 2's file names. Task 4's `pause(maxSecs)` / `resume()`, called as `${HS:-hs} -t 3 -c 'require("svalboard").pause(300)'`. A non-zero exit or a missing `hs` gives a `log_warn` and nothing else.
- New script globals and functions:
  - `UVPY=(uv run --no-project --quiet --with hidapi python -I)`.
  - `resolve_updates`: sets `UP_DIR` (the one directory holding both `.svup` files and both scripts), `UP_LEFT`, `UP_RIGHT`, `UP_VERSION`. Locally, `UP_DIR` is the firmware dir. With `--host`, the newest `*[_.]{left,right}[_.]*.svup` plus `sval_update.py` and `make_update.py` are rsynced into one scratch directory.
  - `svup_version <file>`: prints the manifest version string, using `$UP_DIR/make_update.py` through `$UVPY -c` with `sys.path.insert(0, UP_DIR)`.
  - `updater_info`: runs `$UVPY $UP_DIR/sval_update.py info`. It succeeds, setting `INFO_JSON`, only when that exits 0 and the JSON has `protocol >= 4`.
  - `flash_updater`: pause, then `spin::streamed "updating both halves…" $UVPY $UP_DIR/sval_update.py pair $UP_LEFT $UP_RIGHT --yes`, then always resume (a `TRAPEXIT`/`always` block). Then it runs `updater_info` again and checks the `version` equals `UP_VERSION`.
- The flow: resolve the updates, check that both halves' version strings are equal, then probe. If the probe succeeds, `flash_updater`. If not, `log_info "the board is not on updater firmware (or not reachable): using BOOTSEL"`, then today's path, resolving the `.uf2` files as today.
- Halves: the default becomes `both`. Over the updater, `left` or `right` dies with `the updater always installs both halves (both must run the same build); use BOOTSEL for one half: --bootsel <half>`. A new flag, `--bootsel`, forces the BOOTSEL path. With a single half, it warns `the other half must end up on this same build, or the halves cannot talk`.
- `require_cmd uv` only on the updater path. Today's `require_cmd picotool diskutil xxd` only on the BOOTSEL path.
- `--help` mentions that a raw-HID app such as Keybard must be closed.

- [ ] **Step 1: Write the failing specs** in `tests/qmk-flash_spec.sh`. Add stubs for `uv` and `hs`, both appending to `$CALLS`. The `uv` stub dispatches on its arguments: a `-c` call prints `$FAKE_VERSION_LEFT` or `$FAKE_VERSION_RIGHT`, depending on whether its arguments name the left or the right `.svup` (both default to `abc1234-120000`); `info` prints `$FAKE_INFO` (default `{"protocol": 4, "version": "abc1234-120000"}`) and exits `$FAKE_INFO_RC` (default 0); `pair` exits `$FAKE_PAIR_RC` (default 0). A helper `svup <half>` writes `svalboard_trackball_pmw3389_<half>_townk.svup` plus empty `sval_update.py`/`make_update.py` into `$FW`. Set `HS="$STUBS/hs"` for these examples. Examples:
  - `pairs both halves over the updater`: the status is success; `$CALLS` has `pair` with both `.svup` paths and `--yes`, and no `picotool`.
  - `defaults to both halves`: with no half argument, `pair` runs.
  - `refuses halves from different builds`: the left version is `abc1234-120000` and the right is `abc1234-130000`. The status is failure, stderr names both versions, and `$CALLS` has no `info`, no `hs` and no `pair`.
  - `falls back to BOOTSEL when the probe fails`: `FAKE_INFO_RC=1` with `uf2 left/right` present and `--timeout 1`. The output includes `using BOOTSEL` and the reset prompt, and there's no `pair`.
  - `refuses a single half over the updater`: `qmk-flash right` fails, with stderr including `--bootsel`.
  - `--bootsel right skips the updater`: there's no `uv … info` in `$CALLS`, and the stderr warning includes `same build`.
  - `reports a failed pair loudly`: `FAKE_PAIR_RC=1`. The status is failure, and stderr includes `may now run different builds` and `rerun`.
  - `resumes the overlay when pair fails`: with the same setup, `$CALLS` has an `hs` call containing `resume()` after the `pair` call.
  - `flashes when hs is unavailable`: `HS=/nonexistent`. `pair` still runs and the status is success.
  - `fails when the version does not read back`: the second `info` returns version `zzz`. The status is failure, and stderr names both versions.
- [ ] **Step 2:** `make test-one SPEC=tests/qmk-flash_spec.sh`. Expected: the new examples FAIL. The old examples that relied on `right` as the default still pass, or they're updated to pass `--bootsel right` explicitly. Update them in this step.
- [ ] **Step 3:** Implement as specified. Keep the narration style the existing script uses (`log_info` / `log_ok`, notifications on success and failure, a comment for every non-obvious choice).
- [ ] **Step 4:** Run the same command. Expected: all pass. Then `make lint` in CZ. Expected: clean.
- [ ] **Step 5:** `chezmoi apply ~/.local/bin/qmk-flash ~/.config/qmk-flash`.
- [ ] **Step 6:** Commit in CZ: `feat(qmk-flash): flash both halves over the in-firmware updater`.

### Task 6: `qmk-flash --release [TAG]` (CZ)

**Files:**
- Modify: `home/dot_local/bin/executable_qmk-flash`, `tests/qmk-flash_spec.sh`.

**Interfaces:**
- Consumes: Task 3's asset names. Task 5's `resolve_updates` contract (`UP_DIR`, `UP_LEFT`, `UP_RIGHT`, `UP_VERSION`).
- `--release` (latest) or `--release TAG`. A value that doesn't start with `-` is taken as the tag. It runs `gh release download [TAG] -R Townk/qmk-config -p '*.svup' -p '*.uf2' -p 'sval_update.py' -p 'make_update.py' -D <scratch dir>`. It then fills the same globals as `resolve_updates`, with the `.uf2` files there for the BOOTSEL fallback. `--release` together with `--host` dies with `--release and --host are exclusive`. `require_cmd gh` only with `--release`.

- [ ] **Step 1: Write the failing specs.** Add a `gh` stub that records its arguments and writes release-named fixtures (`svalboard-colemak-dh-townk-1.0.35.left.svup`, etc.) into the `-D` directory.
  - `--release downloads the latest release and pairs it`: the `gh release download -R Townk/qmk-config` call has no tag, and `pair` gets the downloaded `.left.svup`/`.right.svup`.
  - `--release v1.0.35 downloads that tag`.
  - `--release with --host is refused`.
- [ ] **Step 2:** `make test-one SPEC=tests/qmk-flash_spec.sh`. Expected: the new examples FAIL.
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** Run the same command. Expected: all pass. Then `make lint`.
- [ ] **Step 5:** `chezmoi apply ~/.local/bin/qmk-flash`. Commit in CZ: `feat(qmk-flash): flash a GitHub release with --release`.

### Task 7: The migration flash and on-device verification (owner present)

This needs the owner at the keyboard. Each check's result goes into the
final report as passed or failed, with its output.

- [ ] **Step 1:** QC `make`, then `qmk-flash` while the board runs v1.0.34. Expected: the probe fails, the BOOTSEL fallback runs right and then left, as today: the last manual flash.
- [ ] **Step 2:** Check whether svalwatch and `sval_update.py` can share the device. With the overlay running, `uv run --no-project --with hidapi python -I sval_update.py info` exits 0. If it does, the pause can be dropped later (a note, not a change now).
- [ ] **Step 3:** Change one key in the keymap code, run `make` and then `qmk-flash`. Expected: no prompts or keyboard interaction, a rainbow on both halves, then green. The new key works on both halves, and the overlay comes back afterwards.
- [ ] **Step 4:** Rebuild only after a change, keep a stale left `.svup`, and run `qmk-flash`. Expected: it refuses before touching the board.
- [ ] **Step 5:** Double-tap reset on the right half and run `qmk-flash --bootsel right`. Expected: it flashes through picotool. Then `qmk-flash` brings both halves back onto the updater path.
- [ ] **Step 6:** After the QC branch is merged to main and CI publishes, `qmk-flash --release`. Expected: it installs the release over the updater, and the version reads back as `v1.0.N`.
- [ ] **Step 7:** Update QC `README.md`'s flashing section (updater by default, `--bootsel` for recovery, `--release`). Commit: `📝 Document flashing over the updater`.
