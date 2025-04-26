/**
 * \file ET6621S.cpp
 * \brief Implementation of a class for dealing with the Etek ET6621S chip.
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
#include "ET6621S.h"

///////////////////////////////////////////////
//
// Support for ET6621S projector
//

struct DisplayConfig
{
    uint8_t m4b;   // MSB 4 bits
    uint8_t l4b;   // LSB 4 bits
};

static DisplayConfig digMap[4] =
{
    {24, 25}, {26, 27}, {28, 29}, {30, 31}
};

// Bitmaps of each character to be displayed. Each bit
// corresponds to a segment of the display (in a 7
// segment display).
static uint8_t segments1[] = {
    //GBDCFAME
    0b10111110,  // 0
    0b00000110,  // 1
    0b01111100,  // 2
    0b01011110,  // 3
    0b11000110,  // 4
    0b11011010,  // 5
    0b11111010,  // 6
    0b00001110,  // 7
    0b11111110,  // 8
    0b11001110,  // 9
    0b00000000,  // [Empty]
    0b11101000,  // 'F'
    0b01000000,  // '-'
    0b11001100   // '*'
};

// The above digit data, just flipped vertically
static uint8_t segments2[] = {
    //GBDCFAME
    0b10111110,  // 0
    0b10100000,  // 1
    0b01111100,  // 2
    0b11111000,  // 3
    0b11100010,  // 4
    0b11011010,  // 5
    0b11011110,  // 6
    0b10110000,  // 7
    0b11111110,  // 8
    0b11110010,  // 9
    0b00000000,  // [Empty]
    0b01010110,  // 'F'
    0b01000000,  // '-'
    0b01110010   // '*'
};

//
// End of support for ET6621S projector
//
///////////////////////////////////////////////

void ET6621S::begin()
{
  pinMode(_DATA_pin, OUTPUT);
  pinMode(_RW_pin, OUTPUT);
  pinMode(_CS_pin, OUTPUT);

  digitalWrite(_CS_pin, LOW);
  digitalWrite(_RW_pin, LOW);
  digitalWrite(_DATA_pin, LOW);
}

void ET6621S::start()
{
  sendCommand(NORMAL);
  sendCommand(XTAL32K);
  sendCommand(SYS_EN);
  sendCommand(BIAS_THIRD_4_COM);
}

void ET6621S::projector_lamp(bool on)
{
  if (on)
    sendCommand(LCD_ON);
  else
    sendCommand(LCD_OFF);
}

void ET6621S::display_test()
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
void ET6621S::writeBits(uint16_t data, uint16_t mask, uint8_t cnt)
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

void ET6621S::sendCommand(uint16_t cmd, bool first, bool last)
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

void ET6621S::write(uint8_t address, uint8_t data)
{
  TAKE_CS();
  delayMicroseconds(5);
  RELEASE_CS();

  writeBits(WRITE_MODE, 0x80, 3);
  writeBits(address, 0x20, 6); // 6 is because max address is 128
  writeBits(data, 0x8, 4);

  TAKE_CS();
}

void ET6621S::write(uint8_t address, uint8_t data, uint8_t cnt)
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

void ET6621S::write_long(uint8_t address, uint32_t data)
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
void ET6621S::all_elements_off()
{
  long i;
  for (i=0x18; i<=0x1f; i++)
  {
    write(i, 0b0000);
    delayMicroseconds(1);
  }
}

// Turns off all the segments
void ET6621S::all_elements_on()
{
  long i;
  for (i=0x18; i<=0x1f; i++)
  {
    write(i, 0b1111);
    delayMicroseconds(1);
  }
}

void ET6621S::display_numeral(uint8_t pos, uint8_t digit, bool colon, bool am, bool pm)
{
  DisplayConfig df = digMap[pos];
  uint16_t segData;
  uint8_t high4Bit;
  uint8_t low4Bit;

  if (UpsideDown)
    segData = segments1[digit];
  else
    segData = segments2[digit];

  high4Bit = (segData & 0b11110000) >> 4;
  low4Bit  = (segData & 0b00001111);

  //
  // Handle special symbols
  //
  if ((df.l4b == 0x1d)) // Colon address
    if (colon)
      low4Bit |= 0b0001;
    else
      low4Bit &= 0b1110;

  //
  // Handle AM and PM symbols by modifying the lower
  // 4 bits. Wether or not to set the upper bit
  // depends on the address and the flag passed in.
  //
  if (am || pm)
  {
    if (UpsideDown)
    {
      if (df.l4b == 0x19) // PM top address
        if (pm)
          low4Bit |= 0b0001;
        else
          low4Bit &= 0b1110;
    }
    else    // !UpsideDown
    {
      if (df.l4b == 0x1f)  // PM bottom address
        if (pm)
          low4Bit |= 0b0001;
        else
          low4Bit &= 0b1110;
    }
    if ((am) && (pm))
    {
      if (df.l4b == 0x1f) low4Bit |= 0b0001; // PM top
      if (df.l4b == 0x19) low4Bit |= 0b0001; // PM bottom
    }
  }

  write(df.l4b, low4Bit);
  write(df.m4b, high4Bit);
}

void ET6621S::set_orientation(bool orientation)
{
  UpsideDown = orientation;
}