# Embedded C semester assignment
### About
This is my semester work for subject called *Single Chip Controllers Programming*. I've been programming an Arduino Uno microcontroller (ATmega 128p) without using its IDE and built-in functions, which led me to obtain useful knowledge about registers, timers, etc.
*This readme file is meant to help you understand what is the code about, as I didn't want to risk renaming all the variables to english.*

### Parts/Modules used in project
To speak about software itself, I have to list the parts or modules that I have used in this project:
- Arduino Uno (ATmega 128p)
- RTC (Real Time Clock) Module DS3231
- NTC Thermistor DS18B20 10kOhm
- 4bit 7-segment display (able to display 4 numbers)
- 4 Channel Relay Module
- SD Card Module

### What does the software do?
You may see this software as some kind of "home automation" project. The NTC thermistor is used to measure current temperature (or resistance which is later converted to temperature). The temperature is shown on 7-segment display (combined with current time from RTC module). There are 4 possible appliances, which have its own setpoint temperature, pin (on arduino), mode (whether it's turned on or off when set temperature is reached). With the help of built-in timers, software checks and compares current temperature with setpoint temperatures of each appliance. If threshold temperature is detected, appliance will change its state (relay will turn on/off) and the change will be logged to the SD card. Although there are many files used in this project, I programmed just `main.c` and did few editing in library files connected to FatFS (filesystem) library - mainly in `mmc_avr_spi.c` and `ffconf.h`.

### Variables translations
As I stated before, I found it easier to tell you about variables instead of renaming each one in the code. Here's a small table which translates majority of slovak-named variables that I used into english ones.

| Original variable name | Translation | Comment |
| --------------- | --------------- | --------------- |
| `zobrazenie` | View | Used in toggling display between temperature and time |
| `teplota` | Temperature | - |
| `pocitadlo` | Counter | - |
| `zobrazene_cislo` | Number shown | Used in to switching between 4 digits in display |
| `cislice` | Numbers | Hexadecimal representation for numbers 0-9 based on physical connection between 7-segment display and arduino pins |
| `stav` | State | Represents current state of appliance |
| `vystup` | Output | - |
| `cas` | Time | - |