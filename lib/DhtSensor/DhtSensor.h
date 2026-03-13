/**
 * @file DhtSensor.h
 * @brief DHT11 temperature and humidity sensor driver.
 *
 * Wraps the DHT library with a simple C-style interface.
 * The sensor requires a minimum 1 s between reads; this driver
 * enforces that internally and returns the last good reading when
 * called more frequently.
 *
 *  Function              | Caller  | Notes
 *  --------------------- | ------- | ---------------------------------
 *  dhtInit()             | setup   | call once before dhtRead()
 *  dhtRead()             | Task 1  | safe to call every 50 ms
 *  dhtGetTemperature()   | Task 2  | last successful reading in °C
 *  dhtGetHumidity()      | Task 2  | last successful reading in %RH
 *  dhtIsValid()          | Task 2  | false until first good read
 */

#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <Arduino.h>
#include <stdbool.h>

/** Arduino digital pin the DHT11 data line is connected to. */
#define DHT_PIN          2

/** Minimum interval between DHT11 reads (sensor spec: 1 s). */
#define DHT_MIN_INTERVAL_MS 1000UL

/** @brief Initialises the DHT11 sensor. Call once in setup(). */
void dhtInit(void);

/**
 * @brief Attempt to read temperature and humidity from the sensor.
 *        Enforces the 1 s minimum interval internally — safe to call
 *        every 50 ms from the acquisition task.
 *        Updates the internal cached values on success.
 */
void dhtRead(void);

/** @brief Returns the most recently measured temperature in °C.
 *         Value is 0.0 until the first successful read. */
float dhtGetTemperature(void);

/** @brief Returns the most recently measured relative humidity in %.
 *         Value is 0.0 until the first successful read. */
float dhtGetHumidity(void);

/** @brief Returns true if at least one successful read has occurred. */
bool dhtIsValid(void);

#endif
