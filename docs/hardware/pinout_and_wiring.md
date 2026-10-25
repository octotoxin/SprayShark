# Proposed ESP32-S3 Pin Assignment

> These GPIO assignments are a recommended integration plan. The combined wiring has
> not necessarily been tested on hardware.

| ESP32-S3 GPIO | Connect to | Purpose |
| --- | --- | --- |
| GPIO 17 (TX) | RoboClaw S1 | Send motor commands |
| GPIO 16 (RX) | RoboClaw S2, through suitable level protection | Receive RoboClaw replies |
| GND | RoboClaw signal GND (−) | Common signal reference |
| GPIO 1 | IBT-2 RPWM | Actuator extend command |
| GPIO 2 | IBT-2 LPWM | Actuator retract command |
| GPIO 4 | IBT-2 R_EN | Enable right bridge |
| GPIO 5 | IBT-2 L_EN | Enable left bridge |
| GPIO 6 | IBT-2 R_IS, after checking signal circuitry | Extend-side current sensing |
| GPIO 7 | IBT-2 L_IS, after checking signal circuitry | Retract-side current sensing |
| GPIO 8 | Relay/MOSFET control input | Spray pump ON/OFF |
| GND | IBT-2 logic GND and pump-driver control GND | Shared signal reference |