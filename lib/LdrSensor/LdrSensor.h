/**
 * @file LdrSensor.h
 * @brief Photoresistor (LDR) analog light sensor driver.
 *
 * Reads a raw 10-bit ADC value from an analog pin and converts it
 * to a light level percentage (0–100%). Uses a voltage divider with
 * a 10kΩ pull-down resistor.
 *
 *  Function              | Caller  | Notes
 *  --------------------- | ------- | ---------------------------------
 *  ldrInit()             | setup   | call once before ldrRead()
 *  ldrRead()             | Task 1  | reads ADC and updates cache
 *  ldrGetRaw()           | Task 2  | last raw ADC value (0–1023)
 *  ldrGetLightPercent()  | Task 2  | light level as 0.0–100.0 %
 */

#ifndef LDR_SENSOR_H
#define LDR_SENSOR_H

#include <Arduino.h>

/** Arduino analog pin the LDR voltage divider output is connected to. */
#define LDR_PIN         A2

/** ADC resolution (10-bit → 1023). */
#define LDR_ADC_MAX     1023

/** @brief Initialises the LDR analog pin. Call once in setup(). */
void ldrInit(void);

/** @brief Read the ADC and update the cached light value. */
void ldrRead(void);

/** @brief Returns the last raw ADC reading (0–1023). */
uint16_t ldrGetRaw(void);

/** @brief Returns the light level as a percentage (0.0–100.0). */
float ldrGetLightPercent(void);

#endif // LDR_SENSOR_H
