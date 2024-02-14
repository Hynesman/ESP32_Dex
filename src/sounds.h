#ifndef ALARM_MELODIES_H
#define ALARM_MELODIES_H

#include <melody_player.h>
#include <melody_factory.h>

// Variation 1: Quick and Intense
String pattern1[] = { "G6", "C7", "G6", "C7", "G6", "C7", "G6", "C7", "SILENCE" };
Melody melody1 = MelodyFactory.load("Pattern 1", 80, pattern1, 9);

// Variation 2: Up and Down
String pattern2[] = { "E5", "F5", "G5", "A5", "B5", "A5", "G5", "F5", "SILENCE" };
Melody melody2 = MelodyFactory.load("Pattern 2", 120, pattern2, 9);

// Variation 3: Deep and Rapid
String pattern3[] = { "C5", "D5", "E5", "F5", "C5", "D5", "E5", "F5", "SILENCE" };
Melody melody3 = MelodyFactory.load("Pattern 3", 60, pattern3, 9);

// Variation 4: Alternating High and Low
String pattern4[] = { "E5", "C5", "E5", "C5", "E5", "C5", "E5", "C5", "SILENCE" };
Melody melody4 = MelodyFactory.load("Pattern 4", 100, pattern4, 9);

// Variation 5: Descending Steps
String pattern5[] = { "D6", "C6", "B5", "A5", "G5", "F5", "E5", "D5", "SILENCE" };
Melody melody5 = MelodyFactory.load("Pattern 5", 150, pattern5, 9);

String alarmingNotes[] = { "D6", "E6", "D6", "E6", "D6", "E6", "D6", "E6", "SILENCE" };
Melody alarmingMelody = MelodyFactory.load("Alarming Melody", 100, alarmingNotes, 9);

String lowToneBeep[] = { "A3", "SILENCE" };
Melody lowToneBeepMelody = MelodyFactory.load("Low Tone Beep", 100, lowToneBeep, 2);

//R2D2
String happyBeeps[] = { "D6", "A6", "D7", "E7", "SILENCE" };
Melody r2d2HappyBeeps = MelodyFactory.load("R2-D2 Happy Beeps", 120, happyBeeps, 4);

String sadWhistle[] = { "B4", "A4", "G4", "F4", "E4", "SILENCE" };
Melody r2d2SadWhistle = MelodyFactory.load("R2-D2 Sad Whistle", 80, sadWhistle, 5);

String angryBeeps[] = { "E5", "F5", "G5", "F5", "E5", "SILENCE" };
Melody r2d2AngryBeeps = MelodyFactory.load("R2-D2 Angry Beeps", 120, angryBeeps, 5);

String surprise[] = { "G5", "A5", "D6", "E6", "SILENCE" };
Melody r2d2Surprise = MelodyFactory.load("R2-D2 Surprise", 100, surprise, 4);

String r2d2Theme[] = { "E5", "D5", "G5", "F5", "E5", "SILENCE" };
Melody r2d2ThemeMelody = MelodyFactory.load("R2-D2 Theme", 100, r2d2Theme, 5);

Melody* getMelodyByName(const String& name) {
    if (name == "Pattern 1") {
        return &melody1;
    } else if (name == "Pattern 2") {
        return &melody2;
    } else if (name == "Pattern 3") {
        return &melody3;
    } else if (name == "Pattern 4") {
        return &melody4;
    } else if (name == "Pattern 5") {
        return &melody5;
    } else if (name == "Alarming Melody") {
        return &alarmingMelody;
    } else if (name == "Low Tone Beep") {
        return &lowToneBeepMelody;
    } else if (name == "R2-D2 Happy Beeps") {
        return &r2d2HappyBeeps;
    } else if (name == "R2-D2 Sad Whistle") {
        return &r2d2SadWhistle;
    } else if (name == "R2-D2 Angry Beeps") {
        return &r2d2AngryBeeps;
    } else if (name == "R2-D2 Surprise") {
        return &r2d2Surprise;
    } else if (name == "R2-D2 Theme") {
        return &r2d2ThemeMelody;
    }
    // If the name doesn't match any predefined melody
    return nullptr;
}

void playMelodyByName(MelodyPlayer& player, const String& melodyName) {
    Melody* melody = getMelodyByName(melodyName);
    if (melody != nullptr) {
        player.playAsync(*melody);
    } else {
        Serial.println("Melody not found: " + melodyName);
    }
}

const String melodyNames[] = {
    "Pattern 1",
    "Pattern 2",
    "Pattern 3",
    "Pattern 4",
    "Pattern 5",
    "Alarming Melody",
    "Low Tone Beep",
    "R2-D2 Happy Beeps",
    "R2-D2 Sad Whistle",
    "R2-D2 Angry Beeps",
    "R2-D2 Surprise",
    "R2-D2 Theme"
};

const int numberOfMelodies = sizeof(melodyNames) / sizeof(melodyNames[0]);


#endif // ALARM_MELODIES_H

