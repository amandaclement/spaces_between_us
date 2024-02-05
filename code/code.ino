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
int currentStep; // For step motor is currently on

// Variables for first ultrasonic sensors
const int TRIG_PIN_1 = 6;
const int ECHO_PIN_1 = 7;  
int proximity_1, prevProximity_1;

// Varibables for second ultrasonic sensor
const int TRIG_PIN_2 = 3;
const int ECHO_PIN_2 = 4;  
int proximity_2, prevProximity_2;

const int STEP_LIMIT = 2048;        // Furthest step that motor can land on (2048 steps = one full motor revolution)
const int MIN_STEPS = 400;          // Min steps per proximity change 
const int MAX_PROXIMITY_DIFF = 15;  // Max allowed difference between sensors for values to be considered valid
const int MIN_PROXIMITY_CHANGE = 2; // Min difference needed between previous and current proximity sensor readings to affect motor
const int NOISE_THRESHOLD = 100;    // Max acceptable value before we consider it noise


// Function prototypes
void rotateStepper(bool);
int getProximity(int, int);
int mapValue(float, float, float, float);

void setup() {
  Serial.begin(9600);
  
  // Set pin modes
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);

  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  
  for (int i = 0; i < 4; i++) {
    pinMode(STEPPER_PINS[i], OUTPUT);
  }

  // Setup variables
  stepNumber = 0;
  currentStep = 0;
  prevProximity_1 = getProximity(TRIG_PIN_1, ECHO_PIN_1);
  prevProximity_2 = getProximity(TRIG_PIN_2, ECHO_PIN_2);
}

void loop() {
  // Get proximity value from each ultrasonic sensor
  proximity_1 = getProximity(TRIG_PIN_1, ECHO_PIN_1);
  proximity_2 = getProximity(TRIG_PIN_2, ECHO_PIN_2);

//  Serial.print(proximity_1);
//  Serial.print(", ");
//  Serial.println(proximity_2);

  // If proximity values aren't too noisy, are close enough in value and haven't increased or decreased too significantly since the previous reading, then use them to rotate the stepper
  if (proximity_1 < NOISE_THRESHOLD && proximity_2 < NOISE_THRESHOLD && abs(proximity_1 - proximity_2) < MAX_PROXIMITY_DIFF && (abs(proximity_1 - prevProximity_1) > MIN_PROXIMITY_CHANGE || abs(proximity_2 - prevProximity_2) > MIN_PROXIMITY_CHANGE)) {
    int stepCount = 0;

    // If proximity has decreased, rotate stepper clockwise (tighten) unless it has reached its min limit
    if (proximity_1 < prevProximity_1 && proximity_2 < prevProximity_2) {
        while (stepCount < MIN_STEPS && currentStep < STEP_LIMIT) {
          rotateStepper(true);
          stepCount++;
          currentStep++;
          delay(3);
        }
    } 
    // Else if proximity has increased, rotate stepper counterclockwise (loosen) unless it has reached its max limit
    else if (proximity_1 > prevProximity_1 && proximity_2 > prevProximity_2) {
        while (stepCount < MIN_STEPS && currentStep > 0) {
          rotateStepper(false);
          stepCount++;
          currentStep--;
          delay(3);
        }
    } 
    prevProximity_1 = getProximity(TRIG_PIN_1, ECHO_PIN_1);
    prevProximity_2 = getProximity(TRIG_PIN_2, ECHO_PIN_2);
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
