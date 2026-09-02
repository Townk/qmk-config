/* Host-compiled fixture for the MB_* dual-role mouse/modifier engine.
 *
 * Compiles the REAL users/townk/townk_mouse.c on the host -- not a copy -- so
 * these tests exercise the shipping state machine rather than a paraphrase of
 * it. Everything the firmware would supply (QMK's keycode constants, the
 * register/tap API, the Svalboard's mouse_mode(), the pointing-device hook) is
 * stubbed below.
 *
 * The technique is sm_td's: define SMTD_UNIT_TEST, include its
 * sm_td_bindings.c shim -- which stubs QMK and records every register /
 * unregister / emulate event with the mods and layer state at the time -- then
 * drive the code under test and assert against that history.
 *
 * Built as a shared library by tests/test_townk_mouse.py; not part of any
 * firmware build.
 */

/* SMTD_UNIT_TEST is supplied by the compiler invocation in
 * tests/test_townk_mouse.py, not defined here, so the two cannot disagree. */

#define MATRIX_ROWS 1
#define MATRIX_COLS 4
#define TAPPING_TERM 200

/* This fixture compiles the real townk_layers.c, whose layer_state_set_user
 * is the code under test for the game-layer auto-mouse handling. The shim's
 * layer_move/layer_on/layer_off route every mutation through that hook when
 * this is defined -- exactly what QMK does on-device. */
#define SMTD_LAYOUT_DEFINES_LAYER_HOOK

#include "../modules/stasmarkin/tests/unit/sm_td_bindings.c"

/* The shim (0.5.6+) defines KC_LSFT as a macro for its own internal use; every
 * use inside it has expanded by now, and the macro would otherwise mangle the
 * enumerator of the same name (and value) in keycodes.h below. */
#undef KC_LSFT

/* townk_keycodes.h picks RANGE_START = SAFE_RANGE when SVALBOARD is undefined,
 * which is what lets it compile off-device. Deliberately do NOT define
 * SVALBOARD: that path would pull in keymap_support.h and QK_KB_20. */
#define SAFE_RANGE 0x7E00

/* QMK keycodes and modifier masks, with real values -- townk_smtd.c switches on
 * keycode ranges that are only well-formed if the ordering matches. */
#include "keycodes.h"
#include "modifiers.h"

/* ------------------------------------------------------------------------ *
 * QMK API the sm_td shim does not stub
 * ------------------------------------------------------------------------ */

/* sm_td's shim provides only the *_code16 forms; townk_mouse.c uses the plain
 * ones. Delegating keeps every emitted button in the recorded history. */
void register_code(uint16_t keycode) { register_code16(keycode); }
void unregister_code(uint16_t keycode) { unregister_code16(keycode); }
void tap_code(uint16_t keycode) { tap_code16(keycode); }

/* One-shot modifiers, which SMART_SHIFT sets on a tap. Modelled for real
 * rather than stubbed to zero: the Backspace/Delete shift inversion reads
 * get_oneshot_mods(), so a fake would hide exactly the behaviour under test. */
static uint8_t oneshot_mods = 0;
void    set_oneshot_mods(uint8_t mods) { oneshot_mods = mods; }
uint8_t get_oneshot_mods(void) { return oneshot_mods; }
void    clear_oneshot_mods(void) { oneshot_mods = 0; }

/* Svalboard persisted settings. On-device this lives in keymap_support.c;
 * only the field townk_layers.c touches is modelled. */
static struct {
    bool auto_mouse;
} global_saved_values = {.auto_mouse = true};

/* Supplied by the Svalboard keyboard code on-device. Recorded here so tests can
 * assert on mouse-mode transitions, which are otherwise invisible.
 *
 * auto_mouse is captured at call time because that ordering is a real
 * contract: Svalboard's mouse_mode() body is gated on
 * global_saved_values.auto_mouse ("needs to go first to avoid the lockout",
 * keymap_support.c), so a caller that clears the flag before calling has
 * turned the call into a guaranteed no-op on-device. */
static int  mouse_mode_calls = 0;
static bool mouse_mode_state = false;
static bool mouse_mode_saw_auto_mouse = false;

void mouse_mode(bool on) {
    mouse_mode_calls++;
    mouse_mode_state = on;
    mouse_mode_saw_auto_mouse = global_saved_values.auto_mouse;
}

/* Pointing-device plumbing. townk_mouse.c defines pointing_device_task_kb()
 * (the movement -> mouse-button conversion) and hands off to _user.
 *
 * The Svalboard defines MOUSE_EXTENDED_REPORT, so on-device x/y are int16_t
 * (see tmk_core/protocol/report.h). The fixture must match, or a large
 * report would be truncated HERE and hide exactly the truncation bugs the
 * motion tests exist to catch. */
typedef int16_t mouse_xy_report_t;

typedef struct {
    mouse_xy_report_t x;
    mouse_xy_report_t y;
    int8_t            h;
    int8_t            v;
    uint8_t           buttons;
} report_mouse_t;

report_mouse_t pointing_device_task_user(report_mouse_t report) { return report; }

/* Layers. Since 0.6.4 the shim models layer_state as a real bitmask with
 * native additive layer_on/layer_off -- the fidelity this fixture used to
 * bolt on with its own bitmask, now deleted in the shim's favour. Only
 * layer_state_is is missing upstream; townk_mouse.c reads _NAV through it. */
bool layer_state_is(uint8_t layer) { return (layer_state & ((uint32_t)1 << layer)) != 0; }

/* townk_layers.c takes QMK's layer_state_t; the shim models the same state
 * as a bare uint32_t, so the typedef makes the two one type. */
typedef uint32_t layer_state_t;

/* QMK's semantics: an empty state means only layer 0 is active. */
bool layer_state_cmp(layer_state_t state, uint8_t layer) {
    if (!state) {
        return layer == 0;
    }
    return (state & ((layer_state_t)1 << layer)) != 0;
}

/* RGB plumbing for townk_layers.c: the layer table is registered and each
 * layer's segment toggled on layer changes. Nothing under test reads RGB
 * state, so recording is unnecessary -- the stub only has to link. */
#include "rgblight.h" /* the stub in tests/stubs, not QMK's */

const rgblight_segment_t *const *rgblight_layers = 0;

void rgblight_set_layer_state(uint8_t layer, bool enabled) {
    (void)layer;
    (void)enabled;
}

/* The LED idle engine's world: QMK's input-activity clock, modelled as a
 * settable elapsed value, and the rgblight power switch, recorded so tests
 * can assert on transitions (and on their absence -- no churn). */
static uint32_t input_idle_ms     = 0;
static bool     rgb_enabled       = true;
static int      rgb_enable_calls  = 0;
static int      rgb_disable_calls = 0;

uint32_t last_input_activity_elapsed(void) { return input_idle_ms; }

void rgblight_disable_noeeprom(void) {
    rgb_enabled = false;
    rgb_disable_calls++;
}

void rgblight_enable_noeeprom(void) {
    rgb_enabled = true;
    rgb_enable_calls++;
}

/* ------------------------------------------------------------------------ *
 * SM_TD hooks every fixture must define
 * ------------------------------------------------------------------------ */

/* sm_td declares these __attribute__((weak)) and calls them through a NULL
 * check; a shared library still needs concrete definitions to link. Defer to
 * the library defaults -- this fixture has no per-key timing of its own. */
uint32_t get_smtd_timeout(uint16_t keycode, smtd_timeout timeout) {
    (void)keycode;
    return get_smtd_timeout_default(timeout);
}

bool smtd_feature_enabled(uint16_t keycode, smtd_feature feature) {
    return smtd_feature_enabled_default(keycode, feature);
}

/* Only ever used by sm_td's debug printer. A constant beats upstream's
 * switch-without-default, which falls off the end for unlisted keycodes. */
char *smtd_keycode_to_str_user(uint16_t keycode) {
    (void)keycode;
    return "?";
}

/* Called by the shim after each recorded event, for fixtures that need to
 * layer extra behaviour onto a keycode. Nothing to add here. */
void post_register_code16(uint16_t keycode) { (void)keycode; }
void post_unregister_code16(uint16_t keycode) { (void)keycode; }
void post_process_record(keyrecord_t *record) { (void)record; }

/* ------------------------------------------------------------------------ *
 * The code under test -- the real file, compiled as-is
 * ------------------------------------------------------------------------ */

#include "../users/townk/townk_layers.c"
#include "../users/townk/townk_mods.c"
#include "../users/townk/townk_mouse.c"
#include "../users/townk/townk_switcher.c"
#include "../users/townk/townk_idle.c"

/* ------------------------------------------------------------------------ *
 * Minimal keymap + SM_TD action handler
 * ------------------------------------------------------------------------ */

uint16_t const keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {{MB_GUI, CKC_SPC, KC_BTN1, KC_NO}},
};

/* The REAL SM_TD action handler -- the shift-inverted Backspace/Delete, the
 * layer-taps and Smart Shift, not a paraphrase. This is what lets a test say
 * anything trustworthy about Shift+Backspace. */
#include "../users/townk/townk_smtd.c"

/* ------------------------------------------------------------------------ *
 * Test driver, called over ctypes
 * ------------------------------------------------------------------------ */

/* Drive one key event through the engine, as process_record_user() would. */
void T_key(uint16_t keycode, bool pressed) {
    keyrecord_t record = {.event = MAKE_KEYEVENT(0, 0, pressed)};
    process_special_mouse_keys(keycode, &record);
}

/* Drive the SM_TD action handler directly. SM_TD-managed keys reach
 * on_smtd_action() and never process_record_user() -- that asymmetry is what
 * produced the phantom click, and it is also why nothing here could be tested
 * until the real townk_smtd.c was compiled in.
 *
 * TOUCH fires on press; TAP or HOLD when SM_TD resolves the key; RELEASE on
 * let-go. tap_count is how many taps preceded this one. */
void T_smtd_touch(uint16_t keycode) { on_smtd_action(keycode, SMTD_ACTION_TOUCH, 0); }
void T_smtd_tap(uint16_t keycode, uint8_t tap_count) { on_smtd_action(keycode, SMTD_ACTION_TAP, tap_count); }
void T_smtd_hold(uint16_t keycode, uint8_t tap_count) { on_smtd_action(keycode, SMTD_ACTION_HOLD, tap_count); }
void T_smtd_release(uint16_t keycode, uint8_t tap_count) { on_smtd_action(keycode, SMTD_ACTION_RELEASE, tap_count); }

/* Trackball motion. Pointer movement (x/y) is what converts a held key to a
 * held mouse button; h/v are scroll. x/y are 16-bit, as on the board. */
void T_pointing(mouse_xy_report_t x, mouse_xy_report_t y, int8_t h, int8_t v) {
    report_mouse_t report = {.x = x, .y = y, .h = h, .v = v, .buttons = 0};
    pointing_device_task_kb(report);
}

/* Pre-seed an EXTERNAL modifier, so the mods_on_press branch is reachable. */
void T_set_external_mods(uint8_t mods) { set_mods(mods); }

int  T_mouse_mode_calls(void) { return mouse_mode_calls; }
bool T_mouse_mode_state(void) { return mouse_mode_state; }
bool T_mouse_mode_saw_auto_mouse(void) { return mouse_mode_saw_auto_mouse; }

/* The Svalboard's persisted auto-mouse preference, as townk_layers.c sees it. */
bool T_auto_mouse(void) { return global_saved_values.auto_mouse; }
void T_set_auto_mouse(bool on) { global_saved_values.auto_mouse = on; }

uint8_t T_layer_gam1(void) { return _GAM1; }
uint8_t T_layer_base(void) { return _BASE; }

/* Clear the engine's own state between tests. TEST_reset() in the sm_td shim
 * clears the recorded history and sm_td, but knows nothing about mb_states. */
void T_reset(void) {
    for (int i = 0; i < 4; i++) {
        mb_states[i] = (mb_state_t){0};
    }
    mb_motion_accum     = 0;
    mb_motion_last_time = 0;
    mods_reset();
    mouse_mode_calls          = 0;
    mouse_mode_state          = false;
    mouse_mode_saw_auto_mouse = false;
    global_saved_values.auto_mouse = true; /* the Svalboard EEPROM default */
    game_layers_active = false;
    saved_auto_mouse   = false;
    caps_word_off();
    oneshot_mods = 0;
    set_mods(0);
    layer_move(_BASE);
    /* The LAYER_PUSH/LAYER_RESTORE refcount lives in townk_smtd.c now that
     * SM_TD 0.6.2 dropped the macros; the shim's TEST_reset no longer knows
     * about it, so clear it here or a dangling push leaks across tests. */
    smtd_return_layer     = RETURN_LAYER_NOT_SET;
    smtd_return_layer_cnt = 0;
    switcher_reset();
    led_idle_reset();
    input_idle_ms     = 0;
    rgb_enabled       = true;
    rgb_enable_calls  = 0;
    rgb_disable_calls = 0;
}

/* Reference-counted modifier ownership, driven directly. Going through gestures
 * cannot exercise a second claim on the same bit: each MB_* key owns a distinct
 * modifier, so the counting only becomes reachable once keys contribute
 * modifiers to clicks. */
/* Hold / release the left thumb pad, whose SM_TD hold is the only thing that
 * activates _NAV -- which is exactly how townk_mouse.c knows the key is down. */
void T_hold_backspace(bool held) {
    if (held) { layer_on(_NAV); } else { layer_off(_NAV); }
}

/* Mouse mode raises _MBO additively, which is what must survive the pad's
 * hold for a click to be reachable at all. */
void T_mouse_layer(bool on) {
    if (on) { layer_on(_MBO); } else { layer_off(_MBO); }
}

bool T_layer_is(uint8_t layer) { return layer_state_is(layer); }

void T_mods_acquire(uint8_t mods) { mods_acquire(mods); }
void T_mods_release(uint8_t mods) { mods_release(mods); }
uint8_t T_mods_claims(uint8_t mod_bit) { return mods_claim_count(mod_bit); }

/* Keycode values, exported rather than duplicated in Python so the tests
 * cannot drift from the enum in townk_keycodes.h. */
uint16_t T_kc_mb_sft(void) { return MB_SFT; }
uint16_t T_kc_mb_alt(void) { return MB_ALT; }
uint16_t T_kc_mb_gui(void) { return MB_GUI; }
uint16_t T_kc_mb_ctl(void) { return MB_CTL; }
uint16_t T_kc_ckc_spc(void) { return CKC_SPC; }
uint16_t T_kc_ckc_bspc(void) { return CKC_BSPC; }
uint8_t  T_layer_nav(void) { return _NAV; }
uint8_t  T_layer_mbo(void) { return _MBO; }
uint16_t T_kc_del(void) { return KC_DEL; }
uint16_t T_kc_bspc(void) { return KC_BSPC; }
uint16_t T_kc_ckc_tab(void) { return CKC_TAB; }
uint16_t T_kc_ckc_bktab(void) { return CKC_BKTAB; }
uint16_t T_kc_tab(void) { return KC_TAB; }
uint16_t T_kc_bktab(void) { return MKC_BKTAB; }
uint16_t T_kc_spc(void) { return KC_SPC; }
uint16_t T_kc_grv(void) { return KC_GRV; }
uint16_t T_kc_bkgrv(void) { return MKC_BKGRV; }
uint16_t T_kc_b(void) { return KC_B; }

/* The switcher chord's disarm point, as the QMK main loop would run it.
 * (housekeeping_task_user() itself lives in keymap.c, outside this fixture,
 * calling switcher_task() and led_idle_task() in turn.) */
void T_switcher_task(void) { switcher_task(); }

/* The LED idle engine's inputs and observable effects. */
void T_set_input_idle_ms(uint32_t ms) { input_idle_ms = ms; }
bool T_rgb_enabled(void) { return rgb_enabled; }
int  T_rgb_enable_calls(void) { return rgb_enable_calls; }
int  T_rgb_disable_calls(void) { return rgb_disable_calls; }
uint16_t T_kc_btn1(void) { return KC_BTN1; }
uint16_t T_kc_btn2(void) { return KC_BTN2; }
uint16_t T_kc_btn3(void) { return KC_BTN3; }
uint16_t T_kc_plain(void) { return 0x0004; } /* KC_A -- an ordinary, non-SM_TD key */
