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

#include "../modules/stasmarkin/tests/sm_td_bindings.c"

/* townk_keycodes.h picks RANGE_START = SAFE_RANGE when SVALBOARD is undefined,
 * which is what lets it compile off-device. Deliberately do NOT define
 * SVALBOARD: that path would pull in keymap_support.h and QK_KB_20. */
#define SAFE_RANGE 0x7E00

/* ------------------------------------------------------------------------ *
 * QMK keycode constants (quantum/keycodes.h on-device)
 * ------------------------------------------------------------------------ */

enum test_keycodes {
    KC_NO   = 0x0000,
    KC_BTN1 = 0x00D1,
    KC_BTN2 = 0x00D2,
    KC_BTN3 = 0x00D3,
    KC_BTN4 = 0x00D4,
    KC_BTN5 = 0x00D5,
    KC_BTN6 = 0x00D6,
    KC_BTN7 = 0x00D7,
    KC_BTN8 = 0x00D8,
    /* MOD_BIT() masks these to 0x07, giving 1/2/4/8 as on-device. */
    KC_LCTL = 0x00E0,
    KC_LSFT = 0x00E1,
    KC_LALT = 0x00E2,
    KC_LGUI = 0x00E3,
};

/* ------------------------------------------------------------------------ *
 * QMK API the sm_td shim does not stub
 * ------------------------------------------------------------------------ */

/* sm_td's shim provides only the *_code16 forms; townk_mouse.c uses the plain
 * ones. Delegating keeps every emitted button in the recorded history. */
void register_code(uint16_t keycode) { register_code16(keycode); }
void unregister_code(uint16_t keycode) { unregister_code16(keycode); }
void tap_code(uint16_t keycode) { tap_code16(keycode); }

/* townk_mouse.c consults all three modifier sources when deciding whether
 * EXTERNAL mods were held at press time. Only get_mods() is stubbed upstream. */
uint8_t get_oneshot_mods(void) { return 0; }
uint8_t get_weak_mods(void) { return 0; }

/* Supplied by the Svalboard keyboard code on-device. Recorded here so tests can
 * assert on mouse-mode transitions, which are otherwise invisible. */
static int  mouse_mode_calls = 0;
static bool mouse_mode_state = false;

void mouse_mode(bool on) {
    mouse_mode_calls++;
    mouse_mode_state = on;
}

/* Pointing-device plumbing. townk_mouse.c defines pointing_device_task_kb()
 * (the movement -> mouse-button conversion) and hands off to _user. */
typedef struct {
    int8_t  x;
    int8_t  y;
    int8_t  h;
    int8_t  v;
    uint8_t buttons;
} report_mouse_t;

report_mouse_t pointing_device_task_user(report_mouse_t report) { return report; }

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

#include "../users/townk/townk_mouse.c"

/* ------------------------------------------------------------------------ *
 * Minimal keymap + SM_TD action handler
 * ------------------------------------------------------------------------ */

uint16_t const keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {{MB_GUI, CKC_SPC, KC_BTN1, KC_NO}},
};

/* Mirrors the contract of the real on_smtd_action() in users/townk/townk_smtd.c:
 * an SM_TD key being touched must commit any pending MB_* key to its modifier
 * role. Only the contract is reproduced here -- compiling the real townk_smtd.c
 * would additionally need caps_word.h, action_util.h, keycodes.h, modifiers.h
 * and townk_layers.h stubbed. */
smtd_resolution on_smtd_action(uint16_t keycode, smtd_action action, uint8_t tap_count) {
    (void)tap_count;

    if (action == SMTD_ACTION_TOUCH) {
        confirm_pending_modifiers(keycode);
    }

    return SMTD_RESOLUTION_UNHANDLED;
}

/* ------------------------------------------------------------------------ *
 * Test driver, called over ctypes
 * ------------------------------------------------------------------------ */

/* Drive one key event through the engine, as process_record_user() would. */
void T_key(uint16_t keycode, bool pressed) {
    keyrecord_t record = {.event = MAKE_KEYEVENT(0, 0, pressed)};
    process_special_mouse_keys(keycode, &record);
}

/* Simulate an SM_TD-managed keypress: it reaches on_smtd_action() and never
 * process_record_user(). This is the path that produced the phantom click. */
void T_smtd_touch(uint16_t keycode) {
    on_smtd_action(keycode, SMTD_ACTION_TOUCH, 0);
}

/* Trackball motion. Pointer movement (x/y) is what converts a held key to a
 * held mouse button; h/v are scroll. */
void T_pointing(int8_t x, int8_t y, int8_t h, int8_t v) {
    report_mouse_t report = {.x = x, .y = y, .h = h, .v = v, .buttons = 0};
    pointing_device_task_kb(report);
}

/* Pre-seed an EXTERNAL modifier, so the mods_on_press branch is reachable. */
void T_set_external_mods(uint8_t mods) { set_mods(mods); }

int  T_mouse_mode_calls(void) { return mouse_mode_calls; }
bool T_mouse_mode_state(void) { return mouse_mode_state; }

/* Clear the engine's own state between tests. TEST_reset() in the sm_td shim
 * clears the recorded history and sm_td, but knows nothing about mb_states. */
void T_reset(void) {
    for (int i = 0; i < 4; i++) {
        mb_states[i] = (mb_state_t){0};
    }
    mouse_mode_calls = 0;
    mouse_mode_state = false;
    set_mods(0);
}

/* Keycode values, exported rather than duplicated in Python so the tests
 * cannot drift from the enum in townk_keycodes.h. */
uint16_t T_kc_mb_sft(void) { return MB_SFT; }
uint16_t T_kc_mb_alt(void) { return MB_ALT; }
uint16_t T_kc_mb_gui(void) { return MB_GUI; }
uint16_t T_kc_mb_ctl(void) { return MB_CTL; }
uint16_t T_kc_ckc_spc(void) { return CKC_SPC; }
uint16_t T_kc_btn1(void) { return KC_BTN1; }
uint16_t T_kc_btn3(void) { return KC_BTN3; }
uint16_t T_kc_plain(void) { return 0x0004; } /* KC_A -- an ordinary, non-SM_TD key */
