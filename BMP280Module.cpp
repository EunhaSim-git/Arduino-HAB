#include "BMP280Module.h"

BMP280Module::BMP280Module(uint8_t i2cAddress,
                           float seaLevelHpa,
                           unsigned long sampleIntervalMs)
    : i2cAddress_(i2cAddress),
      seaLevelHpa_(seaLevelHpa),
      sampleIntervalMs_(sampleIntervalMs),
      lastSampleMs_(0),
      initialized_(false) {
    lastReading_.timestampMs   = 0;
    lastReading_.temperatureC  = NAN;
    lastReading_.pressurePa    = NAN;
    lastReading_.pressureHpa   = NAN;
    lastReading_.altitudeM     = NAN;
}

bool BMP280Module::begin() {
    // Try to initialize sensor at given I2C address
    if (!bmp_.begin(i2cAddress_)) {         // Adafruit BMP280 begin() [web:3][web:2]
        initialized_ = false;
        return false;
    }

    // HAB‑friendly config: normal mode, decent oversampling, strong filter. [web:4][web:23][web:27]
    bmp_.setSampling(
        Adafruit_BMP280::MODE_NORMAL,
        Adafruit_BMP280::SAMPLING_X2,      // temperature oversampling
        Adafruit_BMP280::SAMPLING_X16,     // pressure oversampling
        Adafruit_BMP280::FILTER_X16,       // IIR filter
        Adafruit_BMP280::STANDBY_MS_500    // standby (internal ODR, fine for low‑noise)
    );

    lastSampleMs_ = millis();
    initialized_  = true;
    return true;
}

bool BMP280Module::update() {
    if (!initialized_) {
        return false;
    }

    unsigned long now = millis();

    if (now - lastSampleMs_ < sampleIntervalMs_) {
        return false;                       // not time yet for a new sample
    }

    // Keep schedule stable
    lastSampleMs_ += sampleIntervalMs_;

    float tC        = bmp_.readTemperature();          // °C [web:4][web:2]
    float pPa       = bmp_.readPressure();             // Pa
    float pHpa      = pPa / 100.0f;                    // hPa
    float altM      = bmp_.readAltitude(seaLevelHpa_); // m (approx) [web:4][web:25]

    lastReading_.timestampMs  = now;
    lastReading_.temperatureC = tC;
    lastReading_.pressurePa   = pPa;
    lastReading_.pressureHpa  = pHpa;
    lastReading_.altitudeM    = altM;

    return true;
}

bool BMP280Module::isValid() const {
    // BMP280 datasheet: 300–1100 hPa operating range. [web:23][web:110]
    if (isnan(lastReading_.pressureHpa)) {
        return false;
    }
    return (lastReading_.pressureHpa >= 300.0f &&
            lastReading_.pressureHpa <= 1100.0f);
}


const BMP280Reading &BMP280Module::getLastReading() const {
    return lastReading_;
}

void BMP280Module::setSeaLevelPressure(float seaLevelHpa) {
    seaLevelHpa_ = seaLevelHpa;
}

void BMP280Module::setSampleIntervalMs(unsigned long intervalMs) {
    sampleIntervalMs_ = intervalMs;
}
