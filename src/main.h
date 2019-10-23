#ifndef main_H_
#define main_H_

#include <Arduino.h>
#include <vector>
#include <string.h>
#include <MyHTTPClient.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>


using namespace std;

struct Network {
  String ssid;
  int32_t channel;
  uint8_t bssid[6];
};

#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5

Adafruit_BMP280 _bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);//software SPI


// Output Relays
#define _pinHeat 22
#define _pinLight 21

bool _invertRelay = true;

unsigned long _lightInterval = 1000;
unsigned long _lastLightTime = 0;
unsigned long _readInterval = 2000; // read temps every 2 seconds
unsigned long _lastReadTime = 0;
int _checkTempInterval = 30;        // change lamp state every 30 _readIntervals (60 seconds)
int _checkTemp = _checkTempInterval - 1;

float _targetInternalTemp = 40.0;
float _minInternalTemp = 35.0;
float _tempInternal = 0;
float _lowTemp = 999;

bool _heat = false;

void SetupBmp280();
void SetupTime();
void HandleTemps(unsigned long currentMillis);
void HandleLights(unsigned long currentMillis);
bool HasTime();
void UpdateTime();
bool UpdateTimeFromNetwork(struct Network &network);
bool UpdateTimeFromHttpResponseHeader(const String &url);
void ShowTime();
bool UpdateTimeFromDateString(const String &date);
void WriteToDebug();
float ReadInternalTempC();
float ReadInternalTempF();
static int stricmp(char const *a, char const *b);

#endif