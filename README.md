ZX Counter
==========

**ZX Counter** is the alternative software for the Arduino-based Geiger counter [DIYGeigerCounter](https://sites.google.com/site/diygeigercounter/). The software uses the same pin layout as the [default sketch](https://sites.google.com/site/diygeigercounter/software), so it can be used without changing the hardware.

Features
--------

 * Displays the average [CPM](http://en.wikipedia.org/wiki/Counts_per_minute) and the [equivalent radiation dose](http://en.wikipedia.org/wiki/Equivalent_dose) each second
 * Automatic and manual [averaging interval](http://en.wikipedia.org/wiki/Moving_average)
 * The automatic averaging interval depends on the current radiation level
 * The manual averaging interval can be set to: 1s, 5s, 10s, 30s, 1m, 10m, the total measuring time
 * Analog bar emulation
 * Displays the total measuring time
 * Displays the total CPM and the accumulated equivalent radiation dose within the measuring time
 * Displays the maximal CPM and the maximal equivalent radiation dose reached within the measuring time
 * CPM, equivalent radiation dose, total measuring time auto-ranging
 * The equivalent radiation dose unit, the CPM to [uSv/h](http://en.wikipedia.org/wiki/Sievert) ratio, the alarm threshold can be changed in settings
 * Settings stored in EEPROM
 * Debug info: the current CPU voltage and the available memory can be shown at startup
 * Low battery check

Display Modes
-------------

The display mode can be switched forward by pressing the **MODE** button or backwards by pressing the **ALT** button:

Auto (default) -> Manual (1s) -> Manual (5s) -> Manual (10s) -> Manual (30s) -> Manual (1m) -> Manual (10m) -> ALL -> MAX -> DOSE

The following display modes are currently available:

**Auto (default)** - displays the average CPM, the equivalent radiation dose, the analog bar. The averaging interval is based on the current radiation level.

**Manual** - displays the average CPM, the equivalent radiation dose, the total measuring time, the current averaging interval. The averaging interval can be set as: 1s, 5s, 10s, 30s, 1m, 10m.

**ALL** - displays the average CPM, the equivalent radiation dose and the total measuring time. The averaging interval is the total measuring time.

**MAX** - displays the maximal CPM, the maximal equivalent radiation dose reached within measuring time and the total measuring time.

**DOSE** - display the total CPM, the accumulated equivalent radiation dose and the total measuring time.

Auto averaging
--------------

In the auto mode the software uses the automatic averaging algorithm that depends on the average CPM within the last 5 seconds: 
 * 1m if CPM < 60
 * 30s if CPM >= 60 and < 300
 * 10s if CPM >= 300 and < 1500
 * 5s if CPM >= 1500 and CPM < 7500
 * 1s if CPM >= 7500

The current equivalent radiation dose value is blinking while the total measuring time is less than the averaging interval.

Settings
--------

The following settings can be changed during startup:
 * The equivalent radiation dose unit: [Sieverts](http://en.wikipedia.org/wiki/Sievert) "Sv" or [Roentgens](http://en.wikipedia.org/wiki/Roentgen) "R"
 * The alarm threshold: Off, from 0.1 to 100 uSv/h
 * The [CPM to uSv/h conversion ratio](http://gmcounter.org.ua/calc/): from 1 to 2000, defaults to 175 for the [SBM-20](https://sites.google.com/site/diygeigercounter/gm-tubes-supported) [tube](http://en.wikipedia.org/wiki/Geiger-M%C3%BCller_tube)

Use the **MODE** button to increate the value and the **ALT** button to decrease.

Debug info
----------

If the **MODE** button or the **ALT** button was pressed while the startup banner is shown, the current CPU voltage and the available memory will be shown at the display.

Pin configuration
-----------------

 * PIN 9 - not used (the tube switch in the default sketch)
 * PIN 10 - MODE button
 * PIN 11 - ALT button (optional)
 * PIN 15 - outputs HIGH (5v) when the alarm threshold is reached

Note: currently, the **ALT** button is optional. All features will be still available without the **ALT** button connected.

Change Log
----------

**1.3**
 * unit setting
 * ratio setting
 * settings mode
 * better button push feedback
 * low battery check
 * analog bar symbols changed
 * bug fix and improvements

**1.2**
 * debug info
 * ALT button support
 * alarm threshold setting
 * bug fix and improvements

**1.1**
The initial version with basic features

**1.0**
Test version
