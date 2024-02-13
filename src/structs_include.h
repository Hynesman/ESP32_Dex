// structs to include

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
};

struct ALARMS {
    ALARM_STRUCT HIGHHIGHBS{"ReallyHigh",true, true, true, true, true, 13.3}; // Example defaults
    ALARM_STRUCT HIGHBS{"High",true, true, true, true, true, 10.0};   // Additional examples
    ALARM_STRUCT LOWBS{"Low",true, true, true, true, false, 3.99};   // You can adjust these
    ALARM_STRUCT LOWLOWBS{"LowLow",true, true, true, true, false, 3.3};  // as needed
};

struct ALARMSV {
    std::vector<ALARM_STRUCT> alarms = {
        {"ReallyHigh", true, true, true, true, true, 13.3},
        {"High", true, true, true, true, true, 10.0},
        {"Low", true, true, true, true, false, 3.99},
        {"LowLow", true, true, true, true, false, 3.3}
    };
};
struct Status {
        bool missed_values;
        bool sdafas;
        bool someOtherStatus;
        // Add more status flags as needed
    };