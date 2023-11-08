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


// Define note frequencies for your melody (using standard MIDI note values)
int melodyFrequencies[] = {
  277, 330, 277, 277, 330,  // CS4, E4, CS4, CS4, E4
  277, 277, 330, 277, 311,  // CS4, CS4, E4, CS4, DS4
  277, 277, 330, 277,       // CS4, CS4, E4, CS4
  247,                      // B3
  277, 330, 277, 277, 330,  // CS4, E4, CS4, CS4, E4
  277, 311, 277, 330,       // CS4, DS4, CS4, E4
  247,                      // B3
  330, 330, 330, 330, 330, 330, 330, 330, 330,  // E4 (repeated 9 times)
  330, 330, 330, 330, 370, 392, 392,             // E4, FS4, GS4, GS4
  392, 330, 370, 494, 392, 392, 392, 392, 392,  // GS4, FS4, B4, GS4, GS4 (repeated 5 times)
  370, 370, 370, 392, 370, 370, 370, 330,  // FS4, FS4, FS4, GS4, FS4, FS4, FS4, FS4, FS4, GS4, FS4
  277, 196, 196, 392, 392,  // E4, CS4, CS4, GS4, GS4
  392, 392, 392, 392, 392, 392, 392, 392, 392, 392, 392, 392, 392, 392, 392, 392,  // GS4 (repeated 16 times)
  277, 277, 277, 277, 277,  // E4, CS4, CS4, CS4, CS4
  207, 247,                // G3, B3
  277, 277, 370, 392, 330, 392,  // CS4, CS4, FS4, GS4, E4, FS4
  247,                      // B3
  330, 392, 494, 392, 330, 277, 330, 392,  // E4, FS4, B4, GS4, FS4, CS4, E4, GS4
  392, 330, 392, 277, 196, 392, 392, 330, 330,  // FS4, E4, CS4, CS4, GS4, GS4, FS4, FS4, E4
  247, 277, 277, 392, 392,  // B3, CS4, CS4, GS4, GS4
  330, 330, 330, 330, 330, 330, 330, 330, 330, 330, 330, 330, 330, 330, 330, 330,  // E4 (repeated 16 times)
  277, 277, 277, 277, 277,  // E4, CS4, CS4, CS4, CS4
  207, 247,                // G3, B3
  330, 330, 392, 392, 330, 392, 392, 330,  // E4, FS4, GS4, GS4, E4, E4, E4, FS4, FS4
  247,                      // B3
  277, 330, 392, 330, 277, 277, 277, 330,  // CS4, E4, GS4, E4, CS4, CS4, CS4, FS4, FS4
  330, 392, 392, 330, 330,  // E4, FS4, FS4, E4, E4
  277, 277,                // CS4, CS4
  207, 247,                // G3, B3
  330, 330, 392, 392, 330, 330, 330, 330,  // E4, FS4, GS4, GS4, E4, E4, E4, FS4, FS4
  330, 330, 392, 392,  // E4, FS4, GS4, GS4
  277, 277                 // CS4, CS4
};

// Define a fixed duration in milliseconds for all notes
const int fixedDuration = 400; // Adjust this value as needed

// Calculate the size of the melodyFrequencies array
const int melodyLength = sizeof(melodyFrequencies) / sizeof(melodyFrequencies[0]);

Melody yourMelody = MelodyFactory.load("Your Melody", 100, melodyFrequencies, fixedDuration, melodyLength);


#endif // ALARM_MELODIES_H

