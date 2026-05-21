/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */

#include "setting.h"
#include "icon.h"
#include "config.h"

RF24 RadioA(NRF_CE_PIN_A, NRF_CSN_PIN_A);
RF24 RadioB(NRF_CE_PIN_B, NRF_CSN_PIN_B);
RF24 RadioC(NRF_CE_PIN_C, NRF_CSN_PIN_C);

void setRadiosNeutralState() {
  RadioA.stopListening();
  RadioA.setAutoAck(false);
  RadioA.setRetries(0, 0);
  RadioA.powerDown(); 
  digitalWrite(NRF_CE_PIN_A, LOW); 

  RadioB.stopListening();
  RadioB.setAutoAck(false);
  RadioB.setRetries(0, 0);
  RadioB.powerDown(); 
  digitalWrite(NRF_CE_PIN_B, LOW); 

  RadioC.stopListening();
  RadioC.setAutoAck(false);
  RadioC.setRetries(0, 0);
  RadioC.powerDown(); 
  digitalWrite(NRF_CE_PIN_C, LOW); 
}

void configureNrf(RF24 &radio) {
  radio.begin();
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setDataRate(RF24_2MBPS);
  radio.setCRCLength(RF24_CRC_DISABLED);
}

void setupRadioA() {
  configureNrf(RadioA);
}

void setupRadioB() {
  configureNrf(RadioB);
}

void setupRadioC() {
  configureNrf(RadioC);
}

void initAllRadios() {
  setupRadioA();
  setupRadioB();
  setupRadioC();
}

void Str(uint8_t x, uint8_t y, const uint8_t* asciiArray, size_t len) {
  char buf[64]; 
  for (size_t i = 0; i < len && i < sizeof(buf) - 1; i++) {
    buf[i] = (char)asciiArray[i];
  }
  buf[len] = '\0';

  u8g2.drawStr(x, y, buf);
}

void CenteredStr(uint8_t screenWidth, uint8_t y, const uint8_t* asciiArray, size_t len, const uint8_t* font) {
  char buf[64];
  for (size_t i = 0; i < len && i < sizeof(buf) - 1; i++) {
    buf[i] = (char)asciiArray[i];
  }
  buf[len] = '\0';

  u8g2.setFont((const uint8_t*)font);
  int16_t w = u8g2.getUTF8Width(buf);
  u8g2.setCursor((screenWidth - w) / 2, y);
  u8g2.print(buf);
}

void utils() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  int void_width = u8g2.getUTF8Width("VOID");
  u8g2.drawStr((128 - void_width) / 2, 15, "VOID");
  
  u8g2.setFont(u8g2_font_5x8_tr);
  int desc1_width = u8g2.getUTF8Width("Modified version");
  int desc2_width = u8g2.getUTF8Width("of the nRFBox repo.");
  
  u8g2.drawStr((128 - desc1_width) / 2, 35, "Modified version");
  u8g2.drawStr((128 - desc2_width) / 2, 45, "of the nRFBox repo.");
  
  u8g2.sendBuffer();
}

int bootSequenceSpeed = 1; // 0 = Quick, 1 = Normal, 2 = Long
int screensaverType = 0; // 0 = Cat, 1 = Matrix

void conf() {
  u8g2.setBitmapMode(1);
  u8g2.clearBuffer();
  CenteredStr(128, 25, txt_n, sizeof(txt_n), u8g2_font_ncenB14_tr);
  CenteredStr(128, 40, txt_c, sizeof(txt_c), u8g2_font_ncenB08_tr);
  CenteredStr(128, 60, txt_v, sizeof(txt_v), u8g2_font_6x10_tf);
  u8g2.sendBuffer();
  
  if (bootSequenceSpeed == 0) delay(1000);
  else if (bootSequenceSpeed == 1) delay(3000);
  else delay(5000);

  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 128, 64, cred); 
  u8g2.sendBuffer();

  if (bootSequenceSpeed == 0) delay(250);
  else if (bootSequenceSpeed == 1) delay(1500);
  else delay(3000);
}

namespace Setting {

#define EEPROM_ADDRESS_NEOPIXEL 0
#define EEPROM_ADDRESS_BRIGHTNESS 1
#define EEPROM_ADDRESS_BOOT_SPEED 2
#define EEPROM_ADDRESS_SCREENSAVER 3

int currentOption = 0;
int totalOptions = 5; 

bool buttonUpPressed = false;
bool buttonDownPressed = false;
bool buttonSelectPressed = false;

void updateFirmware() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 15, "Updating Firmware.");
  u8g2.sendBuffer();

  u8g2.setFont(u8g2_font_5x8_tr);
  if (!SD.begin(SD_CS_PIN)) {
    u8g2.drawStr(0, 30, "[ SD Init Failed ]");
    u8g2.sendBuffer();
    delay(2000);
    return;
  }

  if (!SD.exists(FIRMWARE_FILE)) {
    u8g2.drawStr(0, 30, "[ File Not Found ]");
    u8g2.sendBuffer();
    delay(2000);
    return;
  }

  File firmware = SD.open(FIRMWARE_FILE);
  if (!firmware) {
    u8g2.drawStr(0, 30, "[ Open Failed ]");
    u8g2.sendBuffer();
    delay(2000);
    return;
  }

  if (firmware) {
    u8g2.drawStr(0, 30, "[ Wait a Moment ]");
    u8g2.sendBuffer();
  }

  if (!Update.begin(firmware.size())) {
    u8g2.drawStr(0, 30, "[ Update Init Failed ]");
    u8g2.sendBuffer();
    firmware.close();
    delay(2000);
    return;
  }

  Update.writeStream(firmware);
  if (Update.end(true)) {
    u8g2.drawStr(0, 45, "[ Update Success! ]");
    u8g2.sendBuffer();
    delay(1000);
    ESP.restart();
  } else {
    u8g2.drawStr(0, 45, "[ Update Failed ]");
    u8g2.sendBuffer();
    delay(2000);
  }

  firmware.close();
}

void toggleOption(int option) {
  if (option == 0) { 
    neoPixelActive = !neoPixelActive;
    EEPROM.write(EEPROM_ADDRESS_NEOPIXEL, neoPixelActive);
    EEPROM.commit();
    Serial.print("NeoPixel is now ");
    Serial.println(neoPixelActive ? "Enabled" : "Disabled");

  } else if (option == 1) { 
    uint8_t brightnessPercent = map(oledBrightness, 0, 255, 0, 100); 
    brightnessPercent += 10; 
    if (brightnessPercent > 100) brightnessPercent = 10; 
    oledBrightness = map(brightnessPercent, 0, 100, 0, 255); 

    u8g2.setContrast(oledBrightness); 
    EEPROM.write(EEPROM_ADDRESS_BRIGHTNESS, oledBrightness);
    EEPROM.commit();

    Serial.print("Brightness set to: ");
    Serial.print(brightnessPercent);
    Serial.println("%");

  } else if (option == 2) {
    bootSequenceSpeed = (bootSequenceSpeed + 1) % 3;
    EEPROM.write(EEPROM_ADDRESS_BOOT_SPEED, bootSequenceSpeed);
    EEPROM.commit();
    Serial.print("Boot Speed set to: ");
    Serial.println(bootSequenceSpeed == 0 ? "Quick" : (bootSequenceSpeed == 1 ? "Normal" : "Long"));
  } else if (option == 3) {
    screensaverType = (screensaverType + 1) % 2;
    EEPROM.write(EEPROM_ADDRESS_SCREENSAVER, screensaverType);
    EEPROM.commit();
    Serial.print("Screen Saver set to: ");
    Serial.println(screensaverType == 0 ? "Cat" : "Matrix");
  } else if (option == 4) {
    updateFirmware();
  }
}

void handleButtons() {
  if (!digitalRead(BUTTON_UP_PIN)) {
    if (!buttonUpPressed) {
      buttonUpPressed = true;
      currentOption = (currentOption - 1 + totalOptions) % totalOptions;
    }
  } else {
    buttonUpPressed = false;
  }

  if (!digitalRead(BUTTON_DOWN_PIN)) {
    if (!buttonDownPressed) {
      buttonDownPressed = true;
      currentOption = (currentOption + 1) % totalOptions;
    }
  } else {
    buttonDownPressed = false;
  }

  if (!digitalRead(BTN_PIN_RIGHT)) {
    if (!buttonSelectPressed) {
      buttonSelectPressed = true;
      toggleOption(currentOption);
    }
  } else {
    buttonSelectPressed = false;
  }
}

void displayMenu() {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Settings:");

  u8g2.setFont(u8g2_font_5x8_tr);
  
  // Create a scroll offset if we are on option 4
  int startOption = 0;
  if (currentOption >= 4) startOption = 1;

  int y_pos = 25;
  if (startOption <= 0) { u8g2.drawStr(0, y_pos, currentOption == 0 ? "> NeoPixel: " : "  NeoPixel: "); u8g2.setCursor(80, y_pos); u8g2.print(neoPixelActive ? "Enabled" : "Disabled"); y_pos += 12; }
  if (startOption <= 1) { u8g2.drawStr(0, y_pos, currentOption == 1 ? "> Brightness: " : "  Brightness: "); u8g2.setCursor(80, y_pos); uint8_t brightnessPercent = map(oledBrightness, 0, 255, 0, 100); u8g2.print(brightnessPercent); u8g2.print("%"); y_pos += 12; }
  if (startOption <= 2) { u8g2.drawStr(0, y_pos, currentOption == 2 ? "> Boot Speed: " : "  Boot Speed: "); u8g2.setCursor(80, y_pos); u8g2.print(bootSequenceSpeed == 0 ? "Quick" : (bootSequenceSpeed == 1 ? "Normal" : "Long")); y_pos += 12; }
  if (startOption <= 3) { u8g2.drawStr(0, y_pos, currentOption == 3 ? "> ScreenSaver:" : "  ScreenSaver:"); u8g2.setCursor(80, y_pos); u8g2.print(screensaverType == 0 ? "Cat" : "Matrix"); y_pos += 12; }
  if (y_pos <= 61) { u8g2.drawStr(0, y_pos, currentOption == 4 ? "> Update Firmware" : "  Update Firmware"); }

  u8g2.sendBuffer();
}

void settingSetup() {
  Serial.begin(115200);

  EEPROM.begin(512);

  neoPixelActive = EEPROM.read(EEPROM_ADDRESS_NEOPIXEL);
  oledBrightness = EEPROM.read(EEPROM_ADDRESS_BRIGHTNESS);
  bootSequenceSpeed = EEPROM.read(EEPROM_ADDRESS_BOOT_SPEED);
  screensaverType = EEPROM.read(EEPROM_ADDRESS_SCREENSAVER);
  
  if (bootSequenceSpeed > 2) bootSequenceSpeed = 1; // Default to Normal
  if (screensaverType > 1) screensaverType = 0; // Default to Cat
  
  if (oledBrightness > 255) oledBrightness = 128; 
  if (oledBrightness == 0) oledBrightness = 25; 
  u8g2.setContrast(oledBrightness);

  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN_RIGHT, INPUT_PULLUP);
}

void settingLoop() {
  handleButtons();
  displayMenu();
  }
} 
