#pragma once

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

struct BMP280Reading {
  unsigned long timestampMs; // Time since boot when sample was taken
  float temperatureC;
  float pressurePa;
  float pressureHpa;
  float altitudeM;          // m (approx, valid to ~9 km)
};

class BMP280Module {
  public:
    BMP280Module(uint8_t i2cAddress = 0x76,
                float seaLevelHpa = 1013.25f,
                unsigned long sampleIntervalMs = 200UL);

    // Call once in setup, returns true if the sensor is found and configured.
    bool begin();

    // Call often in loop, returns true when a new reading is available.
    bool update();

    // Check if last reading is within datasheet range
    bool isValid() const;

    const BMP280Reading &getLastReading() const;

    void setSeaLevelPressure(float seaLevelHpa);
    void setSampleIntervalMs(unsigned long intervalMs);

  private:
    Adafruit_BMP280 bmp_;
    BMP280Reading lastReading_;

    uint8_t i2cAddress_;
    float seaLevelHpa_;
    unsigned long sampleIntervalMs_;
    unsigned long lastSampleMs_;
    bool initialized_;
};