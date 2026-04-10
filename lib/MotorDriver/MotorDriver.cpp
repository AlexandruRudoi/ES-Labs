#include "MotorDriver.h"

static uint8_t s_pwm = 0;

void motorInit(void)
{
    // disable PWM output first
    pinMode(MOTOR_PIN_ENA, OUTPUT);
    digitalWrite(MOTOR_PIN_ENA, LOW);

    // direction pins off until motor is actually started
    pinMode(MOTOR_PIN_IN1, OUTPUT);
    pinMode(MOTOR_PIN_IN2, OUTPUT);
    digitalWrite(MOTOR_PIN_IN1, LOW);
    digitalWrite(MOTOR_PIN_IN2, LOW);

    s_pwm = 0;
}

void motorSetSpeed(uint8_t pwm)
{
    s_pwm = pwm;
    if (s_pwm > 0)
    {
        digitalWrite(MOTOR_PIN_IN1, HIGH);
        digitalWrite(MOTOR_PIN_IN2, LOW);
    }
    else
    {
        digitalWrite(MOTOR_PIN_IN1, LOW);
        digitalWrite(MOTOR_PIN_IN2, LOW);
    }
    analogWrite(MOTOR_PIN_ENA, s_pwm);
}

void motorSetPercent(float pct)
{
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    motorSetSpeed((uint8_t)(pct * 2.55f + 0.5f));
}

void motorStop(void)
{
    s_pwm = 0;
    analogWrite(MOTOR_PIN_ENA, 0);
    digitalWrite(MOTOR_PIN_IN1, LOW);
    digitalWrite(MOTOR_PIN_IN2, LOW);
}

void motorBrake(void)
{
    s_pwm = 0;
    analogWrite(MOTOR_PIN_ENA, 0);
    digitalWrite(MOTOR_PIN_IN1, HIGH);
    digitalWrite(MOTOR_PIN_IN2, HIGH);
}

uint8_t motorGetSpeed(void)
{
    return s_pwm;
}

float motorGetPercent(void)
{
    return (float)s_pwm / 2.55f;
}
