#include <WiFi.h>
#include <WiFiManager.h> // github.com/tzapu/WiFiManager
#include "time.h"
#include <LiquidCrystal_I2C.h>

// #define DEBUG
// #define WIPE_STORED_CREDS

// WIFI
const char* AP_NAME = "Clock-AP";
const int WIFI_CHECK_INTERVAL = 60 * 1000;

// TIMEZONE
const char* POSIX_TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3"; // see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

// LCD
const int LCD_COLS = 16;
const int LCD_ROWS = 2;
const int LCD_ADDRESS = 0x27;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

/* Initialize the LCD */
void initLcd() {
  Serial.println("Initializing LCD");
  lcd.init();
  lcd.backlight(); // turns on the backlight
}

void wifiConfigCallback(WiFiManager *myWm) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("AP: %s", myWm->getConfigPortalSSID());
  lcd.setCursor(0, 1);
  lcd.printf("IP: %s", WiFi.softAPIP().toString());
}

/* Initialize the wifi connection with the ssid and password provided above */
void initWifi() {
  Serial.println("Initializing WiFi");

  // connect to wifi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting wifi");

  WiFi.mode(WIFI_STA);

  Serial.println("Starting WiFiManager");

  WiFiManager wm;
  wm.setAPCallback(wifiConfigCallback);

  // For testing only: erase previously saved WiFi credentials
  #ifdef WIPE_STORED_CREDS
  wm.resetSettings();
  #endif

  lcd.setCursor(0, 1);

  if (!wm.autoConnect(AP_NAME)) {
    Serial.println("Failed to connect or configure WiFi");
    lcd.print("Failed, resetting");
    delay(4000);
    ESP.restart();
  }

  Serial.println("Connected to WiFi!");
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString());
  
  lcd.clear();
  lcd.print("Connected!");
  delay(2000);
  lcd.clear();
}

/* Initialize the NTP connection and timezone */
void initTime() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Configuring NTP");
  lcd.setCursor(0, 1);

  // configure NTP server
  configTime(0, 0, "pool.ntp.org");

  // wait for time data
  struct tm timeInfoSetup;
  while (!getLocalTime(&timeInfoSetup, 500)) {
    lcd.print(".");
  }

  // set timezone
  setenv("TZ", POSIX_TIMEZONE, 1);
  tzset();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Got time!");
  delay(2000);
  lcd.clear();
}

void setup() {
  // connect to serial
  #ifdef DEBUG
  Serial.begin(115200);
  delay(1000);
  #endif

  Serial.println("\nStarting");

  initLcd();

  initWifi();

  initTime();
}

struct tm timeinfo;
unsigned long currentMillis;
unsigned long previousMillis;

void loop() {
  // check if wifi connected
  currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= WIFI_CHECK_INTERVAL)) {
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }

  // get localtime
  if (getLocalTime(&timeinfo)) {
    /*
    %A, %B %d %Y %H:%M:%S zone %Z %z 
    Tuesday, August 25 2026 17:18:57 zone CEST +0200
    %A -> day of week name
    %B -> month of year name
    %d -> day of month
    %m -> month of year
    %Y -> year
    %H -> hour
    %M -> minute
    %S -> second
    %Z -> zone abbr
    %z -> zone offset
    */
    lcd.setCursor(0, 0);
    lcd.print(&timeinfo, "date: %d-%m-%Y");
    lcd.setCursor(0, 1);
    lcd.print(&timeinfo, "time:   %H:%M:%S");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to get");
    lcd.setCursor(0, 1);
    lcd.print("time");
  }
  delay(1000);
}
