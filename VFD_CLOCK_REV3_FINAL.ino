#include <DS3231M.h>  // Include the DS3231M RTC library
#include <Adafruit_NeoPixel.h>  // RGB LED library
#include <EEPROM.h> // read/write user settings


// Setting constants
int SET_LONG_PRESS_DUR = 2000;  // duration required to be qualified as a long press, in millis.
int SET_LED_UPDATE_INTERVAL = 50;  // interval for LED to change color, in millis
int SET_EEPROM_ADDR = 0;
int SET_EEPROM_KEY_ADDR = 64;
int SET_EEPROM_KEY = 11;  // for data integrity check
int SET_LED_FLICKER_INTERVAL = 200;

// VFD display pin
int PIN_VFD_DIN = 4;
int PIN_VFD_PWM = 5;
int PIN_VFD_LATCH = 7;
int PIN_VFD_CLK = 8;

// Control buttons pin
int PIN_BTN_L = A2;
int PIN_BTN_M = A1;
int PIN_BTN_R = A0;

// RGB LED pin
int PIN_LED = 13;

// Display Number Constants: dot_up, dot_down, segment 1 ~ 7
int NUM_0[] = {LOW, LOW, HIGH, HIGH, HIGH, LOW, HIGH, HIGH, HIGH};
int NUM_1[] = {LOW, LOW, LOW, HIGH, LOW, LOW, HIGH, LOW, LOW};
int NUM_2[] = {LOW, LOW, HIGH, LOW, HIGH, HIGH, HIGH, LOW, HIGH};
int NUM_3[] = {LOW, LOW, HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH};
int NUM_4[] = {LOW, LOW, LOW, HIGH, LOW, HIGH, HIGH, HIGH, LOW};
int NUM_5[] = {LOW, LOW, HIGH, HIGH, LOW, HIGH, LOW, HIGH, HIGH};
int NUM_6[] = {LOW, LOW, HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH};
int NUM_7[] = {LOW, LOW, LOW, HIGH, LOW, LOW, HIGH, HIGH, HIGH};
int NUM_8[] = {LOW, LOW, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
int NUM_9[] = {LOW, LOW, HIGH, HIGH, LOW, HIGH, HIGH, HIGH, HIGH};
int NUM_DOT_UP[] = {HIGH, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
int NUM_DOT_DOWN[] = {LOW, HIGH, LOW, LOW, LOW, LOW, LOW, LOW, LOW};

// Display Grid Constants: grid 1 ~ 5
int GRID_1[] = {HIGH, LOW, LOW, LOW, LOW};
int GRID_2[] = {LOW, HIGH, LOW, LOW, LOW};
int GRID_3[] = {LOW, LOW, HIGH, LOW, LOW};
int GRID_4[] = {LOW, LOW, LOW, HIGH, LOW};
int GRID_5[] = {LOW, LOW, LOW, LOW, HIGH};

// Time Constants
int YEAR = 0;
int MONTH = 1;
int DATE = 2;
int HOUR = 3;
int MIN = 4;
int SEC = 5;

// state variables
int STATE_LUMIN = 255;  // VFD brightness level
int STATE_TIME[] = {2021, 12, 16, 12, 0, 0};  // year, month, date, hour, minute, second
int STATE_MODE = 0; // 0: clock mode; 1: edit time mode; 2: brightness mode; 3: color mode;
int STATE_LED_LUMIN = 55;
int STATE_LED_MODE = 0;
int STATE_LED_HUE = 0;  // fixed color hue
bool STATE_LED_FIXED = false; // loop color or not
int STATE_LED_FLICKER = false;  // flicker mode indicates the user is editing LED related settings

// button temp variables
int BTN_VAL_L = LOW;
int BTN_VAL_M = LOW;
int BTN_VAL_R = LOW;
int BTN_PREV_VAL_L = LOW;
int BTN_PREV_VAL_M = LOW;
int BTN_PREV_VAL_R = LOW;
unsigned long BTN_RISIN_TIME_L = 0; // timestamp of rising edge
unsigned long BTN_RISIN_TIME_M = 0; // timestamp of rising edge
unsigned long BTN_RISIN_TIME_R = 0; // timestamp of rising edge
bool BTN_LOCK_L = false;
bool BTN_LOCK_M = false;
bool BTN_LOCK_R = false;

// Time Edit Mode temp variables
int TIME_SET_LEVEL = 0; // indicates which part of the time is in on display: 0 for HH::mm, 1 for Date, 2 for Year
int TIME_TEMP_TIME[] = {2021, 12, 16, 12, 0, 0};  // stores edited variables

// Brightness control temp variables
int LUMIN_SET_LEVEL = 0;  // indcates which part of the time is in on display. 0 for VFD display and 1 for LED

// RGB LED temp variables
unsigned long LED_LAST_TIME = 0;
unsigned long LED_LAST_FLICKER = 0;
bool LED_FLICKER_ON = true;
uint32_t color1 = 0;
uint32_t color2 = 0;
uint32_t color3 = 0;
uint32_t color4 = 0;

// EEPROM data
struct ClockConfig {
  int lumin;
  int led_lumin;
  int led_mode;
  int led_hue;
  bool led_fixed;
};

// RTC setup
DS3231M_Class DS3231M;

// RGB LED setup
Adafruit_NeoPixel strip(4, PIN_LED, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  // Setup VFD
  pinMode(PIN_VFD_PWM, OUTPUT);
  pinMode(PIN_VFD_DIN, OUTPUT);
  pinMode(PIN_VFD_LATCH, OUTPUT);
  pinMode(PIN_VFD_CLK, OUTPUT);
  // load settings
  loadConfig();
  // Setup RGB LED
  strip.begin();
  strip.show();
  strip.setBrightness(STATE_LED_LUMIN);
  // Setup Control buttons
  pinMode(PIN_BTN_L, INPUT);
  pinMode(PIN_BTN_M, INPUT);
  pinMode(PIN_BTN_R, INPUT);
  // Connect to RTC
  while (!DS3231M.begin()) {
    Serial.println(F("Unable to find RTC DS3231M. Checking again in 3s."));
    delay(3000);
  }
}

void loop() {
  // load time from RTC
  getTime();
  // Controls
  detectControls();
  // RGB LED
  LEDDisplay();
  // Logging
  logMode();
  logLumin();
  //  logTime();
  //  logTempTime();
  //  logLEDMode();
  //  logBTN();
  delay(10);
}

// Debugger
void logMode() {
  Serial.print("MODE: ");
  Serial.println(STATE_MODE);
}
void logBTN() {
  Serial.print("BTN(LMR): ");
  Serial.print(BTN_VAL_L);
  Serial.print(BTN_VAL_M);
  Serial.println(BTN_VAL_R);
}
void logLumin() {
  Serial.print("LUMIN VFD: ");
  Serial.println(STATE_LUMIN);
  Serial.print("LUMIN LED: ");
  Serial.println(STATE_LED_LUMIN);
}
void logTime() {
  Serial.print("TIME: ");
  for (int i = 0; i < 5; ++i) {
    Serial.print(STATE_TIME[i]);
  }
  Serial.println(STATE_TIME[5]);
}
void logTempTime() {
  Serial.print("TEMP TIME: ");
  for (int i = 0; i < 5; ++i) {
    Serial.print(TIME_TEMP_TIME[i]);
  }
  Serial.println(TIME_TEMP_TIME[5]);
}
void logLEDMode() {
  Serial.print("LED MODE: ");
  Serial.println(STATE_LED_MODE);
}

// Read RTC time
void getTime() {
  DateTime currentTime = DS3231M.now();
  STATE_TIME[YEAR] = currentTime.year();
  STATE_TIME[MONTH] = currentTime.month();
  STATE_TIME[DATE] = currentTime.day();
  STATE_TIME[HOUR] = currentTime.hour();
  STATE_TIME[MIN] = currentTime.minute();
  STATE_TIME[SEC] = currentTime.second();
}
// Push STATE_TIME to RTC
void pushTime() {
  DS3231M.adjust(DateTime(TIME_TEMP_TIME[YEAR], TIME_TEMP_TIME[MONTH], TIME_TEMP_TIME[DATE], TIME_TEMP_TIME[HOUR], TIME_TEMP_TIME[MIN], TIME_TEMP_TIME[SEC]));
}

// display functions
/*
    Steps:
    1. Clock in data on rising edge
    2. Latch out all data on falling edge
    3. PWM Blank pin to control brightness
*/
void flashDisplay(int seq[]) {
  digitalWrite(PIN_VFD_LATCH, HIGH);
  // Clock in data
  for (int i = 13; i >= 0; --i) {
    digitalWrite(PIN_VFD_CLK, LOW);
    digitalWrite(PIN_VFD_DIN, seq[i]);
    digitalWrite(PIN_VFD_CLK, HIGH);
  }
  // Latch data
  digitalWrite(PIN_VFD_LATCH, LOW);
  // PWM brightness control
  analogWrite(PIN_VFD_PWM, STATE_LUMIN);
}

// control button functions
/*
    Detect button press on falling edge.
    There are three control buttons: Left, Mid, and Right (L.M.R)
    In Clock Mode, each control buttons triggers a new mode:
    L: Edit Time Mode
    M: Brightness Mode
    R: RGB LED Mode
    Exit to Clock Mode by pressing the same button again.
    The other two non-trigger buttons in each mode adjust the values up or down.
*/
void detectControls() {
  // read button values
  BTN_PREV_VAL_L = BTN_VAL_L;
  BTN_VAL_L = digitalRead(PIN_BTN_L);
  BTN_PREV_VAL_M = BTN_VAL_M;
  BTN_VAL_M = digitalRead(PIN_BTN_M);
  BTN_PREV_VAL_R = BTN_VAL_R;
  BTN_VAL_R = digitalRead(PIN_BTN_R);
  // Enter mode
  switch (STATE_MODE) {
    case 0:
      // Clock Mode
      controlClock();
      break;
    case 1:
      // Edit Time Mode
      controlTime();
      break;
    case 2:
      // Brightness Mode
      controlLumin();
      break;
    case 3:
      // RGB LED Mode
      controlLED();
      break;
    default:
      // reset mode if somehow it ended up in a bizzare state
      STATE_MODE = 0;
      break;
  }
}
/*
   Clock Mode
   Detect mode triggers
*/
void controlClock() {
  // detect rising edge and start timing
  if (BTN_VAL_L == HIGH && BTN_PREV_VAL_L == LOW) {
    // long press start
    BTN_RISIN_TIME_L = millis();
  } else if (BTN_VAL_M == HIGH && BTN_PREV_VAL_M == LOW) {
    // long press start
    BTN_RISIN_TIME_M = millis();
  } else if (BTN_VAL_R == HIGH && BTN_PREV_VAL_R == LOW) {
    // long press start
    BTN_RISIN_TIME_R = millis();
  }
  // detect long press and set mode
  if (BTN_VAL_L == HIGH && millis() - BTN_RISIN_TIME_L >= SET_LONG_PRESS_DUR) {
    // long L press, triggers Time Set Mode
    STATE_MODE = 1;
    BTN_LOCK_L = true;  // lock button until release
    TIME_SET_LEVEL = 0; // reset edit level
    for (int i = 0; i < 6; ++i) { // copy current time
      TIME_TEMP_TIME[i] = STATE_TIME[i];
    }
    Serial.println("LONG PRESS L");
  } else if (BTN_VAL_M == HIGH && millis() - BTN_RISIN_TIME_M >= SET_LONG_PRESS_DUR) {
    // long M press, triggers Brightness Mode
    STATE_MODE = 2;
    BTN_LOCK_M = true;  // lock button until release
    Serial.println("LONG PRESS M");
  } else if (BTN_VAL_R == HIGH && millis() - BTN_RISIN_TIME_R >= SET_LONG_PRESS_DUR) {
    // long R press, triggers RGB LED Mode
    STATE_MODE = 3;
    BTN_LOCK_R = true;  // lock button until release
    STATE_LED_FLICKER = true; // LED edit mode on
    Serial.println("LONG PRESS R");
  }
  // detect short press
  if (!BTN_LOCK_L && BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
    // short L press
    Serial.println("SHORT PRESS L");
  } else if (!BTN_LOCK_M && BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
    // short M press
    Serial.println("SHORT PRESS M");
  } else if (!BTN_LOCK_R && BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
    // short R press
    Serial.println("SHORT PRESS R");
  }
}
/*

*/
void controlTime() {
  // determine which part of the time is in on display/edit: 0 for HH::mm, 1 for MM::DD, 2 for Year.
  // Value for seconds is set to zero on mode exit.
  // M and R to modify values, L to go to the next level.
  if (!BTN_LOCK_L) {
    switch (TIME_SET_LEVEL) {
      case 0:
        // HH:mm, edit hour and min
        if (BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
          // short M press, loop up hour
          TIME_TEMP_TIME[HOUR] += 1;
          if (TIME_TEMP_TIME[HOUR] > 23) {
            TIME_TEMP_TIME[HOUR] = 0;
          }
        } else if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
          // short R press, loop up min
          TIME_TEMP_TIME[MIN] += 1;
          if (TIME_TEMP_TIME[MIN] > 59) {
            TIME_TEMP_TIME[MIN] = 0;
          }
        } else if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // next edit level
          TIME_SET_LEVEL += 1;
        }
        break;
      case 1:
        // MM:DD
        if (BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
          // short M press, loop up month
          TIME_TEMP_TIME[MONTH] += 1;
          if (TIME_TEMP_TIME[MONTH] > 12) {
            TIME_TEMP_TIME[MONTH] = 1;
          }
        } else if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
          // short R press, loop up date
          TIME_TEMP_TIME[DATE] += 1;
          if (TIME_TEMP_TIME[DATE] > 31) {
            TIME_TEMP_TIME[DATE] = 1;
          }
        } else if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // next edit level
          TIME_SET_LEVEL += 1;
        }
        break;
      case 2:
        // YYYY
        if (BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
          // short M press, increase year
          TIME_TEMP_TIME[YEAR] += 1;
        } else if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
          // short R press, decrease year
          TIME_TEMP_TIME[YEAR] -= 1;
        } else if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // reset seconds
          TIME_TEMP_TIME[SEC] = 0;
          // save time to RTC
          pushTime();
          // reset edit level and exit mode
          TIME_SET_LEVEL = 0;
          STATE_MODE = 0;
        }
        break;
      default:
        // reset edit level if somehow it ends up as a bizzare value
        TIME_SET_LEVEL = 0;
        break;
    }
  }
  // clear lock on falling edge
  if (BTN_LOCK_L && BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
    BTN_LOCK_L = false;
  }
}
/*
   Set brightness of the VFD display and RGB LED. Value range form 0 ~ 255.
*/
void controlLumin() {
  if (!BTN_LOCK_M) {
    // Set brightness: L for decrease, R for increase, M for next level
    switch (LUMIN_SET_LEVEL) {
      case 0:
        // VFD lumin
        if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // short press L, decrease lumin
          STATE_LUMIN -= 10;
          if (STATE_LUMIN <= 4) {
            // Cannot go below 5;
            STATE_LUMIN += 10;
          }
        } else if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
          // short press R, increase lumin
          STATE_LUMIN += 10;
          if (STATE_LUMIN >= 256) {
            // Cannot exceed 255;
            STATE_LUMIN -= 10;
          }
        } else if (BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
          // short press M, go to next level
          LUMIN_SET_LEVEL += 1;
          STATE_LED_FLICKER = true; // LED edit mode on
        }
        break;
      case 1:
        // LED lumin
        if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // short press L, decrease lumin
          STATE_LED_LUMIN -= 10;
          if (STATE_LED_LUMIN <= 9) {
            // Cannot go below 10;
            STATE_LED_LUMIN += 10;
          }
          strip.setBrightness(STATE_LED_LUMIN); // update lumin immediately
        } else if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
          // short press R, increase lumin
          STATE_LED_LUMIN += 10;
          if (STATE_LED_LUMIN >= 256) {
            // Cannot exceed 255;
            STATE_LED_LUMIN -= 10;
          }
          strip.setBrightness(STATE_LED_LUMIN); // update lumin immediately
        } else if (BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
          // short press M, exit mode
          LUMIN_SET_LEVEL = 0;  // reset edit level
          STATE_LED_FLICKER = false; // LED edit mode off
          STATE_MODE = 0;
          // save settings to EEPROM
          putConfig();
        }
        break;
      default:
        LUMIN_SET_LEVEL = 0;  // reset lumin set level in case it somehow ends up in a bizzare value.
        break;
    }
  }
  // clear lock on falling edge
  if (BTN_LOCK_M && BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
    BTN_LOCK_M = false;
  }
}
/*
   LED Mode:
   0. Cycle color / Fixed color
   1. Rainbow color / Fixed color
   2. Off
   Press L to change mode and M to toggle mode value.
*/
void controlLED() {
  // Toggle LED mode
  if (!BTN_LOCK_R) {
    switch (STATE_LED_MODE) {
      case 0:
        // cycle color mode
        if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // short L press, loop LED mode
          STATE_LED_MODE += 1;
        } else if (BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
          // short M press, fix/unfix color
          STATE_LED_FIXED = !STATE_LED_FIXED;
        }
        break;
      case 1:
        // color rainbow mode
        if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // short L press, loop LED mode
          STATE_LED_MODE += 1;
        } else if (BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
          // short M press, fix/unfix color
          STATE_LED_FIXED = !STATE_LED_FIXED;
        }
        break;
      case 2:
        // turn off LED
        if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
          // short L press, loop LED mode
          STATE_LED_MODE = 0;
        }
        break;
      default:
        // reset LED state if somehow it ends up as a bizzare value
        STATE_LED_MODE = 0;
        break;
    }
    // Exit mode
    if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
      STATE_LED_FLICKER = false; // LED edit mode off
      STATE_MODE = 0;
      // save settings to EEPROM
      putConfig();
    }
  }
  // clear lock on falling edge
  if (BTN_LOCK_R && BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
    BTN_LOCK_R = false;
  }
}

// RGB LED display functions
void LEDDisplay() {
  // only update on certain interval
  if (millis() - LED_LAST_TIME >= SET_LED_UPDATE_INTERVAL) {
    // update flicker mode on certain interval
    if (STATE_LED_FLICKER && millis() - LED_LAST_FLICKER >= SET_LED_FLICKER_INTERVAL) {
      LED_FLICKER_ON = !LED_FLICKER_ON;  // toggle mode
      LED_LAST_FLICKER = millis();
    }
    // render color
    switch (STATE_LED_MODE) {
      case 0: // color cycle mode (fill)
        // flicker
        if (!STATE_LED_FLICKER || (STATE_LED_FLICKER && LED_FLICKER_ON)) {
          // turn on LED
          color1 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE));
          strip.fill(color1, 0, strip.numPixels());
        } else {
          // turn off LED
          strip.clear();
        }
        strip.show();
        if (!STATE_LED_FIXED) {
          // update color, if looping is enabled
          STATE_LED_HUE += 256;
          if (STATE_LED_HUE >= 65536) {
            STATE_LED_HUE = 0;
          }
        }
        break;
      case 1:
        // color rainbow mode
        if (!STATE_LED_FLICKER || (STATE_LED_FLICKER && LED_FLICKER_ON)) {
          // turn on LED
          color1 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE));
          color2 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE + 16384));
          color3 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE + 32768));
          color4 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE + 49152));
          // color offset of each strip is 90 deg
          strip.setPixelColor(0, color1);
          strip.setPixelColor(1, color2);
          strip.setPixelColor(2, color3);
          strip.setPixelColor(3, color4);
          strip.show();
        } else {
          // turn off LED
          strip.clear();
        }
        strip.show();
        if (!STATE_LED_FIXED) {
          // update color, if looping is enabled
          STATE_LED_HUE += 256;
          if (STATE_LED_HUE >= 65536) {
            STATE_LED_HUE = 0;
          }
        }
        break;
      case 2:
        // LED off
        strip.clear();
        strip.show();
        break;
      default:
        // reset LED mode in case it somehow ends up in a bizzare value
        STATE_LED_MODE = 0;
        break;
    }
    // record last update time to track consistent interval
    LED_LAST_TIME = millis();
  }
}

// EEPROM functions
/*
   Stores user settings when changed.
   Load user settings on startup.
   Beware that EEPROM only has a lifespan of 100000 write
*/
void putConfig() {
  // save settings
  ClockConfig c = {
    STATE_LUMIN,
    STATE_LED_LUMIN,
    STATE_LED_MODE,
    STATE_LED_HUE,
    STATE_LED_FIXED
  };
  EEPROM.put(SET_EEPROM_ADDR, c);
  // write integrity
  EEPROM.put(SET_EEPROM_KEY_ADDR, SET_EEPROM_KEY);
  Serial.println(F("Data Saved"));
}
void loadConfig() {
  // check EEPROM data integrity
  int key = 0;
  EEPROM.get(SET_EEPROM_KEY_ADDR, key);
  if (key != SET_EEPROM_KEY) {
    // EEPROM data corrupted/not-initialized, load default settings
    putConfig();  // write default settings
    Serial.println(F("No previous setting found, using default..."));
    return;
  }
  // load settings
  ClockConfig c;
  EEPROM.get(SET_EEPROM_ADDR, c);
  STATE_LUMIN = c.lumin;
  STATE_LED_LUMIN = c.led_lumin;
  STATE_LED_MODE = c.led_mode;
  STATE_LED_HUE = c.led_hue;
  STATE_LED_FIXED = c.led_fixed;
  Serial.println(F("Data Loaded"));
}
