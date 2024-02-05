#include <Arduino.h>

// Variables for stepper motor
const int STEPPER_PINS[] = {9, 10, 11, 12}; // Coils
const int SEQUENCE_CW[][4] = {  // Clockwise sequence
  {HIGH, LOW, LOW, LOW},
  {LOW, HIGH, LOW, LOW},
  {LOW, LOW, HIGH, LOW},
  {LOW, LOW, LOW, HIGH}
};
const int SEQUENCE_CCW[][4] = { // Counterclockwise sequence
  {LOW, LOW, LOW, HIGH},
  {LOW, LOW, HIGH, LOW},
  {LOW, HIGH, LOW, LOW},
  {HIGH, LOW, LOW, LOW}
};

int stepNumber;  // For moving/looping over coils
int currentStep; // For tracking which step motor is currently on

// Variables for ultrasonic sensors
//const int TRIG_PIN_1 = 6, ECHO_PIN_1 = 7, TRIG_PIN_2 = 5, ECHO_PIN_2 = 4, TRIG_PIN_3 = 3, ECHO_PIN_3 = 2;
//int proximity_1, prevProximity_1, proximity_2, prevProximity_2, proximity_3, prevProximity_3;

const int NUM_SENSORS = 3;
const int TRIG_PINS[] = {3,5,6};
const int ECHO_PINS[] = {2,4,7};
int proximities[3];
int prevProximities[3];

const int STEP_LIMIT = 2048;        // Furthest step that motor can land on (2048 steps = one full motor revolution)
const int MIN_STEPS = 400;          // Min steps per proximity change 
const int MAX_PROXIMITY_DIFF = 15;  // Max allowed difference between sensors for values to be considered valid
const int MIN_PROXIMITY_CHANGE = 2; // Min difference needed between previous and current proximity sensor readings to affect motor
const int NOISE_THRESHOLD = 100;    // Max acceptable value before we consider it noise

// Function prototypes
void rotateStepper(bool);
int getProximity(int, int);
int mapValue(float, float, float, float);
bool checkNoise(const int[]);
bool checkMaxSensorDiff(const int[]);
bool checkMinProximityChange(const int[], const int[]);
bool checkProximityDec(const int[], const int[]);
bool checkProximityInc(const int[], const int[]);

void setup() {
  Serial.begin(9600);
  
  // Set pin modes and variables
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(TRIG_PINS[i], OUTPUT);
    pinMode(ECHO_PINS[i], INPUT);
    prevProximities[i] = getProximity(TRIG_PINS[i], ECHO_PINS[i]);
  }
  
  for (int i = 0; i < 4; i++) {
    pinMode(STEPPER_PINS[i], OUTPUT);
  }
  
  stepNumber = 0;
  currentStep = 0;
}

void loop() {
  // Get proximity value from each ultrasonic sensor
  for (int i = 0; i < NUM_SENSORS; i++) {
    proximities[i] = getProximity(TRIG_PINS[i], ECHO_PINS[i]);
  }

  // If all tests are passed, rotate the motor
  if (checkNoise(proximities) && checkMaxSensorDiff(proximities) && checkMinProximityChange(proximities, prevProximities)) {
    int stepCount = 0;

    // If proximity has decreased, rotate stepper clockwise (tighten) unless it has reached min limit
    if (checkProximityDec(proximities, prevProximities)) {
        while (stepCount < MIN_STEPS && currentStep < STEP_LIMIT) {
          rotateStepper(true);
          stepCount++;
          currentStep++;
          delay(3);
        }
    } 
    // Else if proximity has increased, rotate stepper counterclockwise (loosen) unless it has reached max limit
    else if (checkProximityInc(proximities, prevProximities)) {
        while (stepCount < MIN_STEPS && currentStep > 0) {
          rotateStepper(false);
          stepCount++;
          currentStep--;
          delay(3);
        }
    } 
    for (int i = 0; i < NUM_SENSORS; i++) {
      prevProximities[i] = getProximity(TRIG_PINS[i], ECHO_PINS[i]);
    }
  } 
  else {
    delay(10);
  }
}

// Function to rotate stepper motor
void rotateStepper(bool cw) {
  // Select appropriate sequence (rotation direction) based on bool value
  const int (*sequence)[4]; 

  if (cw) {
      sequence = SEQUENCE_CW;
  } else {
      sequence = SEQUENCE_CCW;
  }

    // Apply sequence to stepper motor by writing to the coils
    for (int i = 0; i < 4; i++) {
      digitalWrite(STEPPER_PINS[i], sequence[stepNumber][i]);
    }
  
    // Update step number by cycling in a continuous loop
    stepNumber = (stepNumber + 1) % 4; 
}

// Function to read proximity value from ultraosnic sensor
int getProximity(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(5);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  long duration = pulseIn(echoPin, HIGH);

  // Convert time to inches
  return (duration / 2) / 74;
}

// Function for range mapping
int mapValue(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Function that returns false only if too much noise (or distance) is detected with any of the ultrasonic sensors
bool checkNoise(const int proximities[]) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (proximities[i] > NOISE_THRESHOLD) {
      return false;
    }
  }
  return true;
}

// Function that returns false only if there is too large a discrepency in readings across any two sensors
bool checkMaxSensorDiff(const int proximities[]) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    for (int j = i+1; j < NUM_SENSORS; j++) {
      if (abs(proximities[i] - proximities[j]) > MAX_PROXIMITY_DIFF) {
        return false;
      }
    }
  }
  return true;
}

// Function that returns false only if difference between previous and current proximity readings on any individual sensor is too small (and therefore considered too insignificant)
bool checkMinProximityChange(const int proximities[], const int prevProximities[]) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (abs(proximities[i] - prevProximities[i]) > MIN_PROXIMITY_CHANGE) {
      return true;
    }
  }
  return false;  
}

// Function that returns true only if proximity on all sensors has decreased
bool checkProximityDec(const int proximities[], const int prevProximities[]) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (proximities[i] >= prevProximities[i]) {
      return false;
    }
  }
  return true;
}

// Function that returns true only if proximity on all sensors has increased
bool checkProximityInc(const int proximities[], const int prevProximities[]) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (proximities[i] <= prevProximities[i]) {
      return false;
    }
  }
  return true;
}
