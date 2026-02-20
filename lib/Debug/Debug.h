#pragma once

#include <Arduino.h>

/** Set to 1 to enable debug output over Serial, 0 to disable.
 *  When 0 all methods are empty and stripped by the compiler. */
#define DEBUG_ENABLED 1

/** Lightweight debug logger that writes to the hardware Serial port.
 *  All output is conditionally compiled — set DEBUG_ENABLED 0 for release. */
class Debug
{
public:
    /** @brief Initialises the Serial port at the given baud rate.
     *  @param baud  Baud rate (e.g. 9600). */
    void begin(unsigned long baud);

    /** @brief Prints a message followed by a newline.
     *  @param msg  Null-terminated string to print. */
    void print(const char* msg);

    /** @brief Prints a key-value pair where the value is a string.
     *  @param key  Label printed before the value.
     *  @param val  Null-terminated string value. */
    void printVal(const char* key, const char* val);

    /** @brief Prints a key-value pair where the value is a single character.
     *  @param key  Label printed before the value.
     *  @param val  Character value. */
    void printVal(const char* key, char val);
};

extern Debug dbg;
