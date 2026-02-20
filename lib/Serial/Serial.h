#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/** @brief Initialises the Serial port and redirects stdout and stdin to it.
 *  @param baudRate  Baud rate (e.g. 9600). */
void serialInit(unsigned long baudRate);

/** @brief Reads one newline-terminated command from stdin into buffer.
 *         Strips leading/trailing whitespace. Non-blocking — returns false
 *         if no complete command is available yet.
 *  @param buffer  Destination buffer.
 *  @param size    Size of the destination buffer in bytes.
 *  @return true if a complete command was read, false otherwise. */
bool serialReadCommand(char *buffer, size_t size);

/** @brief Prints a message followed by a newline to stdout.
 *  @param message  Null-terminated string to print. */
void serialPrint(const char *message);

/** @brief Removes all leading and trailing whitespace from a string in-place.
 *  @param str  Null-terminated string to trim. */
void trimSpaces(char *str);

#endif