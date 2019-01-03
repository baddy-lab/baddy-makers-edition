/*
        .__        ___       _______   _______  ____    ____ 
    |   _  \      /   \     |       \ |       \ \   \  /   / 
    |  |_)  |    /  ^  \    |  .--.  ||  .--.  | \   \/   /  
    |   _  <    /  /_\  \   |  |  |  ||  |  |  |  \_    _/   
    |  |_)  |  /  _____  \  |  '--'  ||  '--'  |    |  |     
    |______/  /__/     \__\ |_______/ |_______/     |__|  

    
    ###########################################################################
    BADDY_EMBEDDED_CODE_ARDUINO_V01.ino - Embedded source code of BADDY Arduino working with bluetooth
    Copyright (C) 2016  GRESLEBIN Benoit
    ###########################################################################

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    ######################################
    ADDITIONAL TERMS
    ######################################
    Specific non commercial use term: This source code may be copied, distributed, modified by any individual 
    and non profit organizations (Fablab, Hackerspaces, Makerspaces, universities, Schools), as long
    as it follows the GNU GPL license terms here attached.
    Corporate institutions and private companies may only copy and distribute the source code without modification.
    Therefore, they are not allowed to modify the source code prior to distributing it for commercial purpose.
    So for example, if a user embeds or modifies or relies on the source code in a product that is 
    then sold to a third party, this would be a violation of the Non-Commercial Use License 
    additional term, although this type of use would be permitted under the standard GNU GPL
    (assuming the other terms and conditions of the GNU GPL v.3 were followed). By adding this additional 
    restriction, it is intended to encourage the evolution of BADDY source code to be driven
    by the Hacker and makers' communities.

    ######################################
    COPYRIGHT NOTICE
    ######################################
    LedControl.h is a software library under MIT license, Copyright (c) 2007-2015 Eberhard Fahle

*/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include <Servo.h>
#include "stdlib.h"
#include "LedControl.h"

//Arduino PIN configurations
int SHUTTLE_RETAINER_PIN=10;
int SHUTTLE_SWITCH_PIN=11;
int MOTOR_LEFT_PIN=7;
int MOTOR_RIGHT_PIN=8;
int LED_DIN=6;
int LED_CS=5;
int LED_CLK=4;
int BLUETOOTH_RX_PIN =0;
int BLUETOOTH_TX_PIN=1;
int BLUETOOTH_STATE_PIN;

// Led management instances and objects

int refresh_period_square = 100;
int refresh_period_level = 40;
int refresh_period_smiley = 1000;

byte level8[8]=     {B11111111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000};
byte level7[8]=     {B11111111,B11111111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000};
byte level6[8]=     {B11111111,B11111111,B11111111,B00000000,B00000000,B00000000,B00000000,B00000000};
byte level5[8]=     {B11111111,B11111111,B11111111,B11111111,B00000000,B00000000,B00000000,B00000000};
byte level4[8]=     {B11111111,B11111111,B11111111,B11111111,B11111111,B00000000,B00000000,B00000000};
byte level3[8]=     {B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B00000000,B00000000};
byte level2[8]=     {B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B00000000};
byte level1[8]=     {B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111};

byte square1[8]=     {B00000000,B00000000,B00000000,B00011000,B00011000,B00000000,B00000000,B00000000};
byte square2[8]=     {B00000000,B00000000,B00111100,B00100100,B00100100,B00111100,B00000000,B00000000};
byte square3[8]=     {B00000000,B01111110,B01000010,B01000010,B01000010,B01000010,B01111110,B00000000};
byte square4[8]=     {B11111111,B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B11111111};

byte smile[8]=   {B00010000,B00100110,B01000110,B01000000,B01000000,B01000110,B00100110,B00010000};      
byte frown[8]=   {B01000000,B00100110,B00010110,B00010000,B00010000,B00010110,B00100110,B01000000};    

LedControl lc=LedControl(LED_DIN,LED_CLK,LED_CS,0);

// Servos instances

Servo ShuttleRetainer;
Servo ShuttleSwitch;
Servo MotorLeft;
Servo MotorRight;

SoftwareSerial BTserial(BLUETOOTH_RX_PIN, BLUETOOTH_TX_PIN); // RX | TX

int ShuttleCount;
int period_between_occurences;
int TOTAL_SHOTS=0;
char inData[24];
int BUFFER_SIZE=24;
char inChar = -1;
String SpeedRightBuffer="";
String SpeedLeftBuffer="";
int SpeedRight = 850;
int SpeedLeft = 850;
bool command_read= 0;
int index = 0;
int command_index = 0;
char mycommand[24];
bool busyFlag;
bool first_shot;
int BattValue;
float Voltage;

///////////////////////////////////////////////////////////////////////////////////
/////////////////////////////CONFIGURATION PARAMETERS//////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

//Warm up, break timer , stop speed, computation timer - modify only if you know what you are doing
int WARM_UP_SPEED=860;
int TRANSITION_SPEED=875;
int TRANSITION_SPEED_HIGH = 990;
int STOP = 750;
int WARM_UP_TIMER = 2000;
int COMPUTATION_TIMER = 600;
int BREAK_TIMER = 500;

// DROP SHOTS
int DROP_LEFT_SHOT_LEFT_MOTOR=910;//950;
int DROP_LEFT_SHOT_RIGHT_MOTOR=835;//860;
int DROP_CENTER_SHOT_LEFT_MOTOR=867;//880;
int DROP_CENTER_SHOT_RIGHT_MOTOR=885;//880;
int DROP_RIGHT_SHOT_LEFT_MOTOR=833;//860;
int DROP_RIGHT_SHOT_RIGHT_MOTOR=915;//950;
//DRIVE SHOTS
int DRIVE_LEFT_SHOT_LEFT_MOTOR=1035;
int DRIVE_LEFT_SHOT_RIGHT_MOTOR=909;
int DRIVE_CENTER_SHOT_LEFT_MOTOR=980;
int DRIVE_CENTER_SHOT_RIGHT_MOTOR=980;
int DRIVE_RIGHT_SHOT_LEFT_MOTOR=902;
int DRIVE_RIGHT_SHOT_RIGHT_MOTOR=1035;
//CLEAR SHOTS
int CLEAR_LEFT_SHOT_LEFT_MOTOR=1680;
int CLEAR_LEFT_SHOT_RIGHT_MOTOR=1200;
int CLEAR_CENTER_SHOT_LEFT_MOTOR=1600;
int CLEAR_CENTER_SHOT_RIGHT_MOTOR=1600;
int CLEAR_RIGHT_SHOT_LEFT_MOTOR=1200;
int CLEAR_RIGHT_SHOT_RIGHT_MOTOR=1680;

//Switch movement and timers
int SWITCH_LONG_POSITION = 60;
int SWITCH_SHORT_POSITION = 127;
int SWITCH_WAIT_POSITION = 90;
int SWITCH_TIMER = 220; // value in millisecond to wait Switch to fetch its long position
int SWITCH_SPEED=600; // Special speed for slow motion movement (configuration)

// Shuttle retainer movemement and timers
int RETAINER_UP_POSITION = 105;
int RETAINER_DOWN_POSITION=175;
int RETAINER_TIMER=120;

///////////////////////////////////////////////////////////////////////////////////
/////////////////////////////CONFIGURATION PARAMETERS END /////////////////////////
///////////////////////////////////////////////////////////////////////////////////


void setup() 
{
    Serial.begin(9600);
    Serial.println("BADDY Arduino is ready");
 
    // Bluetooth default serial speed for communcation mode is 9600
    BTserial.begin(9600);

    //Attach motor pins
    ShuttleSwitch.attach(SHUTTLE_SWITCH_PIN);
    ShuttleSwitch.write(SWITCH_SHORT_POSITION);
    ShuttleRetainer.attach(SHUTTLE_RETAINER_PIN);
    ShuttleRetainer.write(RETAINER_DOWN_POSITION);
    MotorRight.attach(MOTOR_RIGHT_PIN);
    MotorLeft.attach(MOTOR_LEFT_PIN);

    //This is very important if you don't want Baddy to spin at highest speed at start up...
    MotorRight.writeMicroseconds(STOP);
    MotorLeft.writeMicroseconds(STOP);
    // Wait
    delay(2000);

    // Led set up and ready for pairing
    lc.shutdown(0,false);       //The MAX72XX is in power-saving mode on startup
    lc.setIntensity(0,2);      // Set the brightness to average value
    lc.clearDisplay(0);         // and clear the display
    printLevels();
    lc.clearDisplay(0);         // and clear the display
    warm_up();
    // Speed off, waiting for command...
    MotorLeft.writeMicroseconds(STOP);
    MotorRight.writeMicroseconds(STOP);
    printSmiley();

    //Send battery level to App// - Bêta version as implemented for testing purpose only - works for "always on" BADDY PCB versions

      //Sending battery level info each time we got a command
        BattValue = analogRead(A0); //read the A0 pin value
        Voltage = BattValue * (100.00 / 1023.00) * 2.00; //convert the value to a percentage
        Serial.print("BAT-");
        Serial.println(Voltage);  
      // End of Battery level sending
  
    //End of sending battery level to App
    
    first_shot = 1;
}
 
void loop()
{
  
  while (BTserial.available()){
    
    char *mycommand = FetchBluetoothCommand();
    if (command_read!=1)
    {
      ReadShootSequence(mycommand);
    }
      
  }

}

char *FetchBluetoothCommand(){
 
  while((BTserial.available()) > 0) 
 {
     inChar = BTserial.read(); // Read one character
     if (inChar=='[') {
        index=0;
        command_read=1;
        //Serial.println("Starting new command fetch:");
     }
     if (command_read=1){
        inData[index] = inChar; // Store it
        index++;
     }
     if (inChar==']') {
        inData[index] = inChar; // write last character
        inData[index] = '\0'; // Null terminate the string
        command_read = 0;
        //Serial.println("End of new command fetch");
        //Serial.print("New command received:");
        //Serial.println(inData);
        printSquares();
        busyFlag = 1;


        //Send battery level to App// - Bêta version as implemented for testing purpose only - works for "always on" BADDY PCB versions
      
          //Updating and sending battery level info each time we got a command
        BattValue = analogRead(A0); //read the A0 pin value
        Voltage = BattValue * (100.00 / 1023.00) * 2.00; //convert the value to a percentage
        Serial.print("BAT-");
        Serial.println(Voltage);
  
        // End of Battery level sending
        
        return (inData);
     }
 }
 
}

bool ReadShootSequence(char *command){
      
      //Check if Abort command received first
      if (command[5]=='0'){
          //Serial.println("************************ABORT Received. Stopping motors...**********************");
          MotorLeft.writeMicroseconds(STOP);
          MotorRight.writeMicroseconds(STOP);
          if (busyFlag ==1){
            printFrown();
            busyFlag=0;
          }
          return 0;
      }

      else if (command[1]=='C'){
        
          //Config command received

          if (command[5]=='F'){

            fire_slomo();
            
          }
          else {

              command_index = 5; // fetching Left motor speed
              while (command[command_index]!='-'){
                SpeedLeftBuffer += command[command_index];
                command_index++;
              }
              SpeedLeft = SpeedLeftBuffer.toInt();
    
              command_index++; //Fetching right motor speed
              while (command[command_index]!=']'){
                SpeedRightBuffer+= command[command_index];
                command_index++;
              }
              SpeedRight = SpeedRightBuffer.toInt();
    
              MotorLeft.writeMicroseconds(SpeedLeft);
              MotorRight.writeMicroseconds(SpeedRight);
              SpeedLeftBuffer="";
              SpeedRightBuffer="";
          }
        
      }
      else
      {
          if (command[1]=='S'){
              //Serial.println("************************NEW SHOOTING SEQUENCE************************************");
              //Serial.println("Warming up...");
              warm_up();
              first_shot = 1;
              command_index=5; //initialize
          }

          int type;
          int next_type;
          int occur;
          int freq;
          int stroke_type_count = 0; // for random loop only
          int loop_flag = command[22]-'0';
          //Serial.print("Loop flag value:");
          //Serial.println(loop_flag);
          
          ////////////////////////////////////// IF LOOPS ///////////////////////////////////////
          while(loop_flag==1){
    
              //Serial.println("Starting loop");
          
              if(command[command_index]!='0'){
                  type = command[command_index] - '0';
                  //Serial.print("Shot type is:");
                  //Serial.print(type);
                  occur = command[command_index +1] - '0';
                  //Serial.print(" , Occurence is:");
                  //Serial.print(occur);
                  freq = command[command_index +2] - '0';

                  // Adapt periods
                  // Following freq values are not actual frequencies but rather ids that refer to a frequency 
                  // The aim of the following is to assign a frequency to a paticular frequency id
                  
                  if (freq == 0)
                    {
                      //Minimum values
                      freq = 0; // time needed by the retainer to move to short position
                    }
                  if (freq == 1)
                    {
                      //Every 1 sec
                      freq = 1000; 
                    }
                  if (freq == 2)
                    {
                      // Every 1.5 sec
                      freq=1500;
                    }
                  if (freq == 3)
                    {
                      // Every 2 sec
                      freq = 2000;
                    }
                  if (freq == 4)
                    {
                      // Every 3 sec
                      freq = 3000;
                    }
                  if (freq == 5)
                    {
                      // Every 4 sec
                      freq = 4000;
                    }
                  //End of adapt periods

                  //Serial.print(" , Frequency is:");
                  //Serial.print(freq);
                  next_type = command[command_index +3] - '0';

                  //That's to get the next stroke in case of end of loop, to adapt motor's transition
                  if (next_type==0)
                  {
                    next_type = command[5] - '0';
                  }
                  //Serial.print(" , Next shot type will be:");
                  //Serial.println(next_type);

                  if (!(shot(type, next_type, occur, freq)))
                  {
                    first_shot = 1;
                    loop_flag=0;
                    return 0;
                    break;
                  }

                  // Next shot
                  command_index = command_index+3;
                  
                  if (command[command_index]=='0'){
                    command_index=5; //initialize to begining of seq because we have reached end of loop sequence
                  }
                  
              }
          }

          ////////////////////////////////////// IF RANDOM LOOP ///////////////////////////////////////
          
          while(loop_flag==2){

              if (first_shot){
                freq = command[command_index +2] - '0'; 

                                  // Adapt periods
                  if (freq == 0)
                    {
                      //Minimum values
                      freq = 0; // time needed by the retainer to move to short position
                    }
                  if (freq == 1)
                    {
                      //Every 1 sec
                      freq = 1000; 
                    }
                  if (freq == 2)
                    {
                      // Every 1.5 sec
                      freq=1500;
                    }
                  if (freq == 3)
                    {
                      // Every 2 sec
                      freq = 2000;
                    }
                  if (freq == 4)
                    {
                      // Every 3 sec
                      freq = 3000;
                    }
                  if (freq == 5)
                    {
                      // Every 4 sec
                      freq = 4000;
                    }
                                    //End of adapt periods
                //Serial.println("Starting random loop");
                // We collect the different stroke types in the request
                while (command[command_index]!='0'){
                  stroke_type_count++;
                  command_index = command_index + 3;
                }
                //Serial.print("Total type of stokes: ");
                //Serial.println(stroke_type_count);
                
                type = command[5+random(0,stroke_type_count)*3] - '0';
                //Serial.print("Selected stroke Type: ");
                //Serial.println(type);
                
                next_type = command[5+random(0,stroke_type_count)*3] - '0';
                //Serial.print("Selected next stroke Type: ");
                //Serial.println(next_type);
                first_shot=0;
              }
              else {
                type = next_type;
                Serial.print("Selected stroke Type: ");
                Serial.println(type);
                next_type = command[5+random(0,stroke_type_count)*3] - '0';
                Serial.print("Selected next stroke Type: ");
                Serial.println(next_type);
                
              }

              if (!(shot(type, next_type, 1, freq)))
              {
                loop_flag=0;
                first_shot = 1;
                stroke_type_count = 0;
                return 0;
                break;
              }
  
          }
          

          ////////////////////////////////////// IF NO LOOPS NOR RANDOM LOOP///////////////////////////////////////
          //Serial.print("No loop detected");
          
          //command_index=5; //initialize
      
          while(command[command_index]!='0'){
            
              type = command[command_index] - '0';
              //Serial.print("Shot type is:");
              //Serial.print(type);
              occur = command[command_index +1] - '0';
              //Serial.print(" , Occurence is:");
              //Serial.print(occur);
              freq = command[command_index +2] - '0'; 

             // Adapt periods
                  if (freq == 0)
                    {
                      //Minimum values
                      freq = 0; // time needed by the retainer to move to short position
                    }
                  if (freq == 1)
                    {
                      //Every 1 sec
                      freq = 1000; 
                    }
                  if (freq == 2)
                    {
                      // Every 1.5 sec
                      freq=1500;
                    }
                  if (freq == 3)
                    {
                      // Every 2 sec
                      freq = 2000;
                    }
                  if (freq == 4)
                    {
                      // Every 3 sec
                      freq = 3000;
                    }
                  if (freq == 5)
                    {
                      // Every 4 sec
                      freq = 4000;
                    }
              //End of adapt periods
              //Serial.print(" , Frequency is:");
              //Serial.print(freq);
              next_type = command[command_index+3] - '0';
              //Serial.print(" , Next shot type will be:");
              //Serial.println(next_type);
              
              if (!(shot(type, next_type, occur, freq)))
              {
                first_shot = 1;
                return 0;
                break;
              }

              // Next shot
              command_index = command_index+3;
              
          }
          //Serial.println("************************END OF SHOOTING SEQUENCE**********************");
          MotorLeft.writeMicroseconds(STOP);
          MotorRight.writeMicroseconds(STOP);
          
         // Secure this trick
         command[5]='0'; 
         // End of the trick
         
         // Sending info to Android that sequence is finished
         Serial.print("SEQ-");
         Serial.println("FINISHED");
         // Sending info end
            
         printSmiley();
         busyFlag=0;
         first_shot = 1;
         return 1;
      }
}

 
bool shot(int shot_type, int next_type, int occurence, int period) // That's the fire procedure
{
  //Serial.println("SHOOTING sequence start!!!!!!!");
  //Serial.print("shot_type:");
  //Serial.print(shot_type);
  //Serial.print(", occurence:");
  //Serial.print(occurence);
  //Serial.print(", next type");
  //Serial.print(next_type);
  //Serial.print(", period:");
  //Serial.println(period);
  
  ShuttleCount=0;
  
if (period == 0)
  {
    //Minimum values
    period_between_occurences = 0; // time needed by the retainer to move to short position
  }
else
  {
    period_between_occurences = period - (RETAINER_TIMER+SWITCH_TIMER); 
  }

  
  //Serial.print(", period between occurences in millisec:");
  //Serial.println(period_between_occurences);
  
  if (shot_type == 1){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(DROP_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_LEFT_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
        
      
      while (ShuttleCount < occurence) {

      //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            //Serial.println("SHOOTING sequence aborted");
            return 0;
        }

        fire();

        //Serial.println("SHOOT!");
          
        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      //Serial.println("SHOOTING sequence sucessful!");

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }
      return 1;
  }
  
  if (shot_type == 2){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(DROP_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_CENTER_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }

      while (ShuttleCount < occurence) {

      //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }

        fire(); // let's shoot!
        
        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      Serial.println("SHOOTING sequence successful");

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }
      
      return 1;
  }

  if (shot_type == 3){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
      
      while (ShuttleCount < occurence) {
        
        //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }

        fire();

        //Serial.println("SHOOT!");
        

        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }
      return 1;
  }

    if (shot_type == 4){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
      
      while (ShuttleCount < occurence) {

        //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }

          fire();
        //  Serial.println("SHOOT!");
        
        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }

      return 1;
  }

    if (shot_type == 5){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
      
      while (ShuttleCount < occurence) {

        //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }

        fire();

        //Serial.println("SHOOT!");
        
        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }
      
      return 1;
  }

    if (shot_type == 6){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
      
      while (ShuttleCount < occurence) {
        //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }
        
        fire();

        //Serial.println("SHOOT!");
        
        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }

      return 1;
  }

      if (shot_type == 7){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
      
      while (ShuttleCount < occurence) {

        //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }

        fire();
        //Serial.println("SHOOT!");   

        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }
     
      return 1;
  }

      if (shot_type == 8){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(CLEAR_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_CENTER_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
      
      while (ShuttleCount < occurence) {

        //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }

        fire();

        //Serial.println("SHOOT!");
        
        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }
   
      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }
      

      return 1;
  }

  if (shot_type == 9){
        //Sets the accurate speed on motors
        MotorLeft.writeMicroseconds(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);
        if (first_shot == 1){
          delay(500);
          first_shot=0;
        }
      
      while (ShuttleCount < occurence) {

        //Manage emergency stop with catching abort command from bluetooth
        if(BTserial.available())  
        {
            Serial.println("SHOOTING sequence aborted");
            return 0;
        }

        fire();
        //Serial.println("SHOOT!");   

        if (ShuttleCount+1 < occurence){
           delay(period_between_occurences);
        }
        
        ShuttleCount = ShuttleCount+1;
        TOTAL_SHOTS = TOTAL_SHOTS +1;
        
      }

      if (motor_speed_transition(shot_type,next_type) >0){
        if (period>1000){
            delay(period-(BREAK_TIMER+COMPUTATION_TIMER));
        }
      }
      else{
        if ((period>1000)||(period==1000)){
            delay(period-COMPUTATION_TIMER);
        }
      }
      
      return 1;
  }
  Serial.println("Wrong format in shooting sequence");
  return 0;
}

//Functions for LED forms display

void printByte(byte character [])
{
  int i = 0;
  for(i=0;i<8;i++)
  {
    lc.setRow(0,i,character[i]);
  }
}

void printLevels()
{
    printByte(level8);
    delay(refresh_period_level);
    printByte(level7);
    delay(refresh_period_level);
    printByte(level6);
    delay(refresh_period_level);
    printByte(level5);
    delay(refresh_period_level);
    printByte(level4);
    delay(refresh_period_level);
    printByte(level3);
    delay(refresh_period_level);
    printByte(level2);
    delay(refresh_period_level);
    printByte(level1);
    delay(refresh_period_level);
    printByte(level2);
    delay(refresh_period_level);
    printByte(level3);
    delay(refresh_period_level);
    printByte(level4);
    delay(refresh_period_level);
    printByte(level5);
    delay(refresh_period_level);
    printByte(level6);
    delay(refresh_period_level);
    printByte(level7);
    delay(refresh_period_level);
    printByte(level8);
    delay(refresh_period_level);
    lc.clearDisplay(0);         // clear the display
}

void printSquares(){

    printByte(square1);
    delay(refresh_period_square);
    printByte(square2);
    delay(refresh_period_square);
    printByte(square3);
    delay(refresh_period_square);
    printByte(square4);
    delay(refresh_period_square);
    printByte(square3);
    delay(refresh_period_square);
    printByte(square2);
    delay(refresh_period_square);
    printByte(square1);
    delay(refresh_period_square);
    lc.clearDisplay(0);         // clear the display
    
}

void printSmiley(){
  
    printByte(smile);
    delay(refresh_period_smiley);
    lc.clearDisplay(0);         // clear the display
  
}

void printFrown(){
  
    printByte(frown);
    delay(refresh_period_smiley);
    lc.clearDisplay(0);         // and clear the display
  
}

void fire(){

  ShuttleRetainer.write(RETAINER_UP_POSITION);
  delay(RETAINER_TIMER);

 ShuttleSwitch.write(SWITCH_LONG_POSITION);
  delay(SWITCH_TIMER);

  ShuttleSwitch.write(SWITCH_SHORT_POSITION);
  delay(SWITCH_TIMER);
  ShuttleRetainer.write(RETAINER_DOWN_POSITION);
  
  delay(RETAINER_TIMER); // Just in case

}

// For testing purpose, fire procedure in slow motion
void fire_slomo(){
  ShuttleRetainer.write(RETAINER_UP_POSITION);
  delay(RETAINER_TIMER);

 for (int i=0;i<20;i++)
  {
    delay(SWITCH_SPEED/20);
    ShuttleSwitch.write(SWITCH_SHORT_POSITION+i*(SWITCH_LONG_POSITION - SWITCH_SHORT_POSITION)/20);
  }
  //delay(SWITCH_TIMER);
  delay(2000);

  ShuttleSwitch.write(SWITCH_SHORT_POSITION);
  delay(SWITCH_TIMER);
  ShuttleRetainer.write(RETAINER_DOWN_POSITION);
  delay(RETAINER_TIMER); // No need for that, timer already in switch timer

}

void warm_up(){

  MotorLeft.writeMicroseconds(WARM_UP_SPEED);
  MotorRight.writeMicroseconds(WARM_UP_SPEED);
  delay(WARM_UP_TIMER);
}

void break_motor_right(int outputspeed){

  //Break
  Serial.println("Break Right Motor");
  MotorRight.writeMicroseconds(STOP);
  delay(210);
  //Restart at idle low speed
  //Serial.println("Acceleration");
  MotorRight.writeMicroseconds(1070);
  delay(270);
  //Serial.println("Normal cruise");
  MotorRight.writeMicroseconds(outputspeed);
}

void break_motor_left(int outputspeed){
  //Break
  Serial.println("Break Left Motor");
  MotorLeft.writeMicroseconds(STOP);
  delay(210);
  //Restart at idle low speed
  //Serial.println("Acceleration");
  MotorLeft.writeMicroseconds(1070);
  delay(270);
  //Serial.println("Normal cruise");
  MotorLeft.writeMicroseconds(outputspeed);
}

void break_motor_all(int outputspeed){

  Serial.println("Break all Motors");
  MotorLeft.writeMicroseconds(STOP);
  MotorRight.writeMicroseconds(STOP);
  delay(210);
  //Restart at idle low speed
  //Serial.println("Acceleration");
  MotorLeft.writeMicroseconds(1070);
  MotorRight.writeMicroseconds(1070);
  delay(270);
  //Serial.println("Normal cruise all motors");
  MotorLeft.writeMicroseconds(outputspeed);
  MotorRight.writeMicroseconds(outputspeed);
}

int motor_speed_transition(int type, int next_type){
  // Manage motor speed transitions, to avoid getting too much speed when switching from clear shot to drop
  // Manage transitions from drop shot to clear shot!
  // Left and right transitions as well

  // That's BADDY's trickiest part - shapped with experience and iterative approach. Propose your own parameters!
  
  if (type==next_type){
    return 0;
  }
  
  if ( ((type==1)||(type==2))&&((next_type == 3)) ){
      // Preferable to break in case if Drop Left and right sequence, to ensure excentricity
      break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
      //MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR);
      MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR);
      return 480;
  }

  if ( ((type == 2)||(type == 3))&&((next_type == 1)) ){
      MotorLeft.writeMicroseconds(DROP_LEFT_SHOT_LEFT_MOTOR);
      break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);
      //MotorRight.writeMicroseconds(DROP_LEFT_SHOT_RIGHT_MOTOR);
      return 480;
  }
    // New, manage transitions from drop to clear shot
 
  if ( ((type == 1)||(type == 2)||(type == 3))&&((next_type == 4)) ){
      MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
      MotorRight.writeMicroseconds(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
      return 0;
  }

  if ( ((type == 1)||(type == 2)||(type == 3))&&((next_type == 5)) ){
      MotorLeft.writeMicroseconds(DRIVE_CENTER_SHOT_LEFT_MOTOR);
      MotorRight.writeMicroseconds(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
      return 0;
  }

  if ( ((type == 1)||(type == 2)||(type == 3))&&((next_type == 6)) ){
      MotorLeft.writeMicroseconds(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
      MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
      return 0;
  }
  
  if ( ((type == 1)||(type == 2)||(type == 3))&&((next_type == 7)) ){
      MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
      MotorRight.writeMicroseconds(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
      return 0;
  }

  if ( ((type == 1)||(type == 2)||(type == 3))&&((next_type == 8)) ){
      MotorLeft.writeMicroseconds(CLEAR_CENTER_SHOT_LEFT_MOTOR);
      MotorRight.writeMicroseconds(CLEAR_CENTER_SHOT_RIGHT_MOTOR);
      return 0;
  }

  if ( ((type == 1)||(type == 2)||(type == 3))&&((next_type == 9)) ){
      MotorLeft.writeMicroseconds(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
      MotorRight.writeMicroseconds(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);
      return 0;
  }
    // End manage transitions from drop to clear
  
  if (type>3)
  {
    if (next_type == 1)
    {
       if ((type==4)&&(type==5))
       {
        MotorLeft.writeMicroseconds(TRANSITION_SPEED);
        break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);
        return (480); // we deduce by the time needed to break
       }
       if (type==6)
       {
        MotorLeft.writeMicroseconds(DROP_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==7)
       {
        break_motor_left(DROP_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
      
       }
       if (type==8)
       {
        break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);
        break_motor_left(DROP_LEFT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==9);
       {
        MotorLeft.writeMicroseconds(DROP_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
    }
    else if (next_type == 2)
    {
       if (type==4)
       {
        break_motor_left(DROP_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==5)
       {
        break_motor_all(DROP_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==6)
       {
        MotorLeft.writeMicroseconds(DROP_CENTER_SHOT_LEFT_MOTOR);
        break_motor_right(DROP_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==7)
       {
        break_motor_left(DROP_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==8)
       {
        break_motor_all(DROP_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==9);
       {
        MotorLeft.writeMicroseconds(DROP_CENTER_SHOT_LEFT_MOTOR);
        break_motor_right(DROP_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
    }
    else if (next_type == 3)
    {
       if (type==4)
       {
        break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR+25);
        return(480); // we deduce by the time needed to break
       }
       if (type==5)
       {
        MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR+25);
        break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==6)
       {
        MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR+25);
        break_motor_right(DROP_RIGHT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==7)
       {
        break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR+25);
        return(480); // we deduce by the time needed to break
       }
       if (type==8)
       {
        break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR+25);
        break_motor_right(DROP_RIGHT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       if (type==9);
       {
        MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR);
        break_motor_right(DROP_RIGHT_SHOT_RIGHT_MOTOR+25);
        return(480); // we deduce by the time needed to break
       }
    }
    else if (next_type == 4)
    {

       if (type==5)
       {
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==6)
       {
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==7)
       {
        MotorRight.writeMicroseconds(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==8)
       {
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==9)
       {
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
      else
      {
         MotorRight.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         MotorLeft.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         return(0);
      }
      
    }
    else if (next_type == 5)
    {
       if (type == 6)
       {
        MotorLeft.writeMicroseconds(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        break_motor_right(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==7)
       {
        MotorRight.writeMicroseconds(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
        break_motor_left(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==8)
       {
        break_motor_all(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==9)
       {
        MotorLeft.writeMicroseconds(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        break_motor_right(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
      else
      {
         MotorRight.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         MotorLeft.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         return(0);
      }
    }
    else if (next_type == 6)
    {
       if (type == 4)
       {
        MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        break_motor_left(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type == 5)
       {
        MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        break_motor_left(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==7)
       {
        MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        break_motor_left(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==8)
       {
        break_motor_right(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        break_motor_left(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
       else if (type==9)
       {
        MotorLeft.writeMicroseconds(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        break_motor_right(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
      else
      {
         MotorRight.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         MotorLeft.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         return (0);
      }
    }
    else if (next_type == 7)
    {
      
      // Manage transitions for drop to clear shots

      if ((type==4)||(type==5)||(type==6))
       {
        MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
        return(0); // we deduce by the time needed to break
       }

      // end of manage transitions from drop to clear shots
      
      if (type==9)
      {
        MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
      else if (type==8)
      {
        MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
        break_motor_right(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
        return(480); // we deduce by the time needed to break
       }

      else
      {
         MotorRight.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         MotorLeft.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         return(0);
      }
    }

    else if (next_type == 8)
    {
      
      // Manage transitions for drop to clear shots

      if ((type==4)||(type==5)||(type==6)||(type==7)||(type==9))
       {
        MotorLeft.writeMicroseconds(CLEAR_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_CENTER_SHOT_RIGHT_MOTOR);
        return(0); // we deduce by the time needed to break
       }

      else
      {
         MotorRight.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         MotorLeft.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         return(0);
      }
    }
    
    else if (next_type == 9)
    {
      
      // Manage transitions for drop to clear shots

      if ((type==4)||(type==5)||(type==6))
       {
        MotorLeft.writeMicroseconds(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
        return(0); // we deduce by the time needed to break
       }

      // end of manage transitions from drop to clear shots
      
      if (type==7)
      {
        MotorRight.writeMicroseconds(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);
        break_motor_left(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }
      else if (type==8)
      {
        MotorRight.writeMicroseconds(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);
        break_motor_left(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
        return(480); // we deduce by the time needed to break
       }

      else
      {
         MotorRight.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         MotorLeft.writeMicroseconds(TRANSITION_SPEED); // Restart motor
         return(0);
      }
    }

  }
  else
  {
    MotorLeft.writeMicroseconds(TRANSITION_SPEED);
    MotorRight.writeMicroseconds(TRANSITION_SPEED);
    return(0);
  }
}
