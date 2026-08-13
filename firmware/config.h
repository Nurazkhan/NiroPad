#pragma once
#define I2C_DRIVER I2CD1
#define I2C1_SDA_PIN GP6
 #define I2C1_SCL_PIN GP7
#define RGBLED_NUM 20
// Ensure RGBLIGHT_LED_COUNT is available for drivers that expect it
#ifndef RGBLIGHT_LED_COUNT
#define RGBLIGHT_LED_COUNT RGBLED_NUM
#endif

// USB identity (required by the USB descriptor)
// You can change these to your actual VID/PID if you have them.
#ifndef VENDOR_ID
#define VENDOR_ID 0xFEED
#endif
#ifndef PRODUCT_ID
#define PRODUCT_ID 0x6060
#endif
#ifndef DEVICE_VER
#define DEVICE_VER 0x0001
#endif
#ifndef MANUFACTURER
#define MANUFACTURER "Nuraz"
#endif
#ifndef PRODUCT
#define PRODUCT "NiroPad"
#endif

#define ENCODER_A_PINS { GP29 }
#define ENCODER_B_PINS { GP3 }

#define WS2812_DI_PIN GP0

