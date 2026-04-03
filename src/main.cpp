#include <Arduino.h>

#include "Lab1_1/Lab1_1.h"
#include "Lab1_2/Lab1_2.h"
#include "Lab2_1/Lab2_1.h"
#include "Lab2_2/Lab2_2.h"
#include "Lab3_1/Lab3_1.h"
#include "Lab3_2/Lab3_2.h"
#include "Lab4_1/Lab4_1.h"
#include "Lab4_2/Lab4_2.h"

#define ACTIVE_LAB 8

void setup()
{
#if ACTIVE_LAB == 1
    lab11Setup();
#elif ACTIVE_LAB == 2
    lab12Setup();
#elif ACTIVE_LAB == 3
    lab21Setup();
#elif ACTIVE_LAB == 4
    lab22Setup();
#elif ACTIVE_LAB == 5
    lab31Setup();
#elif ACTIVE_LAB == 6
    lab32Setup();
#elif ACTIVE_LAB == 7
    lab41Setup();
#elif ACTIVE_LAB == 8
    lab42Setup();
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
#elif ACTIVE_LAB == 4
    lab22Loop();
#elif ACTIVE_LAB == 5
    lab31Loop();
#elif ACTIVE_LAB == 6
    lab32Loop();
#elif ACTIVE_LAB == 7
    lab41Loop();
#elif ACTIVE_LAB == 8
    lab42Loop();
#endif
}