#define ESP_DRD_USE_SPIFFS true
#include "hwdef.h"
#include "Arduino.h"
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Fonts/TomThumb.h>
#include <Adafruit_SSD1306.h>
#include "Dexcom_follow.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <logo.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WebServer.h>
#include "Html_scripts.h"
// OTA web Server - Port 81 since WifiManager uses port 80
WebServer server(81);

/////////////////////////////////////////////////////////
// Following for determining the timezone based on GeoIP 
//
#include "esp_http_client.h"
#include <HTTPClient.h>
#define MAX_HTTP_RECV_BUFFER 512
//
/////////////////////////////////////////////////////////

#include "structs_include.h"
#if !defined NO_RGBLED
#include "RGBLED.h"
#endif

#include <melody_player.h>
#include <melody_factory.h>
#include "sounds.h"

MelodyPlayer player(BUZZZER_PIN, 3, LOW);

bool AlarmsEnable = true;
bool Snoozing = false;
// int Snoozetime = 15; // minutes

#include <NTPClient.h>
#include <WiFiUdp.h>

// JSON configuration file
#define JSON_CONFIG_FILE "/config.json"

// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
char D_User[50] = "";
char D_Pass[50] = "";

// ALARMS alarms;
ALARMSV alarmsV;
int numMenuItems = alarmsV.alarms.size();

#define DOUBLE_STRING_SIZE 10
#define DEXCOM_CREDS_SIZE 50
// Convert double to string
char buffer[DOUBLE_STRING_SIZE];

// Located outside of USA?
bool OutsideUsa = true;
char customHtml_checkbox_outside_usa[20] = "type=\"checkbox\"";

// Text box (String) - 50 characters maximum
WiFiManagerParameter Dexcom_Username("Dexcom_User", "Dexcom Username", D_User, DEXCOM_CREDS_SIZE);
WiFiManagerParameter Dexcom_Password("Dexcom_Password", "Dexcom Password", D_Pass, DEXCOM_CREDS_SIZE);
// Checkbox to indicate location (in/outside USA)
WiFiManagerParameter custom_outside_usa_checkbox("OutsideUSA", "Located Outside USA", "T", 2, customHtml_checkbox_outside_usa, WFM_LABEL_AFTER);
WiFiManagerParameter htmlLineBreak("</br></br>");
#ifdef USE_PROJECTOR
// Text boxes for OpenWeatherMap details
#define OWM_API_KEY_SIZE 48
char OWM_API_Key[OWM_API_KEY_SIZE];
WiFiManagerParameter OWM_Key_text("OWMAPIKey", "OWM API Key", OWM_API_Key, OWM_API_KEY_SIZE);
#define OWM_POSTAL_SIZE 16
char OWM_Postal_Code[OWM_POSTAL_SIZE];
WiFiManagerParameter OWM_Postal_text("OWMPostal", "OWM Postal Code", OWM_Postal_Code, OWM_POSTAL_SIZE);
#define OWM_COUNTRY_SIZE 8
char OWM_Country[OWM_COUNTRY_SIZE];
WiFiManagerParameter OWM_Country_text("OWMCountry", "OWM Country", OWM_Country, OWM_COUNTRY_SIZE);

String openWeatherMapApiKey;
String openWeatherMapPostal;
String openWeatherMapCountry;

#endif

std::vector<WiFiManagerParameter *> customParameters;
// Text boxes for double values
WiFiManagerParameter HIGHHIGH_ALARM(alarmsV.alarms[0].name.c_str(), alarmsV.alarms[0].name.c_str(), dtostrf(alarmsV.alarms[0].level, 1, 2, buffer), DOUBLE_STRING_SIZE);
WiFiManagerParameter HIGH_ALARM(alarmsV.alarms[1].name.c_str(), alarmsV.alarms[1].name.c_str(), dtostrf(alarmsV.alarms[1].level, 1, 2, buffer), DOUBLE_STRING_SIZE);
WiFiManagerParameter LOW_ALARM(alarmsV.alarms[2].name.c_str(), alarmsV.alarms[2].name.c_str(), dtostrf(alarmsV.alarms[2].level, 1, 2, buffer), DOUBLE_STRING_SIZE);
WiFiManagerParameter LOWLOW_ALARM(alarmsV.alarms[3].name.c_str(), alarmsV.alarms[3].name.c_str(), dtostrf(alarmsV.alarms[3].level, 1, 2, buffer), DOUBLE_STRING_SIZE);

bool just_wait = false;
// Define WiFiManager Object
WiFiManager wm;
int timeout = 120;

#if !defined NO_RGBLED
RGBLED rgb(R_PIN, G_PIN, B_PIN);
#endif

bool Launch_AP = false;

int Hours_Shown_On_Graph = 3;
#define MAX_HOURS 24
#define MIN_HOURS 1


Button buttons = Button::NOTHING;
bool Next_screen = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int NTP_UTC_OFFSET = 0; // Replace with your UTC offset in seconds
const char *NTP_SERVER = "pool.ntp.org";
const long NTP_UPDATE_INTERVAL = 60 * 1000; // Update NTP time every 60 seconds
static int lastHour = 0; // Detect when the time changes to 2:00AM

WiFiUDP udp;
NTPClient ntpClient(udp, NTP_SERVER, NTP_UTC_OFFSET, NTP_UPDATE_INTERVAL);

double glucoseValue;
String trend = "";

Follower follower(OutsideUsa);

int error_count = 0;

// Global variable to track the current state
State currentState = State::Home_screen;

Homescreen_values Home_values;

unsigned long SnoozeEndTime = 0;
const unsigned long SnoozeDuration = 5 * 60 * 1000; // min Snooze duration in milliseconds (e.g., 5 minutes)

// possible changable values
double SIGNAL_LOST_MIN = 10.01;

// Function to set the timezone offset in the Follower object
void SetFollowerOffset(int offset)
{
  Serial.printf ("SetFollowerOffset: Timezone offset: %d\n", offset);
  follower.TZ_offset = offset;
  ntpClient.setTimeOffset(follower.TZ_offset);
}

int lookupGeoIpTimezone()
{
  char *timeServer = (char *)"http://worldtimeapi.org/api/ip";
  char *buffer = (char *)malloc(MAX_HTTP_RECV_BUFFER + 1);

  esp_http_client_config_t config = {
      .url = timeServer,
      .path = "/"
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t err;
  if ((err = esp_http_client_open(client, 0)) != ESP_OK)
  {
    Serial.printf ("lookupGeoIpTimezone: Failed to open HTTP connection: %s\n", esp_err_to_name(err));
    free(buffer);
    return -1;
  }
  int content_length =  esp_http_client_fetch_headers(client);
  int total_read_len = 0, read_len;
  if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER)
  {
      read_len = esp_http_client_read(client, buffer, content_length);
      if (read_len <= 0)
      {
          Serial.printf ("lookupGeoIpTimezone: Failure during HTTP data read\n");
      }
      buffer[read_len] = 0;
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  // Right now, the return string is ~349 bytes long. If they change the
  // format of the data returned, this valu eof 380 may not be enough.
  DynamicJsonDocument tzdoc(380);
  deserializeJson(tzdoc, buffer);

  int offset = tzdoc["raw_offset"];

  // "raw_offset" does not change with daylight savings time, but 
  //utc_offset does. Ex UTC Offset field: "utc_offset": "-04:00"
  char _offs[16];
  int hr, min;
  strcpy(_offs, tzdoc["utc_offset"]);
  int cnt = sscanf (_offs, "%d:%d", &hr, &min);
  offset = (hr * 60 * 60) + (min * 60);
  if (cnt != 2)
  {
    Serial.printf ("lookupGeoIpTimezone: Invalid timezone offset: %s\n", _offs);
    offset = 0;
  }

  free(buffer);

  SetFollowerOffset(offset);

  return offset;
}

bool DisplayDim = false;

void dimDisplay()
{
  int hours;
  bool setDisplayDim;

  hours = ntpClient.getHours();

  // Between 10:00PM and 7:00AM
  if ((hours >= 22) || (hours < 7))
    setDisplayDim = true;
  else
    setDisplayDim = false;

  if ((DisplayDim == false) && (setDisplayDim == true))
  {
    Serial.printf ("Dimming OLED display\n");
    DisplayDim = true;
    display.dim(true);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
  }

  if ((DisplayDim == true) && (setDisplayDim == false))
  {
    Serial.printf ("Brightening OLED display\n");
    DisplayDim = false;
    display.dim(true);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(255);
  }
}

void startEnhancedOTAWebServer()
{
  server.on("/", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", uploadPage); });
  // Serve files from the root directory ("/") of SPIFFS
  server.on(
      "/update", HTTP_POST, []()
      {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", Update.hasError() ? failedPage : successPage);
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          {
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });


  // Serve files from the root directory ("/") of SPIFFS
  server.serveStatic("/", SPIFFS, "/");
  server.begin();
}

bool Load_from_WM()
{
  strcpy(D_User, Dexcom_Username.getValue());
  strcpy(D_Pass, Dexcom_Password.getValue());
#ifdef USE_PROJECTOR
  strcpy(OWM_API_Key, OWM_Key_text.getValue());
  strcpy(OWM_Postal_Code, OWM_Postal_text.getValue());
  strcpy(OWM_Country, OWM_Country_text.getValue());
  openWeatherMapApiKey = OWM_API_Key;
  openWeatherMapPostal = OWM_Postal_Code;
  openWeatherMapCountry = OWM_Country;
#endif

  OutsideUsa = (strncmp(custom_outside_usa_checkbox.getValue(), "T", 1) == 0);
  follower.update_location(OutsideUsa);

  // For double values, convert from string to double
  for (u8_t i = 0; i < numMenuItems; i++)
  {
    alarmsV.alarms[i].level = atof(customParameters[i]->getValue());
  }

  return true;
}

String serializeAlarmStruct(const ALARM_STRUCT &alarm)
{
  StaticJsonDocument<256> doc; // Adjust size as needed
  doc["name"] = alarm.name;
  doc["active"] = alarm.active;
  doc["continuous"] = alarm.continuous;
  doc["playsound"] = alarm.playsound;
  doc["Blink"] = alarm.Blink;
  doc["high_alarm"] = alarm.high_alarm;
  doc["level"] = alarm.level;
  doc["colorArrayPos"] = alarm.colorArrayPos;
  doc["soundName"] = alarm.soundName;
  String output;
  serializeJson(doc, output);
  return output;
}

void saveConfigFile()
// Save Config in JSON format
{
  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount FS");
    return;
  }
  Serial.println("Mounted File System");
  Serial.println(F("Saving configuration..."));

  char buffer[DOUBLE_STRING_SIZE];

  // Create a JSON document
  StaticJsonDocument<1120> json;
  json["D_User"] = D_User;
  json["D_Pass"] = D_Pass;

  json["OutsideUsa"] = OutsideUsa;
#ifdef USE_PROJECTOR
  json["OWM_API"] = OWM_API_Key;
  json["OWM_Postal"] = OWM_Postal_Code;
  json["OWM_Country"] = OWM_Country;
#endif

  // Serialize ALARMS struct
  JsonObject alarmsJson = json.createNestedObject("alarms");
  for (u8_t i = 0; i < numMenuItems; i++)
  {
    alarmsJson[alarmsV.alarms[i].name] = serializeAlarmStruct(alarmsV.alarms[i]);
  }

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

bool updateOrAppendAlarm(ALARMSV &alarmsV, const ALARM_STRUCT &newAlarm)
{
  for (auto &alarm : alarmsV.alarms)
  {
    if (strcmp(alarm.name.c_str(), newAlarm.name.c_str()) == 0)
    {
      // Found an existing alarm with the same name, update it
      alarm = newAlarm;
      return true; // Alarm updated
    }
  }
  // No existing alarm found, append the new alarm
  alarmsV.alarms.push_back(newAlarm);
  return false; // New alarm added
}

void deserializeAlarmStruct(const JsonObject &json, ALARM_STRUCT &alarm)
{
  alarm.name = json["name"].as<const char *>(); // Assuming 'name' is part of the JSON structure. Adjust as necessary.
  alarm.active = json["active"].as<bool>();
  alarm.continuous = json["continuous"].as<bool>();
  alarm.playsound = json["playsound"].as<bool>();
  alarm.Blink = json["Blink"].as<bool>();
  alarm.high_alarm = json["high_alarm"].as<bool>();
  alarm.level = json["level"].as<double>();
  alarm.colorArrayPos = json["colorArrayPos"].as<int>();
  alarm.soundName = json["soundName"].as<int>();
}

bool loadConfigFile()
{
  // SPIFFS.format();
  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount FS");
    return false;
  }
  Serial.println("Mounted File System");
  if (SPIFFS.exists(JSON_CONFIG_FILE))
  {
    File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
    if (!configFile)
    {
      Serial.println("Failed to open configuration file");
      return false;
    }
    StaticJsonDocument<2044> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close(); // Close the file as soon as it's no longer needed

    if (error)
    {
      Serial.print("Failed to parse configuration file: ");
      Serial.println(error.c_str());
      return false;
    }

    // Dump the JSON data to the serial port
    serializeJsonPretty(doc, Serial);

    // Deserialize user and password as before...
    strcpy(D_User, doc["D_User"]);
    strcpy(D_Pass, doc["D_Pass"]);

    OutsideUsa = doc["OutsideUsa"].as<bool>();
#ifdef USE_PROJECTOR
    strcpy (OWM_API_Key, doc["OWM_API"]);
    strcpy (OWM_Postal_Code, doc["OWM_Postal"]);
    strcpy (OWM_Country, doc["OWM_Country"]);
#endif

    JsonObject alarmsObject = doc["alarms"].as<JsonObject>();
    for (JsonPair kv : alarmsObject)
    {
      const char *key = kv.key().c_str();
      String alarmJsonStr = kv.value().as<String>();

      StaticJsonDocument<550> alarmDoc;
      auto deserializeError = deserializeJson(alarmDoc, alarmJsonStr);
// Moved the JSON dum to above where all JSON data is shown.
//      serializeJsonPretty(alarmDoc, Serial);
      if (deserializeError)
      {
        Serial.print("Failed to parse alarm: ");
        Serial.println(key);
        continue; // Skip this alarm if parsing failed
      }

      ALARM_STRUCT tempAlarm;
      deserializeAlarmStruct(alarmDoc.as<JsonObject>(), tempAlarm);
      tempAlarm.name = key; // Assign the name from the JSON object's key
      updateOrAppendAlarm(alarmsV, tempAlarm);
    }

    Serial.println("");
    Serial.println("Configuration loaded successfully");
    return true;
  }
  else
  {
    Serial.println("Configuration file does not exist");
    return false;
  }
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
  Load_from_WM();
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
        Home_values.mg_dl = follower.GlucoseNow.mg_dl;
        Home_values.trend_Symbol = follower.GlucoseNow.trend_Symbol;
        Home_values.message_2 = "";
        follower.GlucoseLevelsArrayPopulate();

        // figure out the delay to the next value
        unsigned long currentTime = ntpClient.getEpochTime();
        //
        // It has been observed the USA Dexcom server always presents
        // in UTC time. Therefore, remove the TZ_offset if the timestamps
        // do not make sense.
        //
        if (currentTime < follower.GlucoseNow.timestamp)
          currentTime -= follower.TZ_offset;

        delay_time = follower.GlucoseNow.timestamp + (5 * 60) - currentTime;

        // daylight savings check
        if (delay_time < -60 * 60 + 30 || delay_time > 60 * 60 - 30)
        {
          ntpClient.setTimeOffset(follower.TZ_offset);
          // Serial.println("setoffset");
          ntpClient.update();
          vTaskDelay(pdMS_TO_TICKS(200)); // Little offset for some reason makes this work!!?  I think it gives that ntpClient thread time to work.
          currentTime = ntpClient.getEpochTime();
          if (currentTime < follower.GlucoseNow.timestamp)
            currentTime -= follower.TZ_offset;
          delay_time = follower.GlucoseNow.timestamp + (5 * 60) - currentTime;
        }

        // update minutes_since for signal_loss notify
        Home_values.minutes_since = (currentTime - follower.GlucoseNow.timestamp) / 60.0; // only updated every check, relative to timestamp
        Serial.printf ("Minutes since: %0.2f\n", Home_values.minutes_since);

        //
        // The logic here is when we fetch an update from Dexcom, we may be slightly
        // behind the sharing device, like the Dexcom phone app or xDrip that is
        // feeding Dexcom with data asynchronously every 5 mins. So when we pull an
        // update, we try again to get a new reading from Dexcom so the displayed BG
        // reading is not delayed 5 mins.
        //
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
        Home_values.mg_dl = follower.GlucoseNow.mg_dl;
        Home_values.trend_Symbol = follower.GlucoseNow.trend_Symbol;
        Home_values.message_2 = "Wifi Error";

        // wm.autoConnect()
        if (!wm.autoConnect("Glucose Follower"))
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
        Home_values.mg_dl = follower.GlucoseNow.mg_dl;
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
    // Using min() to avoid crazy values when SNTP connect fails, like on a DNS lookup fail
    Serial.printf ("Delaying for %d seconds\n\n", min(delay_time, 300));
    vTaskDelay(pdMS_TO_TICKS(min(delay_time, 300) * 1000));
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

void IRAM_ATTR SelectPinHandler()
{
  if (debounceEndTime < millis())
  {
    buttons = Button::SELECT;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void IRAM_ATTR BackPinHandler()
{
  if (debounceEndTime < millis())
  {
    buttons = Button::BACK;
    debounceEndTime = millis() + buttonDebounceMs;
  }
}

void Access_point()
{
  // vTaskDelay(pdMS_TO_TICKS(1 * 1000));
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connect to:");
  display.setTextSize(2);
  display.println(" \"Glucose Follower\"");
  display.setTextSize(1);
  display.println("to choose Wifi and\nDexcom Credentials");
  display.display();
  vTaskDelay(pdMS_TO_TICKS(1 * 1000));
  just_wait = true;
  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);

  //@@@
  // wm.setDebugOutput(true, WM_DEBUG_VERBOSE);

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  if (!wm.startConfigPortal("Glucose Follower"))
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
    Load_from_WM();

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
  //@@@
  // wm.setDebugOutput(true, WM_DEBUG_NOTIFY);
  return;
}

bool isAlarmConditionMet(const ALARM_STRUCT &alarm, double currentGlucoseLevel)
{
  if (alarm.high_alarm && currentGlucoseLevel > alarm.level)
  {
    return true;
  }
  else if (!alarm.high_alarm && currentGlucoseLevel < alarm.level)
  {
    return true;
  }
  return false;
}

const ALARM_STRUCT *getMostCriticalActiveAlarm(double currentGlucoseLevel)
{
  const ALARM_STRUCT *mostCriticalAlarm = nullptr;
  double minExceedance = 20; // Initialize with zero to track the maximum exceedance of the threshold.

  for (const auto &alarm : alarmsV.alarms)
  {
    if (alarm.active && isAlarmConditionMet(alarm, currentGlucoseLevel))
    {
      double exceedance = -1;
      if (alarm.high_alarm && currentGlucoseLevel > alarm.level)
      {
        // For high alarms, exceedance is how much the glucose level is above the alarm's level.
        exceedance = currentGlucoseLevel - alarm.level;
      }
      else if (!alarm.high_alarm && currentGlucoseLevel < alarm.level)
      {
        exceedance = alarm.level - currentGlucoseLevel;
      }

      // Check if this alarm's exceedance is greater than any previously found alarm's exceedance.
      if (exceedance < minExceedance)
      {
        mostCriticalAlarm = &alarm;
        minExceedance = exceedance;
      }
    }
  }

  return mostCriticalAlarm; // Returns the most critical alarm based on the largest exceedance, or nullptr if no thresholds are exceeded.
}

void Alarm_handler(void *pvParameters)
{
  bool blinkbool = true;
  int count = 0;
  bool blinktoggle = false;
  double lastBG = 0.00;
  bool allowcontinuous = true;
  // minutes since home_values.minutessince
  while (true)
  {
    const ALARM_STRUCT *criticalAlarm;
    if (OutsideUsa)
      criticalAlarm = getMostCriticalActiveAlarm(Home_values.mmol_l);
    else
      criticalAlarm = getMostCriticalActiveAlarm(Home_values.mg_dl);
    bool missedSignal = Home_values.minutes_since > 10.02;
    if (criticalAlarm && !missedSignal)
    {
      if (!Snoozing && !blinktoggle)
      {
        //const int SomeColor[3] = {colorValues[criticalAlarm->colorArrayPos]};
#if !defined NO_RGBLED
        rgb.setColor(colorValues[criticalAlarm->colorArrayPos][0]/6, // Red
             colorValues[criticalAlarm->colorArrayPos][1]/6, // Green
             colorValues[criticalAlarm->colorArrayPos][2]/6); // Blue
#endif

        if (allowcontinuous && !player.isPlaying() && criticalAlarm->playsound)
        {
          playMelodyByName(player, melodyNames[criticalAlarm->soundName]);
          allowcontinuous = false;
        }
        if (criticalAlarm->Blink)
        {
          blinktoggle = !blinktoggle;
        }
      }
      else
      {
#if !defined NO_RGBLED
        rgb.setColor(colorValues[criticalAlarm->colorArrayPos][0]/10, // Red
             colorValues[criticalAlarm->colorArrayPos][1]/10, // Green
             colorValues[criticalAlarm->colorArrayPos][2]/10); // Blue
#endif

        if (!player.isPlaying() && Snoozing)
        {
          player.stop();
        }
        blinktoggle = !blinktoggle;
      }
      if (criticalAlarm->continuous)
      {
        allowcontinuous = true;
      }
      else if (Home_values.mmol_l != lastBG)
      {
        allowcontinuous = true;
      }
      lastBG = Home_values.mmol_l;
    }
    else if (missedSignal)
    {
      // Handle missed signal scenario
#if !defined NO_RGBLED
      rgb.setColor(COLOR_BLUE_DIMMEST); // Use a distinct color to indicate signal loss
#endif
    }
    else
    {
#if !defined NO_RGBLED
      rgb.turnOff(); // No critical alarm or snoozing
#endif
      if (!player.isPlaying())
      {
        player.stop();
      }
    }

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
    else if (buttons == Button::SNOOZE_MINUS)
    {
      buttons = Button::NOTHING;
      if (Snoozing)
      {
        SnoozeEndTime -= SnoozeDuration;
        if (SnoozeEndTime < millis())
        {
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
  if (currentTime < Home_values.timestamp)
    currentTime -= follower.TZ_offset;
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
  if (OutsideUsa)
    dtostrf(Home_values.mmol_l, 2, 1, glucoseStr); // Format: total width = 4, 1 decimal place
  else
    dtostrf(Home_values.mg_dl, 3, 0, glucoseStr); // Format: total width = 4, 0 decimal place

  display.print(glucoseStr);
  int offset1 = display.getCursorX();
  display.println();

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
  if ((lastHour == 1) && (hours == 2))
  {
    // It just changed to 2:00AM...update the timezone offset
    lookupGeoIpTimezone();
  }
  // Save the current hour for the next pass
  lastHour = hours;
  // Test to see if we should change the OLED display brightness
  dimDisplay();

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

unsigned int selectedMenuItem = 0;
unsigned int sub_item = 0;
bool sub_menu_item_selected = false;
bool sub_menu = false;
bool selectedItemBlink = false;
u8_t blinkCount = 0;
bool valuesChanged = false;

//***********************************************************************************************************
void displayMenu(unsigned int selectedItem, bool sub_menu = false, unsigned int sub_item = 0, bool sub_menu_item_selected = false, int increment = 0)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (!sub_menu)
  {
    display.println("Alarm Settings");

    // Dynamically generate menu items based on alarmsV.alarms
    for (size_t i = 0; i < alarmsV.alarms.size(); i++)
    {
      if (i == selectedItem)
      {
        display.setFont();                                  // Use normal font for selected item
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Highlight selected item
      }
      else
      {
        display.setFont(&TomThumb); // Use smaller font for unselected items
        display.setTextColor(SSD1306_WHITE);
      }
      display.print(" ");
      display.print(alarmsV.alarms[i].name); // Display the name of each alarm
      display.println("");
    }
    display.setFont();
    display.setTextColor(SSD1306_WHITE);
    display.println();
    display.println(WiFi.localIP());
  }
  else
  {
    // Assuming you're in a submenu for a specific alarm
    // Display the submenu title with the selected alarm's name
    display.println(alarmsV.alarms[selectedItem].name); // Display title

    // Array or list of submenu items related to the alarm
#if !defined NO_RGBLED
    const char *subMenuItems[] = {"Active", "Level", "Continuous", "Sound", "Melody", "Blinking", "Color"};
#else
// If there is no RGB LED onboard, do not show in menu
    const char *subMenuItems[] = {"Active", "Level", "Continuous", "Sound", "Melody"};
#endif
    String subMenuValues[] = {
        alarmsV.alarms[selectedItem].active ? "Yes" : "No",
        dtostrf(alarmsV.alarms[selectedItem].level, 1, 2, buffer), // Assuming level is a float, adjust precision as needed
        alarmsV.alarms[selectedItem].continuous ? "Yes" : "once",
        alarmsV.alarms[selectedItem].playsound ? "Yes" : "No",
        melodyNames[alarmsV.alarms[selectedItem].soundName]
#if !defined NO_RGBLED
        , alarmsV.alarms[selectedItem].Blink ? "Yes" : "No"
        , colorNames[alarmsV.alarms[selectedItem].colorArrayPos]
#endif
        };
#if !defined NO_RGBLED
    const int numbersubs = 7;
#else
    const int numbersubs = 5;
#endif

    // Define the amount to step by when changing the alarm BG value
    int step;
    if (OutsideUsa)
      step = 0.1;
    else
      step = 5.0;

    for (int i = 0; i < numbersubs; i++)
    { // Adjust the loop condition based on the number of submenu items
      if (i == sub_item)
      {
        blinkCount++;
        // Highlight the selected submenu item
        if (increment != 0 && sub_menu_item_selected)
        {
          if (i == 0)
          {
            alarmsV.alarms[selectedItem].active = !alarmsV.alarms[selectedItem].active;
          }
          else if (i == 1)
          {
            alarmsV.alarms[selectedItem].level = alarmsV.alarms[selectedItem].level + (increment * step);
          }
          else if (i == 2)
          {
            alarmsV.alarms[selectedItem].continuous = !alarmsV.alarms[selectedItem].continuous;
          }
          else if (i == 3)
          {
            alarmsV.alarms[selectedItem].playsound = !alarmsV.alarms[selectedItem].playsound;
          }
          else if (i == 4)
          {
            int newValue = (alarmsV.alarms[selectedItem].soundName + increment) % numberOfMelodies;
            if (newValue < 0)
            {
              newValue = numberOfMelodies - 1;
            }
            alarmsV.alarms[selectedItem].soundName = newValue;
          }
#if !defined NO_RGBLED
// If there is no RGB LED onboard, do not show in menu
          else if (i == 5)
          {
            alarmsV.alarms[selectedItem].Blink = !alarmsV.alarms[selectedItem].Blink;
          }
          else if (i == 6)
          {
            int newValue = (alarmsV.alarms[selectedItem].colorArrayPos + increment) % numberOfColors;
            if (newValue < 0)
            {
              newValue = numberOfColors - 1;
            }
            alarmsV.alarms[selectedItem].colorArrayPos = newValue;
          }
#endif

        }
        if (sub_menu_item_selected)
        {
          display.print(subMenuItems[i]);
          display.print(": ");
        }
        display.setFont();
        if (sub_menu_item_selected && !selectedItemBlink)
        {
          if (blinkCount > 1)
          {
            selectedItemBlink = true;
            blinkCount = 0;
          }
        }
        else
        {
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Inverted colors for highlight
          if (blinkCount > 1)
          {
            selectedItemBlink = false;
            blinkCount = 0;
          }
        }
        display.setTextSize(1);
        if (!sub_menu_item_selected)
        {
          display.print(subMenuItems[i]);
          display.print(": ");
        }
        display.println(subMenuValues[i]);
      }
      else
      {
        display.setFont(&TomThumb);
        display.setTextColor(SSD1306_WHITE); // Normal color
        display.setTextSize(1);
        display.print(subMenuItems[i]);
        display.print(": ");
        display.println(subMenuValues[i]);
      }

      // Reset text color for non-selected items
      display.setTextSize(1);
      display.setFont();
      display.setTextColor(SSD1306_WHITE);
    }
  }

  display.display();
  display.setFont();
}

void Graph_Display()
{
  // follower.GlucoseLevelsArrayPopulate();
  double datapointsY[CASHED_READINGS];
  double YMax = 12.0;
  double YMin = 2.0;
  if (!OutsideUsa)
  {
    // If the user is inside the USA, use mg/dl numbers.
    YMax = 220.0;
    YMin = 40.0;
  }
  unsigned long datapointsX[CASHED_READINGS];
  unsigned long XMax = ntpClient.getEpochTime();
  unsigned long XMin = XMax - (Hours_Shown_On_Graph * 60 * 60);

  for (int i = CASHED_READINGS - 1; i >= 0; i--)
  {
    // Based ont he user's location, use the proper numbers
    if (OutsideUsa)
      datapointsY[i] = follower.GlucoseArray[i].mmol_l;
    else
      datapointsY[i] = follower.GlucoseArray[i].mg_dl;
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

  // Again, use the units local to the user (mg/dl or mmol/l).
  int low_bg, high_bg;
  char buf[8];  // Char string to use with the atoi() calls.
  if (OutsideUsa)
  {
    low_bg = 4;
    high_bg = 10;
  } else {
    low_bg = 72;
    high_bg = 180;
  }
  int LowLimit = int(yScale * (1 - (low_bg - YMin) / (YMax - YMin)));
  for (int j = 0; j < SCREEN_WIDTH; j += 4) // Draw dotted line
  {
    display.drawPixel(j, LowLimit, SSD1306_WHITE);
  }
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(1, LowLimit - 4);
  display.setTextSize(1);
  display.print(itoa(low_bg, buf, 10));

  int HighLimit = int(yScale * (1 - (high_bg - YMin) / (YMax - YMin)));
  for (int j = 0; j < SCREEN_WIDTH; j += 4) // Draw dotted line
  {
    display.drawPixel(j, HighLimit, SSD1306_WHITE);
  }
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(1, HighLimit - 4);
  display.setTextSize(1);
  display.print(itoa(high_bg, buf, 10));

  display.display();
}

void StateLoopTask(void *pvParameters)
{

  while (true)
  {
    int increment = 0;
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
        if (newSnoozeEndTime < millis())
        {
          SnoozeEndTime = millis();
          Snoozing = false;
        }
        else
        {
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
      if (!sub_menu)
      {

        if (buttons == Button::RIGHT)
        {
          buttons = Button::NOTHING;
          currentState = State::Home_screen;
          selectedMenuItem = 0;
          if (valuesChanged)
          {
            saveConfigFile();
            valuesChanged = false;
          }
          break;
        }
        else if (buttons == Button::LEFT)
        {
          buttons = Button::NOTHING;
          currentState = State::Graph;
          selectedMenuItem = 0;
          if (valuesChanged)
          {
            saveConfigFile();
            valuesChanged = false;
          }
          break;
        }
        else if (buttons == Button::DOWN)
        {
          if (selectedMenuItem != numMenuItems - 1)
          {
            selectedMenuItem = (selectedMenuItem + 1) % numMenuItems;
          }
          buttons = Button::NOTHING;
        }
        else if (buttons == Button::UP)
        {
          if (selectedMenuItem != 0)
          {
            selectedMenuItem = (selectedMenuItem - 1) % numMenuItems;
          }
          buttons = Button::NOTHING;
        }
        else if (buttons == Button::SELECT)
        {
          sub_menu = true;
          buttons = Button::NOTHING;
        }
      }

      else if (!sub_menu_item_selected)
      {
        if (buttons == Button::BACK)
        {
          sub_menu = false;
          buttons = Button::NOTHING;
          sub_item = 0;
        }
        else if (buttons == Button::DOWN)
        {
          if (sub_item != 7 - 1)
          {
            sub_item = (sub_item + 1) % 7;
          }
          buttons = Button::NOTHING;
        }
        else if (buttons == Button::UP)
        {
          if (sub_item != 0)
          {
            sub_item = (sub_item - 1) % 7;
          }
          buttons = Button::NOTHING;
        }
        else if (buttons == Button::SELECT)
        {
          sub_menu_item_selected = true;
          buttons = Button::NOTHING;
        }
      }

      else
      {
        if (buttons == Button::BACK)
        {
          sub_menu_item_selected = false;
          buttons = Button::NOTHING;
        }
        else if (buttons == Button::DOWN || buttons == Button::LEFT)
        {
          increment = -1;
          valuesChanged = true;
          buttons = Button::NOTHING;
        }
        else if (buttons == Button::UP || buttons == Button::RIGHT)
        {
          increment = 1;
          valuesChanged = true;
          buttons = Button::NOTHING;
        }
        else if (buttons == Button::SELECT)
        {
          playMelodyByName(player, melodyNames[alarmsV.alarms[selectedMenuItem].soundName]);
#if !defined NO_RGBLED
          rgb.setColor(colorValues[alarmsV.alarms[selectedMenuItem].colorArrayPos][0]/7, // Red
             colorValues[alarmsV.alarms[selectedMenuItem].colorArrayPos][1]/7, // Green
             colorValues[alarmsV.alarms[selectedMenuItem].colorArrayPos][2]/7); // Blue
          //rgb.setColor(alarmsV.alarms[selectedMenuItem].ledColorRed, alarmsV.alarms[selectedMenuItem].ledColorGreen, alarmsV.alarms[selectedMenuItem].ledColorBlue);
#endif
          buttons = Button::NOTHING;
        }
      }

      displayMenu(selectedMenuItem, sub_menu, sub_item, sub_menu_item_selected, increment);
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
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void addCustomParameter(const char *id, const char *placeholder, const char *defaultValue, int length, WiFiManager &wifiManager, std::vector<WiFiManagerParameter *> &customPara)
{
  WiFiManagerParameter *p = new WiFiManagerParameter(id, placeholder, defaultValue, length);
  wifiManager.addParameter(p);
  customPara.push_back(p);
}

void setup()
{
  Serial.begin(115200);

  Serial.printf ("Starting I2C (SDA %d, SCL %d)\n", ESP32_SDA, ESP32_SCL);
  Wire.begin(ESP32_SDA, ESP32_SCL);

  // rgb
#if !defined NO_RGBLED
  rgb.setColor(COLOR_PURPLE_DIM); // RED
#endif
  //  Buttons
#if defined DEXCOM_PCB
  // If we are using the custom PCB (from smeisner), just
  // use the 'BOOT' button as the AP trigger pin.
  pinMode(TRIGGER_AP_PIN, INPUT_PULLUP);
  attachInterrupt(TRIGGER_AP_PIN, APintHandler, FALLING);
#else
  pinMode(TRIGGER_AP_PIN, INPUT);
  attachInterrupt(TRIGGER_AP_PIN, APintHandler, RISING);
#endif
  pinMode(SELECT_PIN, INPUT);
  attachInterrupt(SELECT_PIN, SelectPinHandler, FALLING);
  pinMode(BACK_PIN, INPUT);
  attachInterrupt(BACK_PIN, BackPinHandler, FALLING);
  pinMode(SNOOZE_PIN_PLUS, INPUT);
  attachInterrupt(SNOOZE_PIN_PLUS, SnoozeHandler, FALLING);
  pinMode(SNOOZE_PIN_MINUS, INPUT);
  attachInterrupt(SNOOZE_PIN_MINUS, SnoozeHandler_Minus, FALLING);
  pinMode(RIGHT_PIN, INPUT);
  attachInterrupt(RIGHT_PIN, RightPinHandler, FALLING);
  pinMode(LEFT_PIN, INPUT);
  attachInterrupt(LEFT_PIN, LeftPinHandler, FALLING);
  pinMode(UP_PIN, INPUT);
  attachInterrupt(UP_PIN, UpPinHandler, FALLING);
  pinMode(DOWN_PIN, INPUT);
  attachInterrupt(DOWN_PIN, DownPinHandler, FALLING);

  // Initialize OLED display
  // if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  if (!display.begin(SSD1306_EXTERNALVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  // Set font

  // display.drawBitmap(0, 0, Dexcom_follow_screen, 128, 64, WHITE);
  display.drawBitmap(0, 0, epd_bitmap_logo, 128, 64, WHITE);

  display.display();

  // Set OLED display to full bright
  display.dim(true);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(255);

  srand(time(NULL));
  // Change to true when testing to force configuration every time we run
  bool forceConfig = false;

  // saveConfigFile();

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup)
  {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;

    // Erase SPIFFS
    Serial.println(F("Erasing SPIFFS..."));
    if (SPIFFS.format())
    {
      Serial.println(F("SPIFFS erased successfully."));
    } else {
      Serial.println(F("Error erasing SPIFFS."));
    }
  }

  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);

  // Reset settings (only for development)
  // wm.resetSettings();

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //
  // Start adding parameters to web page for configuration
  //

  // Add checkbox to indicate in./outside USA
  wm.addParameter(&custom_outside_usa_checkbox);
  char ous = OutsideUsa ? 'T' : 'F';
  custom_outside_usa_checkbox.setValue((const char *)(&ous), 1);
  wm.addParameter(&htmlLineBreak);

  // Add all defined parameters
  wm.addParameter(&Dexcom_Username);
  Dexcom_Username.setValue(D_User, DEXCOM_CREDS_SIZE);
  wm.addParameter(&Dexcom_Password);
  Dexcom_Password.setValue(D_Pass, DEXCOM_CREDS_SIZE);
#ifdef USE_PROJECTOR
  wm.addParameter(&OWM_Key_text);
  OWM_Key_text.setValue(OWM_API_Key, OWM_API_KEY_SIZE);
  wm.addParameter(&OWM_Postal_text);
  OWM_Postal_text.setValue(OWM_Postal_Code, OWM_POSTAL_SIZE);
  wm.addParameter(&OWM_Country_text);
  OWM_Country_text.setValue(OWM_Country, OWM_COUNTRY_SIZE);
#endif

  // Switch values to mg/dl if within the USA
  if (!OutsideUsa)
  {
    alarmsV.alarms[0].level = 240;
    alarmsV.alarms[1].level = 180;
    alarmsV.alarms[2].level = 72;
    alarmsV.alarms[3].level = 60;
  }

  // WiFiManagerParameter HIGHHIGH_ALARM(alarmsV.alarms[0].name, alarmsV.alarms[0].name, dtostrf(alarmsV.alarms[0].level, 1, 2, buffer), DOUBLE_STRING_SIZE);
  for (u8_t i = 0; i < numMenuItems; i++)
  {
    addCustomParameter(alarmsV.alarms[i].name.c_str(), alarmsV.alarms[i].name.c_str(), dtostrf(alarmsV.alarms[i].level, 1, 2, buffer), DOUBLE_STRING_SIZE, wm, customParameters);
  }

  // rgb
#if !defined NO_RGBLED
  rgb.setColor(COLOR_BLUE_DIM); // Blue
#endif
  if (forceConfig)
  // Run if we need a configuration
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Connect to:");
    display.setTextSize(2);
    display.println(" \"Glucose Follower\"");
    display.setTextSize(1);
    display.println("to choose Wifi and\nDexcom Credentials");
    display.display();
    if (!wm.startConfigPortal("Glucose Follower"))
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
    if (!wm.autoConnect("Glucose Follower"))
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

  // Based on the GeoIP data, set the local timezone offset
  lookupGeoIpTimezone();

  // Lets deal with the user config values

  Load_from_WM();

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

  // Update the OLED display brightness based on the time
  dimDisplay();

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

#ifdef USE_PROJECTOR  // Must be called before follower.Xxx calls below
  xTaskCreate(projectorUpdateTask, NULL, 8192, NULL, 4, NULL);
  vTaskDelay(pdMS_TO_TICKS(1 * 1000));
#endif

  follower.GlucoseLevelsArrayPopulate();
  follower.GlucoseLevelsNow();
  ntpClient.setTimeOffset(follower.TZ_offset);
  // ntpClient.setTimeOffset(follower.TZ_offset);
  // ntpClient.update();
  //  rgb.setColor(COLOR_YELLOW_DIM); //Yellow
  // startOTAWebServer();
  startEnhancedOTAWebServer();
  // Create and start the glucose update task
  xTaskCreate(glucoseUpdateTask, "GlucoseUpdateTask", 8192, NULL, 2, NULL);
  xTaskCreate(StateLoopTask, "StateLoop", 8192, NULL, 1, NULL);
  xTaskCreate(Alarm_handler, "Alarm_handler", 8192, NULL, 3, NULL);
}

void loop()
{
  // do nothing in the loop. just loops
  // player.playAsync(lowToneBeepMelody);
  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(5 * 1000));
  }
}
