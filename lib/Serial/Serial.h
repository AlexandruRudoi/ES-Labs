#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <stdio.h>

void serialInit(unsigned long baudRate);
bool serialReadCommand(char* buffer, size_t size);
void serialPrint(const char* message);

#endif