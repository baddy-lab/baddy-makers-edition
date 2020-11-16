#include <Arduino.h>
#include <Servo.h>

#define MAX_SIGNAL 2000
#define MIN_SIGNAL 700
#define MOTOR_LEFT_PIN D7
#define MOTOR_RIGHT_PIN D5

Servo motorleft;
Servo motorright;
int SIGNAL;

void setup() {
  Serial.begin(9600);
  Serial.println("Program begin...");
  Serial.println("This program will calibrate the ESC max and min values.");

  motorleft.attach(MOTOR_LEFT_PIN);
  motorright.attach(MOTOR_RIGHT_PIN);

  Serial.println("Now writing maximum output.");
  motorleft.writeMicroseconds(MAX_SIGNAL);
  motorright.writeMicroseconds(MAX_SIGNAL);

  Serial.println("");
  Serial.println("");
  Serial.println("Please make sure BADDY is switched off, and get ready to switch on when told");
  Serial.println("");
  Serial.println("If BADDY is switched off, Let's get ready...");
  delay(3000);

  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.print("Switch BADDY on in 4 ");
  delay(1000);
  Serial.print("3 ");
  delay(1000);
  Serial.print("2 ");
  delay(1000);
  Serial.print("1 ");
  delay(1000);
  Serial.println("Switch BADDY on NOW!!");
  delay(5000);

  // Send min output
  Serial.println("Sending minimum output");
  motorleft.writeMicroseconds(MIN_SIGNAL);
  motorright.writeMicroseconds(MIN_SIGNAL);
  Serial.println("");
  Serial.println("");
  Serial.println("");

  Serial.println("you shall hear 4 beeps with longer beeps on the last two beeps...");

  delay(5000);


  Serial.println("Exit program, if no beeps happened, please redo the procedure ");

}

void loop() {


}
