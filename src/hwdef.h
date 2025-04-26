/////////////////////////////////////////////////////////////////////////
//
// Define GPIO pins used by each board defined in platformio.ini
//
// When custom PCBs are designed, the same GPIO pins as those used
// on development boards can vary. The assignements are described here.
//
/////////////////////////////////////////////////////////////////////////

//
// Defines common across all board types
//

// Common OLED defs used across all boards with an SSD1306
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

//
// Debounce delay used to avoid multiple activations
// of user buttons due to switch bounce.
//
#define DEBOUNCE_DELAY 150

// If no special boards are defined, default
// back to original board used by original developer.
#if !defined HUZZAH32 && !defined DEXCOM_PCB

  #define PIN_BUILTIN_LED    13
  #define BUZZZER_PIN 18
  #define TRIGGER_AP_PIN 4

  // Buttons
  #define SELECT_PIN 14
  #define BACK_PIN 12
  #define RIGHT_PIN 33
  #define LEFT_PIN 25
  #define UP_PIN 27
  #define DOWN_PIN 26
  #define SNOOZE_PIN_PLUS 19
  #define SNOOZE_PIN_MINUS 23

  // i2c protocol support. Just use the default pins.
  #define ESP32_SCL SCL
  #define ESP32_SDA SDA

#endif

#ifdef HUZZAH32

  #define USE_PROJECTOR

  #define PIN_BUILTIN_LED 13
  #define BUZZZER_PIN 18
  #define TRIGGER_AP_PIN 4

  // Buttons
  #define SELECT_PIN 14
  #define BACK_PIN 12
  #define RIGHT_PIN 33
  #define LEFT_PIN 25
  #define UP_PIN 27
  #define DOWN_PIN 26
  #define SNOOZE_PIN_PLUS 19
  #define SNOOZE_PIN_MINUS 23

  #ifdef TYPE_TM1621D
    #define PIN_PROJECTOR_CS     15
    #define PIN_PROJECTOR_CLK    32
    #define PIN_PROJECTOR_DATA   14
  #endif

  #ifdef TYPE_ET6621S
    #define PIN_PROJECTOR_CS     14
    #define PIN_PROJECTOR_CLK    32
    #define PIN_PROJECTOR_DATA   15
  #endif

  // i2c protocol support
  #define ESP32_SCL 22
  #define ESP32_SDA 21

  // This board removes the RGB LED
  #define NO_RGBLED

#endif


#ifdef DEXCOM_PCB

  #define USE_PROJECTOR

  #define PIN_BUILTIN_LED      13
  #define BUZZZER_PIN          26
  #define TRIGGER_AP_PIN       0 //44  44 is a pin that will never trigger...0 is the boot switch

  // Populated buttons
  #define SELECT_PIN           33
  #define BACK_PIN             12
  #define RIGHT_PIN            42
  #define LEFT_PIN             14
  #define UP_PIN               19
  #define DOWN_PIN             41
  #define SNOOZE_PIN_PLUS      1
  #define SNOOZE_PIN_MINUS     2

  #define PIN_PROJECTOR_CS     15
  #define PIN_PROJECTOR_CLK    6
  #define PIN_PROJECTOR_DATA   7

  #define PIN_PROJECTOR_PWM    11

  // i2c protocol support
  #define ESP32_SCL 3
  #define ESP32_SDA 4

  // This board removes the RGB LED
  #define NO_RGBLED

  // ...and adds a 2.42" OLED and a TC74 temp sensor
  #define OLED_SSD1306_I2C_ADDRESS 0x3D
  #define TC74_I2C_ADDRESS 0x48
  
#endif

// If we have an RGB LED, be sure to indicate the pins used
#if !defined NO_RGBLED
  #define R_PIN 17
  #define G_PIN 16
  #define B_PIN 5
#endif

#ifdef USE_PROJECTOR
  //
  // Projector related details
  //

  //
  // RTOS task to update the Projector. Located
  // in projector.cpp and called from main.cpp.
  //
  void projectorUpdateTask(void *);

  // Correction used for onboard temp sensor (to indicate ambient room temp)
  #define TEMP_CORRECTION -8.5

#endif
