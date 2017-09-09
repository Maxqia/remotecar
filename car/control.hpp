/* control loop for ln298 with rotary encoders 
 * to make the wheels run at the same speed 
 */

//#include <wiring.h> // includes PI?
#include "Arduino.h"
#include <Ticker.h>

#define wheelDiaCM 6.0
#define HolesInDisc 20.0
const int D[] = {16,5,4,0,2,14,12,13,15}; // for Wemos Mini

struct Motor {
  // pins
  int fwd;
  int bkwd;
  int enable;
  int sensor;

  // optointerupter count
  int intcount;

  // current PWM w/o balancing
  int curPWM;
};

Motor motors[2]; //0 left, 1 right

#define motorInt(x) motors[x].intcount++;
void leftInt() {
  motorInt(0);
}

void rightInt() {
  motorInt(1);
}

void setMotorPWM(Motor motor) {
  int speed = motor.curPWM;
  /*if (speed > 0) {
    digitalWrite(motor.fwd, HIGH);
    digitalWrite(motor.bkwd, LOW);
  } else if (speed < 0) {
    digitalWrite(motor.fwd, LOW);
    digitalWrite(motor.bkwd, HIGH);
  } else { // it's off Jim!
    digitalWrite(motor.fwd, LOW);
    digitalWrite(motor.bkwd, LOW);
  }*/
  analogWrite(motor.enable, 0); // turn off PWM to avoid any quick short circuits
  digitalWrite(motor.fwd, speed > 0 ? HIGH : LOW);
  digitalWrite(motor.bkwd, speed < 0 ? HIGH : LOW);
  analogWrite(motor.enable, speed);
}

// I'm pretty sure there's no delays cuz it's probably a interupt
void controlLoop() {
  int intcounts[2];

  // copy and set to zero quickly to avoid a race condition
  for(int i = 0; i <= 1; i++) {
    intcounts[i] = motors[i].intcount;
    motors[i].intcount = 0;
    setMotorPWM(motors[i]);
  }

  for(int i = 0; i <= 1; i++) {
    Serial.print("Motor ");
    Serial.print(i + 1);
    Serial.println(":");
    //int rotations = intcounts[i] / HolesInDisc;
    Serial.print("  Notches: ");
    Serial.println(intcounts[i]);
    float mps = ((intcounts[i] / HolesInDisc) * PI * wheelDiaCM);
    Serial.print("  CentiMeters Per Seconds: ");
    Serial.println(mps);
  }

}

Ticker control;
void setupControl() {
  // pin setup
  motors[0].fwd = D[4];
  motors[0].bkwd = D[3];
  
  motors[1].fwd = D[1];
  motors[1].bkwd = D[2];

  motors[0].enable = D[0];
  motors[1].enable = D[5];

  motors[0].sensor = D[6];
  motors[1].sensor = D[7];

  for(int i = 0; i <= 1; i++) {
    pinMode(motors[i].fwd, OUTPUT);
    pinMode(motors[i].bkwd, OUTPUT);
    pinMode(motors[i].enable, OUTPUT);
    pinMode(motors[i].sensor, INPUT);
    motors[i].intcount = 0;
    motors[i].curPWM = 0;
    //motors[i].curPWM = 1023; // maximum for testing
    //setMotorPWM(motors[i]);
  }
  
  /*attachInterrupt(digitalPinToInterrupt(motors[0].sensor), leftInt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(motors[1].sensor), rightInt, CHANGE);*/
  attachInterrupt(digitalPinToInterrupt(motors[0].sensor), leftInt, RISING);
  attachInterrupt(digitalPinToInterrupt(motors[1].sensor), rightInt, RISING);
  //control.attach(0.05, controlLoop); // run every 50ms
  control.attach(0.1,controlLoop); //run every 100ms
  //control.attach(1, controlLoop); // run every second (debug)

}

