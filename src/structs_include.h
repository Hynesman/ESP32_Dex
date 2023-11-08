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
    // Define other members as needed for your application
};

// Define the possible BG states
enum class BGState
{
    LOWLOWBS,
    LOWBS,
    NORMALBS,
    HIGHBS,
    HIGHHIGHBS
};
struct Status {
        bool missed_values;
        bool sdafas;
        bool someOtherStatus;
        // Add more status flags as needed
    };