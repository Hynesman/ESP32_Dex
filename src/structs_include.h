// structs to include

// Define the possible states
enum class State
{
    Launch_AP,
    Home_screen,
    Graph,
    Info,
    HighAlarm,
    LowAlarm
};

struct Homescreen_values
{
    unsigned long timestamp;
    double mmol_l;
    const char *trend_description;
    const char *trend_Symbol;
    const char *message_1;
    const char *message_2;
    // Define other members as needed for your application
};