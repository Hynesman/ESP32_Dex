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

String pattern6[] = {"A4", "B4", "C5", "D5", "E5", "D5", "C5", "B4", "SILENCE"};
Melody melody6 = MelodyFactory.load("Rising and Falling", 110, pattern6, 9);

String pattern7[] = {"G5", "E5", "G5", "E5", "G5", "E5", "G5", "E5", "SILENCE"};
Melody melody7 = MelodyFactory.load("Sprints", 90, pattern7, 9);

String pattern8[] = {"C4", "D4", "E4", "F4", "G4", "F4", "E4", "D4", "SILENCE"};
Melody melody8 = MelodyFactory.load("Gentle Waves", 70, pattern8, 9);

String pattern9[] = {"B5", "SILENCE", "B5", "G5", "SILENCE", "G5", "E5", "SILENCE", "E5"};
Melody melody9 = MelodyFactory.load("Echoes", 130, pattern9, 9);

String pattern10[] = {"E4", "G4", "B4", "G4", "E4", "G4", "B4", "G4", "SILENCE"};
Melody melody10 = MelodyFactory.load("Mystic Journey", 95, pattern10, 9);

String popPulse[] = {"C5", "E5", "G5", "E5", "D5", "C5", "B4", "G4", "SILENCE"};
Melody melodyPopPulse = MelodyFactory.load("Pop Pulse", 120, popPulse, 9);

String electroGroove[] = {"F5", "D5", "F5", "A5", "G5", "F5", "E5", "F5", "SILENCE"};
Melody melodyElectroGroove = MelodyFactory.load("Electro Groove", 128, electroGroove, 9);

String acousticVibes[] = {"G4", "B4", "C5", "D5", "C5", "B4", "A4", "G4", "SILENCE"};
Melody melodyAcousticVibes = MelodyFactory.load("Acoustic Vibes", 95, acousticVibes, 9);

String chillWave[] = {"E4", "G4", "B4", "A4", "G4", "E4", "D4", "C4", "SILENCE"};
Melody melodyChillWave = MelodyFactory.load("Chill Wave", 105, chillWave, 9);

String urbanNights[] = {"B4", "A4", "G4", "F4", "E4", "D4", "C4", "B3", "SILENCE"};
Melody melodyUrbanNights = MelodyFactory.load("Urban Nights", 110, urbanNights, 9);

String whimsicalJumps[] = {"C4", "G5", "E4", "A5", "D4", "B5", "G4", "E5", "SILENCE"};
Melody melodyWhimsicalJumps = MelodyFactory.load("Whimsical Jumps", 95, whimsicalJumps, 9);

String mysticEchoes[] = {"A3", "SILENCE", "B4", "SILENCE", "C5", "SILENCE", "D5", "SILENCE", "E5"};
Melody melodyMysticEchoes = MelodyFactory.load("Mystic Echoes", 85, mysticEchoes, 9);

String spiritedChase[] = {"E5", "D5", "C5", "B4", "A4", "G4", "F4", "E4", "SILENCE"};
Melody melodySpiritedChase = MelodyFactory.load("Spirited Chase", 140, spiritedChase, 9);

String lullabyWhispers[] = {"G4", "E4", "C4", "E4", "G4", "E4", "C4", "E4", "SILENCE"};
Melody melodyLullabyWhispers = MelodyFactory.load("Lullaby Whispers", 60, lullabyWhispers, 9);

String starlightWaltz[] = {"C4", "E4", "G4", "C5", "G4", "E4", "C4", "G3", "SILENCE"};
Melody melodyStarlightWaltz = MelodyFactory.load("Starlight Waltz", 108, starlightWaltz, 9);


Melody* getMelodyByName(const String& name) {
    // Original Variations and Special Melodies
    if (name == "Pattern 1") return &melody1;
    else if (name == "Pattern 2") return &melody2;
    else if (name == "Pattern 3") return &melody3;
    else if (name == "Pattern 4") return &melody4;
    else if (name == "Pattern 5") return &melody5;
    else if (name == "Alarming Melody") return &alarmingMelody;
    else if (name == "Low Tone Beep") return &lowToneBeepMelody;
    else if (name == "R2-D2 Happy Beeps") return &r2d2HappyBeeps;
    else if (name == "R2-D2 Sad Whistle") return &r2d2SadWhistle;
    else if (name == "R2-D2 Angry Beeps") return &r2d2AngryBeeps;
    else if (name == "R2-D2 Surprise") return &r2d2Surprise;
    else if (name == "R2-D2 Theme") return &r2d2ThemeMelody;
    // New Melodies
    else if (name == "Rising and Falling") return &melody6;
    else if (name == "Sprints") return &melody7;
    else if (name == "Gentle Waves") return &melody8;
    else if (name == "Echoes") return &melody9;
    else if (name == "Mystic Journey") return &melody10;
    else if (name == "Pop Pulse") return &melodyPopPulse;
    else if (name == "Electro Groove") return &melodyElectroGroove;
    else if (name == "Acoustic Vibes") return &melodyAcousticVibes;
    else if (name == "Chill Wave") return &melodyChillWave;
    else if (name == "Urban Nights") return &melodyUrbanNights;
    else if (name == "Whimsical Jumps") return &melodyWhimsicalJumps;
    else if (name == "Mystic Echoes") return &melodyMysticEchoes;
    else if (name == "Spirited Chase") return &melodySpiritedChase;
    else if (name == "Lullaby Whispers") return &melodyLullabyWhispers;
    else if (name == "Starlight Waltz") return &melodyStarlightWaltz;
    // If no match is found
    else return nullptr;
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
    "Pattern 1", "Pattern 2", "Pattern 3", "Pattern 4", "Pattern 5",
    "Alarming Melody", "Low Tone Beep", "R2-D2 Happy Beeps", "R2-D2 Sad Whistle",
    "R2-D2 Angry Beeps", "R2-D2 Surprise", "R2-D2 Theme",
    "Rising and Falling", "Sprints", "Gentle Waves", "Echoes", "Mystic Journey",
    "Pop Pulse", "Electro Groove", "Acoustic Vibes", "Chill Wave", "Urban Nights",
    "Whimsical Jumps", "Mystic Echoes", "Spirited Chase", "Lullaby Whispers", "Starlight Waltz"
};


int numberOfMelodies = sizeof(melodyNames) / sizeof(melodyNames[0]);


#endif // ALARM_MELODIES_H

