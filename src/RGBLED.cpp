#include "RGBLED.h"

RGBLED::RGBLED(int redPin, int greenPin, int bluePin)
    : redPin_(redPin), greenPin_(greenPin), bluePin_(bluePin)
{
    pinMode(redPin_, OUTPUT);
    pinMode(greenPin_, OUTPUT);
    pinMode(bluePin_, OUTPUT);
    turnOff();
}

void RGBLED::setColor(int red, int green, int blue)
{
    analogWrite(redPin_, red);
    analogWrite(greenPin_, green);
    analogWrite(bluePin_, blue);
}

// Overloaded setColor method to accept an array of 3 ints
void RGBLED::setColor(const int color[3])
{
    analogWrite(redPin_, color[0]);
    analogWrite(greenPin_, color[1]);
    analogWrite(bluePin_, color[2]);
}

void RGBLED::setColor(const int (&color)[3]) {
    analogWrite(redPin_, color[0]);
    analogWrite(greenPin_, color[1]);
    analogWrite(bluePin_, color[2]);
}

void RGBLED::setColor(const uint8_t color[3]) {
    setColor(static_cast<int>(color[0]), static_cast<int>(color[1]), static_cast<int>(color[2]));
}

void RGBLED::turnOff()
{
    setColor(0, 0, 0);
}
