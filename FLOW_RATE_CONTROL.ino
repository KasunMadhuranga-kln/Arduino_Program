#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int stepPin = 3;
const int dirPin = 2;

const int buttonUp = 4;
const int buttonDown = 5;
const int buttonStart = 6;
const int limitSwitch = 7;

const int buttonFoward = 8;
const int buttonBackward = 9;

float flowRate = 5.0;
float minFlow = 0.5;
float maxFlow = 10.0;
float stepFlow = 0.5;

float k = 732.87;
unsigned long pulseWidth;

bool running = false;
unsigned long lastStepTime = 0;
bool stepState = false;

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(buttonStart, INPUT_PULLUP);
  pinMode(limitSwitch, INPUT_PULLUP);

  pinMode(buttonFoward, INPUT_PULLUP);
  pinMode(buttonBackward, INPUT_PULLUP);

  pulseWidth = k / (flowRate * 2);

  lcd.init();
  lcd.backlight();
  lcd.clear();
}

// ---------------------------------------------------------------------
//                 INSTANT SAFETY CHECK
// ---------------------------------------------------------------------
bool limitHit() {
  return digitalRead(limitSwitch) == LOW;
}

void loop() {

  // Global emergency stop
  if (limitHit()) {
    running = false;
    lcd.clear();
    lcd.print("LIMIT HIT!");
    delay(300);
    return;
  }

  // Manual forward (jog)
  if (digitalRead(buttonFoward) == LOW) {
    digitalWrite(dirPin, LOW);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(50);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(50);
    return;
  }

  // Manual backward (jog)
  if (digitalRead(buttonBackward) == LOW) {
    digitalWrite(dirPin, HIGH);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(50);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(50);
    return;
  }

  // Not running → adjust flowrate
  if (!running) {

    if (digitalRead(buttonUp) == LOW) {
      flowRate += stepFlow;
      if (flowRate > maxFlow) flowRate = maxFlow;
      pulseWidth = k / (flowRate * 2);
      delay(150);
    }

    if (digitalRead(buttonDown) == LOW) {
      flowRate -= stepFlow;
      if (flowRate < minFlow) flowRate = minFlow;
      pulseWidth = k / (flowRate * 2);
      delay(150);
    }

    // Display
    lcd.setCursor(0, 0);
    lcd.print("Flow:");
    lcd.print(flowRate, 2);
    lcd.print(" ml/h  ");

    lcd.setCursor(0, 1);
    lcd.print("PW:");
    lcd.print(pulseWidth);
    lcd.print("ms    ");

    if (digitalRead(buttonStart) == LOW) {
      running = true;
      lcd.clear();
      lcd.print("Running...");
      delay(300);
    }

    return;
  }

  // ---------------------------------------------------------------------
  //                    NON-BLOCKING STEPPER CONTROL  
  //                 (CHECKS LIMIT SWITCH INSTANTLY)
  // ---------------------------------------------------------------------

  unsigned long now = millis();

  // Create the step pulse using millis() instead of delay()
  if (now - lastStepTime >= pulseWidth) {
    lastStepTime = now;

    stepState = !stepState;  // toggle HIGH/LOW
    digitalWrite(stepPin, stepState);

    digitalWrite(dirPin, LOW);  // motor direction
  }

  // Limit switch still checked every loop
  if (limitHit()) {
    running = false;
    lcd.clear();
    lcd.print("Stopped by LS");
    delay(500);
  }
}
