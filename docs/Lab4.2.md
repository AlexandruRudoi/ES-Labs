# Lab 4.2 — Combined Binary & Analog Actuator Control with FreeRTOS

## Objective
Implement a **combined actuator control system** (Variant C — 100 % grade) on an
Arduino Mega 2560 running FreeRTOS.  A single application drives both a
**relay** (binary actuator, output-debounced) and a **DC motor** via an
**L298N H-bridge module** (analog/PWM actuator) with a full signal-conditioning
pipeline: saturation → median filter → exponential moving average → slew-rate
ramp.  All commands are issued over the serial port; an I2C LCD shows alternating
relay and motor status pages.  Motor speed accepts **−100 to +100 %**: negative
values run the motor in reverse.

---

## Requirements

### Hardware Required
- **Microcontroller**: Arduino Mega 2560
- **5 V Relay module**: single-channel, optocoupler-isolated (active-LOW IN)
- **L298N motor driver module**: dual H-bridge module (ENA jumper **removed**)
- **3 V DC motor** (or equivalent small motor from LAFVIN kit)
- **Green LED**: relay ON indicator
- **Red LED**: relay OFF indicator
- **Passive buzzer**: command feedback beep
- **4× Resistors 220 Ω**: LED current limiting (2 for status LEDs, 2 for motor simulation LEDs in Wokwi)
- **LCD 16×2 I2C**: status display (address 0x27, 5 V, SDA/SCL)
- **Breadboard**
- **Jumper wires**: male-to-male
- **USB cable**: Type-B (Arduino to PC)

### Software Required
- Visual Studio Code + PlatformIO extension
- Framework: Arduino
- Libraries: `feilipu/FreeRTOS@^11.1.0-3`
- Build flag: `-DUSE_FREERTOS` (guards Scheduler Timer2 ISR)

---

## Pin Connections

| Component          | Arduino Pin | Notes                                      |
|--------------------|-------------|--------------------------------------------|
| Relay IN           | 3           | Active-LOW, optocoupler relay module       |
| Green LED          | 4           | Relay ON indicator, 220 Ω to GND          |
| Red LED            | 5           | Relay OFF indicator, 220 Ω to GND         |
| L298N ENA          | 6 (PWM)     | Motor speed (analogWrite 0–255); ENA jumper removed |
| L298N IN1          | 7           | Direction bit A                            |
| L298N IN2          | 9           | Direction bit B                            |
| Passive buzzer     | 11          | Positive leg to pin, negative to GND (Timer1A — avoids Timer4 conflict with pin 6) |
| LCD SDA            | 20 (SDA)    | I2C data                                   |
| LCD SCL            | 21 (SCL)    | I2C clock                                  |
| Relay VCC          | 5 V         | Power for relay coil driver                |
| Relay GND          | GND         | Common ground                              |
| L298N 12 V         | Ext. 12 V supply + | Motor supply voltage                       |
| L298N GND          | GND         | External supply − tied to Arduino GND      |
| LCD VCC            | 5 V         | Power                                      |
| LCD GND            | GND         | Ground                                     |

---

## Physical Setup

### Step 0: Power Rails (do this FIRST)

1. Jumper: Arduino **GND** → any hole on **top `−` rail**
2. Jumper: Arduino **5V** → any hole on **top `+` rail**

```
Arduino 5V  ──────→  [+ rail: ─────────────────────────────────────]
Arduino GND ──────→  [- rail: ─────────────────────────────────────]
```

---

### Green LED (Arduino pin 4)

```
      col:   1   2   3   4   5
row a:               [+]  [-]
row b:               [J]   |
row c:                    [=]
row d:                    [=]
row e:                    [G]──────────→ top − rail
```

Steps:
1. LED long leg (anode) → **col 3, row a**
2. LED short leg (cathode) → **col 4, row a**
3. Resistor 220 Ω leg 1 → **col 4, row b**
4. Resistor 220 Ω leg 2 → **col 4, row e**
5. Jumper: Arduino **pin 4** → **col 3, row b**
6. Jumper: **col 4, row e** → **`−` rail**

Circuit: `Pin 4 → col 3 → LED → col 4 → 220 Ω → GND`

---

### Red LED (Arduino pin 5)

```
      col:   8   9   10  11  12
row a:               [+]  [-]
row b:               [J]   |
row c:                    [=]
row d:                    [=]
row e:                    [G]──────────→ top − rail
```

Steps:
1. LED long leg (anode) → **col 10, row a**
2. LED short leg (cathode) → **col 11, row a**
3. Resistor 220 Ω leg 1 → **col 11, row b**
4. Resistor 220 Ω leg 2 → **col 11, row e**
5. Jumper: Arduino **pin 5** → **col 10, row b**
6. Jumper: **col 11, row e** → **`−` rail**

Circuit: `Pin 5 → col 10 → LED → col 11 → 220 Ω → GND`

---

### Passive Buzzer (Arduino pin 11)

```
      col:   26  27  28
row a:       [+]  ·  [-]     ← buzzer legs
row b:       [J]       |
row c:                 └──────→ − rail
```

Steps:
1. Buzzer `+` leg → **col 26, row a**
2. Buzzer `−` leg → **col 28, row a**; jumper from **col 28, row e** → **`−` rail**
3. Jumper: Arduino **pin 11** → **col 26, row e**

Circuit: `Pin 11 → buzzer → GND`

> **Why pin 11?** Pin 8 is OC4C (Timer4), the same timer as pin 6 (OC4A used
> for motor PWM).  Calling `tone()` on pin 8 reconfigures Timer4 in CTC mode
> and destroys `analogWrite` on pin 6.  Pin 11 uses Timer1A and is safe.

---

### L298N Motor Driver Module

The L298N is a ready-made module with screw terminals for motor output and a
3-pin header row for each channel's control signals.  **Remove the yellow ENA
jumper cap** before wiring — it shorts ENA permanently to 5 V and disables PWM
speed control.

```
  L298N module (top view)
  ┌─────────────────────────────────────┐
  │  [12V]  [GND]  [5V]                 │  ← power terminal block
  │                                     │
  │  [ENA]  (jumper removed!)           │  ← ENA 2-pin header
  │  [IN1]  [IN2]  [IN3]  [IN4]  [ENB] │  ← control header
  │                                     │
  │  [OUT1] [OUT2]          [OUT3][OUT4]│  ← motor terminal blocks
  └─────────────────────────────────────┘
```

| L298N terminal | Connects to              |
|----------------|--------------------------|
| 12 V           | External 12 V supply +   |
| GND            | External supply − **and** Arduino GND (common) |
| 5 V            | Not connected (module has on-board regulator) |
| ENA (header)   | Arduino **pin 6** (PWM) — one wire to either exposed ENA pin |
| IN1            | Arduino **pin 7**        |
| IN2            | Arduino **pin 9**        |
| OUT1 / OUT2    | DC motor terminals       |

Steps:
1. Remove the **yellow ENA jumper cap** from the ENA 2-pin header.
2. Wire one of the two exposed ENA header pins → Arduino **pin 6**.
3. Wire IN1 header pin → Arduino **pin 7**.
4. Wire IN2 header pin → Arduino **pin 9**.
5. Connect motor to **OUT1** and **OUT2** screw terminals.
6. Connect external power supply **+** → L298N **12 V** terminal.
7. Connect external power supply **−** → L298N **GND** terminal **and** Arduino **GND** (common ground is mandatory).

> **ENA jumper**: The yellow cap shorts ENA to 5 V permanently.  With it in
> place the motor always runs at full speed regardless of `analogWrite` — the
> same symptom as a broken H-bridge.  Always remove it when using PWM speed
> control.

Circuit: `Arduino pin 6 (PWM) → ENA`, `pin 7 → IN1`, `pin 9 → IN2` → L298N
drives motor via OUT1/OUT2

---

### 5 V Relay Module (Arduino pin 3)

```
  Relay Module
  ┌──────────────────┐
  │  VCC  IN  GND    │
  │   │    │    │    │
  └───┼────┼────┼────┘
      │    │    │
      │    │    └──→ − rail (GND)
      │    └───────→ Arduino pin 3
      └────────────→ + rail (5 V)
```

Steps:
1. Relay **VCC** → **`+` rail** (5 V)
2. Relay **GND** → **`−` rail** (GND)
3. Relay **IN**  → Arduino **pin 3**

> The relay module is **active-LOW**: pulling IN to LOW energises the coil.
> No load is connected to the screw terminals; the on-board LED and audible
> click serve as feedback.

---

### LCD 16×2 I2C

| LCD pin | Arduino Mega |
|---------|--------------|
| VCC     | 5 V          |
| GND     | GND          |
| SDA     | pin 20       |
| SCL     | pin 21       |

---

### Complete Wiring Summary

```
Arduino Mega 2560
┌───────────────────┐
│  5V  ─────────────┼──→  + rail ──→ Relay VCC
│  GND ─────────────┼──→  − rail ──→ Relay GND
│                   │
│  pin 3  ──────────┼──→  Relay IN (active-LOW)
│  pin 4  ──────────┼──→  Green LED anode  → cathode → 220 Ω → − rail
│  pin 5  ──────────┼──→  Red   LED anode  → cathode → 220 Ω → − rail
│  pin 6  ──────────┼──→  L298N ENA (PWM speed; ENA jumper removed)
│  pin 7  ──────────┼──→  L298N IN1 (direction A)
│  pin 9  ──────────┼──→  L298N IN2 (direction B)
│  pin 11 ──────────┼──→  Buzzer + leg     → − leg   → − rail
│  pin 20 (SDA) ────┼──→  LCD SDA
│  pin 21 (SCL) ────┼──→  LCD SCL
│                   │
│  L298N OUT1/OUT2 ─┼──→  Motor terminals
│  External 12 V ───┼──→  L298N 12 V + GND (GND also tied to Arduino GND)
└───────────────────┘
```

LED current:

$$I_{LED} = \frac{V_{CC} - V_{LED}}{R} = \frac{5\text{ V} - 2\text{ V}}{220\text{ Ω}} \approx 13.6\text{ mA}$$

### Final Setup
![Complete circuit](images/lab4.2-setup.jpg)

---

## Software Architecture

### FreeRTOS 4-Task Pipeline

```
Serial ──→ [ T1: Command Parser ] ──queue──→ [ T2: Relay Control ]     ──mutex──→ [ T4: Display ]
               50 ms poll           relay     event-driven (prio 3)     report      500 ms refresh
                                    cmds
                                  ──queue──→ [ T3: Motor Conditioning ] ──mutex──→ [ T4: Display ]
                                    speed     50 ms (prio 3)
                                    cmds
```

### Task 1 — Command Parser (Priority 2, 50 ms)
- Non-blocking serial character accumulation via `serialLineReady()`
- Two-step `sscanf` parsing: first word + offset (`%s%n`), then argument from
  remaining buffer
- Routes relay commands (`on`, `off`, `toggle`) to `s_relayCmdQueue`
- Routes motor commands (`speed N`, `stop`, `max`) to `s_speedCmdQueue`
- `status` sends `CMD_STATUS` to relay queue for a combined status dump

### Task 2 — Relay Control (Priority 3, event-driven)
- Blocks on `s_relayCmdQueue` (`portMAX_DELAY`)
- Applies **output debouncing** via `OutputDebounce` module (500 ms minimum
  hold) before toggling the relay
- Drives green/red LED indicators
- Buzzer: single beep (accepted), double beep (rejected)
- Writes `RelayReport` struct under mutex

### Task 3 — Motor Conditioning (Priority 3, 50 ms)
- Drains `s_speedCmdQueue` (consumes all pending setpoints with `xQueueReceive`
  in a loop, effectively keeping only the latest)
- **Signal conditioning pipeline**: saturate [−100–100 %] → MedianFilter(5) →
  EMA(α = 0.3) → Ramp(50 %/s up, 100 %/s down)
- Writes PWM to motor via `motorSetPercentSigned()` — negative = reverse
- Writes `MotorReport` struct under mutex

### Task 4 — Display (Priority 1, 500 ms)
- Reads both `RelayReport` and `MotorReport` under mutex
- **LCD page 0** (relay): `Relay:ON  MM:SS` / `T:nnn R:nnn  OK`
- **LCD page 1** (motor): `Set:NNN Flt:NNN.N` / `Rmp:NNN.N Pw:NNN%`
- Pages alternate every 500 ms
- Serial: compact one-line report for plotter/logging

### Signal Conditioning Pipeline (Motor)

```
   setpoint (−100 to +100 %)
       │
       ▼
  [ Saturate ]   clamp to [−100, +100]
       │
       ▼
  [ MedianFilter(5) ]   removes impulse noise
       │
       ▼
  [ EMA α=0.3 ]   smooths rapid changes
       │
       ▼
  [ Ramp 50%/s ↑  100%/s ↓ ]   limits rate of change
       │
       ▼
  motorSetPercentSigned()  → IN1/IN2 direction + analogWrite(ENA, 0–255)
```

Positive setpoint → forward (IN1=HIGH, IN2=LOW).  
Negative setpoint → reverse (IN1=LOW, IN2=HIGH).  
Zero → coast stop (IN1=IN2=LOW, ENA=0).

### Output Debounce (Relay)

```
Command → [OutputDebounce: 500 ms min hold] → Relay driver
              │                                    │
              ├── Accepted → toggle count++        ├── relayOn() / relayOff()
              └── Rejected → reject count++        └── (no change)
```

---

## New Library Modules

### `lib/MotorDriver/`
C-style driver for L298N H-bridge module.

| Function                    | Description                                    |
|-----------------------------|------------------------------------------------|
| `motorInit()`               | Set ENA/IN1/IN2 as OUTPUT, motor stopped       |
| `motorSetSpeed(v)`          | Raw PWM 0–255, forward only                    |
| `motorSetPercent(p)`        | Speed 0.0–100.0 %, forward only                |
| `motorSetPercentSigned(p)`  | Speed −100 to +100 %; negative = reverse       |
| `motorStop()`               | Coast stop (ENA=LOW)                           |
| `motorBrake()`              | Electrical brake (IN1=IN2=HIGH)                |
| `motorGetSpeed()`           | Current raw PWM (0–255)                        |
| `motorGetPercent()`         | Current absolute percent (0–100)               |
| `motorGetPercentSigned()`   | Current signed percent (−100 to +100)          |

### `lib/Ramp/`
Linear slew-rate limiter with independent up/down rates.

| Function            | Description                             |
|---------------------|-----------------------------------------|
| `rampInit()`        | Reset value and target to 0             |
| `rampSetTarget(t)`  | Set the ramp's target value             |
| `rampUpdate(dt)`    | Advance ramp by dt seconds, return value|
| `rampGetValue()`    | Current ramped value                    |
| `rampGetTarget()`   | Current target                          |

---

## Serial Commands

| Command           | Action                                          |
|-------------------|-------------------------------------------------|
| `relay on`        | Request relay ON (subject to output debounce)   |
| `relay off`       | Request relay OFF (subject to output debounce)  |
| `relay toggle`    | Request relay toggle (subject to output debounce)|
| `speed N`         | Set motor speed to N % (−100 to 100); negative = reverse |
| `stop`            | Shortcut for `speed 0`                          |
| `max`             | Shortcut for `speed 100`                        |
| `status`          | Print current relay + motor status              |

> Commands are case-insensitive.  The parser uses a two-step `sscanf` approach
> (first word + offset) because AVR libc does not support `%f` in
> `scanf`/`sscanf` and literal-prefix matching is unreliable.

---

## Wokwi Simulation

Since Wokwi does not natively simulate the L298N module, three LEDs substitute for
the motor driver outputs:

| LED colour | Pin | Represents          |
|------------|-----|---------------------|
| Blue       | 6   | Motor PWM (ENA)     |
| White      | 7   | IN1 — forward bit   |
| White      | 9   | IN2 — reverse bit   |

The blue LED brightness reflects the PWM duty cycle (motor speed).

---

## How to Run

1. Set `ACTIVE_LAB` to `8` in `src/main.cpp`
2. Build and upload: `pio run -e mega -t upload`
3. Open Serial Monitor at **9600 baud**
4. Type commands and press Enter:
   - `relay on` / `relay off` / `relay toggle`
   - `speed 50` / `speed -50` (reverse) / `stop` / `max`
   - `status`
5. Observe the LCD alternating between relay and motor status pages
