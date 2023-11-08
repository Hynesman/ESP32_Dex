#define ESP_DRD_USE_SPIFFS true
#include "Arduino.h"
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Dexcom_follow.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <logo.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "structs_include.h"
#include "RGBLED.h"

#include <melody_player.h>
#include <melody_factory.h>
#include "sounds.h"
#define BUZZZER_PIN 18
MelodyPlayer player(BUZZZER_PIN, 3, LOW);

bool AlarmsEnable = true;
bool Snoozing = false;
// int Snoozetime = 15; // minutes
double LowLowAlarm = 3.9;
double LowAlarm = 4.01;
double HighAlarm = 10.0;
double HighHighAlarm = 13.3;

#include <NTPClient.h>
#include <WiFiUdp.h>

// JSON configuration file
#define JSON_CONFIG_FILE "/config.json"

// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
char D_User[50] = "username";
char D_Pass[50] = "password";
// Text box (String) - 50 characters maximum
WiFiManagerParameter Dexcom_Username("Dexcom_User", "Dexcom username", D_User, 50);
WiFiManagerParameter Dexcom_Password("Dexcom_Password", "Dexcom Username", D_Pass, 50);

bool just_wait = false;
// Define WiFiManager Object
WiFiManager wm;
int timeout = 120;

#define R_PIN 17
#define G_PIN 16
#define B_PIN 5

RGBLED rgb(R_PIN, G_PIN, B_PIN);

#define TRIGGER_AP_PIN 4
bool Launch_AP = false;

int Hours_Shown_On_Graph = 3;
#define MAX_HOURS 24
#define MIN_HOURS 1

// Buttons
#define SELECT_PIN 14
#define BACK_PIN 12
#define RIGHT_PIN 33
#define LEFT_PIN 25
#define UP_PIN 27
#define DOWN_PIN 26
#define SNOOZE_PIN_PLUS 19
#define SNOOZE_PIN_MINUS 23
#define DEBOUNCE_DELAY 150

Button buttons = Button::NOTHING;
bool Next_screen = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int NTP_UTC_OFFSET = 0; // Replace with your UTC offset in seconds
const char *NTP_SERVER = "pool.ntp.org";
const long NTP_UPDATE_INTERVAL = 60 * 1000; // Update NTP time every 60 seconds

WiFiUDP udp;
NTPClient ntpClient(udp, NTP_SERVER, NTP_UTC_OFFSET, NTP_UPDATE_INTERVAL);

double glucoseValue;
String trend = "";

Follower follower(true);

int error_count = 0;

// Global variable to track the current state
State currentState = State::Home_screen;

Homescreen_values Home_values;

unsigned long SnoozeEndTime = 0;
const unsigned long SnoozeDuration = 5 * 60 * 1000; // min Snooze duration in milliseconds (e.g., 5 minutes)

void saveConfigFile()
// Save Config in JSON format
{
  Serial.println(F("Saving configuration..."));

  // Create a JSON document
  StaticJsonDocument<512> json;
  json["D_User"] = D_User;
  json["D_Pass"] = D_Pass;

  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    // Error, file did not open
    Serial.println("failed to open config file for writing");
  }

  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    // Error writing file
    Serial.println(F("Failed to write to file"));
  }
  // Close file
  configFile.close();
}

bool loadConfigFile()
// Load existing configuration file
{
  // Uncomment if we need to format filesystem
  // SPIFFS.format();

  // Read configuration from FS json
  Serial.println("Mounting File System...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE))
    {
      // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile)
      {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error)
        {
          Serial.println("Parsing JSON");

          strcpy(D_User, json["D_User"]);
          strcpy(D_Pass, json["D_Pass"]);
          Dexcom_Username.setValue(D_User, 50);
          Dexcom_Password.setValue(D_Pass, 50);

          return true;
        }
        else
        {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  }
  else
  {
    // Error mounting file system
    Serial.println("Failed to mount FS");
  }

  return false;
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager)
// Called when config mode launched
{
  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void IRAM_ATTR glucoseUpdateTask(void *pvParameters)
{
  for (;;)
  {

    int delay_time = 20;

    if (!just_wait)
    {
      // Fetch the latest glucose levels
      if (follower.GlucoseLevelsNow())
      {

        // Display the new glucose value and trend on the OLED display
        Home_values.timestamp = follower.GlucoseNow.timestamp;
        Home_values.mmol_l = follower.GlucoseNow.mmol_l;
        Home_values.trend_Symbol = follower.GlucoseNow.trend_Symbol;
        Home_values.message_2 = "";
        follower.GlucoseLevelsArrayPopulate();

        // figure out the delay to the next value
        unsigned long currentTime = ntpClient.getEpochTime();
        delay_time = follower.GlucoseNow.timestamp + (5 * 60) - currentTime;

        //daylight savings check
        if (delay_time < -60*60+1 || delay_time > 60*60-1)
        {
          ntpClient.setTimeOffset(follower.TZ_offset);
          unsigned long currentTime = ntpClient.getEpochTime();
          delay_time = follower.GlucoseNow.timestamp + (5 * 60) - currentTime;

        }

        if (delay_time < 0 && delay_time > -25)
        {
          delay_time = 2;
        }
        else if (delay_time <= -25 && delay_time > -35)
        {
          delay_time = 5;
        }
        else if (delay_time <= -35)
        {

          while (delay_time < 0)
          {
            delay_time += (5 * 60);
          }
          if (delay_time > ((5 * 60) - 25))
          {
            delay_time = 5; // delay 5 seconds a time for very late values
          }
        }
        error_count = 0;
      }
      else if (WiFi.status() != WL_CONNECTED)
      {
        Home_values.timestamp = follower.GlucoseNow.timestamp;
        Home_values.mmol_l = follower.GlucoseNow.mmol_l;
        Home_values.trend_Symbol = follower.GlucoseNow.trend_Symbol;
        Home_values.message_2 = "Wifi Error";

        // wm.autoConnect()
        if (!wm.autoConnect("ESP_AP"))
        {
          Serial.println("failed to connect and hit timeout");
          delay(3000);
          // if we still have not connected restart and try all over again
          ESP.restart();
          delay(5000);
        }
        delay_time = 5;
      }
      else
      {
        // Display the error
        error_count++;
        Home_values.timestamp = follower.GlucoseNow.timestamp;
        Home_values.mmol_l = follower.GlucoseNow.mmol_l;
        Home_values.trend_Symbol = follower.GlucoseNow.trend_Symbol;
        Home_values.message_2 = "Error last value";

        delay_time = 3;
        if (error_count > 10)
        {
          ESP.restart();
        }
      }
    }

    // Delay for seconds
    Serial.print("delaying for ");
    Serial.print(delay_time);
    Serial.println(" seconds");
    vTaskDelay(pdMS_TO_TICKS(delay_time * 1000));
  }
  return;
}

// button handlers
void IRAM_ATTR APintHandler()
{
  Launch_AP = true;
  currentState = State::Launch_AP;
}

void IRAM_ATTR RightPinHandler()
{
  if (debounceEndTime < millis())
  {
    buttons = Button::RIGHT;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void IRAM_ATTR SnoozeHandler()
{
  if (debounceEndTime < millis())
  {
    buttons = Button::SNOOZE_PLUS;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void IRAM_ATTR SnoozeHandler_Minus()
{
  if (debounceEndTime < millis())
  {
    buttons = Button::SNOOZE_MINUS;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void IRAM_ATTR LeftPinHandler()
{
  if (debounceEndTime < millis())
  {
    buttons = Button::LEFT;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void IRAM_ATTR UpPinHandler()
{

  if (debounceEndTime < millis())
  {
    buttons = Button::UP;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void IRAM_ATTR DownPinHandler()
{
  if (debounceEndTime < millis())
  {
    buttons = Button::DOWN;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void Access_point()
{
  vTaskDelay(pdMS_TO_TICKS(1 * 1000));
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connect to:");
  display.setTextSize(2);
  display.println(" \"ESP_AP\"");
  display.setTextSize(1);
  display.println("to choose Wifi and\nDexcom Credentials");
  display.display();
  vTaskDelay(pdMS_TO_TICKS(1 * 1000));
  just_wait = true;
  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  if (!wm.startConfigPortal("ESP_AP"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
  }
  else
  {
    // If we get here, we are connected to the WiFi

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Lets deal with the user config values

    // Copy the string value
    strncpy(D_User, Dexcom_Username.getValue(), sizeof(D_User));
    // Serial.print("D_User: ");
    // Serial.println(D_User);

    // Copy the string value
    strncpy(D_Pass, Dexcom_Password.getValue(), sizeof(D_Pass));
    // Serial.print("D_User: ");
    // Serial.println(D_Pass);

    follower.Set_user_pass(D_User, D_Pass);
    follower.getNewSessionID();

    // Save the custom parameters to FS
    if (shouldSaveConfig && follower.SessionIDnotDefault())
    {
      saveConfigFile();
      shouldSaveConfig = false;
    }
    else
    {
      loadConfigFile();
    }
    follower.Set_user_pass(D_User, D_Pass);

    follower.getNewSessionID();
    if (!follower.SessionIDnotDefault())
    {

      currentState = State::Launch_AP;
      just_wait = true;
    }
    else
    {

      currentState = State::Home_screen;
      just_wait = false;
    }
  }
  return;
}

const unsigned char *Trend_to_image(const char *trend_char)
{
  if (trend_char == nullptr)
  {
    return arrowsdash;
  }
  else if (strcmp(trend_char, "^^") == 0)
  {
    return arrowsupup;
  }
  else if (strcmp(trend_char, "^") == 0)
  {
    return arrowsup;
  }
  else if (strcmp(trend_char, "/^") == 0)
  {
    return arrowssup;
  }
  else if (strcmp(trend_char, "->") == 0)
  {
    return arrowseven;
  }
  else if (strcmp(trend_char, "\\v") == 0)
  {
    return arrowssdown;
  }
  else if (strcmp(trend_char, "v") == 0)
  {
    return arrowsdown;
  }
  else if (strcmp(trend_char, "vv") == 0)
  {
    return arrowsdowndown;
  }
  else if (strcmp(trend_char, "?") == 0)
  {
    return arrowsQ;
  }
  else if (strcmp(trend_char, "-") == 0)
  {
    return arrowsdash;
  }
  else
  {
    return arrowsQ;
  }
}

void normal_mode()
{
  if (Home_values.mmol_l > HighAlarm && Home_values.mmol_l < HighHighAlarm)
  {
    rgb.setColor(COLOR_YELLOW_DIMMEST);
  }
  else if (Home_values.mmol_l >= HighHighAlarm)
  {
    rgb.setColor(COLOR_RED_DIMMEST);
  }
  else if (Home_values.mmol_l <= LowAlarm && Home_values.mmol_l > LowLowAlarm)
  {
    rgb.setColor(COLOR_PURPLE_DIMMEST);
  }
  else if (Home_values.mmol_l <= LowAlarm)
  {
    rgb.setColor(COLOR_RED_DIMMEST);
  }
  else
  {
    rgb.turnOff();
  }
}

void High_Lights()
{
  if (Home_values.mmol_l > HighAlarm && Home_values.mmol_l < HighHighAlarm)
  {
    rgb.setColor(COLOR_YELLOW);
    if (!player.isPlaying())
          {
            player.playAsync(melody4);
          }
  }
  else if (Home_values.mmol_l >= HighHighAlarm)
  {
    rgb.setColor(COLOR_RED);
    if (!player.isPlaying())
          {
            player.playAsync(melody4);
          }
  }
  else if (Home_values.mmol_l <= LowAlarm && Home_values.mmol_l > LowLowAlarm)
  {
    rgb.setColor(COLOR_PURPLE);
    if (!player.isPlaying())
          {
            player.playAsync(melody4);
          }
  }
  else if (Home_values.mmol_l <= LowAlarm)
  {
    rgb.setColor(COLOR_RED);
    if (!player.isPlaying())
          {
            player.playAsync(melody4);
          }
  }
  else
  {
    rgb.turnOff();
    if (player.isPlaying())
          {
            player.stop();
          }
  }
}



void Alarm_handler(void *pvParameters)
{
  bool blinkbool = true;
  int count = 0;
  while (true)
  {
    if (!AlarmsEnable)
    {
      normal_mode();
    }
    else
    {
      if (Snoozing)
      {
        normal_mode();
        if (player.isPlaying())
          {
            player.stop();
          }
      }
      else
      {
        if (blinkbool)
        {
          High_Lights();
        }
        else
        {
          normal_mode();

        }
        if (count >= 1)
        {
          blinkbool = !blinkbool;
          count = 0;
        }
        else
        {
          count++;
        }
      }
    }

    // Check for button signal to activate snooze
    if (buttons == Button::SNOOZE_PLUS) // Implement this function to detect button press
    {
      buttons = Button::NOTHING;
      if (Snoozing)
      {
        SnoozeEndTime += SnoozeDuration;
      }
      else
      {
        Snoozing = true;
        SnoozeEndTime = millis() + SnoozeDuration;
      }
    }
    else if (buttons == Button::SNOOZE_MINUS){
      buttons = Button::NOTHING;
      if (Snoozing){
        SnoozeEndTime -= SnoozeDuration;
        if (SnoozeEndTime < millis()){
          Snoozing = false;
        }
      }
    }

    // Check if snooze timer has expired
    if (Snoozing && millis() >= SnoozeEndTime)
    {
      Snoozing = false;
      SnoozeEndTime = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void Homescreen_display()
{
  unsigned long currentTime = ntpClient.getEpochTime();
  int time_since_last = int((currentTime - Home_values.timestamp) / 60.0);
  // Convert int to String and concatenate with another string
  String message = String(time_since_last) + "\nmin ago";

  // Convert the String to a C-style string (const char*) using c_str()
  Home_values.message_1 = message.c_str();
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  char glucoseStr[10];                           // Allocate a buffer for the formatted string
  dtostrf(Home_values.mmol_l, 2, 1, glucoseStr); // Format: total width = 4, 1 decimal place

  display.print(glucoseStr);
  int offset1 = display.getCursorX();
  display.println();

  // display.print(" ");  // using custom symbols instead of this code
  // display.setTextSize(3);
  // display.print(Home_values.trend_Symbol);
  // display.setTextSize(4);
  // display.println("");
  // display.setCursor(0,40);
  display.setTextSize(1);
  int offset2 = display.getCursorY();

  if (time_since_last > 10)
  {
    display.drawFastHLine(0, offset2 / 2, offset1, WHITE);
    display.drawFastVLine(offset1 / 2, 0, offset2, WHITE);
    display.drawLine(0, 0, offset1, offset2, WHITE);
    display.drawLine(0, offset2, offset1, 0, WHITE);
  }

  display.println(Home_values.message_1);
  display.setTextSize(2);
  String somemessage = "";
  if (Snoozing)
  {
    somemessage = String((int)((SnoozeEndTime - millis()) / (1000 * 60)) + 1) + " min Szz";
    Home_values.message_2 = somemessage.c_str();
  }
  else
  {
    Home_values.message_2 = "";
  }
  display.println(Home_values.message_2);
  display.setCursor(128 / 2, offset2);
  display.setTextSize(2);
  int hours = ntpClient.getHours();
  String formattedHours = (hours < 10) ? "0" + String(hours) : String(hours);
  display.print(formattedHours);
  display.print(":");
  int minutes = ntpClient.getMinutes();
  String formattedMinutes = (minutes < 10) ? "0" + String(minutes) : String(minutes);
  display.print(formattedMinutes);
  // const unsigned char *trend_sym = Trend_to_image(Home_values.trend_Symbol);
  display.drawBitmap(SCREEN_WIDTH - 25, 0, Trend_to_image(Home_values.trend_Symbol), 24, 24, WHITE);
  display.display();
  return;
}

void Settings()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Settings");
  display.display();
}

unsigned int selectedMenuItem = 0;
const int numMenuItems = 5;
const char *menuItems[numMenuItems] = {"Settings", "Item 1", "Item 2", "Item 3", "Item 4"};

void displayMenu(unsigned int selectedItem)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  for (int i = 0; i < 4; i++)
  {
    if ((selectedItem + i) < numMenuItems)
    {
      int item = (selectedItem + i) % numMenuItems;
      if (item == selectedItem)
      {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Highlight selected item
      }
      else
      {
        display.setTextColor(SSD1306_WHITE);
      }

      // display.setCursor(0, i * 16);
      display.println(menuItems[item]);
    }
  }

  display.display();
}

void Graph_Display()
{
  // follower.GlucoseLevelsArrayPopulate();
  double datapointsY[CASHED_READINGS];
  double YMax = 12.0;
  double YMin = 2.0;
  unsigned long datapointsX[CASHED_READINGS];
  unsigned long XMax = ntpClient.getEpochTime();
  unsigned long XMin = XMax - (Hours_Shown_On_Graph * 60 * 60);

  for (int i = CASHED_READINGS - 1; i >= 0; i--)
  {
    datapointsY[i] = follower.GlucoseArray[i].mmol_l;
    if (datapointsY[i] > YMax)
      YMax = datapointsY[i];
    if (datapointsY[i] < YMin)
      YMin = datapointsY[i];
    datapointsX[i] = follower.GlucoseArray[i].timestamp;
  }

  display.clearDisplay();

  // Draw graph axes
  display.drawLine(0, 0, 0, SCREEN_HEIGHT - 1, SSD1306_WHITE);
  display.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, SSD1306_WHITE);

  // Calculate scaling factors
  float xScale = float(SCREEN_WIDTH - 1) / (CASHED_READINGS - 1);
  float yScale = float(SCREEN_HEIGHT - 1);

  // Plot data points
  for (int i = CASHED_READINGS - 1; i >= 0; i--)
  {
    // int x = int((SCREEN_WIDTH - 1) - (i * xScale));
    if (datapointsX[i] >= XMin)
    {
      int x = int((datapointsX[i] - XMin) * (SCREEN_WIDTH - 1) / (XMax - XMin));
      int y = int(yScale * (1 - (datapointsY[i] - YMin) / (YMax - YMin)));
      display.drawPixel(x, y, SSD1306_WHITE);
      display.drawPixel(x - 1, y, SSD1306_WHITE);
      display.drawPixel(x + 1, y, SSD1306_WHITE);
      display.drawPixel(x, y - 1, SSD1306_WHITE);
      display.drawPixel(x, y + 1, SSD1306_WHITE);

      // display.drawRect(x - 1, y - 1, 3, 3, SSD1306_WHITE);
    }
  }

  // Draw vertical gridlines
  unsigned long lastPerfectHourTimestamp = XMax - (XMax % 3600);

  // int lasthour = int((lastPerfectHourTimestamp - XMin) * (SCREEN_WIDTH - 1) / (XMax - XMin));
  for (lastPerfectHourTimestamp; lastPerfectHourTimestamp > XMin;)
  {
    int lasthour = int((lastPerfectHourTimestamp - XMin) * (SCREEN_WIDTH - 1) / (XMax - XMin));
    for (int j = 0; j < SCREEN_HEIGHT; j += 4) // Draw dotted line every 4 pixels
    {
      display.drawPixel(lasthour, j, SSD1306_WHITE);
    }
    lastPerfectHourTimestamp -= 3600;
  }

  // Draw horizontal gridlines

  int LowLimit = int(yScale * (1 - (4 - YMin) / (YMax - YMin)));
  for (int j = 0; j < SCREEN_WIDTH; j += 4) // Draw dotted line
  {
    display.drawPixel(j, LowLimit, SSD1306_WHITE);
  }
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(1, LowLimit - 4);
  display.setTextSize(1);
  display.print("4");

  int HighLimit = int(yScale * (1 - (10 - YMin) / (YMax - YMin)));
  for (int j = 0; j < SCREEN_WIDTH; j += 4) // Draw dotted line
  {
    display.drawPixel(j, HighLimit, SSD1306_WHITE);
  }
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(1, HighLimit - 4);
  display.setTextSize(1);
  display.print("10");

  display.display();
}

void StateLoopTask(void *pvParameters)
{

  while (true)
  {
    switch (currentState)
    {
    case State::Launch_AP:
      Access_point();
      break;
    case State::Home_screen:

      if (buttons == Button::RIGHT)
      {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));
        buttons = Button::NOTHING;
        currentState = State::Graph;
        break;
      }
      else if (buttons == Button::LEFT)
      {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));
        buttons = Button::NOTHING;
        currentState = State::Settings;
        break;
      }
      else if (buttons == Button::UP)
      {
        SnoozeEndTime += 5 * 60 * 1000;
        buttons = Button::NOTHING;
      }
      else if (buttons == Button::DOWN)
      {
        unsigned long newSnoozeEndTime = SnoozeEndTime - 5 * 60 * 1000;
        if (newSnoozeEndTime < millis()){
          SnoozeEndTime = millis();
          Snoozing = false;
        }
        else{
          SnoozeEndTime = newSnoozeEndTime;
        }
        buttons = Button::NOTHING;
      }
      Homescreen_display();
      break;
    case State::Graph:

      if (buttons == Button::RIGHT)
      {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));
        buttons = Button::NOTHING;
        currentState = State::Settings;
        break;
      }
      else if (buttons == Button::LEFT)
      {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));
        buttons = Button::NOTHING;
        currentState = State::Home_screen;
        break;
      }
      else if (buttons == Button::DOWN)
      {
        if (Hours_Shown_On_Graph > MIN_HOURS)
        {
          Hours_Shown_On_Graph = (Hours_Shown_On_Graph - 1) % MAX_HOURS;
        }
        buttons = Button::NOTHING;
      }
      else if (buttons == Button::UP)
      {
        Hours_Shown_On_Graph = (Hours_Shown_On_Graph + 1) % MAX_HOURS;
        buttons = Button::NOTHING;
      }
      Graph_Display();
      // stateFunction3();
      break;
    case State::Settings:
      if (buttons == Button::RIGHT)
      {
        buttons = Button::NOTHING;
        currentState = State::Home_screen;
        break;
      }
      else if (buttons == Button::LEFT)
      {
        buttons = Button::NOTHING;
        currentState = State::Graph;
        break;
      }
      else if (buttons == Button::DOWN)
      {
        selectedMenuItem = (selectedMenuItem + 1) % numMenuItems;
        buttons = Button::NOTHING;
      }
      else if (buttons == Button::UP)
      {
        selectedMenuItem = (selectedMenuItem - 1) % numMenuItems;
        buttons = Button::NOTHING;
      }
      displayMenu(selectedMenuItem);
      // Settings();
      //  stateFunction4();
      break;
    case State::Info:
      // stateFunction5();
      break;
    default:
      // Handle an invalid state
      Serial.println("Invalid state encountered");
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup()
{
  Serial.begin(115200);

  Wire.begin();

  // rgb
  rgb.setColor(COLOR_PURPLE_DIM); // RED
  //player.playAsync(lowToneBeepMelody);
  // Buttons
  // AP manual trigger
  pinMode(TRIGGER_AP_PIN, INPUT);
  attachInterrupt(TRIGGER_AP_PIN, APintHandler, RISING);

  pinMode(SELECT_PIN, INPUT);
  //attachInterrupt(SELECT_PIN, SnoozeHandler, FALLING);
  pinMode(SNOOZE_PIN_PLUS, INPUT);
  attachInterrupt(SNOOZE_PIN_PLUS, SnoozeHandler, FALLING);
  pinMode(SNOOZE_PIN_MINUS, INPUT);
  attachInterrupt(SNOOZE_PIN_MINUS, SnoozeHandler_Minus, FALLING);
  // Display toggle
  pinMode(RIGHT_PIN, INPUT);
  attachInterrupt(RIGHT_PIN, RightPinHandler, FALLING);
  pinMode(LEFT_PIN, INPUT);
  attachInterrupt(LEFT_PIN, LeftPinHandler, FALLING);
  pinMode(UP_PIN, INPUT);
  attachInterrupt(UP_PIN, UpPinHandler, FALLING);
  pinMode(DOWN_PIN, INPUT);
  attachInterrupt(DOWN_PIN, DownPinHandler, FALLING);

  // Buzzer Test at startup
  // player.playAsync(melody3);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  // display.drawBitmap(0, 0, Dexcom_follow_screen, 128, 64, WHITE);
  display.drawBitmap(0, 0, logo2, 128, 64, WHITE);

  display.display();

  // Change to true when testing to force configuration every time we run
  bool forceConfig = false;

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup)
  {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }
  // player.playAsync(yourMelody);

  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);

  delay(5000);

  // Reset settings (only for development)
  // wm.resetSettings();

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  // Add all defined parameters
  wm.addParameter(&Dexcom_Username);
  wm.addParameter(&Dexcom_Password);

  // rgb
  rgb.setColor(COLOR_BLUE_DIM); // Blue
  if (forceConfig)
  // Run if we need a configuration
  {
    if (!wm.startConfigPortal("ESP_AP"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  }
  else
  {
    if (!wm.autoConnect("ESP_AP"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }

  // If we get here, we are connected to the WiFi

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Lets deal with the user config values

  // Copy the string value
  strncpy(D_User, Dexcom_Username.getValue(), sizeof(D_User));
  Serial.print("D_User: ");
  Serial.println(D_User);

  // Convert the number value
  strncpy(D_Pass, Dexcom_Password.getValue(), sizeof(D_Pass));
  Serial.print("D_Pass: ");
  Serial.println(D_Pass);

  follower.Set_user_pass(D_User, D_Pass);
  follower.getNewSessionID();
  // Save the custom parameters to FS
  if (shouldSaveConfig && follower.SessionIDnotDefault())
  {
    saveConfigFile();
    shouldSaveConfig = false;
  }
  else
  {
    loadConfigFile();
  }
  follower.Set_user_pass(D_User, D_Pass);
  // Initialize NTPClient
  ntpClient.begin();
  ntpClient.update();

  follower.getNewSessionID();
  if (!follower.SessionIDnotDefault())
  {
    Launch_AP = true;
    currentState = State::Launch_AP;
  }
  else
  {
    currentState = State::Home_screen;
  }
  follower.GlucoseLevelsArrayPopulate();
  //ntpClient.setTimeOffset(follower.TZ_offset);
  //ntpClient.update();
  // rgb.setColor(COLOR_YELLOW_DIM); //Yellow

  // Create and start the glucose update task
  xTaskCreate(glucoseUpdateTask, "GlucoseUpdateTask", 8192, NULL, 2, NULL);
  xTaskCreate(StateLoopTask, "StateLoop", 8192, NULL, 1, NULL);
  xTaskCreate(Alarm_handler, "Alarm_handler", 8192, NULL, 3, NULL);
}

void loop()
{
  // do nothing in the loop. just loops
  //player.playAsync(lowToneBeepMelody);
  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(5 * 1000));
  }
}
