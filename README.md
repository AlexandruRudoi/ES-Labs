# ES-Labs - Embedded Systems Laboratory

Arduino-based embedded systems projects using PlatformIO.

## Lab Assignments

### [Lab 1.1 - STDIO Serial Interface](docs/Lab1.1.md)
LED control via serial commands using STDIO library.
- **Hardware**: Arduino Uno, LED, 220Ω resistor
- **Commands**: `led on`, `led off`
- **Status**: Completed
### [Lab 1.2 - STDIO LCD + Keypad](docs/Lab1.2.md)
Code lock using 4x4 keypad input and LCD display via STDIO library.
- **Hardware**: Arduino Uno, LCD 2x16 (I2C), Keypad 4x4, green/red LEDs, 2x 220Ω resistors
- **Features**: Enter 4-digit code, green LED = valid, red LED = invalid
- **Status**: Completed
### [Lab 2.1 - Sequential Non-Preemptive Task Scheduling](docs/Lab2.1.md)
Bare-metal scheduler using Timer2 ISR with button press detection, LED feedback and periodic STDIO reporting.
- **Hardware**: Arduino Uno, push button, green/red/yellow LEDs, 3x 220Ω resistors
- **Features**: 1 ms tick scheduler, short/long press classification, provider/consumer signal model
- **Status**: Completed
### [Lab 2.2 - Preemptive Task Scheduling with FreeRTOS](docs/Lab2.2.md)
FreeRTOS port of Lab 2.1 with preemptive scheduling, binary semaphore for press events and mutex-protected statistics.
- **Hardware**: Arduino Mega 2560, push button, green/red/yellow LEDs, 3x 220Ω resistors
- **Features**: `vTaskDelayUntil` periodic tasks, binary semaphore (T1→T2), mutex-protected Stats module (T2/T3)
- **Status**: Completed
### [Lab 3.1 - Binary Signal Conditioning](docs/Lab3.1.md)
Binary conditioning pipeline (saturation → hysteresis → debounce) for DHT11 and NTC sensors under FreeRTOS.
- **Hardware**: Arduino Mega 2560, DHT11, NTC 10 kΩ, green/red LEDs, passive buzzer, LCD I2C
- **Features**: BinaryConditioner module, 3-task FreeRTOS pipeline, LCD + Serial display
- **Status**: Completed
### [Lab 3.2 - Analog Signal Conditioning](docs/Lab3.2.md)
Full analog conditioning pipeline (saturation → median filter → EMA → binary alert) for DHT11, NTC and LDR sensors.
- **Hardware**: Arduino Mega 2560, DHT11, NTC 10 kΩ, LDR, green/red LEDs, passive buzzer, LCD I2C
- **Features**: MedianFilter + WeightedAverage modules, multi-stage pipeline, Serial Plotter output
- **Status**: Completed
### [Lab 4.1 - Binary Actuator Control](docs/Lab4.1.md)
Binary actuator control via relay with output debounce conditioning under FreeRTOS.
- **Hardware**: Arduino Mega 2560, 5 V relay module, green/red LEDs, passive buzzer, LCD I2C
- **Features**: Relay + OutputDebounce modules, serial command interface (on/off/toggle/status), 3-task FreeRTOS pipeline
- **Status**: Completed
### [Lab 4.2 - Analog Actuator Control](docs/Lab4.2.md)
PWM-based analog actuator control with signal conditioning and serial command interface under FreeRTOS.
- **Hardware**: Arduino Mega 2560, L293D H-bridge, DC motor, green/red LEDs, passive buzzer, LCD I2C
- **Features**: MotorDriver module, PWM speed control, serial commands (speed/up/down/stop/status), 4-task FreeRTOS pipeline
- **Status**: Completed
### [Lab 5.1 - ON-OFF Temperature Control with Hysteresis](docs/Lab5.1.md)
ON-OFF temperature control with hysteresis dead band, two-speed fan zones, and runtime parameter tuning under FreeRTOS.
- **Hardware**: Arduino Mega 2560, DHT11, L293D H-bridge, DC motor, green/red LEDs, passive buzzer, LCD I2C
- **Features**: Hysteresis controller, two-speed zones (50%/100%), serial commands (set/hyst/status), FreeRTOS queue + mutex, Serial Plotter output
- **Status**: Completed
## Project Structure

```
ES-Labs/
├── lib/          # reusable peripheral modules
├── src/          # main application code
├── docs/         # lab documentation
├── report/       # LaTeX lab reports
└── include/      # global headers
```

## Setup

1. Install [PlatformIO](https://platformio.org/) in VS Code
2. Connect Arduino Uno via USB
3. Build: `platformio run`
4. Upload: `platformio run --target upload`
5. Monitor: `platformio device monitor`

## Architecture

- **Modular design** - each peripheral in separate lib
- **CamelCase** naming convention
- **Reusable components** for future labs