// Invisible Butler - Core Robot firmware (ESP32)
// The "AI Brain" sends one-letter commands over serial:
//   F = forward, L = turn left, R = turn right, S = stop
// Safety is handled here, independent of the Brain.

// ---- Pins ----
const int TRIG_PIN    = 5;
const int ECHO_PIN    = 18;
const int RED_LED_1   = 27;
const int RED_LED_2   = 14;
const int GREEN_LED_1 = 12;
const int GREEN_LED_2 = 13;
const int MOTOR_LEFT  = 25;   // HIGH = left wheel drives
const int MOTOR_RIGHT = 26;   // HIGH = right wheel drives

// ---- Settings ----
const float STOP_DISTANCE_CM = 20.0;
const unsigned long SENSE_INTERVAL_MS  = 60;
const unsigned long STATUS_INTERVAL_MS = 500;
// Real robot: use ~500 ms. 5000 ms here so you can type commands by hand.
const unsigned long WATCHDOG_MS = 5000;

char requestedCmd = 'S';          // what the Brain asked for
unsigned long lastCmdTime = 0;
unsigned long lastSense = 0;
unsigned long lastStatus = 0;
float distanceCm = 999;
char lastApplied = '?';

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;          // no echo = nothing close
  return duration * 0.0343 / 2.0;
}

void drive(char cmd) {
  bool left = false, right = false;
  switch (cmd) {
    case 'F': left = true;  right = true;  break;  // straight
    case 'L': left = false; right = true;  break;  // turn left
    case 'R': left = true;  right = false; break;  // turn right
    default:  break;                               // stop
  }
  digitalWrite(MOTOR_LEFT,  left  ? HIGH : LOW);
  digitalWrite(MOTOR_RIGHT, right ? HIGH : LOW);
}

void setLeds(bool blocked) {
  digitalWrite(RED_LED_1,   blocked ? HIGH : LOW);
  digitalWrite(RED_LED_2,   blocked ? HIGH : LOW);
  digitalWrite(GREEN_LED_1, blocked ? LOW : HIGH);
  digitalWrite(GREEN_LED_2, blocked ? LOW : HIGH);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(GREEN_LED_1, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  pinMode(MOTOR_LEFT, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);
  drive('S');
  Serial.println("CORE ROBOT READY. Commands: F L R S");
}

void loop() {
  unsigned long now = millis();

  // 1) Read commands from the AI Brain
  while (Serial.available()) {
    char c = toupper(Serial.read());
    if (c == 'F' || c == 'L' || c == 'R' || c == 'S') {
      requestedCmd = c;
      lastCmdTime = now;
    }
  }

  // 2) Measure distance
  if (now - lastSense >= SENSE_INTERVAL_MS) {
    lastSense = now;
    distanceCm = readDistanceCm();
  }

  // 3) Safety decisions (these override the Brain)
  bool blocked = (distanceCm <= STOP_DISTANCE_CM);
  bool brainSilent = (now - lastCmdTime > WATCHDOG_MS);

  char applied = requestedCmd;
  const char* reason = "COMMAND";
  if (blocked) {
    applied = 'S';
    reason = "OBSTACLE_OVERRIDE";
  } else if (brainSilent) {
    applied = 'S';
    reason = "WATCHDOG_TIMEOUT";
  }

  drive(applied);
  setLeds(blocked);

  // 4) Report status when it changes, and every 500 ms
  if (applied != lastApplied || now - lastStatus >= STATUS_INTERVAL_MS) {
    lastApplied = applied;
    lastStatus = now;
    Serial.print("STATUS dist=");
    Serial.print(distanceCm, 1);
    Serial.print("cm requested=");
    Serial.print(requestedCmd);
    Serial.print(" applied=");
    Serial.print(applied);
    Serial.print(" reason=");
    Serial.println(reason);
  }
}
