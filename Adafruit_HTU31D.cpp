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

static uint8_t htu31d_crc(uint16_t value);

/**
 * Constructor for the HTU31D driver.
 */
Adafruit_HTU31D::Adafruit_HTU31D() {
  /* Assign default values to internal tracking variables. */
  _humidity = 0.0f;
  _temperature = 0.0f;
}

/*!
 * @brief  HTU31D destructor
 */
Adafruit_HTU31D::~Adafruit_HTU31D(void) {
  if (temp_sensor) {
    delete temp_sensor;
  }
  if (humidity_sensor) {
    delete humidity_sensor;
  }
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

  if (!reset()) {
    return false;
  }

  if (readSerial() == 0) {
    return false;
  }

  humidity_sensor = new Adafruit_HTU31D_Humidity(this);
  temp_sensor = new Adafruit_HTU31D_Temp(this);

  return true;
}

/**
 * Sends a 'reset' request to the HTU31D, followed by a 15ms delay.
 * @returns True if was able to write the command successfully
 */
bool Adafruit_HTU31D::reset(void) {
  uint8_t cmd = HTU31D_RESET;
  if (!i2c_dev->write(&cmd, 1)) {
    return false;
  }

  delay(15);
  return true;
}

/**
 * Gets the ID register contents.
 *
 * @return The 32-bit ID register.
 */
uint32_t Adafruit_HTU31D::readSerial(void) {
  uint8_t reply[4];
  uint32_t serial = 0;

  Adafruit_BusIO_Register sernumreg =
      Adafruit_BusIO_Register(i2c_dev, HTU31D_READSERIAL, 4);
  // verify we can read from the device, it will nak a failure
  if (!sernumreg.read(reply, 4)) {
    return 0;
  }

  serial = reply[0];
  serial <<= 8;
  serial |= reply[1];
  serial <<= 8;
  serial |= reply[2];
  serial <<= 8;
  serial |= reply[3];
  return serial;
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

/**************************************************************************/
/*!
    @brief  Gets the humidity sensor and temperature values as sensor events
    @param  humevent Sensor event object that will be populated with humidity
   data
    @param  tempevent Sensor event object that will be populated with temp data
    @returns true if the event data was read successfully
*/
/**************************************************************************/
bool Adafruit_HTU31D::getEvent(sensors_event_t *humevent,
                               sensors_event_t *tempevent) {
  uint32_t t = millis();

  uint8_t convert_cmd = HTU31D_CONVERSION;
  if (!i2c_dev->write(&convert_cmd, 1)) {
    return false;
  }

  // wait conversion time
  delay(20);

  Adafruit_BusIO_Register threg =
      Adafruit_BusIO_Register(i2c_dev, HTU31D_READTEMPHUM, 6);
  uint8_t thdata[6];
  if (!threg.read(thdata, 6)) {
    return false;
  }

  // Calculate temperature value
  uint16_t raw_temp = thdata[0];
  raw_temp <<= 8;
  raw_temp |= thdata[1];

  uint8_t crc = htu31d_crc(raw_temp);
  // Serial.print("CRC: 0x"); Serial.println(crc, HEX);
  if (crc != thdata[2]) {
    // some error :(
    return false;
  }

  _temperature = raw_temp;
  _temperature /= 65535.0;
  _temperature *= 165;
  _temperature -= 40;

  // Calculate temperature value
  uint16_t raw_hum = thdata[3];
  raw_hum <<= 8;
  raw_hum |= thdata[4];

  crc = htu31d_crc(raw_hum);
  // Serial.print("CRC: 0x"); Serial.println(crc, HEX);
  if (crc != thdata[5]) {
    // some error :(
    return NAN;
  }

  _humidity = raw_hum;
  _humidity /= 65535.0;
  _humidity *= 100;

  // use helpers to fill in the events
  if (tempevent)
    fillTempEvent(tempevent, t);
  if (humevent)
    fillHumidityEvent(humevent, t);
  return true;
}

void Adafruit_HTU31D::fillTempEvent(sensors_event_t *temp, uint32_t timestamp) {
  memset(temp, 0, sizeof(sensors_event_t));
  temp->version = sizeof(sensors_event_t);
  temp->sensor_id = _sensorid_temp;
  temp->type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
  temp->timestamp = timestamp;
  temp->temperature = _temperature;
}

void Adafruit_HTU31D::fillHumidityEvent(sensors_event_t *humidity,
                                        uint32_t timestamp) {
  memset(humidity, 0, sizeof(sensors_event_t));
  humidity->version = sizeof(sensors_event_t);
  humidity->sensor_id = _sensorid_humidity;
  humidity->type = SENSOR_TYPE_RELATIVE_HUMIDITY;
  humidity->timestamp = timestamp;
  humidity->relative_humidity = _humidity;
}

/**
 * @brief Gets the Adafruit_Sensor object for the HTU31D's humidity sensor
 *
 * @return Adafruit_Sensor*
 */
Adafruit_Sensor *Adafruit_HTU31D::getHumiditySensor(void) {
  return humidity_sensor;
}

/**
 * @brief Gets the Adafruit_Sensor object for the HTU31D's humidity sensor
 *
 * @return Adafruit_Sensor*
 */
Adafruit_Sensor *Adafruit_HTU31D::getTemperatureSensor(void) {
  return temp_sensor;
}
/**
 * @brief  Gets the sensor_t object describing the HTU31D's humidity sensor
 *
 * @param sensor The sensor_t object to be populated
 */
void Adafruit_HTU31D_Humidity::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy(sensor->name, "HTU31D_H", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name) - 1] = 0;
  sensor->version = 1;
  sensor->sensor_id = _sensorID;
  sensor->type = SENSOR_TYPE_RELATIVE_HUMIDITY;
  sensor->min_delay = 0;
  sensor->min_value = 0;
  sensor->max_value = 100;
  sensor->resolution = 2;
}
/**
    @brief  Gets the humidity as a standard sensor event
    @param  event Sensor event object that will be populated
    @returns True
 */
bool Adafruit_HTU31D_Humidity::getEvent(sensors_event_t *event) {
  _theHTU31D->getEvent(event, NULL);

  return true;
}
/**
 * @brief  Gets the sensor_t object describing the HTU31D's tenperature sensor
 *
 * @param sensor The sensor_t object to be populated
 */
void Adafruit_HTU31D_Temp::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy(sensor->name, "HTU31D_T", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name) - 1] = 0;
  sensor->version = 1;
  sensor->sensor_id = _sensorID;
  sensor->type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
  sensor->min_delay = 0;
  sensor->min_value = -40;
  sensor->max_value = 85;
  sensor->resolution = 0.3; // depends on calibration data?
}
/*!
    @brief  Gets the temperature as a standard sensor event
    @param  event Sensor event object that will be populated
    @returns true
*/
bool Adafruit_HTU31D_Temp::getEvent(sensors_event_t *event) {
  _theHTU31D->getEvent(NULL, event);

  return true;
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
