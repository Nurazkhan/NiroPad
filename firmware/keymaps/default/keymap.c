#include QMK_KEYBOARD_H
#include "raw_hid.h"
#include <string.h>
#include <stdio.h>

#define HOLD_THRESHOLD 350

// Display modes
enum display_modes {
    DISP_MODE_SYS = 0,
    DISP_MODE_MEDIA = 1,
    DISP_MODE_NET = 2,
    DISP_MODE_COUNT
};

static uint8_t current_disp_mode = DISP_MODE_SYS;

// Button 9 state tracking (Row 2, Col 2 in matrix)
static uint16_t b9_press_time = 0;
static bool     b9_is_down = false;
static bool     b9_hold_triggered = false;

// Last actions for standalone / offline display
static char last_action[16] = "Ready";
static char last_encoder_action[16] = "None";

// Encoder direction glow pulse timers
static uint16_t enc_ccw_timer = 0;
static uint16_t enc_cw_timer = 0;

// PC Companion Stats
static bool     pc_connected = false;
static uint16_t last_pc_time = 0;
static uint8_t  stat_cpu = 0;
static uint8_t  stat_ram = 0;
static uint8_t  stat_gpu = 45;
static uint8_t  stat_wifi = 80;
static uint8_t  stat_vol = 50;
static bool     stat_muted = false;
static char     media_title[24] = "No Media";
static char     media_artist[24] = "Idle";
static char     net_info[24] = "Online";

void keyboard_pre_init_user(void) {
    // Turn off Seeed Studio Xiao RP2040 onboard user RGB LED (Active-LOW on GP16, GP17, GP25)
    gpio_set_pin_output(GP16);
    gpio_write_pin_high(GP16);
    gpio_set_pin_output(GP17);
    gpio_write_pin_high(GP17);
    gpio_set_pin_output(GP25);
    gpio_write_pin_high(GP25);

    // Cut power to onboard NeoPixel / auxiliary lines if present
    gpio_set_pin_output(GP11);
    gpio_write_pin_low(GP11);
    gpio_set_pin_output(GP12);
    gpio_write_pin_low(GP12);
}

void keyboard_post_init_user(void) {
    // Re-assert HIGH after QMK driver initialization to ensure onboard LEDs stay off
    gpio_set_pin_output(GP16);
    gpio_write_pin_high(GP16);
    gpio_set_pin_output(GP17);
    gpio_write_pin_high(GP17);
    gpio_set_pin_output(GP25);
    gpio_write_pin_high(GP25);
    gpio_set_pin_output(GP11);
    gpio_write_pin_low(GP11);
    gpio_set_pin_output(GP12);
    gpio_write_pin_low(GP12);
}

// 4 Layers defined (3 rows x 4 columns matrix, with col 3 = virtual encoder)
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_1,    KC_2,    KC_3,    KC_NO,
        KC_4,    KC_5,    KC_6,
        KC_7,    KC_8,    KC_NO
    ),
    [1] = LAYOUT(
        KC_MPRV, KC_MPLY, KC_MNXT, KC_NO,
        KC_MUTE, KC_VOLD, KC_VOLU,
        KC_LEFT, KC_DOWN, KC_RGHT
    ),
    [2] = LAYOUT(
        C(KC_Z), C(KC_X), C(KC_C), KC_NO,
        C(KC_V), C(KC_A), C(KC_S),
        KC_HOME, KC_UP,   KC_END
    ),
    [3] = LAYOUT(
        KC_F1,   KC_F2,   KC_F3,   KC_NO,
        KC_F4,   KC_F5,   KC_F6,
        KC_F7,   KC_F8,   KC_NO
    )
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [2] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [3] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
};
#endif

#if defined(ENCODER_ENABLE)
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) {
        snprintf(last_encoder_action, sizeof(last_encoder_action), "%s", clockwise ? "Enc CW (+)" : "Enc CCW (-)");
        if (clockwise) {
            enc_cw_timer = timer_read();
        } else {
            enc_ccw_timer = timer_read();
        }
    }
    return true; // allow encoder_map to handle keycodes!
}
#endif

#ifdef RGB_MATRIX_ENABLE
led_config_t g_led_config = {
    {
        // Matrix to LED index: [row][col] -> LED index
        { 5,  6,  7, NO_LED },
        { 8,  9, 10, NO_LED },
        { 11, 12, 13, NO_LED }
    },
    {
        // Exact physical points {x, y} scaled to 0..224, 0..64 from KiCad PCB layout
        { 117,  4 }, /*  0: D10 (Display TL - Layer) */
        { 141,  4 }, /*  1: D11 (Display TR - Mute) */
        { 116, 10 }, /*  2: D12 (Display BL - Caps Lock) */
        { 142,  9 }, /*  3: D13 (Display BR - Disp Mode) */
        {  41,  8 }, /*  4: D14 (Display Accent) */
        {  10, 30 }, /*  5: D15 (Key 1) */
        {  56, 30 }, /*  6: D16 (Key 2) */
        { 105, 30 }, /*  7: D17 (Key 3) */
        {   4, 46 }, /*  8: D18 (Key 4) */
        {  56, 47 }, /*  9: D19 (Key 5) */
        { 104, 47 }, /* 10: D20 (Key 6) */
        {   6, 63 }, /* 11: D21 (Key 7) */
        {  55, 63 }, /* 12: D22 (Key 8) */
        { 105, 63 }, /* 13: D23 (Key 9) */
        { 179, 31 }, /* 14: D28 (Enc Top) */
        { 141, 43 }, /* 15: D29 (Enc Left - CCW) */
        { 216, 43 }, /* 16: D30 (Enc Right) */
        { 179, 56 }  /* 17: D31 (Enc Bottom - CW) */
    },
    {
        // LED flags
        LED_FLAG_INDICATOR, /*  0: D10 */
        LED_FLAG_INDICATOR, /*  1: D11 */
        LED_FLAG_INDICATOR, /*  2: D12 */
        LED_FLAG_INDICATOR, /*  3: D13 */
        LED_FLAG_INDICATOR, /*  4: D14 */
        LED_FLAG_KEYLIGHT,  /*  5: D15 */
        LED_FLAG_KEYLIGHT,  /*  6: D16 */
        LED_FLAG_KEYLIGHT,  /*  7: D17 */
        LED_FLAG_KEYLIGHT,  /*  8: D18 */
        LED_FLAG_KEYLIGHT,  /*  9: D19 */
        LED_FLAG_KEYLIGHT,  /* 10: D20 */
        LED_FLAG_KEYLIGHT,  /* 11: D21 */
        LED_FLAG_KEYLIGHT,  /* 12: D22 */
        LED_FLAG_KEYLIGHT,  /* 13: D23 */
        LED_FLAG_INDICATOR, /* 14: D28 */
        LED_FLAG_INDICATOR, /* 15: D29 */
        LED_FLAG_INDICATOR, /* 16: D30 */
        LED_FLAG_INDICATOR  /* 17: D31 */
    }
};

// Soft indicator helper: scales colors down to a gentle dim glow (~8% max brightness)
// and dynamically respects VIA brightness settings.
#define IND_MAX_BRIGHTNESS 20 // Max channel value out of 255 (soft, calm glow)

static inline void set_indicator_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t max_val = IND_MAX_BRIGHTNESS;
#if defined(RGB_MATRIX_ENABLE)
    if (rgb_matrix_is_enabled()) {
        uint8_t val = rgb_matrix_get_val();
        // Scale with VIA brightness
        max_val = ((uint16_t)IND_MAX_BRIGHTNESS * val) / 255;
        if (max_val < 2 && val > 0) max_val = 2; // Keep minimal visibility if brightness > 0
    } else {
        // If RGB matrix effects are toggled off, keep indicators at a faint standby glow
        max_val = 6;
    }
#endif
    uint8_t r_out = ((uint16_t)r * max_val) / 255;
    uint8_t g_out = ((uint16_t)g * max_val) / 255;
    uint8_t b_out = ((uint16_t)b * max_val) / 255;
    rgb_matrix_set_color(index, r_out, g_out, b_out);
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // 1. D10 (Index 0): Current Layer Indicator (Soft, dim colors)
    uint8_t layer = get_highest_layer(layer_state);
    switch (layer) {
        case 0:  set_indicator_rgb(0, 255, 0, 0); break;     // Dim Red
        case 1:  set_indicator_rgb(0, 255, 180, 0); break;   // Dim Yellow
        case 2:  set_indicator_rgb(0, 0, 120, 255); break;   // Dim Blue
        case 3:  set_indicator_rgb(0, 200, 0, 255); break;   // Dim Purple
        default: set_indicator_rgb(0, 200, 200, 200); break;
    }

    // 2. D11 (Index 1): Mute Indicator
    if (stat_muted) {
        set_indicator_rgb(1, 255, 0, 0); // Dim Red when muted
    } else {
        rgb_matrix_set_color(1, 0, 0, 0); // Off when unmuted
    }

    // 3. D12 (Index 2): Caps Lock Indicator
    if (host_keyboard_led_state().caps_lock) {
        set_indicator_rgb(2, 255, 0, 0); // Dim Red when Caps Lock is ON
    } else {
        rgb_matrix_set_color(2, 0, 0, 0); // Off when Caps Lock is OFF
    }

    // 4. D13 (Index 3): Current Display Mode Indicator
    switch (current_disp_mode) {
        case DISP_MODE_SYS:   set_indicator_rgb(3, 0, 220, 255); break; // Dim Cyan
        case DISP_MODE_MEDIA: set_indicator_rgb(3, 0, 255, 50); break;  // Dim Green
        case DISP_MODE_NET:   set_indicator_rgb(3, 255, 100, 0); break; // Dim Orange
        default: break;
    }

    // 5. D14 (Index 4): Accent / Off
    rgb_matrix_set_color(4, 0, 0, 0);

    // 6. Encoder Direction Reactive Glow on D29 (Index 15) and D31 (Index 17)
    uint8_t val = rgb_matrix_get_val();
    if (timer_elapsed(enc_ccw_timer) < 300) {
        uint8_t decay = 120 - (timer_elapsed(enc_ccw_timer) * 120 / 300);
        decay = ((uint16_t)decay * val) / 255;
        rgb_matrix_set_color(15, 0, decay, decay); // Soft Cyan on CCW (Left)
    }
    if (timer_elapsed(enc_cw_timer) < 300) {
        uint8_t decay = 120 - (timer_elapsed(enc_cw_timer) * 120 / 300);
        decay = ((uint16_t)decay * val) / 255;
        rgb_matrix_set_color(17, decay, (decay * 120) / 255, 0); // Soft Amber on CW (Right)
    }

    return true;
}
#endif

// Sanitizer for incoming strings: keep only printable ASCII (32..126)
static void sanitize_str(char *dest, const uint8_t *src, size_t max_len) {
    size_t i = 0;
    for (; i < max_len - 1 && src[i] != '\0'; i++) {
        uint8_t c = src[i];
        dest[i] = (c >= 32 && c <= 126) ? (char)c : ' ';
    }
    dest[i] = '\0';
}

#if defined(VIA_ENABLE)
bool via_command_kb(uint8_t *data, uint8_t length) {
    if (length < 2) return false;

    switch (data[0]) {
        case 0x51: // System Stats: [0x51, cpu, ram, gpu, wifi, vol, is_muted]
            stat_cpu  = (data[1] <= 100) ? data[1] : 0;
            stat_ram  = (data[2] <= 100) ? data[2] : 0;
            stat_gpu  = data[3];
            stat_wifi = (data[4] <= 100) ? data[4] : 0;
            stat_vol  = (data[5] <= 100) ? data[5] : 0;
            if (length >= 7) {
                stat_muted = (data[6] != 0);
            }
            pc_connected = true;
            last_pc_time = timer_read();
            return true;

        case 0x52: // Media Title
            sanitize_str(media_title, &data[1], sizeof(media_title));
            pc_connected = true;
            last_pc_time = timer_read();
            return true;

        case 0x53: // Media Artist
            sanitize_str(media_artist, &data[1], sizeof(media_artist));
            pc_connected = true;
            last_pc_time = timer_read();
            return true;

        case 0x54: // Network Info
            sanitize_str(net_info, &data[1], sizeof(net_info));
            pc_connected = true;
            last_pc_time = timer_read();
            return true;

        default:
            return false;
    }
}
#elif defined(RAW_ENABLE)
void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 2) return;

    switch (data[0]) {
        case 0x51:
            stat_cpu  = (data[1] <= 100) ? data[1] : 0;
            stat_ram  = (data[2] <= 100) ? data[2] : 0;
            stat_gpu  = data[3];
            stat_wifi = (data[4] <= 100) ? data[4] : 0;
            stat_vol  = (data[5] <= 100) ? data[5] : 0;
            if (length >= 7) {
                stat_muted = (data[6] != 0);
            }
            pc_connected = true;
            last_pc_time = timer_read();
            break;
        case 0x52:
            sanitize_str(media_title, &data[1], sizeof(media_title));
            pc_connected = true;
            last_pc_time = timer_read();
            break;
        case 0x53:
            sanitize_str(media_artist, &data[1], sizeof(media_artist));
            pc_connected = true;
            last_pc_time = timer_read();
            break;
        case 0x54:
            sanitize_str(net_info, &data[1], sizeof(net_info));
            pc_connected = true;
            last_pc_time = timer_read();
            break;
        default:
            break;
    }
}
#endif

// Friendly human-readable translation for common keycodes on OLED
static const char *get_keycode_name(uint16_t keycode) {
    if (keycode >= KC_A && keycode <= KC_Z) {
        static char let[4];
        snprintf(let, sizeof(let), "%c", 'A' + (keycode - KC_A));
        return let;
    }
    if (keycode >= KC_1 && keycode <= KC_9) {
        static char num[4];
        snprintf(num, sizeof(num), "%c", '1' + (keycode - KC_1));
        return num;
    }
    if (keycode == KC_0) return "0";

    switch (keycode) {
        case KC_MPLY: return "Play";
        case KC_MNXT: return "Next";
        case KC_MPRV: return "Prev";
        case KC_MSTP: return "Stop";
        case KC_MUTE: return "Mute";
        case KC_VOLU: return "Vol Up";
        case KC_VOLD: return "Vol Dn";

        case KC_UP:   return "Up";
        case KC_DOWN: return "Down";
        case KC_LEFT: return "Left";
        case KC_RGHT: return "Right";
        case KC_ENT:  return "Enter";
        case KC_ESC:  return "Esc";
        case KC_BSPC: return "Bspc";
        case KC_TAB:  return "Tab";
        case KC_SPC:  return "Space";
        case KC_DEL:  return "Del";
        case KC_HOME: return "Home";
        case KC_END:  return "End";
        case KC_PGUP: return "PgUp";
        case KC_PGDN: return "PgDn";
        case KC_CAPS: return "Caps";

        case KC_LCTL:
        case KC_RCTL: return "Ctrl";
        case KC_LSFT:
        case KC_RSFT: return "Shift";
        case KC_LALT:
        case KC_RALT: return "Alt";
        case KC_LGUI:
        case KC_RGUI: return "Win";

        case KC_F1:  return "F1";
        case KC_F2:  return "F2";
        case KC_F3:  return "F3";
        case KC_F4:  return "F4";
        case KC_F5:  return "F5";
        case KC_F6:  return "F6";
        case KC_F7:  return "F7";
        case KC_F8:  return "F8";
        case KC_F9:  return "F9";
        case KC_F10: return "F10";
        case KC_F11: return "F11";
        case KC_F12: return "F12";

        case KC_MINS: return "-";
        case KC_EQL:  return "=";
        case KC_COMM: return ",";
        case KC_DOT:  return ".";
        case KC_SLSH: return "/";
        case KC_SCLN: return ";";
        case KC_QUOT: return "'";
        case KC_LBRC: return "[";
        case KC_RBRC: return "]";
        case KC_BSLS: return "\\";
        case KC_GRV:  return "`";

        default:
            return NULL;
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.key.row == 2 && record->event.key.col == 2) {
        if (record->event.pressed) {
            b9_is_down = true;
            b9_hold_triggered = false;
            b9_press_time = timer_read();
        } else {
            b9_is_down = false;
            if (!b9_hold_triggered) {
                uint8_t current_layer = get_highest_layer(layer_state);
                uint8_t next_layer = (current_layer + 1) % 4;
                layer_move(next_layer);
                snprintf(last_action, sizeof(last_action), "Layer %u", next_layer);
            }
        }
        return false;
    }

    if (record->event.pressed) {
        if (keycode == KC_MUTE) {
            stat_muted = !stat_muted;
        }

        uint8_t row = record->event.key.row;
        uint8_t col = record->event.key.col;
        const char *name = get_keycode_name(keycode);

        if (row == 0 && col == 3) {
            // Rotary encoder push switch
            if (name) {
                snprintf(last_action, sizeof(last_action), "Knob (%s)", name);
            } else {
                snprintf(last_action, sizeof(last_action), "Knob Press");
            }
        } else {
            uint8_t btn_num = row * 3 + col + 1;
            if (name) {
                snprintf(last_action, sizeof(last_action), "Btn %u (%s)", btn_num, name);
            } else {
                snprintf(last_action, sizeof(last_action), "Button %u", btn_num);
            }
        }
    }

    return true;
}

#if defined(OLED_ENABLE)
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_180;
}

// Exactly 7 characters: [=====] or [==---]
static void make_bar(char *out, uint8_t pct) {
    if (pct > 100) pct = 100;
    uint8_t filled = (pct * 5) / 100;
    out[0] = '[';
    for (uint8_t i = 0; i < 5; i++) {
        out[1 + i] = (i < filled) ? '=' : '-';
    }
    out[6] = ']';
    out[7] = '\0';
}

// Renders exactly 21 characters padded with spaces to avoid buffer overrun
static void draw_line(uint8_t line, const char *text) {
    if (line > 3) return;
    char clean[22];
    snprintf(clean, sizeof(clean), "%-21.21s", text);
    oled_set_cursor(0, line);
    oled_write(clean, false);
}

bool oled_task_user(void) {
    char l0[24], l1[24], l2[24], l3[24];
    uint8_t layer = get_highest_layer(layer_state);
    const char *layer_tags[] = {"BASE", "MED ", "PROD", "NAV "};
    const char *tag = (layer < 4) ? layer_tags[layer] : "USER";

    if (pc_connected) {
        switch (current_disp_mode) {
            case DISP_MODE_SYS: {
                char b_cpu[8], b_ram[8];
                make_bar(b_cpu, stat_cpu);
                make_bar(b_ram, stat_ram);

                snprintf(l0, sizeof(l0), "[L%u:%s] SYS MON", layer, tag);
                snprintf(l1, sizeof(l1), "CPU:%3u%% %s", stat_cpu, b_cpu);
                snprintf(l2, sizeof(l2), "RAM:%3u%% %s", stat_ram, b_ram);
                snprintf(l3, sizeof(l3), "GPU:%2uC   WiFi:%2u%%", stat_gpu, stat_wifi);
                break;
            }

            case DISP_MODE_MEDIA: {
                char b_vol[8];
                make_bar(b_vol, stat_vol);

                snprintf(l0, sizeof(l0), "[L%u:%s] NOW PLAY", layer, tag);
                snprintf(l1, sizeof(l1), "> %s", media_title);
                snprintf(l2, sizeof(l2), "  %s", media_artist);
                snprintf(l3, sizeof(l3), "VOL:%3u%% %s %s", stat_vol, b_vol, stat_muted ? "[M]" : "   ");
                break;
            }

            case DISP_MODE_NET: {
                char b_wifi[8];
                make_bar(b_wifi, stat_wifi);

                snprintf(l0, sizeof(l0), "[L%u:%s] NETWORK", layer, tag);
                snprintf(l1, sizeof(l1), "Net: %s", net_info);
                snprintf(l2, sizeof(l2), "Sig:%3u%% %s", stat_wifi, b_wifi);
                snprintf(l3, sizeof(l3), "Status: ONLINE");
                break;
            }

            default:
                current_disp_mode = DISP_MODE_SYS;
                return false;
        }
    } else {
        led_t led_state = host_keyboard_led_state();
        snprintf(l0, sizeof(l0), "[L%u:%s]   NIROPAD", layer, tag);
        snprintf(l1, sizeof(l1), "Last: %s", last_action);
        snprintf(l2, sizeof(l2), "Enc:  %s", last_encoder_action);
        snprintf(l3, sizeof(l3), "Caps:%-3s   Num:ON", led_state.caps_lock ? "ON " : "OFF");
    }

    draw_line(0, l0);
    draw_line(1, l1);
    draw_line(2, l2);
    draw_line(3, l3);

    return false;
}
#endif

void housekeeping_task_user(void) {
    if (b9_is_down && !b9_hold_triggered) {
        if (timer_elapsed(b9_press_time) >= HOLD_THRESHOLD) {
            b9_hold_triggered = true;
            current_disp_mode = (current_disp_mode + 1) % DISP_MODE_COUNT;
            const char *mode_names[] = {"Sys Mon", "Media", "Network"};
            snprintf(last_action, sizeof(last_action), "Mode: %s", mode_names[current_disp_mode]);
        }
    }

    if (pc_connected && timer_elapsed(last_pc_time) >= 3500) {
        pc_connected = false;
    }
}
