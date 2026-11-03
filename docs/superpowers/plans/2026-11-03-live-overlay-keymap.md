# Live overlay keymap: implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** When a Svalboard connects, the overlay reads the keymap from the
keyboard and shows it. Whatever is flashed is what the overlay draws, on any
Mac, with no build-time or flash-time install of the overlay page.

**Architecture:** On connect, svalwatch's `Session` reads the dynamic keymap
over the existing wrapped-VIA channel and emits it as a `WatchEvent.keymap`.
svaloverlay hashes it and looks for a cached page. On a miss, it runs a
bundled Python renderer through `uv` with skim. The renderer turns the numeric
keycodes back into the keymap's own spellings, so skim's config labels still
apply. It then renders the layers with skim, exactly as today, and writes a
page into the cache. The overlay loads that page. The page that ships with the
overlay is a fallback until a render finishes, and it covers a Mac that can't
render (for example, offline on the very first connect).

**Tech Stack:** Swift 6 (SvalWatchCore / SvalWatchDevice / svaloverlay),
Python 3.9-compatible renderer, skim `qmk-skim==0.7.5` via `uv`, GitHub
Actions, Hammerspoon (unchanged).

**Spec:** none written. The owner skipped design questions ("just start, we
will do more work on the overlay later, but now I just want it to update as
the keyboard connects"). The owner's principle: "whenever I change my keymap
and flash it, everything that displays the keymap must be updated to not have
a drift". Ideal stated: "our overlay is built only when itself changes, and it
updates itself on keyboard connect". The look is out of scope; it must stay as
it is today.

## Global Constraints

- The look doesn't change. Rendering goes through the same skim version (`qmk-skim==0.7.5`), the same merged config (`skim-config.yaml` + `overlay/skim-overlay.yaml`) and the same `inline()` and manifest logic as `overlay/build_assets.py` today.
- The keymap is read on connect only, so every reconnect (including the reboot after a flash) reads it again. Never poll for it.
- Render only on a cache miss. The cache key is the SHA-256 of (bundle version + the numeric keymap bytes). Pages live in `~/Library/Caches/svaloverlay/pages/<hash>/`, keeping the 5 most recent. Never write into `~/.local/share/svaloverlay`, which is a chezmoi external with `exact = true`.
- At most one render at a time, with a 120 s timeout. On failure, keep showing the current page and log one line on stderr, which Hammerspoon's log captures.
- The renderer runs as `uv run --no-project --quiet --with qmk-skim==0.7.5 python -I <bundle>/render_page.py …`. svaloverlay finds `uv` on `PATH`, then `~/.local/share/mise/shims/uv`, then `/opt/homebrew/bin/uv`. Hammerspoon starts it with a minimal `PATH`.
- Keycode spelling, in order: (1) the exact-spelling table built at overlay build time; (2) a generic QMK decoder from the bundled keycode spec (basic names, `MO`/`TG`/`TO`/`DF`/`OSL`/`TT`/`LT`/`MT`/`OSM`/mod-wrapped `LSFT(...)` and the like); (3) `0x%04X`. An unknown keycode never fails a render.
- No hardware writes. Reading uses only VIA `0x11` (layer count) and `0x12` (keymap buffer), wrapped as svalwatch already wraps VIA (`SvalProtocol` `viaProtocol = 0xFE`).
- Commit styles: QC uses a gitmoji subject and a prose body; CZ uses `type(scope): summary`. Neither has trailers. The QC pre-commit hook stages keymap SVGs; that's expected.

## Review Focus

1. **The keyboard is unplugged mid-read, or a reply is malformed or late.** The session fails like any other read and reconnects; no half-filled keymap is emitted. (Task 1: `drops a partial keymap on timeout`.)
2. **The first connect with no network, or `uv` missing.** The shipped page stays up and the overlay keeps working (layer, keys, pointer). One log line, no retry loop. (Task 4: `keeps the current page when the render fails`.)
3. **A keymap with a keycode the build-time table never saw** (for example, a new `LT(3,KC_A)`). It renders with the generic spelling. (Task 3: `spells an unseen layer-tap generically`.)
4. **The keyboard reconnects with the same keymap.** No render runs and the cached page loads at once. (Task 4: `reuses a cached page`.)
5. **A page swap happens while keys are held or a layer is active.** The new page shows the current layer, held keys and pointer state straight away. (Task 4: `replays state after a page swap`.)

---

### Task 1: The session reads the keymap on connect (Swift, SvalWatchCore)

**Files:**
- Modify: `tools/svalwatch/Sources/SvalWatchCore/SvalProtocol.swift`, `Session.swift`, `WatchEvent.swift`.
- Test: `tools/svalwatch/Tests/SvalWatchCoreTests/SessionTests.swift` (follow its style).

**Interfaces:**
- Produces: `WatchEvent.keymap(Keymap)`, where `public struct Keymap: Equatable, Sendable { public let layers: Int; public let rows: Int; public let cols: Int; public let keycodes: [UInt16] }` holds `[layer][row][col]` in matrix order, row-major, with `rows`/`cols` from `SvalProtocol.matrixRows`/`matrixCols` (10 and 6). It's emitted once per connect, after info and before the first layer read. The `svalwatch` JSON-lines output prints it as `{"event":"keymap","layers":N,"keycodes":[…]}` (`WatchEvent`'s existing encoding style).
- Packets: `SvalProtocol.keymapLayerCountRequest(client:)` sends VIA `0x11`. `SvalProtocol.keymapBufferRequest(client:offset:size:)` sends VIA `0x12` with a big-endian `UInt16` offset and a one-byte size. The size is the largest that fits a wrapped 64-byte report: read the comment at `modules/svalboard/core/client_wrapper.c:60` in `qmk_home` (`~/Projects/platforms/keyboards/qmk`) on how wrapped VIA replies lose the wrapper header, and derive it. Replies are parsed in `SvalProtocol.parse` into new cases.
- Session phases: insert `.keymapCount` and `.keymapBuffer(offset)` between `.info`/subscription and `.layer`. Keycodes are big-endian (VIA `dynamic_keymap_get_buffer`).

- [ ] **Step 1: Write the failing tests** in `SessionTests.swift`:
  - `reads the keymap after info`: drive bootstrap → info → (subscription if announced) and expect a `.send` of the layer-count request. Feed a reply of 2 layers, then expect buffer requests covering `2*10*6*2 = 240` bytes in order. Feed replies, then expect `.emit(.keymap(...))` with `layers == 2` and 120 keycodes decoded big-endian, followed by the first layer read.
  - `drops a partial keymap on timeout`: a timeout mid-buffer emits no `.keymap` and follows the existing failure path (`fail(at:)`).
  - `ignores another client's buffer reply`: a reply carrying a different client id doesn't advance the read.
  - Packet byte tests for both requests, in `SvalProtocol`'s existing test style.
- [ ] **Step 2:** `cd tools/svalwatch && make test`. Expected: the new tests FAIL.
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** `make test`. Expected: all pass.
- [ ] **Step 5:** With the keyboard plugged in, run `swift run svalwatch` for a few seconds (read-only; it is the existing watcher). Expected: one `keymap` line with `layers` equal to the firmware's dynamic layer count, and keycodes that look right (layer 0 has non-zero entries). Record the line's first 20 keycodes in the report.
- [ ] **Step 6:** Commit in QC: `✨ Read the keymap from the keyboard on connect`.

### Task 2: The render bundle and an exact-spelling table (Python, build time)

**Files:**
- Create: `tools/svalwatch/overlay/page_build.py`: the shared page code, moved out of `build_assets.py` (`merge`, `skim_config`, `layer_images`, `inline`, `manifest`, page assembly).
- Create: `tools/svalwatch/overlay/spelling.py` (build time): it builds `spelling.json`.
- Modify: `tools/svalwatch/overlay/build_assets.py`: it keeps building the default page as today, and it also writes the bundle to `<out>/render/`.
- Modify: `tools/svalwatch/Makefile` (`overlay-assets` needs a built firmware ELF; a clear error if it's missing).
- Test: `tests/test_overlay_spelling.py` (QC unittest, picked up by `python3 tests/run_tests.py`).

**Interfaces:**
- Produces, for Tasks 3 and 4, the bundle `<assets>/render/`:
  - `render_page.py` (Task 3 writes it; this task copies it in);
  - `page_build.py`, `page.html`, `page.css`, `page.js` and `icons.ttf` (if present);
  - `skim-config.yaml` (already merged);
  - `manifest.json` (the `manifest()` output: layer names plus the layout's matrix positions);
  - `spelling.json`: `{"<value>": "<keymap.c spelling>"}`, built by pairing `qmk c2json --no-cpp` (layout order, the spelling skim's config is keyed on) with the numeric `keymaps` array read out of the built firmware ELF (`<qmk_home>/.build/svalboard_trackball_pmw3389_right_townk.elf`, which `qmk userspace-compile` leaves in qmk_home, not this repo, symbol `keymaps`, little-endian `uint16`, `[layer][row][col]`). The two are paired through `manifest.json`'s matrix positions. A value that appears with two different spellings keeps the first, plus a warning line.
  - `keycodes.json`: QMK's keycode spec for the `qmk_home` version (`qmk.keycodes.load_spec`, or the `data/constants/keycodes` files it reads), as `{"keycodes": {"0xNNNN": {"key": "KC_A", …}}, "ranges": {…}}`, unchanged from the spec format.
  - `VERSION`: the SHA-256 of every other bundle file, so a rebuilt bundle invalidates cached pages.
- Function: `spelling.pair(layout_spellings: list[list[str]], elf_keymaps: list[list[list[int]]], matrix: list[list[int]]) -> dict[int, str]`.
- Read the ELF with the ARM toolchain that's already installed (`arm-none-eabi-nm` / `arm-none-eabi-objcopy`). Don't add a Python dependency.

- [ ] **Step 1: Write the failing tests:**
  - `test_pair_matches_positions`: on a 2-layer, 2-key fixture with a matrix that swaps positions, `pair` maps each value to the spelling at its matrix position.
  - `test_pair_keeps_first_duplicate`: the same value spelled two ways keeps the first.
  - `test_pair_rejects_shape_mismatch`: a different number of layers or keys raises `SystemExit` naming both counts.
- [ ] **Step 2:** `python3 tests/run_tests.py`. Expected: the new tests FAIL.
- [ ] **Step 3:** Implement `spelling.py` and the `page_build.py` refactor, and wire the bundle into `build_assets.py`.
- [ ] **Step 4:** `python3 tests/run_tests.py`. Expected: all pass.
- [ ] **Step 5: Verify:** `make` in QC, then `make -C tools/svalwatch overlay-assets ASSETS=<scratch dir>`. Expected: `<scratch>/index.html` is byte-identical to a build of the parent commit's `build_assets.py` with the same inputs (the default page hasn't changed), and `<scratch>/render/` holds every bundle file. `spelling.json` maps `KC_A`'s value `0x0004` to `"KC_A"` (or that key's spelling in `keymap.c`), and each custom `CKC_*` value to its name.
- [ ] **Step 6:** Commit in QC: `✨ Ship a render bundle with the overlay page`.

### Task 3: The runtime renderer (Python)

**Files:**
- Create: `tools/svalwatch/overlay/render_page.py`.
- Test: `tests/test_overlay_render.py`.

**Interfaces:**
- Consumes: Task 2's bundle layout. Task 1's numeric keymap, as a JSON file `{"layers": N, "rows": 10, "cols": 6, "keycodes": [...]}` in matrix order.
- CLI: `render_page.py --bundle DIR --keymap FILE --out DIR`. It writes `DIR/index.html`, `page.css`, `page.js` and `icons.ttf`, the same set as the default page, and exits 0. Any failure exits non-zero with one stderr line.
- Functions: `spell(value: int, table: dict[int, str], spec: dict) -> str` (the three-step order in Global Constraints); `layout_keymap(numeric: dict, matrix: list[list[int]], table, spec) -> dict` (a skim-ready `keymap.json` dict in layout order, in the same shape `qmk c2json` emits: keyboard, keymap, layout, layers).
- The page is assembled with `page_build.py`, the same code the default page uses.

- [ ] **Step 1: Write the failing tests:**
  - `test_spell_uses_table_first`: a value that's in the table returns the table's spelling.
  - `test_spells_basic_from_spec`: `0x0004` with an empty table gives `KC_A`.
  - `test_spells_an_unseen_layer_tap_generically`: the QMK value of `LT(3, KC_A)` with an empty table gives `LT(3,KC_A)`, using the spec's QK_LAYER_TAP range.
  - `test_spells_mod_tap_and_wrapped_mods`: `MT(MOD_LSFT,KC_A)` and `LSFT(KC_A)` come out in QMK spelling.
  - `test_unknown_value_is_hex`: an unassigned value gives `0x7E35`-style hex.
  - `test_layout_keymap_reorders_by_matrix`: matrix order is turned into layout order through the manifest positions.
- [ ] **Step 2:** `python3 tests/run_tests.py`. Expected: the new tests FAIL.
- [ ] **Step 3:** Implement. The spec's range table drives the generic decoder; no hand-written range constants.
- [ ] **Step 4:** `python3 tests/run_tests.py`. Expected: all pass.
- [ ] **Step 5: Verify end to end, offline from the keyboard:** take Task 1's recorded keymap line (or build one from the ELF), then run `uv run --no-project --quiet --with qmk-skim==0.7.5 python -I <scratch>/render/render_page.py --bundle <scratch>/render --keymap km.json --out <scratch2>`. Expected: `<scratch2>/index.html` matches the default page for every layer's SVG (the same keymap gives the same drawing). Report any differences and why.
- [ ] **Step 6:** Commit in QC: `✨ Render the overlay page from a keymap read off the keyboard`.

### Task 4: svaloverlay renders on connect and swaps pages (Swift)

**Files:**
- Create: `tools/svalwatch/Sources/SvalWatchCore/PageCache.swift`, the pure part: `PageCache.key(bundleVersion: String, keymap: Keymap) -> String` (SHA-256 hex via CryptoKit), `PageCache.prune(dirs: [(name: String, modified: Date)], keep: Int) -> [String]` (the names to delete).
- Create: `tools/svalwatch/Sources/svaloverlay/PageRenderer.swift`: the process side. It finds `uv` (Global Constraints order), writes the keymap JSON to a temp file, runs the renderer with a 120 s timeout, renders into `pages/<key>.tmp` and then renames it to `pages/<key>`, allows one render at a time (a newer keymap replaces a queued one), and calls back on the main actor.
- Modify: `OverlayWindow.swift` (`show(.keymap)`: if the cache hits, load the page; otherwise render, then load; a `load(page:)` that swaps the WKWebView page and replays the current state once the page posts "ready"). Modify `main.swift` (the shipped assets dir is still the default page; the bundle is `<assets>/render`).
- Test: `tools/svalwatch/Tests/SvalWatchCoreTests/PageCacheTests.swift`.

**Interfaces:**
- Consumes: Task 1 `WatchEvent.keymap(Keymap)`. Task 2's `<assets>/render/VERSION`. Task 3's CLI.
- Replaying state: OverlayWindow already keeps the last layer, keys, pointer and connection state it pushed; after a page "ready" it pushes them again. Read the current `show`/`showIfReady` to see how.

- [ ] **Step 1: Write the failing tests** (`PageCacheTests`):
  - `key changes with the keymap`, `key changes with the bundle version`, `key is stable for equal input`.
  - `prune keeps the newest five`.
- [ ] **Step 2:** `make test`. Expected: FAIL.
- [ ] **Step 3:** Implement PageCache, PageRenderer and the OverlayWindow wiring.
- [ ] **Step 4:** `make test`. Expected: all pass.
- [ ] **Step 5: Verify on this Mac** (the keyboard is plugged in; the overlay is driven by Hammerspoon, and the page is read-only):
  - `make -C tools/svalwatch install` (it builds, installs the binaries and assets, and the svalboard module restarts the overlay).
  - Turn the overlay on with `hs -c 'require("svalboard").setMode("background")'`.
  - Expected:
    - `pages/<hash>/` appears within about 30 s of the overlay starting (`reads the keymap → renders`), and the overlay swaps to it.
    - `reuses a cached page`: a restart with `pause(10)` then `resume()` loads the same page with no new render (no new dir, and no render log line).
    - `keeps the current page when the render fails`: with `uv` hidden (temporarily run the binary with `PATH=/usr/bin:/bin` and a bundle path whose `render_page.py` is renamed, both reverted afterwards), the default page stays and one log line names the failure.
    - `replays state after a page swap`: hold a layer key while the swap happens, and the new page shows that layer.
  - Restore the owner's overlay mode to `off` afterwards.
- [ ] **Step 6:** Commit in QC: `✨ Draw the overlay from the keyboard's own keymap`.

### Task 5: Packaging, and nothing installs the page at build or flash time (QC + CZ)

**Files:**
- Modify: QC `Makefile`: remove the `overlay` step and its comment block from `all` (commit 948c7eb). `make` doesn't touch the overlay any more.
- Modify: QC `.github/workflows/build_binaries.yaml`: the overlay page build now needs the firmware ELF (it's in the build job, after `make`); `render/` is inside `overlay-page/` and so inside `svaloverlay-macos.tar.gz` under `share/svaloverlay/render/`. Check that the archive listing step shows it.
- Modify: CZ `home/.chezmoiexternal.toml.tmpl`: confirm that `include = ["share/svaloverlay/**"]` already covers `render/` (no change if it does). Update its comment, because the page now follows the keyboard, not the release.
- Modify: QC `tools/svalwatch/Makefile` comments, and `README.md`'s overlay section: the page updates itself on connect, and `overlay-assets` is only for changing the overlay itself.

- [ ] **Step 1:** Make the edits.
- [ ] **Step 2: Verify:** `make` in QC prints no overlay step. Push a branch and check CI is green, and that `tar -tzf svaloverlay-macos.tar.gz` in the run's artifact lists `share/svaloverlay/render/render_page.py`, `spelling.json` and `VERSION`.
- [ ] **Step 3:** Commit in QC: `👷 Ship the overlay's render bundle and stop installing pages at build time`. In CZ, only if the template changed: `docs(chezmoi): the overlay page follows the keyboard`.

### Task 6: On-device check with the owner

- [ ] Change a key in code, then `make && qmk-flash`. Expected: after the keyboard reconnects, the overlay redraws within about 30 s with the change, with no other step.
- [ ] Revert it and flash again. Expected: the overlay goes back to the earlier page at once (a cache hit).
- [ ] On another Mac, after `chezmoi apply` brings the new overlay: plug the keyboard in. Expected: the overlay shows the flashed keymap, not the release's.
