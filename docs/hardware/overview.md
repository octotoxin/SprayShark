# Hardware Overview

## Control and drive system

| Component | Role and details |
|---|---|
| Raspberry Pi 5 | High-level control; 8 GB RAM. |
| ESP32-S3 | Low-level control. |
| RoboClaw 2x30A | Controls the two drive motors. |
| Brushed DC drive motors | Two 24 V motors, each with an encoder. The encoders can be connected to the RoboClaw for precise motor control. |

## Battery system

The battery system uses two 25.6 V, 9 Ah batteries connected in parallel. This keeps the system voltage at 25.6 V while increasing the available capacity. Each battery is rated for 20 A continuous current and 30 A peak current.

## Actuator system

The linear actuator is rated at 12 V and has a 50 mm stroke. An IBT-2 motor driver module, also commonly referred to as a BTS7960 module, is dedicated solely to driving the actuator.