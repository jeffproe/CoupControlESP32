#ifndef main_H_
#define main_H_

#include <Arduino.h>
#include <vector>
#include <string.h>
#include <MyHTTPClient.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_system.h>

using namespace std;

struct Network {
  String ssid;
  int32_t channel;
  uint8_t bssid[6];
};

bool hasTime();
void updateTime();
bool updateTimeFromNetwork(struct Network &network);
bool updateTimeFromHttpResponseHeader(const String &url);
void showTime();
bool updateTimeFromDateString(const String &date);
static int stricmp(char const *a, char const *b);

#endif