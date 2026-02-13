#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void serialInit(unsigned long baudRate);
bool serialReadCommand(char* buffer, size_t size);
void serialPrint(const char* message);
void trimSpaces(char* str);

#endif