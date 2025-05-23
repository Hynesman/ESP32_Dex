#include "Arduino.h"
#include "Dexcom_follow.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Dexcom base API urls
const char *DEXCOM_BASE_URL = "https://share2.dexcom.com/ShareWebServices/Services";
const char *DEXCOM_BASE_URL_OUS = "https://shareous1.dexcom.com/ShareWebServices/Services";

const char *DEFAULT_SESSION_ID = "00000000-0000-0000-0000-000000000000";

// Trend directions mapping
const char *DEXCOM_TREND_DIRECTIONS[] = {
    "None", // unconfirmed
    "DoubleUp",
    "SingleUp",
    "FortyFiveUp",
    "Flat",
    "FortyFiveDown",
    "SingleDown",
    "DoubleDown",
    "NotComputable", // unconfirmed
    "RateOutOfRange" // unconfirmed
};

// Trend arrows
const char *DEXCOM_TREND_ARROWS[] = {
    "",
    "↑↑",
    "↑",
    "↗",
    "→",
    "↘",
    "↓",
    "↓↓",
    "?",
    "-"};

String applicationId = "d8665ade-9673-4e27-9ff6-92db4ce13d13";
const char *DEXCOM_APPLICATION_ID = "d89443d2-327c-4a6f-89e5-496bbb0317db"; // alternative

/*
    dsfefsef
*/
Follower::Follower(bool ous, String user, String pass, String sessionID)
{
    Username = user;
    Password = pass;
    SessionID = sessionID;
    update_json_string();
    if (ous)
    {
        DexcomServer = DEXCOM_BASE_URL_OUS;
    }
    else
    {
        DexcomServer = DEXCOM_BASE_URL;
    }
};

void Follower::update_location(bool ous)
{
  if (ous)
  {
      DexcomServer = DEXCOM_BASE_URL_OUS;
  }
  else
  {
      DexcomServer = DEXCOM_BASE_URL;
  }
}

void Follower::update_json_string()
{
    String jsonString = "{";
    jsonString += "\"accountName\": \"" + Username + "\",";
    jsonString += "\"applicationId\": \"" + applicationId + "\",";
    jsonString += "\"password\": \"" + Password + "\"";
    jsonString += "}";
    jsonStr = jsonString;
};

void Follower::Set_user_pass(String user, String pass)
{
    Username = user;
    Password = pass;
    update_json_string();
};

void Follower::Set_sessionID(String sessionID)
{
    SessionID = sessionID;
};

bool Follower::getNewSessionID()
{
    String response;
    bool ok = false;
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        http.begin(DexcomServer + "/General/LoginPublisherAccountByName");

        http.addHeader("Content-Type", "application/json");

        int httpCode = http.POST(jsonStr);

        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK)
            {
                char characterToRemove = '\"';

                response = removeCharacterFromString(http.getString(), characterToRemove);
                // response = http.getString();
                Serial.println("HTTP POST was successful!");
                Serial.println("Response:");
                Serial.println(response);
                SessionID = response;
                ok = true;
            }
            else
            {
                Serial.print("HTTP error code: ");
                Serial.println(httpCode);
                response = "error";
                SessionID = "00000000-0000-0000-0000-000000000000";
                ok = false;
            }
        }
        else
        {
            Serial.println("HTTP connection failed!");
            response = "no connection";
            ok = false;
        }

        http.end();
    }
    return ok;
};

bool Follower::SessionIDnotDefault(){
    if (SessionID == "00000000-0000-0000-0000-000000000000"){
        return false;
    }
    else if (SessionID == ""){
        return false;
    }
    else{
        return true;
    }
}

bool Follower::GlucoseLevelsNow()
{
    bool result = false;
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        String url = DexcomServer + "/Publisher/ReadPublisherLatestGlucoseValues?sessionId=";
        url += SessionID;
        url += "&minutes=1440&maxCount=1";

        http.begin(url);

        int httpResponseCode = http.GET();
        if (httpResponseCode == HTTP_CODE_OK)
        {
            String response = http.getString();

            // Parse the JSON response
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, response);
            if (error)
            {
                Serial.print("Error parsing JSON response: ");
                Serial.println(error.c_str());
                http.end();
                return false;
            }

            // Uncomment the next line to dump the JSON to the serial console
            // serializeJsonPretty(doc, Serial);

            // Extract the blood glucose value
            GlucoseNow.mg_dl = doc[0]["Value"];
            GlucoseNow.mmol_l = convertToMmol(GlucoseNow.mg_dl);
            Serial.printf ("Blood Glucose Level: %d mg/dl  %2.1f mmol/l\n", GlucoseNow.mg_dl, GlucoseNow.mmol_l);

            GlucoseNow.trend_description = doc[0]["Trend"].as<const char *>();
            GlucoseNow.trend_Symbol = getTrendSymbol(GlucoseNow.trend_description);
            Serial.printf ("Current trend: %s\n", GlucoseNow.trend_Symbol);

            GlucoseNow.timestamp = convertToUnixTimestamp(doc[0]["DT"].as<const char *>());
            Serial.printf ("Dexcom Timestamp:  %u\n", GlucoseNow.timestamp);

            result = true;
        }
        else
        {
            Serial.print("HTTP GET request failed, error code: ");
            Serial.println(httpResponseCode);
            getNewSessionID();

            result = false;
        }

        http.end();
    }
    else{
        result = false;
    }
    return result;
};

double Follower::convertToMmol(int mgdl)
{
    return mgdl / 18.01559;
};

String Follower::removeCharacterFromString(String input, char characterToRemove)
{
    String result;
    for (char c : input)
    {
        if (c != characterToRemove)
        {
            result += c;
        }
    }
    return result;
};

extern bool OutsideUsa;

unsigned long Follower::convertToUnixTimestamp(const char *dtValue)
{
    // Extract the numerical part of the string
    const char *numericalPart = dtValue + 5; // Skip "Date("

    // Find the position of the first 10 digits
    const char *offset = numericalPart + 10;

    if (offset)
    {
        // Convert the timestamp part to a long integer
        char timestampStr[11]; // Assuming timestamp will always have 13 digits
        strncpy(timestampStr, numericalPart, offset - numericalPart);
        timestampStr[offset - numericalPart] = '\0';

        unsigned long timestamp = atol(timestampStr);

        // Get the time zone offset in hours and minutes
        sscanf(offset + 3, "+%2d%2d", &TZ_hours, &TZ_minutes);

        /*

        Quick note regarding the following:

        In the USA, the Dexcom server returns the following as an example:

        16:47:51.338 > [
        16:47:51.338 >   {
        16:47:51.339 >     "WT": "Date(1740001473000)",
        16:47:51.342 >     "ST": "Date(1740001473000)",
        16:47:51.345 >     "DT": "Date(1740001473000+0000)",
        16:47:51.348 >     "Value": 174,
        16:47:51.350 >     "Trend": "Flat"
        16:47:51.353 >   }
        16:47:51.353 > ]

        The DT string always has "+0000" as the offset. Therefore, for the US-basedd
        server, we should not update the TZ_offset.

        */

        if (OutsideUsa)
        {
          TZ_offset = TZ_hours*60*60 + TZ_minutes*60;

          // Convert the offset to seconds and adjust the timestamp
          unsigned long offsetSeconds = TZ_hours * 60 * 60 + TZ_minutes * 60;
          timestamp += offsetSeconds;
        }

        return timestamp; // Convert microseconds to seconds
    }
    else
    {
        return 0; // Invalid DT value format
    }
}

const char *Follower::getTrendSymbol(const char *trendDescription)
{
    if (strcmp(trendDescription, "None") == 0)
    {
        return "";
    }
    else if (strcmp(trendDescription, "DoubleUp") == 0)
    {
        return "^^";
    }
    else if (strcmp(trendDescription, "SingleUp") == 0)
    {
        return "^";
    }
    else if (strcmp(trendDescription, "FortyFiveUp") == 0)
    {
        return "/^";
    }
    else if (strcmp(trendDescription, "Flat") == 0)
    {
        return "->";
    }
    else if (strcmp(trendDescription, "FortyFiveDown") == 0)
    {
        return "\\v";
    }
    else if (strcmp(trendDescription, "SingleDown") == 0)
    {
        return "v";
    }
    else if (strcmp(trendDescription, "DoubleDown") == 0)
    {
        return "vv";
    }
    else if (strcmp(trendDescription, "NotComputable") == 0)
    {
        return "?";
    }
    else if (strcmp(trendDescription, "RateOutOfRange") == 0)
    {
        return "-";
    }
    else
    {
        return ""; // Default to an empty string for unknown trends
    }
};

void Follower::parseAndStoreData(String jsonString)
{
    size_t length = jsonString.length();
    // Serial.println(length);
    DynamicJsonDocument doc(length*2); // Adjust the size as needed

    DeserializationError error = deserializeJson(doc, jsonString);

    if (error)
    {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
        return;
    }

    JsonArray jsonArray = doc.as<JsonArray>();

    int index = 0;
    for (JsonObject obj : jsonArray)
    {
        if (index >= CASHED_READINGS)
            break;

        //GlucoseArray[index].timestamp = obj["WT"].as<unsigned long>();
        GlucoseArray[index].timestamp = convertToUnixTimestamp(obj["DT"].as<const char *>());
        GlucoseArray[index].mg_dl = obj["Value"].as<int>();
        GlucoseArray[index].mmol_l = GlucoseArray[index].mg_dl * 0.0555; // Convert to mmol/L
        GlucoseArray[index].trend_description = obj["Trend"].as<const char *>();
        GlucoseArray[index].trend_Symbol = obj["Trend"].as<const char *>();
        //Serial.println(GlucoseArray[index].mmol_l);
        //Serial.println(GlucoseArray[index].timestamp);

        index++;
    }
    
}

bool Follower::GlucoseLevelsArrayPopulate()
{
    bool result = false;
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        String url = DexcomServer + "/Publisher/ReadPublisherLatestGlucoseValues?sessionId=";
        url += SessionID;
        url += "&minutes=1440&maxCount=";
        url += CASHED_READINGS;

        http.begin(url);

        int httpResponseCode = http.GET();
        if (httpResponseCode == HTTP_CODE_OK)
        {
            String response = http.getString();

            parseAndStoreData(response);

            result = true;
        }
        else
        {
            Serial.print("HTTP GET request failed, error code: ");
            Serial.println(httpResponseCode);
            getNewSessionID();

            result = false;
        }

        http.end();
    }
    else{
        result = false;
    }
    return result;
};

String Follower::serializeGlucoseReadings(const GlucoseReading glucoseReadings[]) {
    DynamicJsonDocument doc(1024); // Adjust size based on your needs
    JsonArray readings = doc.to<JsonArray>();

    for (size_t i = 0; i < CASHED_READINGS; ++i) {
        JsonObject readingObj = readings.createNestedObject();
        readingObj["timestamp"] = glucoseReadings[i].timestamp;
        readingObj["mg_dl"] = glucoseReadings[i].mg_dl;
        readingObj["mmol_l"] = glucoseReadings[i].mmol_l;
        readingObj["trend_description"] = glucoseReadings[i].trend_description ? glucoseReadings[i].trend_description : "";
        readingObj["trend_Symbol"] = glucoseReadings[i].trend_Symbol ? glucoseReadings[i].trend_Symbol : "";
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}
