/*!
 * @file Adafruit_HTU31D.h
 */

#ifndef _ADAFRUIT_HTU31D_H
#define _ADAFRUIT_HTU31D_H

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>

/** Default I2C address for the HTU21D. */
#define HTU31D_DEFAULT_I2CADDR (0x40)

/** Read temperature and humidity. */
#define HTU31D_READTEMPHUM (0x00)

/** Start a conversion! */
#define HTU31D_CONVERSION (0x40)

/** Read serial number command. */
#define HTU31D_READSERIAL (0x0A)

/** Enable heater */
#define HTU31D_HEATERON (0x04)

/** Enable heater */
#define HTU31D_HEATEROFF (0x02)

/** Reset command. */
#define HTU31D_RESET (0x1E)

static uint8_t htu31d_crc(uint16_t value);

/**
 * Driver for the Adafruit HTU31D breakout board.
 */
class Adafruit_HTU31D {
public:
  Adafruit_HTU31D();

  bool begin(uint8_t i2c_addr = HTU31D_DEFAULT_I2CADDR,
             TwoWire *theWire = &Wire);
  float readTemperature(void);
  float readHumidity(void);
  void reset(void);
  bool enableHeater(bool en);

private:
  Adafruit_I2CDevice *i2c_dev = NULL; ///< Pointer to I2C bus interface
  float _last_humidity, _last_temp;
};

#endif /* _ADAFRUIT_HTU31D_H */
