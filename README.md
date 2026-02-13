# ES-Labs - Embedded Systems Laboratory

Arduino-based embedded systems projects using PlatformIO.

## Lab Assignments

### [Lab 1.1 - STDIO Serial Interface](docs/Lab1.1.md)
LED control via serial commands using STDIO library.
- **Hardware**: Arduino Uno, LED, 220Ω resistor
- **Commands**: `led on`, `led off`
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