/**
 * \file TM1621D.cpp
 * \brief Implementation of a class for dealing with the Holtek TM1621D chip.
 * \author Enrico Formenti
 * \date 31 january 2015
 * \version 1.0
 * \copyright BSD license, check the License page on the blog for more information. All this text must be
 *  included in any redistribution.
 *  <br><br>
 *  See macduino.blogspot.com for more details.
 *
 */

#include "hwdef.h"
#include "TM1621D.h"

///////////////////////////////////////////////
//
// Support for TM1621D projector
//

struct DisplayConfig
{
    uint8_t m4b;   // MSB 4 bits
    uint8_t l4b;   // LSB 4 bits
};

// These are groups of TM1621D memory locations
// to write to for each digit. There are 4 digits.
// The first is the Low segments and the second are
// the High segments.
static DisplayConfig digMap[4] =
{
    {0x11, 0x10}, {0x0f, 0x0e}, {0x13, 0x14}, {0x15, 0x16}
};

// Bitmaps of each character to be displayed. Each bit
// corresponds to a segment of the display (in a 7
// segment display).
static uint8_t segments1[] = {
    //GBDCFAME
    0b11010111,  // 0
    0b01010000,  // 1
    0b11100011,  // 2
    0b11110001,  // 3
    0b01110100,  // 4
    0b10110101,  // 5
    0b10110111,  // 6
    0b11010000,  // 7
    0b11110111,  // 8
    0b11110101,  // 9
    0b00000000,  // [Empty]
    0b10100110,  // 'F'
    0b00100000,  // '-'
    0b11100100   // '*'
};

// The above digit data, just flipped vertically
static uint8_t segments2[] = {
    //GBDCFAME
    0b11010111,  // 0
    0b00000110,  // 1
    0b11100011,  // 2
    0b10100111,  // 3
    0b00110110,  // 4
    0b10110101,  // 5
    0b11110101,  // 6
    0b00000111,  // 7
    0b11110111,  // 8
    0b10110111,  // 9
    0b00000000,  // [Empty]
    0b01110001,  // 'F'
    0b00100000,  // '-'
    0b00110011   // '*'
};

//
// End of support for TM1621D projector
//
///////////////////////////////////////////////

void TM1621D::begin()
{
  pinMode(_DATA_pin, OUTPUT);
  pinMode(_RW_pin, OUTPUT);
  pinMode(_CS_pin, OUTPUT);

  digitalWrite(_CS_pin, LOW);
  digitalWrite(_RW_pin, LOW);
  digitalWrite(_DATA_pin, LOW);
}

void TM1621D::start()
{
  sendCommand(NORMAL); //vTaskDelay(pdMS_TO_TICKS(2));
  sendCommand(SYS_EN); //vTaskDelay(pdMS_TO_TICKS(2));
  sendCommand(BIAS_THIRD_4_COM); //vTaskDelay(pdMS_TO_TICKS(2));
  sendCommand(RC256K); //vTaskDelay(pdMS_TO_TICKS(2));
}

void TM1621D::projector_lamp(bool on)
{
  if (on)
    sendCommand(LCD_ON);
  else
    sendCommand(LCD_OFF);
}

void TM1621D::display_test()
{
  int dig;
  if (UpsideDown)
    dig = 0;
  else
    dig = 3;
  for (int i=0; i < 14; i++) { Serial.printf ("%d\n", i); display_numeral(dig, i); delay(1000); }
  display_numeral(dig, 10); //clear
  Serial.printf ("colon\n"); display_numeral(2, 10, true, false, false); delay(2000); display_numeral(2, 10); // Clear
  Serial.printf ("PM\n"); 
  display_numeral(dig, 10, false, false, true);
  delay(2000);
  display_numeral(dig, 10); display_numeral(3, 10); // Clear
  Serial.printf ("Both PM\n"); 
  display_numeral(0, 10, false, true, true); display_numeral(3, 10, false, true, true);
  delay(2000);
  display_numeral(0, 10); display_numeral(3, 10); // Clear
}

// 
// In the example a delay is given after each write
// by 20 microseconds..
//
void TM1621D::writeBits(uint16_t data, uint16_t mask, uint8_t cnt)
{
  register uint8_t i;
  // register mask = 2 ^ (cnt -1);

  for (i = 0; i < cnt; i++, data <<= 1)
  {
    delayMicroseconds(45);
    digitalWrite(_DATA_pin, data & mask ? HIGH : LOW);
    delayMicroseconds(40);
    digitalWrite(_RW_pin, HIGH);
    delayMicroseconds(85);
    digitalWrite(_RW_pin, LOW);
  }
}

void TM1621D::sendCommand(uint16_t cmd, bool first, bool last)
{
  TAKE_CS();
  delayMicroseconds(5);
  RELEASE_CS();

  writeBits(COMMAND_MODE, 0x80, 3);

  writeBits(cmd, 0x100, 0x9);

  // if (last)
  //     RELEASE_CS();
  TAKE_CS();
}

void TM1621D::write(uint8_t address, uint8_t data)
{
  TAKE_CS();
  delayMicroseconds(5);
  RELEASE_CS();

  writeBits(WRITE_MODE, 0x80, 3);
  writeBits(address, 0x20, 6); // 6 is because max address is 128
  writeBits(data, 0x8, 4);

  TAKE_CS();
}

void TM1621D::write(uint8_t address, uint8_t data, uint8_t cnt)
{
  register uint8_t i;

  TAKE_CS();
  delayMicroseconds(5);
  RELEASE_CS();

  writeBits(WRITE_MODE, 0x80, 3);
  writeBits(address, 0x20, 6);
  for (i = 0; i < cnt; i++)
  {
    writeBits(data, 0x8, 4);
  }

  TAKE_CS();
}

void TM1621D::write_long(uint8_t address, uint32_t data)
{
  register uint8_t i;

  TAKE_CS();
  delayMicroseconds(5);
  RELEASE_CS();

  writeBits(WRITE_MODE, 0x80, 3);
  writeBits(address, 0x20, 6);

  writeBits (((data & 0xff000000) >> 24), 0x80, 8);
  writeBits (((data & 0x00ff0000) >> 16), 0x80, 8);
  writeBits (((data & 0x0000ff00) >> 8), 0x80, 8);
  writeBits ((data & 0x000000ff), 0x80, 8);

  TAKE_CS();
}

// Turns off all the segments
void TM1621D::all_elements_off()
{
  long i;
  for (i=0x0e; i<=0x16; i++)
  {
    write(i, 0b0000);
    delayMicroseconds(1);
  }
}

// Turns off all the segments
void TM1621D::all_elements_on()
{
  long i;
  for (i=0x0e; i<=0x16; i++)
  {
    write(i, 0b1111);
    delayMicroseconds(1);
  }
}


void TM1621D::display_numeral(uint8_t pos, uint8_t digit, bool colon, bool am, bool pm)
{
  DisplayConfig df = digMap[pos];
  uint16_t segData;
  uint8_t high4Bit;
  uint8_t low4Bit;

  if (UpsideDown)
    segData = segments2[digit];
  else
    segData = segments1[digit];

  high4Bit = (segData & 0b11110000) >> 4;
  low4Bit  = (segData & 0b00001111);

  //
  // Handle special symbols
  //
  if ((df.l4b == 0x14)) // Colon address
    if (colon)
      low4Bit |= 0b1000;
    else
      low4Bit &= 0b0111;


  //
  // Handle AM and PM symbols by modifying the lower
  // 4 bits. Wether or not to set the upper bit
  // depends on the address and the flag passed in.
  //
  if (UpsideDown)
  {
    if (df.l4b == 0x0e) // AM bottom address
      if (am)
        low4Bit |= 0b1000;
      else
        low4Bit &= 0b0111;

    if (df.l4b == 0x10) // PM bottom address
      if (pm)
        low4Bit |= 0b1000;
      else
        low4Bit &= 0b0111;

    // Default "lower" AM symbol to off
    write(0x12, 0b0000); // AM top address
    // Turn on "lower AM" symbol?
    if ((am) && (pm))
    {
      write(0x12, 0b1000);  // AM top address
      if (df.l4b == 0x16) low4Bit |= 0b1000; // PM top
    }
  }
  else    // !UpsideDown
  {
    if (df.l4b == 0x16)  // PM top address
      if (pm)
        low4Bit |= 0b1000;
      else
        low4Bit &= 0b0111;

    write(0x12, 0b0000);  // AM top address
    if ((am) || ((am) && (pm)))
      write(0x12, 0b1000);  // AM top address

  }

  write(df.l4b, low4Bit);
  write(df.m4b, high4Bit);
}

void TM1621D::set_orientation(bool orientation)
{
  UpsideDown = !orientation;
}