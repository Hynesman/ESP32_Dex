#include "hwdef.h"

#ifdef USE_PROJECTOR

#include <WiFi.h>
#ifdef TYPE_TM1621D
#include "TM1621D.h"
#endif
#ifdef TYPE_ET6621S
#include "ET6621S.h"
#endif
#include "tc74.h"
// #include "MCP4018.h"
#include <NTPClient.h>    // included to know how to address struct elements

//
// Defines for PWM signal to projector LED
//
const int PWM_CHANNEL = 0;    // ESP32 has 16 channels which can generate 16 independent waveforms
const int PWM_FREQ = 500;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int PWM_RESOLUTION = 8; // We'll use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits 
// The max duty cycle value based on PWM resolution (will be 255 if resolution is 8 bits)
const int MAX_DUTY_CYCLE = (int)(pow(2, PWM_RESOLUTION) - 1); 


//
// Copied from structs_include.h
//
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

extern Homescreen_values Home_values;

///////////////////////////////////////////////
//
// Code to retrieve local outside temperature
//
#include <HTTPClient.h>
#include <ArduinoJson.h>

extern NTPClient ntpClient;

extern bool OutsideUsa;
void SetFollowerOffset(int offset);

unsigned long lastTime = 0;
// Set timer to 10 seconds (10000)
unsigned long timerDelay = 10000;

extern String openWeatherMapApiKey;
extern String openWeatherMapPostal;
extern String openWeatherMapCountry;
String OWM_City;

//
///////////////////////////////////////////////

typedef enum
{
  CLOCK = 0,
  GLUCOSE,
  TEMPERATURE_OUTSIDE,
  TEMPERATURE_INSIDE
} MODE;

MODE mode = CLOCK;

int modeTimeout = 4000;


#ifdef TYPE_ET6621S
// Instantiate the ET6621S device
ET6621S ht(PIN_PROJECTOR_CS, PIN_PROJECTOR_CLK, PIN_PROJECTOR_DATA);
#endif
#ifdef TYPE_TM1621D
// Instantiate the TM1621D device
TM1621D ht(PIN_PROJECTOR_CS, PIN_PROJECTOR_CLK, PIN_PROJECTOR_DATA);
#endif

///////////////////////////////////////////////
//
// Code to read the local temp sensor (the TC74)
//
// Instantiate TC74 temperature sensor
TC74 tc74(TC74_I2C_ADDRESS); //0x48 is the i2c address of the TC74 sensor
//
///////////////////////////////////////////////

// MCP4018 mcp;

//
// Call to OpenWeatherMap to retrieve local temperature
//
String httpGETRequest(const char* serverName)
{
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0)
  {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

//
// Convert degress C to F
//
float CtoF(float c)
{
  return 9.0 / 5.0 * c + 32;
}

//
// Retrieve the location codes (city name and country) from OpenWeatherMap
// using the user inputted postal & country codes.
//
bool getOwmCity()
{
  String serverPath = "http://api.openweathermap.org/geo/1.0/zip?zip=" + openWeatherMapPostal + "," + openWeatherMapCountry + "&APPID=" + openWeatherMapApiKey;

  Serial.printf ("GeoIP URL: %s\n", serverPath.c_str());
  Serial.printf ("OWM Key: %s\n", openWeatherMapApiKey.c_str());

  String payload = httpGETRequest(serverPath.c_str());
  DynamicJsonDocument jsonBuffer(128);
  DeserializationError error = deserializeJson(jsonBuffer, payload);
  if (error)
  {
    Serial.print("Deserialization failed with code: ");
    Serial.println(error.c_str());
    return false;
  }

  Serial.printf ("HTTP response:\n");
  serializeJsonPretty(jsonBuffer, Serial);
  Serial.printf("\n");

  OWM_City = jsonBuffer["name"].as<const char *>();
  Serial.printf("City retrieved from OWM: %s\n", OWM_City);

  return true;
}

//
// Function to fetch local temp frmo OpenWaetherMap
// and parse the JSON response.
//
float getOutsideTemp()
{
  float temp;
  
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + OWM_City + "," + openWeatherMapCountry + "&APPID=" + openWeatherMapApiKey;

  if ((millis() - lastTime) > timerDelay)
  {
    String payload = httpGETRequest(serverPath.c_str());
    DynamicJsonDocument jsonBuffer(1024);
    DeserializationError error = deserializeJson(jsonBuffer, payload);
    if (error)
    {
      Serial.print("Deserialization failed with code: ");
      Serial.println(error.c_str());
      temp = -99;
    }
    JsonArray array = jsonBuffer["weather"].as<JsonArray>();
    temp = (float)(jsonBuffer["main"]["temp"]) - 273.15;
    // Serial.printf("Temperature = %.2f°C\r\n", temp);
  }
  
  return (CtoF(temp) + 0.5);  // + 0.5 to round up to next integer
}



void i2c_scanner()
{
  byte error, address; //variable for error and I2C address
  int nDevices;

  Serial.println("\n\n\nScanning I2C...");

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  // delay(5000); // wait 5 seconds for the next I2C scan
}


/////////////////////////////////////
// Primary loop for the LED Projector
//
// Here we will cycle through the states or modes defined in the typedef
// MODE above. After each timeout, the mode is advanced to display the
// Next data.

void IRAM_ATTR projectorUpdateTask(void *)
{
  // PROJ_DATA *data = (PROJ_DATA *)pvParameters;
  bool colon = false;
  struct tm epoch_time;
  time_t now;
  double glucose;
  int temp;
  int h, t, o;
  unsigned long tmo;
  bool am_on, pm_on;
  uint8_t dig1, dig2, dig3, dig4;

#ifdef TYPE_TM1621D
  Serial.printf ("\n*****    Built for TM1621D projector    *****\n\n");
#endif
#ifdef TYPE_ET6621S
  Serial.printf ("\n*****    Built for ET6621S projector    *****\n\n");
#endif

  pinMode(PIN_BUILTIN_LED, OUTPUT);
  digitalWrite(PIN_BUILTIN_LED, HIGH);

  pinMode (PIN_PROJECTOR_CS, OUTPUT); digitalWrite(PIN_PROJECTOR_CS, LOW);
  pinMode (PIN_PROJECTOR_CLK, OUTPUT); digitalWrite(PIN_PROJECTOR_CLK, LOW);
  pinMode (PIN_PROJECTOR_DATA, OUTPUT); digitalWrite(PIN_PROJECTOR_DATA, LOW);

  pinMode (PIN_PROJECTOR_PWM, OUTPUT); digitalWrite(PIN_PROJECTOR_PWM, LOW);

  ht.begin();
  ht.start();
  ht.set_orientation(false);
  if (ht.UpsideDown)
  {
    dig1 = 0; dig2 = 1; dig3 = 2; dig4 = 3;
  }
  else
  {
    dig1 = 3; dig2 = 2; dig3 = 1; dig4 = 0;
  }

  // Serial.printf("Projector display initialized\n");
  ht.all_elements_off();
  // vTaskDelay(pdMS_TO_TICKS(20));
  // Turn on the projector lamp
  ht.projector_lamp(true);
  vTaskDelay(pdMS_TO_TICKS(1000));

  //
  // Run a basic test pattern
  //
  Serial.printf ("All projector elements on\n");
  ht.all_elements_on();
  vTaskDelay(pdMS_TO_TICKS(2000));

  Serial.printf ("Starting TC74 temp sensor...");
  tc74.begin();
  while (tc74.isStandby()) { vTaskDelay(pdMS_TO_TICKS(100)); Serial.printf ("."); }  //wait until the sensor is ready
  Serial.printf (" TC74 started!\n");

  Serial.printf ("Setting up PWM for projector\n");
  // Sets up a channel (0-15), a PWM duty cycle frequency, and a PWM resolution (1 - 16 bits) 
  // ledcSetup(uint8_t channel, double freq, uint8_t resolution_bits);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  // ledcAttachPin(uint8_t pin, uint8_t channel);
  ledcAttachPin(PIN_PROJECTOR_PWM, PWM_CHANNEL);

  //
  // Keep the test pattern displayed until we are
  // connected to a wifi network. Nothing works
  // until we are connected.
  //
  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  Serial.printf ("Wifi connected. Clear projector display\n");
  ht.all_elements_off();

  // Serial.printf ("Testing display\n");
  // ht.display_test();

  Serial.printf ("Fetching local city name\n");
  getOwmCity();

  Serial.printf ("Starting main loop...\n");

  // Set initial timeout
  tmo = millis() + modeTimeout;

  for (;;)
  {
    switch (mode)
    {
      case CLOCK:
        digitalWrite(PIN_BUILTIN_LED, HIGH);
        //@@@
        ledcWrite(PWM_CHANNEL, 100);

        // ntpClient.setTimeOffset(tzo);
        // ntp->setTimeOffset(-18000);
        ntpClient.update();
        vTaskDelay(pdMS_TO_TICKS(200));
        now = ntpClient.getEpochTime();
        memcpy(&epoch_time, localtime(&now), sizeof (struct tm));

        // Adjust for 12-hour clock
        if (epoch_time.tm_hour >= 12)
        {
          epoch_time.tm_hour -= 12;
          pm_on = true;
          am_on = false;
        }
        else
        {
          pm_on = false;
          am_on = true;
        }
        // Math above causes both noon and midnight to show 
        // as 0:00. Fix it below:
        if (epoch_time.tm_hour == 0) 
          epoch_time.tm_hour = 12;

        // If hour is less than 10, blank first digit
        if (epoch_time.tm_hour < 10)
          ht.display_numeral(dig1, ht.BLANK, colon, am_on, pm_on); // Digit "10" is blank
        else
          ht.display_numeral(dig1, epoch_time.tm_hour / 10, colon, am_on, pm_on);
        ht.display_numeral(dig2, epoch_time.tm_hour % 10, colon, am_on, pm_on);
        ht.display_numeral(dig3, epoch_time.tm_min / 10, colon, am_on, pm_on);
        ht.display_numeral(dig4, epoch_time.tm_min % 10, colon, am_on, pm_on);

        digitalWrite(PIN_BUILTIN_LED, LOW);
        vTaskDelay(pdMS_TO_TICKS(150));
        colon = !colon;

        if (millis() > tmo) 
        {
          mode = GLUCOSE;
          tmo = millis() + modeTimeout;
          // Serial.printf ("Mode: GLUCOSE\n");
        }
        break;

      case GLUCOSE:
        digitalWrite(PIN_BUILTIN_LED, HIGH);
        //@@@
        ledcWrite(PWM_CHANNEL, 75);

        if (OutsideUsa)
          // We have no decimal place in the display...how to show mmol/litre????
          glucose = Home_values.mmol_l;
        else
          glucose = (double)Home_values.mg_dl;

        // Blank out leftmost digit (rightmost if upside down),
        // and turn on all display widgets except the colon.
        ht.display_numeral(dig4, ht.BLANK, false, true, true);

        if (Home_values.minutes_since > 10.0)
        {
          ht.display_numeral(dig1, ht.BLANK, false, true, true);
          ht.display_numeral(dig2, ht.DASH, false, true, true);
          ht.display_numeral(dig3, ht.DASH, false, true, true);
        }
        else
        {
          // Determine how many digits we have to work with
          if (glucose >= 100)
          {
            h = glucose / 100;
            ht.display_numeral(dig1, h, false, true, true);
          }
          else
          {
            h = 0;
            ht.display_numeral(dig1, ht.BLANK, false, true, true);
          }
          t = (glucose - (h * 100)) / 10;
          ht.display_numeral(dig2, t, false, true, true);
          o = glucose - (h * 100) - (t * 10);
          ht.display_numeral(dig3, o, false, true, true);
        }

        digitalWrite(PIN_BUILTIN_LED, LOW);
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (millis() > tmo) 
        {
          mode = TEMPERATURE_OUTSIDE;
          tmo = millis() + modeTimeout;
          // Serial.printf ("Mode: TEMPERATURE_OUTSIDE\n");
        }
        break;

      case TEMPERATURE_OUTSIDE:
        //@@@
        ledcWrite(PWM_CHANNEL, 50);

        if (openWeatherMapApiKey.length() == 0)
        {
          Serial.printf ("No Open Weather Map API key set\n");
          mode = TEMPERATURE_INSIDE;
          continue;
        }
        digitalWrite(PIN_BUILTIN_LED, HIGH);
        temp = getOutsideTemp();
        if (temp == -99)
        {
          ht.display_numeral(dig2, ht.DASH);
          ht.display_numeral(dig3, ht.DASH);
        }
        else
        {
          if (temp < 0)
          {
            ht.display_numeral(dig1, ht.DASH);
            temp *= -1;
            h = 0;
          }
          else if (temp < 100)
          {
            h = 0;
            ht.display_numeral(dig1, ht.BLANK);
          }
          else
          {
            h = temp / 100;
            ht.display_numeral(dig1, h);
          }

          t = (temp - (h * 100)) / 10;
          if ((t == 0) && (temp != 100))
            ht.display_numeral(dig2, ht.BLANK);
          else
            ht.display_numeral(dig2, t);

          o = temp - (h * 100) - (t * 10);
          ht.display_numeral(dig3, o);
        }

        ht.display_numeral(dig4, ht.DEGREE);

        digitalWrite(PIN_BUILTIN_LED, LOW);
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (millis() > tmo) 
        {
          mode = TEMPERATURE_INSIDE;
          tmo = millis() + modeTimeout;
          // Serial.printf ("Mode: TEMPERATURE_INSIDE\n");
        }
        break;

      case TEMPERATURE_INSIDE:
        digitalWrite(PIN_BUILTIN_LED, HIGH);
        //@@@
        ledcWrite(PWM_CHANNEL, 25);

        // Read local temp sensor
        temp = tc74.readTemperature('f');
        if (temp == -998)
        {
          ht.display_numeral(dig1, ht.DASH);
          ht.display_numeral(dig2, ht.DASH);
          ht.display_numeral(dig3, ht.LETTER_F);
        }
        else
        {
          temp += TEMP_CORRECTION;
          if (temp >= 100)
          {
            h = temp / 100;
            ht.display_numeral(dig1, h);
          }
          else
          {
            h = 0;
            ht.display_numeral(dig1, ht.BLANK);
          }
          if (temp < 0)
          {
            ht.display_numeral(dig1, ht.DASH);
            temp *= -1;
          }
          t = (temp - (h * 100)) / 10;
          ht.display_numeral(dig2, t);
          o = temp - (h * 100) - (t * 10);
          ht.display_numeral(dig3, o);
        }

        ht.display_numeral(dig4, ht.LETTER_F);

        digitalWrite(PIN_BUILTIN_LED, LOW);
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (millis() > tmo) 
        {
          // Serial.printf ("Temp = %d\n", temp);
          mode = CLOCK;
          tmo = millis() + modeTimeout;
          // Serial.printf ("Mode: CLOCK\n");
        }
        break;
    }
  }
}

//#ifdef USE_PROJECTOR
#endif
