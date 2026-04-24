/**
 * @file MotorDriver.h
 * @brief L293D DC motor driver — PWM speed control with direction.
 *
 *  Function            | Notes
 *  ------------------- | -----------------------------------------------
 *  motorInit()              | configures ENA, IN1, IN2 pins; motor stopped
 *  motorSetSpeed()          | set PWM duty 0-255 (forward only)
 *  motorSetPercent()        | set speed 0-100 % (forward only)
 *  motorSetPercentSigned()  | set speed -100 to +100 %; negative = reverse
 *  motorStop()              | coast stop (ENA LOW)
 *  motorBrake()             | active brake (IN1=IN2=HIGH)
 *  motorGetSpeed()          | current PWM value (0-255)
 *  motorGetPercent()        | current speed in percent (0-100)
 *  motorGetPercentSigned()  | current speed -100 to +100 % (signed)
 */

#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <Arduino.h>

#define MOTOR_PIN_ENA  6   // PWM speed (L293D EN1)
#define MOTOR_PIN_IN1  7   // direction A
#define MOTOR_PIN_IN2  9   // direction B

void    motorInit(void);
void    motorSetSpeed(uint8_t pwm);
void    motorSetPercent(float pct);
void    motorSetPercentSigned(float pct);
void    motorStop(void);
void    motorBrake(void);
uint8_t motorGetSpeed(void);
float   motorGetPercent(void);
float   motorGetPercentSigned(void);

#endif
