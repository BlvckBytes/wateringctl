# WateringCtl

![logo](readme_images/logo.png)

A web-based solenoid-valve controller used for watering the garden, plants and the like.

* [Result](#result)
* [Requirements](#requirements)
* [Hardware](#hardware)
  * [usb2serial](#usb2serial)
  * [wifi](#wifi)
  * [relays](#relays)

## Result

![enclosure](readme_images/enclosure.jpg)

![enclosure_inside](readme_images/enclosure_inside.jpg)

![terminal_box](readme_images/terminal_box.jpg)

![ui_wateringctl](readme_images/ui_wateringctl.png)

![ui_fileman](readme_images/ui_fileman.png)

## Requirements

* [x] Accomplish all I/O over WiFi
* [x] Long WiFi range to reach the shed
* [x] No external power-supply
* [x] USB connection to debug/upload code
* [x] Sync up multiple UI-clients (websocket event-system)
* [x] Status-LED where blinking-speed indicate system-states
* [x] Control as many valves as needed by having modular boards using shift-registers
* [x] Run a scheduler on-board that can be fully configured
* [x] Switch valves manually
* [x] Store representative string names for each valve
* [x] Disableable intervals, days and valves
* [x] Manually activate valves for a certain duration
* [x] Mobile-friendly web-UI
* [x] Serve the UI from an SD-card
* [x] Manage all files on the SD-card remotely
* [x] Store configurations on the SD-card
* [x] Upload compiled binaries and then flash from SD
* [x] Web SD-file browser
* [ ] Log all debug-relevant information to SD (yyyy-mm-dd_log.txt)
* [ ] Multiple logging levels that can be streamed using a socket endpoint

## Hardware

I use AutoDesk Eagle to design my PCBs, you can find the projects below in `hardware`.

### usb2serial

![usb2serial_layout](readme_images/layout_usb2serial.png)

![usb2serial_board](readme_images/board_usb2serial.jpg)

Converts USB signals to RX, TX, Reset and Programming-Bootmode signals and is used to flash code onto the micro as well as debug through the serial monitor. It's mounted in the project's enclosure and provides a relatively modern port to interface with computers. With this, I can use cheap barebones ESP SoC's without the surrounding arduino-like development board.

### wifi

![wifi_layout](readme_images/layout_wifi.png)

![wifi_board_front](readme_images/board_wifi_front.jpg)

![wifi_board_back](readme_images/board_wifi_back.jpg)

This board plugs straight into the usb2serial module and represents the most barebones wifi circuit possible. Besides the 3.3V regulation as well as driving an external status-led and the relay-board breakouts, there's not much on there. The antenna is also broken out to an external antenna screw-connector to ensure proper wifi reception.

### relays

![relays_layout](readme_images/layout_relays.png)

![relays_board_front](readme_images/board_relays_front.jpg)

![relays_board_back](readme_images/board_relays_back.jpg)

This board requires 5VDC to power logic, 12VDC for the relays and 24VAC to control the valves using said relays. The 595 shift-register controls all eight relays through on-board transistors safely, since each relay has it's own reverse diode to catch collapsing field currents. Boards can be chained together to have as many valves as you'd like, since D_IN and D_OUT are exposed separately.

### supply

![buck_boost](readme_images/buck_boost.png)

As a power-supply I'm using a simple buck-/boost-converter which steps down 12V to 5V. The 5V will power the 595 relay board directly where as the ESP board will regulate it down further to it's required 3.3V. The toroid-transformer has two equal 12V secondary windings: I'm using one of them through a full bridge rectifier as well as the converter and then both windings in series for the external valves, as they run on 24VAC.