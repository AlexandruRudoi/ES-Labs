#include <Arduino.h>

#include "Lab1_1/Lab1_1.h"
#include "Lab1_2/Lab1_2.h"
#include "Lab2_1/Lab2_1.h"

// switch active lab here
#define ACTIVE_LAB 3

void setup()
{
#if ACTIVE_LAB == 1
    lab11Setup();
#elif ACTIVE_LAB == 2
    lab12Setup();
#elif ACTIVE_LAB == 3
    lab21Setup();
#endif
}

void loop()
{
#if ACTIVE_LAB == 1
    lab11Loop();
#elif ACTIVE_LAB == 2
    lab12Loop();
#elif ACTIVE_LAB == 3
    lab21Loop();
#endif
}

