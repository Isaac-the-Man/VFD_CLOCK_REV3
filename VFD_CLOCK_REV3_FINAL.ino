/*
 * IVL2-7/5 VFD CLOCK REV 3.1 SOFTWARE
 * WRITTEN BY ISAAC_THE_MAN
 * SEE PROJECT ON https://github.com/Isaac-the-Man/VFD_CLOCK_REV3
 * LAST UPDATED 8/21/2021
 * MIT LICENSE
 */
#include <DS3231M.h>  // Include the DS3231M RTC library
#include <Adafruit_NeoPixel.h>  // RGB LED library
#include <EEPROM.h> // read/write user settings


// Setting constants
const int SET_LONG_PRESS_DUR = 2000;  // duration required to be qualified as a long press, in millis.
const int SET_LED_UPDATE_INTERVAL = 50;  // interval for LED to change color, in millis
const int SET_EEPROM_ADDR = 0;
const int SET_EEPROM_KEY_ADDR = 64;
const int SET_EEPROM_KEY = 11;  // for data integrity check
const int SET_LED_FLICKER_INTERVAL = 200;
const int SET_VFD_FLICKER_INTERVAL = 200;
const int SET_CLOCK_SHORT_DUR = 2000; // duration of short displays such as date and temperature, in millis.
const int SET_TEMP_OFFSET = -500; // 100th of degrees
const int SET_POWER_SAVING_ON_TIME[] = {23, 59};
const int SET_POWER_SAVING_OFF_TIME[] = {7, 0};
const int SET_POWER_SAVING_LUMIN = 205;
const int SET_POWER_SAVING_LED_LUMIN = 15;
const int SET_LED_VALUE = 128;
const int SET_LED_SATURATION = 128;

// VFD display pin
const int PIN_VFD_DIN = 4;
const int PIN_VFD_PWM = 5;
const int PIN_VFD_LATCH = 7;
const int PIN_VFD_CLK = 8;

// Control buttons pin
const int PIN_BTN_L = A2;
const int PIN_BTN_M = A1;
const int PIN_BTN_R = A0;

// RGB LED pin
const int PIN_LED = 13;

// Display Number Constants: segment 1 ~ 7
const int NUM_0[] = {HIGH, HIGH, HIGH, LOW, HIGH, HIGH, HIGH};
const int NUM_1[] = {LOW, HIGH, LOW, LOW, HIGH, LOW, LOW};
const int NUM_2[] = {HIGH, LOW, HIGH, HIGH, HIGH, LOW, HIGH};
const int NUM_3[] = {HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH};
const int NUM_4[] = {LOW, HIGH, LOW, HIGH, HIGH, HIGH, LOW};
const int NUM_5[] = {HIGH, HIGH, LOW, HIGH, LOW, HIGH, HIGH};
const int NUM_6[] = {HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH};
const int NUM_7[] = {LOW, HIGH, LOW, LOW, HIGH, HIGH, HIGH};
const int NUM_8[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
const int NUM_9[] = {HIGH, HIGH, LOW, HIGH, HIGH, HIGH, HIGH};
const int ALP_C[] = {HIGH, LOW, HIGH, LOW, LOW, HIGH, HIGH};  //10
const int ALP_H[] = {LOW, HIGH, HIGH, HIGH, HIGH, HIGH, LOW}; //11
const int ALP_E[] = {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH}; //12
const int ALP_L[] = {HIGH, LOW, HIGH, LOW, LOW, HIGH, LOW}; //13

// Time Constants
const int YEAR = 0;
const int MONTH = 1;
const int DATE = 2;
const int HOUR = 3;
const int MIN = 4;
const int SEC = 5;

// state variables
int STATE_LUMIN = 255;  // VFD brightness level
int STATE_TIME[] = {2021, 12, 16, 12, 0, 0};  // year, month, date, hour, minute, second
int STATE_MODE = 0; // 0: clock mode; 1: edit time mode; 2: brightness mode; 3: color mode;
int STATE_LED_LUMIN = 55;
int STATE_LED_MODE = 0;
int STATE_LED_HUE = 0;  // fixed color hue
bool STATE_LED_FIXED = false; // loop color or not
int STATE_LED_FLICKER = false;  // flicker mode indicates the user is editing LED related settings
int STATE_DISPLAY[] = {0, 0, 0, 0, 0}; // digit, digit, colon, digit, digit
bool STATE_VFD_FLICKER = false; // VFD blink or not
int STATE_TEMP = 0; // temperature in Celsius
bool STATE_POWER_SAVING_ON = false;

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

// Clock mode temp variables
int CLOCK_MODE = 0; // 0 for normal clock; 1 for Date; 2 for Year; 3 for Hello
int CLOCK_DATE_YEAR_MODE = 0; // 0 for DATE, 1 for YEAR
unsigned long CLOCK_DATE_YEAR_SHOW_TIME = 0;
unsigned long CLOCK_TEMP_TIME = 0;
unsigned long CLOCK_HELLO_TIME = 0;

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

// VFD display temp variables
int VFD_TEMP[14] = {0}; // initialize to all 0
unsigned long VFD_LAST_FLICKER = 0;
bool VFD_FLICKER_ON = true;

// Power saving temp variables
int POWER_SAVING_PREV_LUMIN = 245;
int POWER_SAVING_PREV_LED_LUMIN = 5;

// EEPROM data
struct ClockConfig {
  int lumin;
  int led_lumin;
  int led_mode;
  int led_hue;
  bool led_fixed;
  bool power_saving_on;
  int power_saving_prev_lumin;
  int power_saving_prev_led_lumin;
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
  // Setup Control buttons
  pinMode(PIN_BTN_L, INPUT);
  pinMode(PIN_BTN_M, INPUT);
  pinMode(PIN_BTN_R, INPUT);
  // load settings
  loadConfig();
  // Setup RGB LED
  strip.begin();
  strip.show();
  strip.setBrightness(STATE_LED_LUMIN);
  // Connect to RTC
  while (!DS3231M.begin()) {
    Serial.println(F("FATAL ERROR: Unable to find RTC DS3231M. Checking again in 3s."));
    delay(3000);
  }
}

void loop() {
  // load time from RTC
  getTime();
  powerSaving();
  // Controls
  detectControls();
  // VFD display
  writeDisplay();
  flashDisplay();
  // RGB LED
  LEDDisplay();
  // Logging
  //  logMode();
  //  logLumin();
  //  logTime();
  //  logTempTime();
  //  logLEDMode();
  //  logBTN();
  //  logDisplay();
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
void logDisplay() {
  Serial.print("DISPLAY: ");
  for (int i = 0; i < 4; ++i) {
    Serial.print(STATE_DISPLAY[i]);
  }
  Serial.println(STATE_DISPLAY[4]);
}

// Read RTC time
void getTime() {
  // read time
  DateTime currentTime = DS3231M.now();
  STATE_TIME[YEAR] = currentTime.year();
  STATE_TIME[MONTH] = currentTime.month();
  STATE_TIME[DATE] = currentTime.day();
  STATE_TIME[HOUR] = currentTime.hour();
  STATE_TIME[MIN] = currentTime.minute();
  STATE_TIME[SEC] = currentTime.second();
  // read temperature
  STATE_TEMP = DS3231M.temperature() + SET_TEMP_OFFSET;
}
// Push STATE_TIME to RTC
void pushTime() {
  DS3231M.adjust(DateTime(TIME_TEMP_TIME[YEAR], TIME_TEMP_TIME[MONTH], TIME_TEMP_TIME[DATE], TIME_TEMP_TIME[HOUR], TIME_TEMP_TIME[MIN], TIME_TEMP_TIME[SEC]));
}
// check power saving status
void powerSaving() {
  if (!STATE_POWER_SAVING_ON && STATE_TIME[HOUR] == SET_POWER_SAVING_ON_TIME[0] && STATE_TIME[MIN] == SET_POWER_SAVING_ON_TIME[1]) {
    // turn on power saving
    Serial.println(F("Power saving on."));
    STATE_POWER_SAVING_ON = true;
    POWER_SAVING_PREV_LUMIN = STATE_LUMIN;
    POWER_SAVING_PREV_LED_LUMIN = STATE_LED_LUMIN;
    STATE_LUMIN = SET_POWER_SAVING_LUMIN;
    STATE_LED_LUMIN = SET_POWER_SAVING_LED_LUMIN;
    putConfig();
  }
  if (STATE_POWER_SAVING_ON && STATE_TIME[HOUR] == SET_POWER_SAVING_OFF_TIME[0] && STATE_TIME[MIN] == SET_POWER_SAVING_OFF_TIME[1]) {
    // turn off power saving
    Serial.println(F("Power saving off."));
    STATE_POWER_SAVING_ON = false;
    STATE_LUMIN = POWER_SAVING_PREV_LUMIN;
    STATE_LED_LUMIN = POWER_SAVING_PREV_LED_LUMIN;
    putConfig();
  }
}

// display functions
/*
   Determines what to display base on STATE vars
*/
void writeDisplay() {
  switch (STATE_MODE) {
    case 0:
      // normal clock mode, show clock
      switch (CLOCK_MODE) {
        case 0:
          // show normal clock
          STATE_DISPLAY[0] = STATE_TIME[HOUR] / 10;
          STATE_DISPLAY[1] = STATE_TIME[HOUR] % 10;
          STATE_DISPLAY[2] = STATE_TIME[SEC] % 2 == 0 ? 0 : -1;  // only show colon at even seconds
          STATE_DISPLAY[3] = STATE_TIME[MIN] / 10;
          STATE_DISPLAY[4] = STATE_TIME[MIN] % 10;
          break;
        case 1:
          // show date and year
          if (millis() - CLOCK_DATE_YEAR_SHOW_TIME >= SET_CLOCK_SHORT_DUR) {
            // switch state
            CLOCK_DATE_YEAR_MODE += 1;
            CLOCK_DATE_YEAR_SHOW_TIME = millis(); // timestamp of mode transition
          }
          switch (CLOCK_DATE_YEAR_MODE) {
            case 0:
              // show date
              STATE_DISPLAY[0] = STATE_TIME[MONTH] / 10;
              STATE_DISPLAY[1] = STATE_TIME[MONTH] % 10;
              STATE_DISPLAY[2] = 2;  // only show bottom colon
              STATE_DISPLAY[3] = STATE_TIME[DATE] / 10;
              STATE_DISPLAY[4] = STATE_TIME[DATE] % 10;
              break;
            case 1:
              // show year
              STATE_DISPLAY[0] = STATE_TIME[YEAR] / 1000;
              STATE_DISPLAY[1] = STATE_TIME[YEAR] % 1000 / 100;
              STATE_DISPLAY[2] = -1;  // turn off colon
              STATE_DISPLAY[3] = STATE_TIME[YEAR] % 100 / 10;
              STATE_DISPLAY[4] = STATE_TIME[YEAR] % 10;
              break;
            default:
              // change back to normal clock mode
              CLOCK_DATE_YEAR_MODE = 0;
              CLOCK_MODE = 0;
              break;
          }
          break;
        case 2:
          // show temperature
          if (millis() - CLOCK_TEMP_TIME < SET_CLOCK_SHORT_DUR) {
            STATE_DISPLAY[0] = STATE_TEMP / 1000;
            STATE_DISPLAY[1] = STATE_TEMP / 100 % 10;
            STATE_DISPLAY[2] = 2; // decimal point
            STATE_DISPLAY[3] = STATE_TEMP / 10 % 10;
            STATE_DISPLAY[4] = 10;
          } else {
            // go back to normal clock mode
            CLOCK_MODE = 0;
          }
          break;
        case 3:
          // show HELLO
          if (millis() - CLOCK_HELLO_TIME < SET_CLOCK_SHORT_DUR) {
            STATE_DISPLAY[0] = 11;
            STATE_DISPLAY[1] = 12;
            STATE_DISPLAY[2] = -1; // decimal point off
            STATE_DISPLAY[3] = 13;
            STATE_DISPLAY[4] = 0;
          } else {
            // go back to normal clock mode
            CLOCK_MODE = 0;
          }
          break;
        default:
          CLOCK_MODE = 0; // defualt
          break;
      }
      STATE_VFD_FLICKER = false;
      break;
    case 1:
      // time edit mode, blink clock
      switch (TIME_SET_LEVEL) {
        case 0:
          // edit time
          STATE_DISPLAY[0] = TIME_TEMP_TIME[HOUR] / 10;
          STATE_DISPLAY[1] = TIME_TEMP_TIME[HOUR] % 10;
          STATE_DISPLAY[2] = 0; // turn on both colon
          STATE_DISPLAY[3] = TIME_TEMP_TIME[MIN] / 10;
          STATE_DISPLAY[4] = TIME_TEMP_TIME[MIN] % 10;
          break;
        case 1:
          // edit date
          STATE_DISPLAY[0] = TIME_TEMP_TIME[MONTH] / 10;
          STATE_DISPLAY[1] = TIME_TEMP_TIME[MONTH] % 10;
          STATE_DISPLAY[2] = 2;  // only show bottom colon
          STATE_DISPLAY[3] = TIME_TEMP_TIME[DATE] / 10;
          STATE_DISPLAY[4] = TIME_TEMP_TIME[DATE] % 10;
          break;
        case 2:
          // edit year
          STATE_DISPLAY[0] = TIME_TEMP_TIME[YEAR] / 1000;
          STATE_DISPLAY[1] = TIME_TEMP_TIME[YEAR] % 1000 / 100;
          STATE_DISPLAY[2] = -1;  // turn off colon
          STATE_DISPLAY[3] = TIME_TEMP_TIME[YEAR] % 100 / 10;
          STATE_DISPLAY[4] = TIME_TEMP_TIME[YEAR] % 10;
          break;
      }
      STATE_VFD_FLICKER = true;
      break;
    case 2:
      // Lumin mode, display lumin as 0 ~ 10
      if (LUMIN_SET_LEVEL == 0) {  // VFD Lumin
        STATE_DISPLAY[0] = -1;  // no display
        STATE_DISPLAY[1] = -1;  // no display
        STATE_DISPLAY[2] = -1;  // no display
        STATE_DISPLAY[3] = (25 - (STATE_LUMIN - 5) / 10) / 10;
        STATE_DISPLAY[4] = (25 - (STATE_LUMIN - 5) / 10) % 10;
        STATE_VFD_FLICKER = true;
      } else if (LUMIN_SET_LEVEL == 1) {  // LED Lumin
        STATE_DISPLAY[0] = -1;  // no display
        STATE_DISPLAY[1] = -1;  // no display
        STATE_DISPLAY[2] = -1;  // no display
        STATE_DISPLAY[3] = (STATE_LED_LUMIN - 5) / 100;
        STATE_DISPLAY[4] = (STATE_LED_LUMIN - 5) / 10 % 10;
        STATE_VFD_FLICKER = false;
      }
      break;
    case 3:
      // LED mode, show normal clock (only LED is blinking)
      STATE_DISPLAY[0] = STATE_TIME[HOUR] / 10;
      STATE_DISPLAY[1] = STATE_TIME[HOUR] % 10;
      STATE_DISPLAY[2] = STATE_TIME[SEC] % 2 == 0 ? 0 : -1;  // only show colon at even seconds
      STATE_DISPLAY[3] = STATE_TIME[MIN] / 10;
      STATE_DISPLAY[4] = STATE_TIME[MIN] % 10;
      STATE_VFD_FLICKER = false;
      break;
    default:
      // reset display state in case it somehow ends up in a bizzare value
      STATE_MODE = 0;
      STATE_VFD_FLICKER = false;
      break;
  }
}
/*
   Display a single digit
   Steps:
   1. Clock in data on rising edge
   2. Latch out all data on falling edge
   3. PWM Blank pin to control brightness
*/
void flashDigit() {
  digitalWrite(PIN_VFD_LATCH, HIGH);
  // Clock 14 bits of data
  for (int i = 13; i >= 0; --i) {
    digitalWrite(PIN_VFD_CLK, LOW); // CLK LOW
    digitalWrite(PIN_VFD_DIN, VFD_TEMP[i]);  // Serial data
    digitalWrite(PIN_VFD_CLK, HIGH);  // CLK HIGH
  }
  // Latch data
  digitalWrite(PIN_VFD_LATCH, LOW);
  // PWM brightness control
  analogWrite(PIN_VFD_PWM, STATE_LUMIN);
}
/*
   Control full display by flashing digits real quick.
   Digits are flashed from left to right.
   Colon can be flashed with all the digits.
*/
void flashDisplay() {
  if (STATE_VFD_FLICKER) {
    // update per interval
    if (millis() - VFD_LAST_FLICKER >= SET_VFD_FLICKER_INTERVAL) {
      VFD_FLICKER_ON = !VFD_FLICKER_ON; // toggle flicker
      VFD_LAST_FLICKER = millis();  // save last flicker time
    }
    if (!VFD_FLICKER_ON) {
      // flash clear display
      analogWrite(PIN_VFD_PWM, 255);  // pull HIGH, which turns off the display
      return;
    }
  }
  // flash 4 number digits
  for (int i = 0; i < 4; ++i) {
    loadDisplaySeq(i, STATE_DISPLAY[i < 2 ? i : i + 1], STATE_DISPLAY[2] == 0 || STATE_DISPLAY[2] == 1, STATE_DISPLAY[2] == 0 || STATE_DISPLAY[2] == 2);
    flashDigit();
    delay(5);
  }
}
/*
   Clean display seq to 0
*/
void cleanDisplaySeq() {
  for (int i = 0; i < 14; ++i) {
    VFD_TEMP[i] = LOW;
  }
}
/*
   Helper function for building the right display shift sequence.
   Sequence should be reversed when displaying since shifiting to the registers starts at the largest index.
   Shift registers 0 ~ 13 corresponds to GRID_1 ~ 5, DOT_DOWN, DOT_UPPER, SEG_1 ~ 7
   For grid state, 0: both on, 1: up, 2: bottom
   "digit" parameter indicates which digit is the render target (0~3)
*/
void loadDisplaySeq(int digit, int num, bool dotTop, bool dotBot) {
  // clear previous data
  cleanDisplaySeq();
  if (num == -1) {
    // leave blank, don't display digit
    return;
  }
  // Load grid
  switch (digit) {
    case 0:
      // first digit
      VFD_TEMP[4] = HIGH;
      break;
    case 1:
      // second digit
      VFD_TEMP[3] = HIGH;
      break;
    case 2:
      // third digit
      VFD_TEMP[1] = HIGH;
      break;
    case 3:
      // fourth digit
      VFD_TEMP[0] = HIGH;
      break;
  }
  // Check colon grid
  if (dotTop || dotBot) {
    // set colon grid HIGH
    VFD_TEMP[2] = HIGH;
    // colon anodes
    if (dotBot) {
      VFD_TEMP[5] = HIGH;
    }
    if (dotTop) {
      VFD_TEMP[6] = HIGH;
    }
  }
  // Segment anodes (actual display numbers)
  switch (num) {
    case 0:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_0[i];
      }
      break;
    case 1:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_1[i];
      }
      break;
    case 2:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_2[i];
      }
      break;
    case 3:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_3[i];
      }
      break;
    case 4:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_4[i];
      }
      break;
    case 5:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_5[i];
      }
      break;
    case 6:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_6[i];
      }
      break;
    case 7:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_7[i];
      }
      break;
    case 8:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_8[i];
      }
      break;
    case 9:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = NUM_9[i];
      }
      break;
    case 10:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = ALP_C[i];
      }
      break;
    case 11:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = ALP_H[i];
      }
      break;
    case 12:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = ALP_E[i];
      }
      break;
    case 13:
      for (int i = 0; i < 7; ++i) {
        VFD_TEMP[i + 7] = ALP_L[i];
      }
      break;
  }
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
    // short L press, show Date and Year
    CLOCK_MODE = 1;
    CLOCK_DATE_YEAR_SHOW_TIME = millis();
    CLOCK_DATE_YEAR_MODE = 0;
    Serial.println("SHORT PRESS L");
  } else if (!BTN_LOCK_M && BTN_PREV_VAL_M == HIGH && BTN_VAL_M == LOW) {
    // short M press, show temperature (from DS3231)
    CLOCK_MODE = 2;
    CLOCK_TEMP_TIME = millis();
    Serial.println("SHORT PRESS M");
  } else if (!BTN_LOCK_R && BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
    // short R press, display HELLO
    CLOCK_MODE = 3;
    CLOCK_HELLO_TIME = millis();
    Serial.println("SHORT PRESS R");
  }
}
/*
   Set time
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
          // short M press, decrease year
          TIME_TEMP_TIME[YEAR] -= 1;
        } else if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
          // short R press, increase year
          TIME_TEMP_TIME[YEAR] += 1;
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
        if (BTN_PREV_VAL_R == HIGH && BTN_VAL_R == LOW) {
          // short press L, decrease lumin
          STATE_LUMIN -= 10;
          if (STATE_LUMIN <= 4) {
            // Cannot go below 5;
            STATE_LUMIN += 10;
          }
        } else if (BTN_PREV_VAL_L == HIGH && BTN_VAL_L == LOW) {
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
          color1 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE, SET_LED_SATURATION, SET_LED_VALUE));
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
          color1 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE, SET_LED_SATURATION, SET_LED_VALUE));
          color2 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE + 16384, SET_LED_SATURATION, SET_LED_VALUE));
          color3 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE + 32768, SET_LED_SATURATION, SET_LED_VALUE));
          color4 = strip.gamma32(strip.ColorHSV(STATE_LED_HUE + 49152, SET_LED_SATURATION, SET_LED_VALUE));
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
    STATE_LED_FIXED,
    STATE_POWER_SAVING_ON,
    POWER_SAVING_PREV_LUMIN,
    POWER_SAVING_PREV_LED_LUMIN,
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
  STATE_POWER_SAVING_ON = c.power_saving_on;
  POWER_SAVING_PREV_LUMIN = c.power_saving_prev_lumin;
  POWER_SAVING_PREV_LED_LUMIN = c.power_saving_prev_led_lumin;
  Serial.println(F("Data Loaded"));
}
