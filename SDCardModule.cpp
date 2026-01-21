#include "SDCardModule.h"

File myFile;
bool sdReady = false;

bool initSDCard(uint8_t csPin) {
    if (!SD.begin(csPin)) {
        Serial.println(F("SD card initialization failed!"));
        return false;
    }
    Serial.println(F("SD card initialized."));

    myFile = SD.open("log.txt", FILE_WRITE);
    if (!myFile){
        Serial.println(F("Failed to open log.txt for writing!"));
        sdReady = false;
        return false;
    }

    sdReady = true;
    return true;
}

String getTimestamp(){
    unsigned long currentTime = millis();  

    unsigned long hours = (currentTime / 3600000) % 24;
    unsigned long minutes = (currentTime / 60000) % 60;
    unsigned long seconds = (currentTime / 1000) % 60;

    String timestamp = String(hours) + ":" + String(minutes) + ":" + String(seconds);

    return timestamp;
}

void logToSDCard(const String &data) {
    if(!sdReady || !myFile) {
        Serial.println(F("SD not ready, cannot log."));
        return;
    }

    String timestamp = getTimestamp();
    
    myFile.print(timestamp);
    myFile.print(" -> ");     
    myFile.println(data);
    
    Serial.print("Logged to SD card: ");
    Serial.println(timestamp + " -> " + data);
}

void logErrorToSDCard(const String &message) {
    if(!sdReady || !myFile) {
        Serial.println(F("SD not ready, log error!"));
        return;
    }

    String timestamp = getTimestamp();

    
    myFile.print("**ERROR** ");  
    myFile.print(timestamp);  
    myFile.print(" -> ");      
    myFile.println(message);     

    Serial.print("Logged to SD card: ");
    Serial.println(timestamp + " -> " + message);
    }
}

void flushSD() {
    if (sdReady && myFile) {
        myFile.flush();
    }
}

void closeSD() {
    if (sdReady && myFile) {
        myFile.flush();
        myFile.close();
        seReady = false;
    }
}