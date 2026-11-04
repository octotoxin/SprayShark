## 1. One architectural recommendation
The most significant missing piece is a dedicated RoboClaw driver module.

A possible future ESP32 structure:

```
firmware/esp32/
├── esp32.ino
├── communication.cpp
├── communication.h
├── movement.cpp
├── movement.h
├── roboclaw.cpp
├── roboclaw.h
├── encoders.cpp
├── encoders.h
├── safety.cpp
├── safety.h
├── spraying.cpp
└── spraying.h
```

`roboclaw.cpp` would own the Basicmicro library interaction, controller initialization, motor commands, and encoder access through RoboClaw.

`movement.cpp` should remain dedicated to steering actuator control.

This separation avoids mixing steering actuator control with drive-motor control.