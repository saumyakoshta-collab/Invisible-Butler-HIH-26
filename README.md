# Invisible Butler — Functional Simulation (Hack in Hills '26)

**Team:** Binary Rebels — Saumya Koshta (Robotics & Hardware), Sai Shweta Rao (AI & Software)
**Track:** Vision-Based Autonomous Robot
**Live Wokwi simulation:** https://wokwi.com/projects/478882043586329601

Invisible Butler is a voice-controlled, person-following assistant robot. It is split into an **AI Brain** (Raspberry Pi: voice, vision, decisions) and a **Core Robot** (ESP32: motor control and safety). This repository documents a **functional simulation** of the Core Robot in Wokwi, plus a Python simulation of the Brain's decision logic, because physical hardware could not be arranged in time.

## What is real and what is simulated (read this first)

| Part | Status in this submission |
|---|---|
| ESP32 firmware (command parsing, motor outputs, obstacle override, watchdog) | **Real code, running in the Wokwi simulator** |
| HC-SR04 distance sensing, LEDs, motor-output signals | **Simulated electronics in Wokwi** (interactive distance slider) |
| Brain decision logic (FOLLOW/STOP + LEFT/CENTER/RIGHT/LOST → F/L/R/S) | **Real Python code, tested** (`brain_sim.py`) |
| Camera person detection | **Not implemented.** Person position is a simulated input. |
| Voice recognition / voice feedback | **Not implemented.** Voice commands are simulated as typed text. |
| Live link between Python and the Wokwi ESP32 | **Not demonstrated.** Both use the same serial protocol, and commands are typed into the Wokwi Serial Monitor. A serial mode exists in `brain_sim.py` but was not tested against Wokwi. |

## System architecture

```
 [Microphone] -> Voice command  --\
                                   +--> AI BRAIN (Raspberry Pi, Python)
 [Camera] -> Person LEFT/CENTER/RIGHT/LOST --/        |
                                                      |  serial link: F / L / R / S
                                                      v
                                          CORE ROBOT (ESP32, C++)
        HC-SR04 distance --> SAFETY OVERRIDE (<= 20 cm => STOP)  [independent of Brain]
        Brain-silent WATCHDOG (no command => STOP)
                                          |
                                          v
                              Left motor / Right motor outputs
```

Key design rule: the ESP32 enforces safety **by itself**. If an obstacle is at 20 cm or closer, or the Brain stops sending commands, the motors stop regardless of what the Brain requested.

## Hardware components and specifications

**Simulated in Wokwi:**

| Component | Role | Key specs |
|---|---|---|
| ESP32 DevKit | Core Robot controller | 3.3 V logic, dual-core, 240 MHz |
| HC-SR04 | Obstacle distance sensor | 2–400 cm range (sim), 5 V supply |
| 2 × red LED | STOP indicators | with series resistors |
| 2 × green LED | PATH CLEAR indicators | with series resistors |
| 2 × LED (motor channels) | Left / right motor drive outputs | GPIO 25 = left, GPIO 26 = right |

**Proposed for the physical build:** Raspberry Pi 4/5 (AI Brain), Pi Camera or USB webcam, USB microphone, small speaker, ESP32 DevKit, HC-SR04 (with 5 V→3.3 V divider on ECHO), L298N or TB6612FNG motor driver, 2 × DC gear motors with wheels, caster wheel, 2-wheel chassis, battery pack (motors) plus a separate 5 V supply for the Pi.

## Pin mapping (ESP32)

| Function | GPIO |
|---|---|
| HC-SR04 TRIG | 5 |
| HC-SR04 ECHO | 18 |
| Red STOP LEDs | 27, 14 |
| Green PATH CLEAR LEDs | 12, 13 |
| Left motor output | 25 |
| Right motor output | 26 |

Circuit diagram: open the Wokwi link above, and see `docs/wokwi-circuit.png` and `diagram.json` in this repo.

## Serial protocol (Brain → Core Robot, 115200 baud)

| Command | Meaning | Left motor | Right motor |
|---|---|---|---|
| `F` | Forward | ON | ON |
| `L` | Turn left | OFF | ON |
| `R` | Turn right | ON | OFF |
| `S` | Stop | OFF | OFF |

Differential drive: to turn left, only the right wheel drives. The ESP32 replies with lines like
`STATUS dist=15.0cm requested=F applied=S reason=OBSTACLE_OVERRIDE`.

## How to run and test

### A. ESP32 simulation (Wokwi)
1. Open https://wokwi.com/projects/478882043586329601 (or create a new ESP32 project and paste `sketch.ino` and `diagram.json`).
2. Click **Start simulation**.
3. Click the HC-SR04 and set its distance to about 100 cm. Green LEDs should be on.
4. In the Serial Monitor input box, type a command and press Enter.

### B. Brain logic test (Python 3.10+)
```
python brain_sim.py
```
Runs the decision table and prints PASS/FAIL per row. Other modes: `--print-stream` (prints a demo command sequence), `--port <port>` (sends the sequence over serial; needs `pip install pyserial`; not tested against Wokwi).

## Test scenarios and results

**ESP32 simulation (Wokwi), manual tests:**

| # | Scenario | Expected | Result |
|---|---|---|---|
| 1 | Distance 100 cm, no obstacle | Green LEDs on, red off | Pass |
| 2 | Send `F` | Both motor outputs ON | Pass |
| 3 | Send `L` / `R` / `S` | Right only / left only / both off | Pass |
| 4 | Send `F`, then distance ≤ 20 cm | Motors stop, red LEDs on, status `OBSTACLE_OVERRIDE` | Pass |
| 5 | Send `F`, then send nothing for 5 s | Motors stop, status `WATCHDOG_TIMEOUT` | Pass |

**Brain decision logic (`python brain_sim.py`):** 10 of 10 cases pass, including unknown command and unknown position, which both fail safe to `S`.

Screenshots of these runs are in `docs/`.

## Simulation limitations

- No real camera or person detection; position (LEFT/CENTER/RIGHT/LOST) is a simulated input.
- No real speech recognition or voice feedback; commands are typed.
- Wokwi has no stock DC motor or L298N part, so motors are represented by output signals on GPIO 25/26 shown with LEDs. Speed control (PWM) and real motor dynamics are not simulated.
- The Python Brain and the Wokwi ESP32 are not connected automatically; commands are typed into the Serial Monitor.
- The watchdog is 5 s in simulation so commands can be typed by hand; the real robot would use about 0.5 s.
- No avoidance manoeuvre: the robot stops at obstacles but does not steer around them.

## Proposed physical implementation plan

1. **Core Robot:** mount ESP32, motor driver, two DC motors and HC-SR04 on a 2-wheel chassis. Drive the motor driver's direction pins from the ESP32 (this replaces the GPIO 25/26 LEDs) and add PWM for speed. Put a voltage divider on the HC-SR04 ECHO line (5 V → 3.3 V). Power motors from a separate battery with common ground.
2. **Verify safety first:** with no Brain connected, confirm the obstacle stop and watchdog work on real hardware.
3. **AI Brain:** Raspberry Pi with camera and microphone. Add offline person detection (for example a lightweight detector) and map the person's horizontal position in the frame to LEFT/CENTER/RIGHT/LOST. Add offline speech recognition for FOLLOW/STOP and text-to-speech for feedback ("I'm following you").
4. **Integrate:** connect the Pi to the ESP32 over USB serial using the `F/L/R/S` protocol above, and send commands about every 0.5 s.
5. **Test and tune:** following distance, turn thresholds, and a safe stop distance based on real braking behaviour.

## Repository contents

- `README.md` — this document
- `sketch.ino` — ESP32 Core Robot firmware
- `diagram.json` — Wokwi circuit definition
- `brain_sim.py` — Brain decision logic and test table (simulated inputs)
- `docs/` — screenshots (Wokwi circuit, serial output, test runs)
