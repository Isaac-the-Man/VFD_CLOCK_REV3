# VFD_CLOCK_REV3
An arduino-compatible IVL2-7/5 VFD clock, version 3.1.

![VFD_demo](https://user-images.githubusercontent.com/29915393/130341756-2adb8b7f-e638-4dcc-941b-279ed5508ceb.gif)
(NOTE: flashing is only seen on camera due to high FPS)

## Features
- IVL2-7/5 VFD Display
- Arduino-compatible (programmable via Arduino IDE)
- Built-in extremely accurate real time clock with error less than a second per year, with calendar
- Four NeoPixel RGB LED
- On-board push buttons for setting time, changing brightness, LED pattern, and more
- Micro-USB powered
- In-ciruit temperature sensor
- Automatially saves user settings on shutdown
- Power saving mode: automatically lower brightness at night and turns back on in the morning.

## Hardware
- Atmega328P-PU with external 16MHz cyrstal as the MCU.
- FT232RL for USB to TTL converstion (programming the MCU).
- DS3231M+TRL as the RTC. Includes a temperature sensing feedback ciruit for tuning the crystal. Error less than a second per year. A CR2032 external power source is used as a back-up power when the USB is not connected.
- WS2812 NeoPixel RGB LED. A serial and individually-addressable RGB LED.
- Adjustable boost converter (AP3012KTR, replaceable): Boost voltage up to 25V. Adjustable via a potentiometer. Highly efficient.
- Adjustable buck converter (ST1S12GR, replaceable): Buck voltage down to 2V. Adjustable via a potentiometer. Highly efficient.
- MAX6921AUI+ VFD driver: 70V high-side switch with integrated 20-bit shift register.

## Usage
Carefully solder each components on the PCB. Insert a CR2032 battery for backup power (critical for time-keeping). Download `VFD_CLOCK_REV3_FINAL.ino` and upload it to the board via USB using the Arduino IDE, and there you have it. There are a lot more settings available in the code (variables starting with `SET_` are configurable at compile time), but requries further investigation of the code.

## Control Button Manual
This document covers the default behavior of the software. For simplicity, ignoring the left most button (RESET), the rest three buttons are referred as `L`, `M`, and `R` from left to right. Button presses shorter than 2 second is a short press, and a long press if longer.

The default display view is the current time (Hour:Minute). The colon in the middle turns on at even second and off during odd. From this state you can perform the following operation:

### SHORT L Press
Display date and year, each two seconds, then return to normal clock mode.

### SHORT M Press
Display temperature from the DS3231 IC for two seconds, then return.

### SHORT R Press
Display "HELO", for fun.

### LONG L PRESS
Enters time-set mode. The entire VFD display will start to blink. You can then modify Hour:Minute, Month.Date, and Year in a sequence of steps with `M` and `R` changing the values and `L` advancing the modified steps. After setting the year, another `L` press with bring you back to normal clock mode with the new set time.

### LONG M PRESS
Enters edit brightness mode. Press `L` to decrease lumin and `R` to increase. Value ranges from 1 ~ 25. The VFD or the LED will blink based on whatever is the target of the edit. Pressing `M` will advances into the next step (first VFD -> LED -> back to normal clock mode).

### LONG R PRESS
Enters edit LED mode. Press `L` to loop between LED patterns (single color cycle -> serial color cycle -> off -> single color cycle...). Press `M` to fix the current color (freezes the color in the cycle). Press `R` to return to normal clock mode.

Software and Hardware designed by *Isaac_the_Man*, aka *Yu-Kai "Steven" Wang*.
Published under MIT LICENSE
