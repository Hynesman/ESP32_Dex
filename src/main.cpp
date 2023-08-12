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
#define TRIGGER_AP_PIN 0
bool Launch_AP = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int NTP_UTC_OFFSET = 2 * 60 * 60; // Replace with your UTC offset in seconds
const char *NTP_SERVER = "pool.ntp.org";
const long NTP_UPDATE_INTERVAL = 60 * 1000; // Update NTP time every 60 seconds

WiFiUDP udp;
NTPClient ntpClient(udp, NTP_SERVER, NTP_UTC_OFFSET, NTP_UPDATE_INTERVAL);

double glucoseValue;
String trend = "";

Follower follower(true);

int error_count = 0;

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
        display.clearDisplay();
        display.setTextSize(4);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(follower.GlucoseNow.mmol_l);
        display.println(follower.GlucoseNow.trend_Symbol);
        display.display();
        // Print out the current time in timestamp format
        // Serial.print("isr:Current time (Unix timestamp): ");
        // Serial.println(currentTime);
        // Serial.println(ntpClient.getDay());
        // Serial.println(ntpClient.getFormattedTime());
        unsigned long currentTime = ntpClient.getEpochTime();
        delay_time = follower.GlucoseNow.timestamp + (5 * 60) - currentTime;

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
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Wifi disconnected");
        display.println("attempting reconnect");
        display.display();
        // wm.autoConnect()
        if (!wm.autoConnect("Follower_AP"))
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
        display.clearDisplay();
        display.setTextSize(3);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("??.??");
        display.println("?");
        display.setTextSize(1);
        display.println("error getting data check wifi");
        display.display();
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

void IRAM_ATTR APintHandler()
{
  Launch_AP = true;
}

void StateLoopTask(void *pvParameters)
{
  while (true)
  {
    if (Launch_AP)
    {
      just_wait = true;
      // Explicitly set WiFi mode
      WiFi.mode(WIFI_STA);

      // Set config save notify callback
      wm.setSaveConfigCallback(saveConfigCallback);

      // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
      wm.setAPCallback(configModeCallback);

      if (!wm.startConfigPortal("Follower_AP"))
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
        Serial.print("D_User: ");
        Serial.println(D_User);

        // Convert the number value
        strncpy(D_Pass, Dexcom_Password.getValue(), sizeof(D_Pass));
        Serial.print("D_User: ");
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

        follower.getNewSessionID();
        if (!follower.SessionIDnotDefault())
        {
          Launch_AP = true;
          just_wait = true;
        }
        else
        {
          Launch_AP = false;
          just_wait = false;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5 * 1000)); // Wait for 5 seconds before making the next request
  }
}

void setup()
{
  Serial.begin(115200);

  Wire.begin();

  pinMode(TRIGGER_AP_PIN, INPUT);
  attachInterrupt(TRIGGER_AP_PIN, APintHandler, RISING);

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

  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);

  // Setup Serial monitor
  Serial.begin(115200);
  delay(10);

  // Reset settings (only for development)
  // wm.resetSettings();

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  // Add all defined parameters
  wm.addParameter(&Dexcom_Username);
  wm.addParameter(&Dexcom_Password);

  if (forceConfig)
  // Run if we need a configuration
  {
    if (!wm.startConfigPortal("Follower_AP"))
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
    if (!wm.autoConnect("Follower_AP"))
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
  }

  // Create and start the glucose update task
  // xTaskCreatePinnedToCore(glucoseUpdateTask, "GlucoseUpdateTask", 8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(glucoseUpdateTask, "GlucoseUpdateTask", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(StateLoopTask, "StateLoop", 10000, NULL, 2, NULL, 1);
  // vTaskStartScheduler();
}

void loop()
{

  vTaskDelay(pdMS_TO_TICKS(5 * 1000)); // Wait for 5 seconds before making the next request
}
