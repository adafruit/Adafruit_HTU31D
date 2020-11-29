/*!
 * @file Adafruit_HTU31D.cpp
 *
 * @mainpage Adafruit HTU31D Sensor
 *
 * @section intro_sec Introduction
 *
 * This is a library for the HTU31D Humidity & Temp Sensor
 *
 * Designed specifically to work with the HTU31D sensor from Adafruit
 * ----> https://www.adafruit.com/products/4832
 *
 * These displays use I2C to communicate, 2 pins are required to
 * interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 */

#include "Adafruit_HTU31D.h"

/**
 * Constructor for the HTU31D driver.
 */
Adafruit_HTU31D::Adafruit_HTU31D() {
  /* Assign default values to internal tracking variables. */
  _last_humidity = 0.0f;
  _last_temp = 0.0f;
}

/**
 * Initialises the I2C transport, and configures the IC for normal operation.
 * @param theWire Pointer to TwoWire I2C object, uses &Wire by default
 * @param i2c_addr The sensor I2C address, default is 0x40 but can be 0x41
 * @return true if the device was successfully initialised
 */
bool Adafruit_HTU31D::begin(uint8_t i2c_addr, TwoWire *theWire) {
  if (i2c_dev) {
    delete i2c_dev;
  }
  i2c_dev = new Adafruit_I2CDevice(i2c_addr, theWire);

  if (!i2c_dev->begin()) {
    return false;
  }

  reset();

  Adafruit_BusIO_Register sernumreg =
      Adafruit_BusIO_Register(i2c_dev, HTU31D_READSERIAL, 4);
  uint8_t serdata[4];
  // verify we can read from the device, it will nak a failure
  if (!sernumreg.read(serdata, 4)) {
    return false;
  }

  return true;
}

/**
 * Sends a 'reset' request to the HTU31D, followed by a 15ms delay.
 */
void Adafruit_HTU31D::reset(void) {
  uint8_t cmd = HTU31D_RESET;
  i2c_dev->write(&cmd, 1);

  delay(15);
}

/**
 * Enable or disable the built in heater
 * @param en Enable or disable the heater
 * @returns True or false on command write success
 */
bool Adafruit_HTU31D::enableHeater(bool en) {
  uint8_t cmd;
  if (en) {
    cmd = HTU31D_HEATERON;
  } else {
    cmd = HTU31D_HEATEROFF;
  }
  return i2c_dev->write(&cmd, 1);
}

/**
 * Performs a single temperature conversion in degrees Celsius.
 *
 * @return a single-precision (32-bit) float value indicating the measured
 *         temperature in degrees Celsius or NAN on failure.
 */
float Adafruit_HTU31D::readTemperature(void) {
  uint8_t convert_cmd = HTU31D_CONVERSION;
  i2c_dev->write(&convert_cmd, 1);

  // wait conversion time
  delay(20);

  Adafruit_BusIO_Register threg =
      Adafruit_BusIO_Register(i2c_dev, HTU31D_READTEMPHUM, 6);
  uint8_t thdata[6];
  if (!threg.read(thdata, 6)) {
    return false;
  }

  uint16_t temp = thdata[0];
  temp <<= 8;
  temp |= thdata[1];

  uint8_t crc = htu31d_crc(temp);
  // Serial.print("CRC: 0x"); Serial.println(crc, HEX);
  if (crc != thdata[2]) {
    // some error :(
    return NAN;
  }

  float result = temp;
  result /= 65535.0;
  result *= 165;
  result -= 40;

  return result;
}

/**
 * Performs a single relative humidity conversion.
 *
 * @return A single-precision (32-bit) float value indicating the relative
 *         humidity in percent (0..100.0%).
 */
float Adafruit_HTU31D::readHumidity(void) {
  uint8_t convert_cmd = HTU31D_CONVERSION;
  i2c_dev->write(&convert_cmd, 1);

  // wait conversion time
  delay(20);

  Adafruit_BusIO_Register threg =
      Adafruit_BusIO_Register(i2c_dev, HTU31D_READTEMPHUM, 6);
  uint8_t thdata[6];
  if (!threg.read(thdata, 6)) {
    return false;
  }

  uint16_t hum = thdata[3];
  hum <<= 8;
  hum |= thdata[4];

  uint8_t crc = htu31d_crc(hum);
  // Serial.print("CRC: 0x"); Serial.println(crc, HEX);
  if (crc != thdata[5]) {
    // some error :(
    return NAN;
  }

  float result = hum;
  result /= 65535.0;
  result *= 100;

  return result;
}

/**
 * Performs a CRC8 calculation on the supplied values.
 *
 * @param data  Pointer to the data to use when calculating the CRC8.
 * @param len   The number of bytes in 'data'.
 *
 * @return The computed CRC8 value.
 */
static uint8_t htu31d_crc(uint16_t value) {
  uint32_t polynom = 0x988000; // x^8 + x^5 + x^4 + 1
  uint32_t msb = 0x800000;
  uint32_t mask = 0xFF8000;
  uint32_t result = (uint32_t)value << 8; // Pad with zeros as specified in spec

  while (msb != 0x80) {
    // Check if msb of current value is 1 and apply XOR mask
    if (result & msb)
      result = ((result ^ polynom) & mask) | (result & ~mask);

    // Shift by one
    msb >>= 1;
    mask >>= 1;
    polynom >>= 1;
  }
  return result;
}
