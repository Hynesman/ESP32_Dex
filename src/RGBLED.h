#ifndef RGBLED_H
#define RGBLED_H

#include <Arduino.h>

class RGBLED
{
public:
    RGBLED(int redPin, int greenPin, int bluePin);
    void setColor(int red, int green, int blue);
    void setColor(const int color[3]);
    void setColor(const int (&color)[3]);
    void setColor(const uint8_t color[3]);
    void turnOff();

#define COLOR_RED 255, 0, 0
#define COLOR_GREEN 0, 128, 0     // Reduced green intensity
#define COLOR_BLUE 0, 0, 128      // Reduced blue intensity
#define COLOR_YELLOW 255, 85, 0   // Reduced green intensity to balance
#define COLOR_CYAN 0, 128, 128    // Reduced blue intensity to balance
#define COLOR_MAGENTA 128, 0, 128 // Reduced red intensity to balance
#define COLOR_WHITE 170, 57, 57   // Equal intensities to balance
#define COLOR_PURPLE 64, 0, 64    // Reduced intensities
#define COLOR_ORANGE 255, 43, 0   // Reduced green intensity to balance
#define COLOR_TEAL 0, 64, 64      // Reduced intensities
#define COLOR_PINK 255, 96, 102   // Reduced intensities
#define COLOR_LIME 0, 255, 0      // Full green intensity for "lime"
#define COLOR_INDIGO 47, 0, 81    // Reduced red intensity, balanced blue
#define COLOR_BROWN 87, 22, 22    // Reduced intensities
#define COLOR_GRAY 85, 85, 85     // Balanced intensities

// Dim versions of colors
#define COLOR_RED_DIM 64, 0, 0      // Dim red
#define COLOR_GREEN_DIM 0, 32, 0    // Dim green
#define COLOR_BLUE_DIM 0, 0, 32     // Dim blue
#define COLOR_YELLOW_DIM 64, 21, 0  // Dim yellow
#define COLOR_CYAN_DIM 0, 32, 32    // Dim cyan
#define COLOR_MAGENTA_DIM 32, 0, 32 // Dim magenta
#define COLOR_WHITE_DIM 43, 14, 14  // Dim white
#define COLOR_PURPLE_DIM 16, 0, 16  // Dim purple
#define COLOR_ORANGE_DIM 64, 11, 0  // Dim orange
#define COLOR_TEAL_DIM 0, 16, 16    // Dim teal
#define COLOR_PINK_DIM 64, 19, 20   // Dim pink
#define COLOR_LIME_DIM 0, 64, 0     // Dim lime
#define COLOR_INDIGO_DIM 15, 0, 27  // Dim indigo
#define COLOR_BROWN_DIM 29, 7, 7    // Dim brown
#define COLOR_GRAY_DIM 28, 28, 28   // Dim gray

// Dimmest versions of colors
#define COLOR_RED_DIMMEST 16, 0, 0    // Dimmest red
#define COLOR_GREEN_DIMMEST 0, 8, 0   // Dimmest green
#define COLOR_BLUE_DIMMEST 0, 0, 8    // Dimmest blue
#define COLOR_YELLOW_DIMMEST 16, 5, 0 // Dimmest yellow
#define COLOR_CYAN_DIMMEST 0, 8, 8    // Dimmest cyan
#define COLOR_MAGENTA_DIMMEST 8, 0, 8 // Dimmest magenta
#define COLOR_WHITE_DIMMEST 10, 3, 3  // Dimmest white
#define COLOR_PURPLE_DIMMEST 4, 0, 4  // Dimmest purple
#define COLOR_ORANGE_DIMMEST 16, 3, 0 // Dimmest orange
#define COLOR_TEAL_DIMMEST 0, 4, 4    // Dimmest teal
#define COLOR_PINK_DIMMEST 16, 5, 5   // Dimmest pink
#define COLOR_LIME_DIMMEST 0, 16, 0   // Dimmest lime
#define COLOR_INDIGO_DIMMEST 4, 0, 7  // Dimmest indigo
#define COLOR_BROWN_DIMMEST 7, 2, 2   // Dimmest brown
#define COLOR_GRAY_DIMMEST 7, 7, 7    // Dimmest gray

private:
    int redPin_;
    int greenPin_;
    int bluePin_;
};



#endif // RGBLED_H
