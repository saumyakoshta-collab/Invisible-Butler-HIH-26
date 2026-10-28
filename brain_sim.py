"""
Invisible Butler - AI Brain (SIMULATED INPUTS)

Plays the role of the Raspberry Pi "AI Brain". It turns a voice command and a
person position into a one-letter motor command for the ESP32 Core Robot:
    F = forward, L = turn left, R = turn right, S = stop

IMPORTANT: voice and camera are NOT real here. The command (FOLLOW/STOP) and the
person position (LEFT/CENTER/RIGHT/LOST) are simulated inputs. Obstacle safety is
NOT decided here; it is enforced independently in the ESP32 firmware.

Usage:
    python brain_sim.py                     # run the decision test table
    python brain_sim.py --print-stream      # print a demo command sequence
    python brain_sim.py --port COM5         # send the demo sequence over serial
    python brain_sim.py --port rfc2217://localhost:4000   # Wokwi VS Code serial bridge
(serial mode needs:  pip install pyserial)
"""
import argparse
import time

VALID_COMMANDS = ("FOLLOW", "STOP")
VALID_POSITIONS = ("LEFT", "CENTER", "RIGHT", "LOST")


def decide(command: str, position: str) -> str:
    """Return F/L/R/S for a voice command and person position."""
    command = command.upper()
    position = position.upper()
    if command not in VALID_COMMANDS or position not in VALID_POSITIONS:
        return "S"  # unknown input -> fail safe
    if command == "STOP":
        return "S"
    # command == FOLLOW
    return {"CENTER": "F", "LEFT": "L", "RIGHT": "R", "LOST": "S"}[position]


# (command, position, expected motor command)
TEST_TABLE = [
    ("FOLLOW", "CENTER", "F"),
    ("FOLLOW", "LEFT", "L"),
    ("FOLLOW", "RIGHT", "R"),
    ("FOLLOW", "LOST", "S"),
    ("STOP", "CENTER", "S"),
    ("STOP", "LEFT", "S"),
    ("STOP", "RIGHT", "S"),
    ("STOP", "LOST", "S"),
    ("DANCE", "CENTER", "S"),   # unknown command -> safe stop
    ("FOLLOW", "UP", "S"),      # unknown position -> safe stop
]

# Demo scenario: (command, position, seconds to hold)
DEMO = [
    ("FOLLOW", "CENTER", 3),
    ("FOLLOW", "LEFT", 2),
    ("FOLLOW", "RIGHT", 2),
    ("FOLLOW", "CENTER", 3),
    ("FOLLOW", "LOST", 2),
    ("FOLLOW", "CENTER", 2),
    ("STOP", "CENTER", 2),
]


def run_tests() -> bool:
    print(f"{'COMMAND':<8} {'POSITION':<9} {'EXPECTED':<9} {'GOT':<4} RESULT")
    ok = True
    for cmd, pos, expected in TEST_TABLE:
        got = decide(cmd, pos)
        passed = got == expected
        ok &= passed
        print(f"{cmd:<8} {pos:<9} {expected:<9} {got:<4} {'PASS' if passed else 'FAIL'}")
    print("ALL PASSED" if ok else "SOME FAILED")
    return ok


def run_demo(port: str | None) -> None:
    ser = None
    if port:
        import serial  # pip install pyserial
        ser = serial.serial_for_url(port, baudrate=115200, timeout=0.2)
        time.sleep(2)
    for cmd, pos, seconds in DEMO:
        motor = decide(cmd, pos)
        print(f"[BRAIN] voice={cmd:<6} person={pos:<6} -> send '{motor}' for {seconds}s")
        end = time.time() + seconds
        while time.time() < end:
            if ser:
                ser.write((motor + "\n").encode())
                # print whatever the ESP32 reports back
                while ser.in_waiting:
                    print("   [ESP32]", ser.readline().decode(errors="ignore").strip())
            time.sleep(0.5)  # resend every 0.5 s (keeps the ESP32 watchdog happy)
    if ser:
        ser.write(b"S\n")
        ser.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help="serial port (COM5) or rfc2217://localhost:4000")
    parser.add_argument("--print-stream", action="store_true", help="print demo without serial")
    args = parser.parse_args()

    if args.port or args.print_stream:
        run_demo(args.port)
    else:
        raise SystemExit(0 if run_tests() else 1)
