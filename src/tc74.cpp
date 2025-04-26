/* 
   TC74 - Arduino Library for TC74, Temperature Sensor.
   By: Chawin 'FaultyTwo' Treesugol <https://github.com/FaultyTwo/TC74-arduino-lib>
*/

#include "tc74.h"

TC74::TC74(uint8_t adr)
{
  _adr = adr;
}

void TC74::begin(TwoWire &wire)
{
  _wire = &wire;
  // _wire->begin();
  mTemperature.begin(SMOOTHED_AVERAGE, 10);
  mTemperature.clear();
}

float TC74::readTemperature(char unit)
{
  int8_t val;

  _wire->beginTransmission(_adr);
  _wire->write(0x00); //Read Temperature
  _wire->endTransmission(false);
  _wire->requestFrom(_adr, byte(1));

  if (_wire->available())
  {
    val = _wire->read();
  }
  else
  {
    return -998; //device not found
  }

  mTemperature.add((float)val);

  switch(unit)
  {
	  case 'c':
	  case 'C':
		  return float(mTemperature.get());
	  case 'k':
	  case 'K':
    	return float(float(mTemperature.get()) + 273.15);
	  case 'f':
	  case 'F':
		  return float((float(mTemperature.get())*(9.0/5)) + 32);
	  default:
		  return -999;
  }
 
}

void TC74::TC74Mode(bool mode)
{
  _wire->beginTransmission(_adr);
  _wire->write(0x01); //R/W Config
  _wire->write(0x00 | (mode << 7)); //D[7] -> TC74_STANDBY switch
  _wire->endTransmission();
}

bool TC74::isStandby()
{
  _wire->beginTransmission(_adr);
  _wire->write(0x01);
  _wire->endTransmission(false);
  _wire->requestFrom(_adr, byte(1));

  if (_wire->available())
  {
    if (_wire->read() == 0x40 || _wire->read() == 0x00)
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  return false;
}
