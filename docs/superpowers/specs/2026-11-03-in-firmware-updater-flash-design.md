# Flash over the in-firmware updater — design

Date: 2026-11-03. Status: draft for owner approval.

## Intent

Flashing the Svalboard today means double-clicking the RESET button under a
half to get into BOOTSEL, and for the left half, swapping the USB cable over.
Svalboard's upstream firmware (`svalboard/qmk`, branch `svalboard`) now has an
in-firmware updater: a running half takes new firmware over raw HID, and the
half with USB relays the image to the other half over the split cable. The
owner wants `qmk-flash` to use it, so a flash is one command with the keyboard
left as it is: no reset button, no cable swap.

These builds are for the owner alone. They never go to anyone else, so
upstream's dev-only conveniences are fine to use.

Upstream references, all in `svalboard/qmk@svalboard`: `docs/updater.md`,
`keyboards/svalboard/rules.mk` (the flags), `keyboards/svalboard/tools/`
(`sval_update.py`, `make_update.py`), and `util/sval_dev_flash.sh`. That last
one is upstream's own dev loop (build, `make_update.py --diagnostic`,
`sval_update.py pair L.svup R.svup --yes`) and the closest prior art.

## Goals

1. Every firmware build in this repo, both local `qmk userspace-compile` and
   CI, is an updater build with no approval chord:
   `SVAL_UPDATER=yes SVAL_UPDATE_NO_CHORD=yes`.
2. Each build also produces one `.svup` per half next to its `.uf2`. CI
   attaches the `.svup` files to the GitHub release alongside the `.uf2` files.
3. `qmk-flash` installs both halves through the updater with no prompts and no
   keyboard interaction. It keeps what it does today: it resolves the newest
   build, reports how old it is, validates the files before touching the
   board, supports `--host` fetching, and sends notifications.
4. `qmk-flash` can flash a GitHub release directly (`--release [TAG]`,
   default the latest) on a Mac with no build of its own. Owner condition: only
   if it adds no friction to the normal workflow. So **no pinning**: CI keeps
   building against the tip of `svalboard/qmk@svalboard`. It attaches the
   `sval_update.py`/`make_update.py` from that same checkout to the release
   and records the upstream commit in the release notes. Upstream bumps then
   need no extra step, and the scripts always match the firmware they flash.
   *Builder's choice:* how to download (`gh release download` or the
   `releases/latest/download/` URLs the overlay archive already uses).
5. `qmk-flash` falls back to its current BOOTSEL + picotool path when the board
   isn't running an updater build. That covers the one-time migration flash and
   recovery after an interrupted update, when a half comes up as `RPI-RP2`.

## Non-goals

- Keybard. The owner never uses it.
- Signing, `SVAL_UPDATE_RELEASE` builds, upstream's pair-UF2 release format.
- Keeping stored settings across flashes. See the anti-goals.

## Anti-goals (failures even if the flash "works")

- **A flash that doesn't apply the keymap from the code.** The owner edits the
  keymap in code and never in Keybard. The first-boot settings reset (upstream
  `build_reset.c`, D39) is what makes a new build boot with its compiled
  keymap, combos, overrides and so on. So `SVAL_DEV_KEEP_SETTINGS` must NOT be
  set. The owner decided this explicitly.
- **A flash that leaves the halves on different builds and reports success.**
  Updater builds of different commits can't talk over the split link (upstream
  "one release on both halves (V)"), so a half-done update leaves a keyboard
  that types only on one side. `qmk-flash` must report it loudly and say how to
  finish (rerun, or the BOOTSEL fallback).
- Any manual setup step on a Mac (installing Python packages and so on) beyond
  what chezmoi already provisions.

## Decided

- **Chord off, settings reset kept.** Owner decision, see above.
- **Always both halves.** This follows from upstream's constraint, not from a
  preference. Each updater flash installs both halves (`sval_update.py pair`).
  Today's habit of flashing only the right half for a keymap change goes away.
  *My call:* the default changes from `right` to `both`, on the updater and the
  BOOTSEL fallback alike. A single half over BOOTSEL is still allowed when named
  explicitly, because that's the normal way to recover a half stuck as
  `RPI-RP2`. It warns that the half must end up on the same build as the other
  one. Over the updater, a named single half is refused with an explanation.
- **"Same build" means the same image identity, not just the same upstream
  release.** The firmware's own presence MATCH compares only `fw_version`, the
  upstream QMK git hash and the protocol, so it passes for two different builds
  of this repo. *My call:* both `.svup` files must carry the same
  `SVAL_FW_VERSION_STRING` before the flash starts. After `pair`, `qmk-flash`
  reads the version string back from both halves (INFO page 1, and
  `info --other` if it exposes it; otherwise the USB half plus `pair`'s MATCH).
  The string must change on every build, dirty trees included. Local builds
  use `<sha7>-<HHMMSS>` or similar, at most 16 characters. CI builds use the
  release version (`v1.0.N`).
- **Keyboard targets stay as they are.** These are
  `svalboard/trackball/pmw3389/{left,right}` with keymap `townk`, all still
  present upstream. Upstream's "auto" any-module images and the `either` target
  are not adopted.
- **Upstream bump is part of this work.** The local `qmk_home` and CI both
  build from `svalboard/qmk@svalboard`. The local checkout sits at `eddc56ca`
  (2026-10-21), 111 commits behind and older than the updater releases (latest
  upstream: fw_version 18, v4.3.5). Bring the userspace up to current upstream
  first, keeping the existing gotchas in mind: re-copy `sval.json` verbatim,
  and recheck the `SV_SAFE_RANGE` collision, `MAC_DIVISOR`, and the RGB idle
  dim.

## Constraints

- The flags have to reach both build paths automatically: `qmk.json`
  `build_targets` entries accept an `env` dict (`lib/python/qmk/userspace.py`),
  and CI runs `qmk userspace-compile`. One source of truth; no second command
  to remember.
- `sval_update.py` needs `hidapi`. On Linux it needs the hidraw backend, but
  `qmk-flash` only runs on macOS today.
- `sval_update.py` imports `make_update.py` from its own directory. They
  belong to the firmware they talk to, so take them from the same upstream
  commit the firmware was built from, not from a separately pinned copy.
- A raw-HID client must not be open while flashing, because replies go to
  every open handle. The owner doesn't run Keybard, but **svalwatch is a
  raw-HID client that is always running**. Hammerspoon starts it on every Mac
  (`dot_config/hammerspoon/modules/svalboard/init.lua`; it restarts the task 5 s
  after it exits and gives up if it keeps dying). It opens the same interface
  (usage page 0xFF61, usage 0x62) non-exclusively. hidapi on macOS opens
  devices exclusively by default, so `sval_update.py` may fail to open the
  device, or take it over. *My call:* `qmk-flash` pauses svalwatch for the
  updater flash and always resumes it afterwards, failures included (a trap).
  It goes through Hammerspoon, not a bare kill, which the restart logic would
  undo or turn into "keeps dying". First check on-device whether the two can
  share the device; if they can, skip the pause.
- `.svup` files from a non-release build should be made with `--diagnostic`,
  as upstream's dev script does. A board on a release build then refuses them,
  which is the correct behaviour.
- `qmk-flash` lives in chezmoi
  (`~/.local/share/chezmoi/home/dot_local/bin/executable_qmk-flash`). Edit the
  source, then `chezmoi apply`, per `~/AGENTS.md`. It uses
  `~/.local/lib/common.zsh` helpers (spinners, `log_*`, `notify`, `poll::until`)
  and the existing config idiom (`config.toml` via yq+jq).
- Owner priorities: no polling loops left running, and no background cost.
  A one-shot wait with a timeout, as `qmk-flash` already does, is fine.

## The flow, as intended

1. Resolve and validate, before anything touches the board: the newest
   matching `.svup` for each half (locally, or fetched with `--host`). Report
   its age. Check it offline with `make_update.py verify`. Check the two halves
   come from the same build.
2. Detect whether the board answers as an updater build. *Builder's choice:*
   `sval_update.py info`/`list` is the obvious probe.
3. If it does: `sval_update.py pair LEFT.svup RIGHT.svup --yes`, which updates
   the other half first, then the USB half, then waits for both to report the
   same version. Show progress behind the existing spinner and stream helpers.
   Notify on success or failure.
4. If it doesn't, or it shows up as `RPI-RP2`: today's BOOTSEL + picotool path
   with the `.uf2` files, unchanged.

## Left to the builder

- Where the `.svup` step runs locally: a Makefile target after
  `qmk userspace-compile`, a userspace post-build hook, or similar. It must run
  for a plain `make` and in CI.
- How `qmk-flash` gets `sval_update.py`/`make_update.py` and `hidapi`. My call,
  open to change: run them through `uv` (already on the owner's Macs via mise)
  with `--with hidapi`. Take the scripts from `qmk_home` locally. For the
  `--host` path, fetch them from the build host the same way the firmware is
  fetched, so a Mac with no QMK checkout still works. For `--release`, take them
  from the release assets (goal 4).
- `.svup` naming. Keep the `_<half>_` / `.<half>.` patterns the current
  resolvers glob on.
- The exact local `SVAL_FW_VERSION_STRING` format, within the rules in
  "Decided". Upstream's `SVAL_FW_VERSION` number comes from its
  `fw_version.txt`, so leave that alone.
- Exact fallback trigger and messages.

## Migration (one time, on-device)

The board runs v1.0.34, a non-updater build. The first updater build has to
go on through BOOTSEL on both halves. `qmk-flash both` already does that
through the fallback. Every flash after that should go over the updater. To
verify on-device:

- a keymap change in code, built and flashed with no keyboard interaction,
  shows up on both halves;
- `qmk-flash` with one half's `.svup` missing or mismatched refuses before
  touching the board;
- a board forced into BOOTSEL still flashes through the fallback;
- `qmk-flash --release` installs the latest GitHub release over the updater;
- the overlay (svalwatch) is back and reading the board after a flash,
  including after a failed one.
