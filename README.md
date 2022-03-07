# WateringCtl

![logo](readme_images/logo.png)

A web-based solenoid-valve controller used for watering the garden, plants and the like.

## Requirements

* Accomplish all I/O over WiFi
* Control as many valves as needed by having modular boards using shift-registers
* Run a scheduler on-board that can be fully configured
* Switch valves manually
* Store representative string names for each valve
* Disableable intervals, days and valves
* Mobile-friendly web-UI

## Hardware

### usb2serial

![usb2serial](readme_images/layout_usb2serial.png)

Converts USB signals to RX, TX, Reset and Programming-Bootmode signals and is used to flash code onto the micro as well as debug through the serial monitor. It's mounted in the project's enclosure and provides a relatively modern port to interface with computers. With this, I can use cheap barebones ESP SoC's without the surrounding arduino-like development board.

### relays

![relays](readme_images/layout_relays.png)

This board requires 5VDC to power logic, 12VDC for the relays and 24VAC to control the valves using said relays. The 595 shift-register controls all eight relays through on-board transistors safely, since each relay has it's own reverse diode to catch collapsing field currents. Boards can be chained together to have as many valves as you'd like, since D_IN and D_OUT are exposed separately.