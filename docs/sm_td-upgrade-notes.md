# Upgrading SM_TD

Notes for whoever moves `modules/stasmarkin` off the pinned version. Written
while building the host test suite, so that the reasoning behind the pin and the
shape of the safety net do not have to be re-derived.

## Where things stand

The submodule is pinned at commit `ebcf33c`, whose commit message is
`version 0.5.4`. `git describe` reports `v0.5.2-4-gebcf33c` only because the
local clone has never fetched the newer tags — the pin is coherent, not drifted.

Upstream tags at the time of writing:

```
v0.5.0  v0.5.2  v0.5.6  v0.6.0  v0.6.1  v0.6.2  v0.6.3  v0.6.4
```

A 0.5 → 0.6 jump is the risk. This userspace does not call SM_TD's primitives
directly; it wraps them in its own macros in `users/townk/townk_smtd.c`
(`CUSTOM_LT`, `SHIFTED_LT`, `SMART_SHIFT`, `SHIFT_ACTION`, and friends), all
built on `SMTD_DANCE`, `SMTD_LIMIT`, `SMTD_TAP_16`, `LAYER_PUSH` /
`LAYER_RESTORE` and the `smtd_action` / `smtd_resolution` enums. Anything
upstream renames or re-times lands there first.

The upgrade was deliberately kept separate from the mouse/modifier work that
preceded it, so that a broken upgrade could never block a fix people wanted
daily, and so a behaviour change could always be attributed to one or the other.

## The test suite is the point

```sh
python3 tests/run_tests.py     # ~26 tests, milliseconds, no keyboard needed
```

`tests/townk_mouse_layout.c` compiles the **real** `users/townk/townk_mouse.c`
and `users/townk/townk_smtd.c` on the host, via SM_TD's own harness shim
(`modules/stasmarkin/tests/sm_td_bindings.c`), which itself `#include`s
`sm_td.c` directly. So the suite runs against **whichever SM_TD version is
checked out**. Bump the submodule, run the suite, and read the failures.

Run it *before* touching anything, so you have a green baseline to compare
against.

### What it will catch

- **API breakage, loudly.** Renamed macros, changed signatures, moved enums —
  all become compile errors, and the fixture builds with `-Werror`.
- **Handler behaviour.** The shift-inverted Backspace/Delete, the `MB_*`
  dual-role resolution rules, the modifier reference counting, and the
  contribution of Option to a click are all asserted directly.

### What it will NOT catch

Read this part twice; it is the reason a green suite is necessary but not
sufficient.

- **Resolution timing.** The fixture calls `on_smtd_action(kc, SMTD_ACTION_TAP,
  n)` and friends **directly**. It tests the handlers, not SM_TD's decision
  about *when* a press becomes a tap versus a hold. That decision is exactly
  what a minor-version bump is most likely to change, and nothing here would
  notice. Budget for on-keyboard testing of tap-vs-hold feel.
- **Reachability.** The engine is driven straight through `T_key(...)`,
  bypassing keymap and layer lookup entirely. A test can prove what the engine
  does *given* a keypress; it cannot know whether that key is on an active
  layer. A feature was once shipped completely unreachable, with every test
  passing — see the `_MBO` / `_NAV` note in the CHANGELOG.
- **Coverage gaps.** `townk_layers.c` and `townk_overrides.c` are not compiled
  into the fixture at all.
- **Layer fidelity.** SM_TD's shim tracks `layer_state` as a single layer
  *number*; the fixture adds a bitmask alongside it so `layer_on` / `layer_off` /
  `layer_state_is` behave additively as on the board. If the upgrade changes how
  SM_TD reads layer state, check that model still holds.

## Known unknown worth resolving early

The `MB_*` engine would benefit from being expressed as SM_TD keys rather than
as a second, competing resolution engine in `townk_mouse.c` — SM_TD's
TOUCH/TAP/HOLD/RELEASE model already matches the rules documented at the top of
that file. The blocker is one-directional: converting a held key to a mouse
button on trackball motion needs to force a resolution from
`pointing_device_task_kb()`, and while `smtd_apply_stage()` is public,
`smtd_active_states` is `static` — there is no way to find a keycode's state
from outside the library.

**Check whether 0.6.x exposes a lookup.** If it does, that migration becomes
worth doing and would delete a lot of hand-rolled machinery. If it does not, it
is worth an upstream issue rather than a local fork.

## Sequence that has worked

1. `python3 tests/run_tests.py` — capture the green baseline.
2. Bump the submodule one minor version at a time (`v0.5.6`, then `v0.6.0`, …)
   rather than jumping straight to the newest. The suite runs in milliseconds;
   the information from a small step is worth far more than the time saved.
3. At each step: compile errors first, then test failures, then
   `qmk compile -kb svalboard/trackball/pmw3389/right -km townk`.
4. Flash and live with it before the next step. **Only the right half needs
   flashing** — it is the master and updates the left.
5. Pay attention to tap-vs-hold feel specifically, since the suite is blind to
   it. `CKC_BSPC` is the most sensitive key: a press slow enough to resolve as
   HOLD raises `_NAV` and emits nothing at all, which reads as "the key did
   nothing" rather than as a timing change.

## Gotchas that cost time before

- `qmk compile` needs the toolchain on `PATH`:
  `export PATH="$HOME/Library/Application Support/qmk/bin:$PATH"`.
- The pre-commit hook regenerates keymap images via qmk-skim/playwright and
  prints alarming `TargetClosedError` traces on the way. They are harmless; the
  hook reports success afterwards.
- Two pre-existing `-Wunused-variable` warnings come from `sm_td.h` itself
  (`return_layer`, `return_layer_cnt`). That is what
  `CFLAGS += -Wno-error=unused-variable` in `users/townk/rules.mk` exists for.
  If the upgrade removes them, consider removing the flag too.
