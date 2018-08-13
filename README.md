# ESP-32 tm1637-driven LCD library

## Introduction

This is an library of control TM1637 LCD 7-segment display using ESP-32 IDF toolchain [ESP-IDF](https://github.com/espressif/esp-idf).

## Features

 * Display numbers
 * Display raw segment data
 * Display floating point numbers
 
## Important notes

This library uses `ets_delay_us()` function to generate i2c-like control sequences. Please note - while using within FreeRTOS task will be blocked while data is transmitted. 

## Source Code

The source is available from [GitHub petrows/esp-32-tm1637](https://github.com/petrows/esp-32-tm1637).

## License

The code in this project is licensed under the MIT license - see LICENSE for details.

Inital idea based on Arduino <tm1637.h> library, written by Frankie.Chu Copyright (c) 2012 seeed technology inc.

## Contacts

 * Email: petro@petro.ws