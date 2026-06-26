#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS]={
    [0]= LAYOUT(
        KC_1,KC_2,KC_3,
        KC_4,KC_5,KC_6,
        KC_7,KC_8,KC_9
    )
};

#if defined(ENCODER_ENABLE)
bool encoder_update_user(uint8_t index, bool clockwise){
    if(index ==0){
        if(clockwise){
            tap_code(KC_VOLU);
        }
        else{
            tap_code(KC_VOLD);
        }
    }
    return false;
}
#endif


#if defined(OLED_ENABLE)
bool oled_task_user(void){
    led_t led_state = host_keyboard_led_state();
    oled_write_ln(
    led_state.caps_lock ? "CAPS LOCK" : "Normal",
    false
);
    return false;
}
#endif

#if defined(RGBLIGHT_ENABLE)
void keyboard_post_init_user(void)
{
    rgblight_setrgb(255, 0, 0);
}
#endif