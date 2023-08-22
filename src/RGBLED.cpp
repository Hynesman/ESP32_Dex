#include "RGBLED.h"

RGBLED::RGBLED(int redPin, int greenPin, int bluePin) 
    : redPin_(redPin), greenPin_(greenPin), bluePin_(bluePin) {
    pinMode(redPin_, OUTPUT);
    pinMode(greenPin_, OUTPUT);
    pinMode(bluePin_, OUTPUT);
    turnOff();
}

void RGBLED::setColor(int red, int green, int blue) {
    analogWrite(redPin_, red);
    analogWrite(greenPin_, green);
    analogWrite(bluePin_, blue);
}

void RGBLED::turnOff() {
    setColor(0, 0, 0);
}
