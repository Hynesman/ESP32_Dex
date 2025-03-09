// structs to include
#include <RGBLED.h>
// Define the possible states
enum class State
{
    Launch_AP,
    Home_screen,
    Graph,
    Info,
    Settings
};

enum class Button
{
    NOTHING,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    SELECT,
    BACK,
    SNOOZE_PLUS,
    SNOOZE_MINUS
};
unsigned long debounceEndTime = 0;
int buttonDebounceMs = 300;

struct Homescreen_values
{
    unsigned long timestamp;
    double mmol_l=5.0;
    uint16_t mg_dl=100;
    const char *trend_description;
    const char *trend_Symbol;
    const char *message_1;
    const char *message_2;
    double minutes_since= 0.0;
    // Define other members as needed for your application
};

// Define the possible BG states
enum class BGState
{
    REALLY_HIGH,
    HIGHBS,
    LOWBS,
    REALLY_LOW
};

struct ALARM_STRUCT{
    String name;
    bool active;
    bool continuous;
    bool playsound;
    bool Blink;
    bool high_alarm;
    double level;
    int colorArrayPos; // RGB values for the LED color
    int soundName;
};

/*
struct ALARMS {
    ALARM_STRUCT HIGHHIGHBS{"ReallyHigh",true, true, true, true, true, 13.3}; // Example defaults
    ALARM_STRUCT HIGHBS{"High",true, true, true, true, true, 10.0};   // Additional examples
    ALARM_STRUCT LOWBS{"Low",true, true, true, true, false, 3.99};   // You can adjust these
    ALARM_STRUCT LOWLOWBS{"LowLow",true, true, true, true, false, 3.3};  // as needed
};*/

struct ALARMSV {
    std::vector<ALARM_STRUCT> alarms = {
        {"ReallyHigh", true, true, true, true, true, 13.3, 0, 3},
        {"High", true, true, true, true, true, 10.0, 3, 3},
        {"Low", true, true, true, true, false, 4.0, 7, 3},
        {"LowLow", true, true, true, true, false, 3.3, 0, 3}
    };
};
struct Status {
        bool missed_values;
        bool sdafas;
        bool someOtherStatus;
        // Add more status flags as needed
    };

// Define the RGB values for the colors
    const uint8_t colorValues[][3] = {
        {255, 0, 0},    // COLOR_RED 0
        {0, 128, 0},    // COLOR_GREEN 1
        {0, 0, 128},    // COLOR_BLUE 2
        {255, 85, 0},   // COLOR_YELLOW 3
        {0, 128, 128},  // COLOR_CYAN 4
        {128, 0, 128},  // COLOR_MAGENTA 5
        {170, 57, 57},  // COLOR_WHITE 6
        {64, 0, 64},    // COLOR_PURPLE 7
        {255, 43, 0},   // COLOR_ORANGE 8
        {0, 64, 64},    // COLOR_TEAL 9
        {255, 96, 102}, // COLOR_PINK 10
        {0, 255, 0},    // COLOR_LIME 11
        {47, 0, 81},    // COLOR_INDIGO 12
        {87, 22, 22},   // COLOR_BROWN 13
        {85, 85, 85}    // COLOR_GRAY 14 
    };

    // Define the names for the colors
    const char *colorNames[] = {"Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "White",
        "Purple", "Orange", "Teal", "Pink", "Lime", "Indigo", "Brown", "Gray"};

    // Assuming you know the number of colors
    int numberOfColors = sizeof(colorValues) / sizeof(colorValues[0]);