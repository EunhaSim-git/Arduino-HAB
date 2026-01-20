#include <Wire.h>
#include <WDT.h> 
#include "GPSModule.h"
#include "SDCardModule.h"
#include "BMP280Module.h"
#include "TempModule.h" 
#include "Arduino_LED_Matrix.h" // Include the LED Matrix library
#include "config.h"

ArduinoLEDMatrix matrix; // Create an LED matrix object

const long wdtInterval = 5000;  // 5 seconds timeout
unsigned long wdtMillis = 0;

// LED blink timing
const int errorBlinkInterval = 500; // milliseconds
bool errorLedState = false;
unsigned long lastBlinkTime = 0;

bool isTerminating = false;
unsigned long terminationStart = 0;

// 5 Hz -> 200 ms interval for BMP280
const unsigned long SENSOR_INTERVAL_MS = 200UL; // 5Hz
unsigned long lastSensorMs = 0;

// 1 Hz -> 1000 ms interval for AHT10
const unsigned long AHT10_INTERVAL_MS = 1000UL;  // 1 Hz
unsigned long lastAht10Ms = 0;


// Create BMP280 module instance
BMP280Module bmp280(0x77, 1013.25f, SENSOR_INTERVAL_MS);


uint8_t smallCircle[8][12] = {
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,1,1,1,1,0,0,0,0,0},
  {0,0,1,0,0,0,0,1,0,0,0,0},
  {0,1,0,0,0,0,0,0,1,0,0,0},
  {0,1,0,0,0,0,0,0,1,0,0,0},
  {0,0,1,0,0,0,0,1,0,0,0,0},
  {0,0,0,1,1,1,1,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0}
};

uint8_t xPattern[8][12] = {
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {0,1,0,0,0,0,0,0,0,0,1,0},
  {0,0,1,0,0,0,0,0,0,1,0,0},
  {0,0,0,1,0,0,0,0,1,0,0,0},
  {0,0,0,0,1,0,0,1,0,0,0,0},
  {0,0,0,0,0,1,1,0,0,0,0,0},
  {0,0,0,0,0,1,1,0,0,0,0,0},
  {0,0,0,0,1,0,0,1,0,0,0,0}
};

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  matrix.begin(); 

  Serial.begin(115200);
  while (!Serial);

  delay(1000);
  Serial.println(F("BMP280 5Hz"));
  Serial.println(F("NEO-6M GPS + SD Logging"));
  Serial.print(F("TinyGPSPlus v")); Serial.println(TinyGPSPlus::libraryVersion());

  // Track module initialization status
  bool sdOK = initSDCard(SD_CS_PIN);
  bool bmpOK = bmp280.begin();
  bool tempOK = initAHT10();

  if (!sdOK) Serial.println(F("SD card initialization failed."));
  if (!bmpOK) Serial.println(F("BMP280 initialization failed."));


  Serial.print("SD Card status: ");
  Serial.println(sdOK ? "Success" : "Failed");

  Serial.print("BMP280 status: ");
  Serial.println(bmpOK ? "Success" : "Failed");

  Serial.print("AHT10 init: ");
  Serial.println(tempOK ? "Success" : "FAILED");

  
  if (!sdOK || !bmpOK|| !tempOK) {
    errorLedState = true;
  }

  Serial1.begin(9600); // GPS

  lastSensorMs = millis();
  lastAht10Ms = millis();
}

void loop() {
  // Refresh watchdog (if enabled)
  /*
  if (millis() - wdtMillis >= wdtInterval - 1) {
    WDT.refresh();
    wdtMillis = millis();
  }
  */

  // Error LED handling
  if (errorLedState) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if(hasGPSFix()){
    matrix.renderBitmap(smallCircle, 8, 12);
  }
  else{
    matrix.renderBitmap(xPattern, 8, 12);
  }

  unsigned long now = millis();

  // 5Hz sensor logging 
  if (now - lastSensorMs >= SENSOR_INTERVAL_MS) {
    // Move the timestamp forward by exactly one interval
    lastSensorMs += SENSOR_INTERVAL_MS;

    // BMP280 data
    if (bmp280.update()) {                     // will be true whenever it takes a new sample
    const BMP280Reading &r = bmp280.getLastReading();

      if (!bmp280.isValid()) {                // checks the last reading's pressure against the range (300-1100 hPa)
        logToSDCard("Warning: BMP280 out-of-range reading");
      } else {
        String barometerLog = String("Temp: ") + r.temperatureC +
                                "C, Pressure: " + r.pressureHpa +
                                "hPa, Altitude: " + r.altitudeM +
                                "m";
        logToSDCard(barometerLog);
      }
    } 

    // GPS data
    if (readGPSData(gps)) {
      String gpsLog = String("Lat: ") + gps.location.lat() +
                      ",Lng: " + gps.location.lng() +
                      ",Date: " + gps.date.month() + "/" + gps.date.day() + "/" + gps.date.year() +
                      ",Time: " + gps.time.hour() + ":" + gps.time.minute() + ":" + gps.time.second();
      logToSDCard(gpsLog);  
    } else {
      logToSDCard("Error: GPS reading failed!");
    }
  }
  

   // Temp and humidity data at 1Hz
   if (now - lastAht10Ms >= AHT10_INTERVAL_MS) {
    lastAht10Ms += AHT10_INTERVAL_MS;

    float tC, hPct;
    if (readAHT10Data(tC, hPct)) {
      String tempLog = String("Temp: ") + tC + "C, Humidity: " + hPct + "%";
      logToSDCard(tempLog);
    } else {
      logToSDCard("Error: AHT10 reading failed!");
    }
  }

  // Before termination logic, get current altitude from BMP280
  float altitude = bmp280.getLastReading().altitudeM;

  // Termination
  unsigned long currentTime = millis();  
  if(terminationStart == 0 && ((currentTime > TERMINATION_TIME  && !terminationStart) || altitude > TERMINATION_HEIGHT)) {
    logToSDCard("TERMINATING FLIGHT"); 
    digitalWrite(RELAY_PIN, HIGH); // Turn relay ON
    isTerminating = true;
    terminationStart = currentTime;
  }
  else if(terminationStart && currentTime - terminationStart > TERMINATION_CUT_TIME){
    logToSDCard("FLIGHT TERMINATION COMPLETE"); 
    digitalWrite(RELAY_PIN, LOW);  // Turn relay OFF
    isTerminating = false;
  }
  
}