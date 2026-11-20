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

// PC Companion Stats
static bool     pc_connected = false;
static uint16_t last_pc_time = 0;
static uint8_t  stat_cpu = 0;
static uint8_t  stat_ram = 0;
static uint8_t  stat_gpu = 45;
static uint8_t  stat_wifi = 80;
static uint8_t  stat_vol = 50;
static char     media_title[24] = "No Media";
static char     media_artist[24] = "Idle";
static char     net_info[24] = "Online";


// 4 Layers defined
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
        snprintf(last_encoder_action, sizeof(last_encoder_action), "%s", clockwise ? "Enc CW" : "Enc CCW");
    }
    return true; // allow encoder_map to handle keycodes!
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
        case 0x51: // System Stats: [0x51, cpu, ram, gpu, wifi, vol]
            stat_cpu  = (data[1] <= 100) ? data[1] : 0;
            stat_ram  = (data[2] <= 100) ? data[2] : 0;
            stat_gpu  = data[3];
            stat_wifi = (data[4] <= 100) ? data[4] : 0;
            stat_vol  = (data[5] <= 100) ? data[5] : 0;
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
        switch (keycode) {
            case KC_1: snprintf(last_action, sizeof(last_action), "Key 1"); break;
            case KC_2: snprintf(last_action, sizeof(last_action), "Key 2"); break;
            case KC_3: snprintf(last_action, sizeof(last_action), "Key 3"); break;
            case KC_4: snprintf(last_action, sizeof(last_action), "Key 4"); break;
            case KC_5: snprintf(last_action, sizeof(last_action), "Key 5"); break;
            case KC_6: snprintf(last_action, sizeof(last_action), "Key 6"); break;
            case KC_7: snprintf(last_action, sizeof(last_action), "Key 7"); break;
            case KC_8: snprintf(last_action, sizeof(last_action), "Key 8"); break;
            default:   snprintf(last_action, sizeof(last_action), "Key 0x%02X", keycode & 0xFF); break;
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
                snprintf(l3, sizeof(l3), "VOL:%3u%% %s", stat_vol, b_vol);
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
            snprintf(last_action, sizeof(last_action), "Disp Mode %u", current_disp_mode);
        }
    }

    if (pc_connected && timer_elapsed(last_pc_time) >= 3500) {
        pc_connected = false;
    }
}
