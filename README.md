# WateringCtl

A web-based solenoid-valve controller used for watering the garden, plants and the like.

## Requirements

* Accomplish all I/O over WiFi
* Control as many valves as needed by having modular boards using shift-registers
* Run a scheduler on-board that can be fully configured
* Switch valves manually
* Get the number of valves and their states
* Store representative string names for each valve
* Valve health check by measuring the drawn current
* Active valve slot detection by measuring the drawn current