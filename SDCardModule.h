#ifndef SDCARD_MODULE_H
#define SDCARD_MODULE_H

#include <SD.h>
#include <SPI.h>

bool initSDCard(uint8_t csPin = 4);

void logToSDCard(const String &data);

void logErrorToSDCard(const String &message);

void flushSD();

void closeSD();

String getTimestamp();

#endif
