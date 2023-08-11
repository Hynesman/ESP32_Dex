/*
  Dexcom_follow.h - Library for following blood glucous levels of a dexcom sensor remotely.
  Created by Michael J. Hynes 1st August 2023.
  Released into the public domain.
*/
#ifndef Dexcom_follow
#define Dexcom_follow

#include "Arduino.h"

extern const char* DEXCOM_TREND_DIRECTIONS[];
extern const char* DEXCOM_TREND_ARROWS[];
extern const char* DEXCOM_BASE_URL;
extern const char* DEXCOM_BASE_URL_OUS;
extern String applicationId;

#define CASHED_READINGS 36

// GlucoseReading structure
struct GlucoseReading
{
    // ToDo need timestamp here as well
    unsigned long timestamp;
    int mg_dl;
    double mmol_l;
    const char *trend_description;
    const char *trend_Symbol;
    // Define other members as needed for your application
};

class Follower
{
public:
    Follower(bool ous=true, String user = "", String pass = "", String sessionID = "");
    void Set_user_pass(String user, String pass);
    void Set_sessionID(String sessionID);
    bool getNewSessionID();
    bool GlucoseLevelsNow();
    GlucoseReading GlucoseNow;
    bool SessionIDnotDefault();

private:
    String DexcomServer;
    String SessionID;
    String Username;
    String Password;
    String jsonStr;
    int head = 0;
    int tail = 0;
    GlucoseReading GlucouseArray[CASHED_READINGS];
    double convertToMmol(int mgdl);
    String removeCharacterFromString(String input, char characterToRemove);
    const char *getTrendSymbol(const char *trendDescription);
    void update_json_string();
    unsigned long convertToUnixTimestamp(const char* dtValue);
};

#endif