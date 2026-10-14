/*
HapticEMGTrainer

This is a simple control system that uses a MyoWare2.0 EMG sensor 
that triggers a haptic motor upon a muscle signal event
and measures the difference of time between the event and when the capacitive button is pressed.

MyoWare 2.0 sheild, haptic motor and an H Bridge motor driver are required. 

Never use a MyoWare sensor while your computer is connected to 120V wall power.
*/



// Motor connections
const int enA = 13;
const int in1 = 32;
const int in2 = 14;
int steps[6] = {0, 50, 100, 150, 200, 255};

// Motor pulse (non-blocking)
#define MOTOR_PULSE_MS 300      // How long the motor runs after a trigger
bool motorOn = false;
unsigned long motorStartTime = 0;

//Touch config
const int TOUCH_PIN = 4;
const int TOUCH_ACTIVATE = 30;   // Set from your readings: untouched is often ~40-80 and DROPS when touched
bool waitingForTouch = false;    // true after an EMG trigger, until a touch
unsigned long emgTime = 0;       // micros() timestamp of the EMG trigger

// Muscle configuration
#define EMG_PIN 26              // Pin for EMG signal (ADC2 - fine as long as WiFi is off)
#define THRESHOLD 200           // Threshold value (0-4095 for 12-bit ADC)
#define DEBOUNCE_TIME 300       // Debounce time in milliseconds
#define ENVELOPE_WINDOW_MS 40   // Moving average window (40ms)
#define SAMPLE_RATE_MS 5        // How often to sample EMG (5ms = 200Hz)

// Toggle for using envelope detection
bool useEnvelope = true;  // Set to false to use raw signal

// ===== GLOBALS =====
// Moving average buffer
#define BUFFER_SIZE (ENVELOPE_WINDOW_MS / SAMPLE_RATE_MS)  // 40ms / 5ms = 8 samples
int emgBuffer[BUFFER_SIZE];
int bufferIndex = 0;
long bufferSum = 0;

// Debouncing
unsigned long lastTriggerTime = 0;
bool isTriggered = false;

// Timing
unsigned long lastSampleTime = 0;
unsigned long latency = 0;       // EMG-to-touch time in microseconds

// ===== FUNCTIONS =====

/**
 * Initialize EMG processing
 * Call this in setup()
 */
void initEMG() {
  // Initialize buffer with zeros
  for (int i = 0; i < BUFFER_SIZE; i++) {
    emgBuffer[i] = 0;
  }
  bufferSum = 0;
  bufferIndex = 0;
  lastTriggerTime = 0;
  isTriggered = false;

  pinMode(EMG_PIN, INPUT);
}

/**
 * Moving Average Envelope Detection
 *
 * @param newValue - New rectified EMG reading
 * @return Smoothed envelope value
 */
int getEnvelope(int newValue) {
  // Remove oldest value from sum
  bufferSum -= emgBuffer[bufferIndex];

  // Add new value to buffer and sum
  emgBuffer[bufferIndex] = newValue;
  bufferSum += newValue;

  // Move to next position (circular buffer)
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;

  // Return average
  return bufferSum / BUFFER_SIZE;
}

/**
 * Debounced Threshold Detection
 *
 * @param signalValue - Current signal value (raw or envelope)
 * @param threshold - Threshold value to detect
 * @return true if threshold crossed AND debounce period has elapsed
 */
bool detectThresholdCrossing(int signalValue, int threshold) {
  unsigned long currentTime = millis();

  // Check if signal exceeds threshold
  bool aboveThreshold = (signalValue >= threshold);

  // Check if debounce period has elapsed
  bool debounceElapsed = (currentTime - lastTriggerTime) >= DEBOUNCE_TIME;

  // Trigger only if above threshold AND debounce time has passed
  if (aboveThreshold && debounceElapsed && !isTriggered) {
    lastTriggerTime = currentTime;
    isTriggered = true;
    return true;  // New trigger detected!
  }

  // Reset trigger flag when signal drops below threshold
  if (!aboveThreshold) {
    isTriggered = false;
  }

  return false;  // No trigger
}

/**
 * Process EMG Signal
 * Call this in loop() - handles timing automatically
 *
 * @return true if threshold crossing detected (debounced)
 */
bool processEMG() {
  unsigned long currentTime = millis();

  // Check if it's time to sample
  if (currentTime - lastSampleTime < SAMPLE_RATE_MS) {
    return false;  // Not time to sample yet
  }
  lastSampleTime = currentTime;

  // Read raw EMG value (assumed to be already rectified)
  int rawEMG = analogRead(EMG_PIN);

  // Choose signal processing path
  int processedSignal;
  if (useEnvelope) {
    processedSignal = getEnvelope(rawEMG);
  } else {
    processedSignal = rawEMG;
  }

  // Detect threshold crossing with debouncing
  bool triggered = detectThresholdCrossing(processedSignal, THRESHOLD);

  return triggered;
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  // Diagnostic: 1 = normal power-on, 4 = crash, 9 = brownout
  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());

  // Set all the pins as outputs
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  initEMG();

  // Turn off the motor to start with
  digitalWrite(enA, LOW);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void loop() {
  int touchValue = touchRead(TOUCH_PIN);
  //Serial.println(touchValue);

  // Process EMG
  bool motorTrigger = processEMG();

  // EMG event: start the timer (ignored if already waiting for a touch)
  if (motorTrigger && !waitingForTouch) {
    emgTime = micros();          // timestamp first, before printing
    waitingForTouch = true;
    Serial.println("TRIGGERED");

    mid();
    motorOn = true;
    motorStartTime = millis();
  }

  // Stop the motor after the pulse, without blocking
  if (motorOn && millis() - motorStartTime >= MOTOR_PULSE_MS) {
    off();
    motorOn = false;
  }

  // Touch only counts after an EMG trigger
  if (waitingForTouch && touchValue <= TOUCH_ACTIVATE) {
    latency = micros() - emgTime;  // measure first, before printing
    waitingForTouch = false;

    Serial.print("TOUCHED | EMG to touch: ");
    Serial.print(latency / 1000.0, 2);   // microseconds -> ms, 2 decimals
    Serial.println(" ms");
  }
}

void off() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, steps[0]);
}

void mid() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, steps[4]);
}


