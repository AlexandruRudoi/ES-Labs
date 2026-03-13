/**
 * @file NtcSensor.h
 * @brief NTC thermistor analog sensor driver.
 *
 * Reads a raw 10-bit ADC value from an analog pin and converts it to
 * temperature in °C using a two-point linear calibration formula:
 *
 *   T = T1 + (T2 - T1) * (raw - raw1) / (raw2 - raw1)
 *
 * where (raw1, T1) and (raw2, T2) are two calibration points taken
 * from the sensor's datasheet or measured experimentally.
 *
 * NOTE: The two-point linear conversion is sufficient for Lab 3.1
 * (binary threshold alerting). Lab 3.2 will extend this driver with
 * saturation, median filter, weighted average, and a full NTC LUT.
 *
 *  Function              | Caller  | Notes
 *  --------------------- | ------- | ---------------------------------
 *  ntcInit()             | setup   | call once before ntcRead()
 *  ntcRead()             | Task 1  | reads ADC and updates cache
 *  ntcGetRaw()           | Task 2  | last raw ADC value (0–1023)
 *  ntcGetTemperature()   | Task 2  | converted temperature in °C
 */

#ifndef NTC_SENSOR_H
#define NTC_SENSOR_H

#include <Arduino.h>

/** Arduino analog pin the NTC voltage divider output is connected to. */
#define NTC_PIN         A1

/** ADC reference voltage (5 V for Arduino Mega/Uno). */
#define NTC_VREF        5.0f

/** ADC resolution (10-bit → 1023). */
#define NTC_ADC_MAX     1023

/**
 * Calibration point 1: raw ADC value and corresponding temperature (°C).
 * Adjust to match your specific thermistor + voltage divider.
 *
 * For a 10 kΩ NTC with 10 kΩ fixed resistor (voltage divider to 5 V):
 *   raw ≈ 487 at ~22 °C  (measured)
 *   raw ≈ 340 at ~40 °C  (estimated from NTC curve)
 */
#define NTC_RAW1        340
#define NTC_TEMP1       40.0f

/** Calibration point 2. */
#define NTC_RAW2        600
#define NTC_TEMP2       10.0f

/** @brief Initialises the NTC sensor (sets pin to INPUT). Call once in setup(). */
void ntcInit(void);

/**
 * @brief Reads the ADC value and converts it to temperature.
 *        Safe to call every 20–100 ms from the acquisition task.
 */
void ntcRead(void);

/** @brief Returns the raw ADC value (0–1023) from the last ntcRead(). */
uint16_t ntcGetRaw(void);

/** @brief Returns the converted temperature in °C from the last ntcRead(). */
float ntcGetTemperature(void);

#endif
