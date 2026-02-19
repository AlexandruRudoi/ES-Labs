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
## Project Structure

```
ES-Labs/
├── lib/          # reusable peripheral modules
├── src/          # main application code
├── docs/         # lab documentation
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