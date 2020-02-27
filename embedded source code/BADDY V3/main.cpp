/*
        .__        ___       _______   _______  ____    ____
    |   _  \      /   \     |       \ |       \ \   \  /   /
    |  |_)  |    /  ^  \    |  .--.  ||  .--.  | \   \/   /
    |   _  <    /  /_\  \   |  |  |  ||  |  |  |  \_    _/
    |  |_)  |  /  _____  \  |  '--'  ||  '--'  |    |  |
    |______/  /__/     \__\ |_______/ |_______/     |__|


    ###########################################################################
    BADDY_EMBEDDED_CODE_ESP_V01.ino - Embedded source code of BADDY ESP (wifi version)
    Copyright (C) 2017  GRESLEBIN Benoit
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
    ArduinoJson.h is a software library, Copyright (c) Benoit Blanchon
    ESPAsyncWebServer.h and ESPAsyncTCP.h are software libraries, Copyright (c) by Hristo Gochkov

    ######################################
    HISTORY
    ######################################
    May 2018 - ESP based code (wifi version), using aRest library, but not a final choice
    July 2018 - BUDDY feature implemented (connect 2 BADDY), get rid of aRest library, implemented simple webserver instead
    Sep 2018 - Async implementation to make sure server interrupts are taken into account without delay by BADDY
    Dec 2018 - Battery level management, taking into account latest battery level management
    Jan 2019 - Change acceleration settings to prevent triggering battery protect feature. Update transition_speed() method between drop shots
    June 2019 - Firmware v2.0.1 / hot spot connection, compatibility with GO BADDY app V3
    July 2019 - Firmware v2.0.5 / updates on BUDDY mode control
    Jan 2020 - optimization and fixes for BUDDY synchro issues. Activation of All you can eat feature for BUDDY mode
*/

#include <Arduino.h>
#include <LedControl.h>
#include <ESP8266WiFi.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <ESP8266HTTPClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <mDNSResolver.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ESP Pin configurations

#define LED_DIN D2
#define LED_CS D1
#define LED_CLK D3
#define MOTOR_RIGHT_PIN D5
#define MOTOR_LEFT_PIN D7
#define SHUTTLE_SWITCH_PIN D6
#define SHUTTLE_RETAINER_PIN D8
//In case you want to use the EXT pin header for your Retainer servo motor, comment the line above and uncomment the line just below
// #define SHUTTLE_RETAINER_PIN D0
#define ANALOG_READ_BATTERY A0

// ESP Pin configurations end

String baddy_firmware_version = "2.0.7";

String connect_mode = "";
bool flag_new_sequence = false;
bool station_connected = false;
bool buddy_around = false;
String JsonSequence;

//Wifi related static values for BADDY as a server
const char *BADDY_ID = "BADDY-V2"; // Usually CCDDD where CC is your country and DDD is a 3 digit number between 000 and 999
String ssid; // Keep "BADDY-" and add your BADDY ID at the end
const char *ssid_init = "BADDY";
const char *password_init = "";
String password;

using namespace mDNSResolver;
WiFiUDP udp;
Resolver resolver(udp);
IPAddress BaddyMasterIP (0, 0, 0, 0); // This is BADDY master's address when BUDDY mode
String buddy_ip_address;

// Those are the addresses when BADDY in AP/Wifi access point mode (happens when BADDY can't connect on Hot spot/Client mode)
IPAddress baddy_ip(192,168,1,2);
IPAddress baddy_gateway(192,168,1,1);
IPAddress baddy_subnet(255,255,255,0);

// BADDY BUDDY server related parameters
const uint16_t port = 80;
const char * host = "192.168.4.2"; // ip or dns
HTTPClient buddy;
WiFiClient client_buddy;

// Wifi event handler functions

WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;


// Wifi event handler functions end

// Wifi related static values end

// Create an instance of the server
AsyncWebServer server(80);

/////////////////////////////JSON DOCUMENT FORMAT//////////////////////////////////
/*
{
    "Type": "SEQ",
    "Strokes": [ 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 11, 12, 13, 14, 15, 16, 17, 18, 19, 2 ],
    "Speeds": [ 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9 , 1],
    "LoopMode": 0
}*/
    // For testing purpose
   // char json[] = "{\"Type\":\"SEQ\",\"Strokes\": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 ],\"Speeds\": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9 , 10],\"LoopMode\": 0}";
    // Computed with Arduino Json assistant

//Json Object values:
String type_seq;
int stroke_seq[20];
int stroke_seq_buddy[20];
int speed_seq[20];
int speed_seq_buddy[20];
int loop_mode_seq;
int PlayMode = 0;
int i; //integer that parses the sequence int Array. We make a global value to manage it in the main loop()
int speed;
int flag_first_shot=1;
int random_id; // used in random loop
int random_id_next; //used in random_loop
int stroke_count=0; //used in random loop
String json_config; // Needed to expose variable to Rest API
String json_status;
String json_playmode_dump;
bool flag_test_shot = 0;
bool all_you_can_eat_flag = 0;

bool running_status = false; // boolean flag that indicates if BADDY running or stopped (motors don't spin), initialized with 0
int battery_level;     // Battery level management

/////////////////////////////JSON DOCUMENT FORMAT END//////////////////////////////

/////////////////////////////Statistics counter and functions//////////////////////////////

int COUNTER_DROP_LEFT = 0;
int COUNTER_DROP_CENTER = 0;
int COUNTER_DROP_RIGHT = 0;
int COUNTER_DRIVE_LEFT = 0;
int COUNTER_DRIVE_CENTER = 0;
int COUNTER_DRIVE_RIGHT = 0;
int COUNTER_CLEAR_LEFT = 0;
int COUNTER_CLEAR_CENTER = 0;
int COUNTER_CLEAR_RIGHT = 0;

void reset_stats_counter()
{
    Serial.println("Reset stats counter... ");
    COUNTER_DROP_LEFT = 0;
    COUNTER_DROP_CENTER = 0;
    COUNTER_DROP_RIGHT = 0;
    COUNTER_DRIVE_LEFT = 0;
    COUNTER_DRIVE_CENTER = 0;
    COUNTER_DRIVE_RIGHT = 0;
    COUNTER_CLEAR_LEFT = 0;
    COUNTER_CLEAR_CENTER = 0;
    COUNTER_CLEAR_RIGHT = 0;
}

/////////////////////////////Statistics counter END//////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
/////////////////////////////CONFIGURATION PARAMETERS - FACTORY SETTINGS //////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

//Warm up, break timer, stop speed, computation timer - modify only if you know what you are doing
int WARM_UP_SPEED=890;
int TRANSITION_SPEED=875;
int TRANSITION_SPEED_HIGH = 990;
int STOP = 750;
int WARM_UP_TIMER = 2000;
int COMPUTATION_TIMER = 600;
int BREAK_TIMER = 500;
int BUDDY_TIMER = 700; // Timer to synchronize BADDY and BUDDY

// DROP SHOTS
int DROP_LEFT_SHOT_LEFT_MOTOR=910;//950;
int DROP_LEFT_SHOT_RIGHT_MOTOR=843;//860;
int DROP_CENTER_SHOT_LEFT_MOTOR=845;//880;
int DROP_CENTER_SHOT_RIGHT_MOTOR=845;//880;
int DROP_RIGHT_SHOT_LEFT_MOTOR=840;//860;
int DROP_RIGHT_SHOT_RIGHT_MOTOR=910;//950;
//DRIVE SHOTS
int DRIVE_LEFT_SHOT_LEFT_MOTOR=1050;
int DRIVE_LEFT_SHOT_RIGHT_MOTOR=875;
int DRIVE_CENTER_SHOT_LEFT_MOTOR=990;
int DRIVE_CENTER_SHOT_RIGHT_MOTOR=990;
int DRIVE_RIGHT_SHOT_LEFT_MOTOR=875;
int DRIVE_RIGHT_SHOT_RIGHT_MOTOR=1050;
//CLEAR SHOTS
int CLEAR_LEFT_SHOT_LEFT_MOTOR=1300;
int CLEAR_LEFT_SHOT_RIGHT_MOTOR=930;
int CLEAR_CENTER_SHOT_LEFT_MOTOR=1350;
int CLEAR_CENTER_SHOT_RIGHT_MOTOR=1350;
int CLEAR_RIGHT_SHOT_LEFT_MOTOR=930;
int CLEAR_RIGHT_SHOT_RIGHT_MOTOR=1300;

//Switch movement and timers
int SWITCH_LONG_POSITION = 75;
int SWITCH_SHORT_POSITION = 140;
int SWITCH_WAIT_POSITION = 90;
int SWITCH_TIMER = 300;//320 // value in millisecond to wait Switch to fetch its long position
int SWITCH_SPEED=600; // Special speed for slow motion movement (configuration)

// Shuttle retainer movemement and timers
int RETAINER_UP_POSITION = 115;
int RETAINER_DOWN_POSITION = 158;
int RETAINER_TIMER = 160;//150

//Network profile init parameters
const char* network_init_ssid = "BADDY-init";
const char* network_init_pwd = "BADDY1234";

// Use at Json config object creation
int SpeedProfileInit[18] =      {DROP_LEFT_SHOT_LEFT_MOTOR, DROP_LEFT_SHOT_RIGHT_MOTOR, DROP_CENTER_SHOT_LEFT_MOTOR, DROP_CENTER_SHOT_RIGHT_MOTOR, DROP_RIGHT_SHOT_LEFT_MOTOR, DROP_RIGHT_SHOT_RIGHT_MOTOR, DRIVE_LEFT_SHOT_LEFT_MOTOR, DRIVE_LEFT_SHOT_RIGHT_MOTOR, DRIVE_CENTER_SHOT_LEFT_MOTOR, DRIVE_CENTER_SHOT_RIGHT_MOTOR, DRIVE_RIGHT_SHOT_LEFT_MOTOR, DRIVE_RIGHT_SHOT_RIGHT_MOTOR, CLEAR_LEFT_SHOT_LEFT_MOTOR, CLEAR_LEFT_SHOT_RIGHT_MOTOR, CLEAR_CENTER_SHOT_LEFT_MOTOR, CLEAR_CENTER_SHOT_RIGHT_MOTOR, CLEAR_RIGHT_SHOT_LEFT_MOTOR, CLEAR_RIGHT_SHOT_RIGHT_MOTOR};
int SwitchProfileInit[2]=     {SWITCH_LONG_POSITION, SWITCH_SHORT_POSITION};
int RetainerProfileInit[2]=     {RETAINER_UP_POSITION, RETAINER_DOWN_POSITION};

// Servos instances

Servo ShuttleRetainer;
Servo ShuttleSwitch;
Servo MotorLeft;
Servo MotorRight;

// Servo instances End

// Servo functions

void servo_fire(){

ShuttleRetainer.write(RETAINER_UP_POSITION);
delay(RETAINER_TIMER);

ShuttleSwitch.write(SWITCH_LONG_POSITION);
delay(SWITCH_TIMER);

ShuttleSwitch.write(SWITCH_SHORT_POSITION);
delay(SWITCH_TIMER);
ShuttleRetainer.write(RETAINER_DOWN_POSITION);

delay(RETAINER_TIMER); // Just in case

}

// Servo functions end

void dump_play_mode() {

    // JSON Object for Playmode dump
    DynamicJsonBuffer jsonPlaymode;
    JsonObject& PlayModeDump = jsonPlaymode.createObject();

    JsonArray& CurrentSpeeds = PlayModeDump.createNestedArray("CurrentSpeeds");
    PlayModeDump["PlayMode"] = PlayMode;

    CurrentSpeeds.add(DROP_LEFT_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(DROP_LEFT_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(DROP_CENTER_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(DROP_CENTER_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(DROP_RIGHT_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(DROP_RIGHT_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(DRIVE_LEFT_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(DRIVE_CENTER_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(CLEAR_LEFT_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(CLEAR_CENTER_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(CLEAR_CENTER_SHOT_RIGHT_MOTOR);
    CurrentSpeeds.add(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
    CurrentSpeeds.add(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);

    json_playmode_dump = ""; //empty first
    PlayModeDump.printTo(json_playmode_dump);
}

bool format_file_system_spifs() {
    // We format the file system
    SPIFFS.format();

    // JSON Object for BADDY configuration
    DynamicJsonBuffer jsonConfig;
    JsonObject& Config = jsonConfig.createObject();

    JsonArray& SpeedProfile = Config.createNestedArray("SpeedProfile");
    JsonArray& SpeedfloorProfile = Config.createNestedArray("SpeedfloorProfile");
    JsonArray& SpeedbuddyProfile = Config.createNestedArray("SpeedbuddyProfile");
    JsonArray& RetainerProfile = Config.createNestedArray("RetainerProfile");
    JsonArray& SwitchProfile = Config.createNestedArray("SwitchProfile");

    // We implement Config file with factory settings
    int index;

    for(index=0;index<18;index++)
    {
        SpeedProfile.add(SpeedProfileInit[index]);
        SpeedfloorProfile.add(SpeedProfileInit[index]);
        SpeedbuddyProfile.add(SpeedProfileInit[index]);
        /*Serial.print("Speed profile index value: ");
        Serial.print(index);
        Serial.print(", Speed profile index value: ");
        Serial.println(SpeedProfileInit[index]);
        */
    }

    for(index=0;index<2;index++)
    {
        RetainerProfile.add(RetainerProfileInit[index]);
        /*
        Serial.print("Retainer profile index value: ");
        Serial.print(index);
        Serial.print(", Retainer profile index value: ");
        Serial.println(RetainerProfileInit[index]);
        */
    }

    for(index=0;index<2;index++)
    {
        SwitchProfile.add(SwitchProfileInit[index]);
        /*
        Serial.print("Switch profile index value: ");
        Serial.print(index);
        Serial.print(", Switch profile index value: ");
        Serial.println(SwitchProfileInit[index]);
        */
    }

    Serial.println("Config object print before save to config file");
    //Config.prettyPrintTo(Serial);
    json_config = ""; // clean before print
    Config.printTo(json_config); // Saving global variable exposed to API

    // Saving into file system
    // We open a brand new file
    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "w"); // Open BADDY config file
    Config.printTo(file); // Export and save JSON object to SPIFFS
    Serial.println("configuration saved to File system: ");
    //Config.prettyPrintTo(Serial);
    file.close();

    File file2 = SPIFFS.open("/BADDY_NET_FILE", "w"); // Open BADDY config file
    file2.print(network_init_ssid);
    file2.print('\n');
    file2.print(network_init_pwd);
    file2.print('\n');
    Serial.println("Network configuration saved to File system: ");
    file2.close();

    PlayMode = 0; // set to default if not already the case
    dump_play_mode(); // expose current playmode
    return 1;
}

bool set_network(const char* ssid_input, const char* password_input)
{
    Serial.println("Set_network end point triggered...");

    Serial.print("New ssid: ");
    Serial.println(ssid_input);
    Serial.print("New password: ");
    Serial.println(password_input);

    // We fetch data from file system
    File file = SPIFFS.open("/BADDY_NET_FILE", "r");

    if (!file){

        Serial.println("No BADDY network file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else {

        size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            //format_file_system();
            return false;
        }
        else {

            String read = file.readStringUntil('\n');
            Serial.println(read);
            read = file.readStringUntil('\n');
            Serial.println(read);            

            file.close();

            File file = SPIFFS.open("/BADDY_NET_FILE", "w");
            file.flush();

            // Now writing...
            file.print(ssid_input);
            file.print('\n');
            file.print(password_input);
            file.print('\n');
            Serial.println("Network configuration saved to File system: ");
            
        }

    file.close();
    return true;

    }
}


int set_speed_spifs(String Json)
{
    Serial.println("Getting data from wifi - set_speed end point triggered...");
    Serial.println(Json);

    // Parsing Json Object and extracting the values
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(Json);

    if (!root.success()){
        Serial.println("ERROR");
        return 0;
    }

    Serial.println("Parsing Json Object");
    PlayMode = root["PlayMode"]; // We set global PlayMode value
    Serial.println("PlayMode: ");
    Serial.println(PlayMode);

    for(int index=0;index<18;index++)
    {
        Serial.print("New speeds to implement: ");
        if (root["Speeds"][index]<750 || root["Speeds"][index] > 2000) // We test if speed values are in the correct range...
        {
            Serial.print("Invalid speed value: ");
            Serial.println(root["Speeds"][index].asString());
            return 0;
        }
        //Serial.println(root["Speeds"][index].asString()); // For debug only
    }

    // Loading filesystem config object

   // We fetch data from file system
    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "r");

    if (!file){

        Serial.println("No BADDY config file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else {
       size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            //format_file_system();
            return false;
        }
        else {

            std::unique_ptr<char[]> buf (new char[size]);
            file.readBytes(buf.get(), size);
            // Let's read BADDY configuration data form file system...
            // Create Json object to dump config file
            DynamicJsonBuffer jsonConfig;
            JsonObject& Config = jsonConfig.parseObject(buf.get());
            Serial.println("File system before update:");
            //Config.prettyPrintTo(Serial); // For debug
            file.close();

            // Now writing...

            File file = SPIFFS.open("/BADDY_CONFIG_FILE", "w");

            if (PlayMode == 0) {

                int index =0;
                for(index=0;index<18;index++){
                    Config["SpeedProfile"][index] = root["Speeds"][index];
                }

                Serial.println("File system after update:");
                // Config.prettyPrintTo(Serial); // for debug

                Serial.println("New Config applied on global variables...");

                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][17];

                Serial.println("Tripod Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);
                file.flush();
                Config.printTo(file);

            }
            else if (PlayMode == 1) {

                int index =0;
                for(index=0;index<18;index++){
                    Config["SpeedfloorProfile"][index] = root["Speeds"][index];
                }

                Serial.println("File system after update:");
                // Config.prettyPrintTo(Serial); // For debug only

                Serial.println("New Config applied on global variables...");
                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][17];

                Serial.println("Floor Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);
                file.flush();
                Config.printTo(file);

            }
            else if (PlayMode == 2) {

                int index =0;
                for(index=0;index<18;index++){
                    Config["SpeedbuddyProfile"][index] = root["Speeds"][index];
                }

                Serial.println("File system after update:");
                // Config.prettyPrintTo(Serial); // For debug

                Serial.println("New Config applied on global variables...");

                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][17];

                Serial.println("Buddy Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);
                file.flush();
                Config.printTo(file);
            }

            else{
                    Serial.println("ERROR: Wrong PlayMode value");
            }
        }
    }
    file.close();
    dump_play_mode(); //Expose Playmode dump variable
    return 1;
}

bool set_retainer_up(){

    Serial.println("Set retainer UP end point triggered");
    RETAINER_DOWN_POSITION = RETAINER_DOWN_POSITION - 3;
    RETAINER_UP_POSITION = RETAINER_UP_POSITION - 3;

    ShuttleRetainer.write(RETAINER_DOWN_POSITION); // Set actual new position

    Serial.println("Saving to file system");

    // We fetch data from file system
    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "r");

    if (!file){

        Serial.println("No BADDY config file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else
    {
       size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            format_file_system_spifs();
            return false;
        }
        else {

            std::unique_ptr<char[]> buf (new char[size]);
            file.readBytes(buf.get(), size);
            // Let's read BADDY configuration data form file system...
            // Create Json object to dump config file
            DynamicJsonBuffer jsonConfig;
            JsonObject& Config = jsonConfig.parseObject(buf.get());
            Serial.println("File system before update:");
            Config.prettyPrintTo(Serial);
            file.close();

            // Now writing...

            File file = SPIFFS.open("/BADDY_CONFIG_FILE", "w");

            Config["RetainerProfile"][0] = RETAINER_UP_POSITION;
            Config["RetainerProfile"][1] = RETAINER_DOWN_POSITION;

            Serial.println("File system after update:");
            Config.prettyPrintTo(Serial);

            Serial.println("New Config applied and saves to filesystem");

            json_config = ""; // clean before print
            Config.printTo(json_config);
            file.flush();
            Config.printTo(file);

            file.close();
            return 1;
        }
    }
}

bool set_retainer_down(){

    Serial.println("Set retainer DOWN end point triggered");
    RETAINER_DOWN_POSITION = RETAINER_DOWN_POSITION + 3;
    RETAINER_UP_POSITION = RETAINER_UP_POSITION + 3;

    ShuttleRetainer.write(RETAINER_DOWN_POSITION); // Set actual new position


    // We fetch data from file system
    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "r");

    if (!file){

        Serial.println("No BADDY config file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else
    {
       size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            format_file_system_spifs();
            return false;
        }
        else {

            std::unique_ptr<char[]> buf (new char[size]);
            file.readBytes(buf.get(), size);
            // Let's read BADDY configuration data form file system...
            // Create Json object to dump config file
            DynamicJsonBuffer jsonConfig;
            JsonObject& Config = jsonConfig.parseObject(buf.get());
            Serial.println("File system before update:");
            Config.prettyPrintTo(Serial);
            file.close();

            // Now writing...

            File file = SPIFFS.open("/BADDY_CONFIG_FILE", "w");

            Config["RetainerProfile"][0] = RETAINER_UP_POSITION;
            Config["RetainerProfile"][1] = RETAINER_DOWN_POSITION;

            Serial.println("File system after update:");
            Config.prettyPrintTo(Serial);

            Serial.println("New Config applied and saves to filesystem");

            json_config = ""; // clean before print
            Config.printTo(json_config);
            file.flush();
            Config.printTo(file);

            file.close();
            dump_play_mode(); //Expose Playmode dump variable
            return 1;
        }
    }

}

bool set_switch_forward(){

    Serial.println("Set switch FORWARD end point triggered");
    SWITCH_LONG_POSITION = SWITCH_LONG_POSITION - 2;
    SWITCH_SHORT_POSITION = SWITCH_SHORT_POSITION - 2;

    ShuttleSwitch.write(SWITCH_SHORT_POSITION); // Set actual new position

        Serial.println("Saving to file system");

    // We fetch data from file system
    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "r");

    if (!file){

        Serial.println("No BADDY config file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else
    {
       size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            format_file_system_spifs();
            return false;
        }
        else {

            std::unique_ptr<char[]> buf (new char[size]);
            file.readBytes(buf.get(), size);
            // Let's read BADDY configuration data form file system...
            // Create Json object to dump config file
            DynamicJsonBuffer jsonConfig;
            JsonObject& Config = jsonConfig.parseObject(buf.get());
            Serial.println("File system before update:");
            Config.prettyPrintTo(Serial);
            file.close();

            // Now writing...

            File file = SPIFFS.open("/BADDY_CONFIG_FILE", "w");

            Config["SwitchProfile"][0] = SWITCH_LONG_POSITION;
            Config["SwitchProfile"][1] = SWITCH_SHORT_POSITION;

            Serial.println("File system after update:");
            Config.prettyPrintTo(Serial);

            Serial.println("New Config applied and saves to filesystem");

            json_config = ""; // clean before print
            Config.printTo(json_config);
            file.flush();
            Config.printTo(file);

            file.close();
            return 1;
        }
    }
}

bool set_switch_backward(){

    Serial.println("Set switch BACKWARD end point triggered");
    SWITCH_LONG_POSITION = SWITCH_LONG_POSITION + 2;
    SWITCH_SHORT_POSITION = SWITCH_SHORT_POSITION + 2;

    ShuttleSwitch.write(SWITCH_SHORT_POSITION); // Set actual new position

    Serial.println("Saving to file system");

    // We fetch data from file system
    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "r");

    if (!file){

        Serial.println("No BADDY config file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else
    {
       size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            format_file_system_spifs();
            return false;
        }
        else {

            std::unique_ptr<char[]> buf (new char[size]);
            file.readBytes(buf.get(), size);
            // Let's read BADDY configuration data form file system...
            // Create Json object to dump config file
            DynamicJsonBuffer jsonConfig;
            JsonObject& Config = jsonConfig.parseObject(buf.get());
            Serial.println("File system before update:");
            Config.prettyPrintTo(Serial);
            file.close();

            // Now writing...

            File file = SPIFFS.open("/BADDY_CONFIG_FILE", "w");

            Config["SwitchProfile"][0] = SWITCH_LONG_POSITION;
            Config["SwitchProfile"][1] = SWITCH_SHORT_POSITION;

            Serial.println("File system after update:");
            Config.prettyPrintTo(Serial);

            Serial.println("New Config applied and saves to filesystem");

            json_config = ""; // clean before print
            Config.printTo(json_config);
            file.flush();
            Config.printTo(file);

            file.close();
            return 1;
        }
    }

}

bool LoadConfig (int Mode){

    Serial.println("Loading configuration");
    // We fetch data from file system

    // First checking network parameters

    File file2 = SPIFFS.open("/BADDY_NET_FILE", "r");

    if (!file2){

        Serial.println("No BADDY config file exists");
        file2.close();
        format_file_system_spifs();
        return false;

    }
    else
    {

        size_t size2 = file2.size();
        if ( size2 == 0 ) {
            Serial.println("Config file empty !");
            file2.close();
            format_file_system_spifs();
            return false;
        }

        String read = file2.readStringUntil('\n');
        read.trim();

        Serial.print("ssid hot spot = ");
        Serial.println(read);
        ssid = read;
        read = file2.readStringUntil('\n');
        read.trim();
        password = read;
        Serial.print("password hot spot = ");
        Serial.println(read);
        
        file2.close();

    }

    // Then the motor speeds and playmodes

    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "r");

    if (!file){

        Serial.println("No BADDY config file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else
    {
        size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            format_file_system_spifs();
            return false;
        }
        else
        {
            std::unique_ptr<char[]> buf (new char[size]);
            file.readBytes(buf.get(), size);
            // Let's read BADDY configuration data form file system...
            // Create Json object to dump config file
            DynamicJsonBuffer jsonConfig;
            JsonObject& Config = jsonConfig.parseObject(buf.get());
            //Serial.println("That's the config file dumped from Filesystem");
            //Config.prettyPrintTo(Serial);

            if (Mode == 0) {

                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][17];

                Serial.println("Tripod Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);

            }
            else if (Mode == 1) {
                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][17];

                Serial.println("Floor Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);

            }
            else if (Mode == 2) {

                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][17];

                Serial.println("Buddy Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);
            }

            else{
                    Serial.println("ERROR: Wrong PlayMode value");
            }

            RETAINER_UP_POSITION = Config["RetainerProfile"][0];
            RETAINER_DOWN_POSITION = Config["RetainerProfile"][1];
            SWITCH_LONG_POSITION = Config["SwitchProfile"][0];
            SWITCH_SHORT_POSITION = Config["SwitchProfile"][1];
        }
    }

    file.close();
    dump_play_mode(); // we expose playmode Json variable
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////
/////////////////////////////CONFIGURATION PARAMETERS END /////////////////////////
///////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
//////////////////// OTHER FUNCTIONS DECLARATION///////////////////////////////
///////////////////////////////////////////////////////////////////////////////


String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;

}

String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
     // yield();
    }
    
   return encodedString;
}

void buddy_send_abort ()
{

    Serial.println("SENDING ABORT TO BUDDY...");
    // Preparing Json object
    DynamicJsonBuffer jsonBufferBuddy;
    JsonObject& SequenceBuddy = jsonBufferBuddy.createObject();

    // Add values in the object

    SequenceBuddy["Type"] = "ABT";
    JsonArray& Strokes_buddy = SequenceBuddy.createNestedArray("Strokes");
    Strokes_buddy.add(0);
    Strokes_buddy.add(0);
    Strokes_buddy.add(0);
    Strokes_buddy.add(0);
    Strokes_buddy.add(0);
    Strokes_buddy.add(0);
    Strokes_buddy.add(0);

    JsonArray& Speeds_buddy = SequenceBuddy.createNestedArray("Speeds");
    Speeds_buddy.add(0);
    Speeds_buddy.add(0);
    Speeds_buddy.add(0);
    Speeds_buddy.add(0);
    Speeds_buddy.add(0);
    Speeds_buddy.add(0);
    Speeds_buddy.add(0);

    SequenceBuddy["LoopMode"] = 0;

    String sequence_buddy;
    SequenceBuddy.printTo(sequence_buddy);

    buddy.begin("http://"+buddy_ip_address+"/sequence?data="+ urlencode(sequence_buddy));

    //buddy.sendRequest("POST","/status");
    //buddy.addHeader("content-type","text/plain");
    int http_code = buddy.GET();

    Serial.print("Http code value: ");
    Serial.println(http_code);

    String Payload = buddy.getString();
    Serial.println(Payload);

    buddy.end();
}

void buddy_send_json(String json)
{
        buddy.begin("http://"+buddy_ip_address+"/sequence?data="+ urlencode(json));

    //buddy.sendRequest("POST","/status");
    //buddy.addHeader("content-type","text/plain");
    int http_code = buddy.GET();

    Serial.print("Http code value: ");
    Serial.println(http_code);

    String Payload = buddy.getString();
    Serial.println(Payload);

    buddy.end();
}

///////////////////////////////////////////////////////////////////////////////
//////////////////// END OF OTHER FUNCTIONS DECLARATION////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Functions that sends Json status object to GO BADDY app

void update_status()
{
    Serial.println("Updating Status variable for exposure on API");

    battery_level = analogRead(ANALOG_READ_BATTERY);

    if (battery_level > 195)
    {
        battery_level = 195;
    }
    if (battery_level < 135)
    {
        battery_level = 135;
    }

    battery_level = map(battery_level, 135, 195, 0, 100);

    DynamicJsonBuffer jsonBuffer;
    JsonObject& status = jsonBuffer.createObject();

    status["Running"] = running_status;
    status["Battery"] = battery_level;
    status["Baddy_id"] = BADDY_ID;
    status["Firmware_version"] = baddy_firmware_version;
    status["Connect_mode"] = connect_mode;
    JsonArray& Stats = status.createNestedArray("Stats");
    Stats.add(COUNTER_DROP_LEFT);
    Stats.add(COUNTER_DROP_CENTER);
    Stats.add(COUNTER_DROP_RIGHT);
    Stats.add(COUNTER_DRIVE_LEFT);
    Stats.add(COUNTER_DRIVE_CENTER);
    Stats.add(COUNTER_DRIVE_RIGHT);
    Stats.add(COUNTER_CLEAR_LEFT);
    Stats.add(COUNTER_CLEAR_CENTER);
    Stats.add(COUNTER_CLEAR_RIGHT);

    Serial.println("Json status object dump");
    status.prettyPrintTo(Serial);
    json_status = ""; // clean before print
    status.printTo(json_status);
    // We reset stats counter
    reset_stats_counter();

}

// Led management instances and objects

bool flag_print_smiley = 0;
bool flag_print_frown = 0;
bool flag_print_squares = 0;

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

// Led management instances end

// Functions for Led display
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
// End of functions for Led display

int set_stroke(int stroke)
{
    if (stroke==1)
    {
        Serial.println("Drop left shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DROP_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_LEFT_SHOT_RIGHT_MOTOR);
        if (PlayMode ==2){
            COUNTER_DROP_CENTER++;
        }
        else{
           COUNTER_DROP_LEFT++;
        }

    }
    else if (stroke==2)
    {
        Serial.println("Drop Center shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DROP_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_CENTER_SHOT_RIGHT_MOTOR);
        if (PlayMode ==2){
            COUNTER_DROP_RIGHT++;
        }
        else{
            COUNTER_DROP_CENTER++;
        }

    }
    else if (stroke==3)
    {
        Serial.println("Drop Right shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR);
        COUNTER_DROP_RIGHT++;
        // Stroke not used in BADDY BUDDY mode
    }
    else if (stroke==4)
    {
        Serial.println("Drive Left shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        if (PlayMode ==2){
            COUNTER_DRIVE_CENTER++;
        }
        else{
           COUNTER_DRIVE_LEFT++;
        }

    }
    else if (stroke==5)
    {
        Serial.println("Drive Center shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
        if (PlayMode ==2){
            COUNTER_DRIVE_RIGHT++;
        }
        else{
            COUNTER_DRIVE_CENTER++;
        }

    }
    else if (stroke==6)
    {
        Serial.println("Drive Right shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        COUNTER_DRIVE_RIGHT++;
        // Stroke not used in BADDY BUDDY mode
    }
    else if (stroke==7)
    {
        Serial.println("Clear Left shot speed command sent to motor");
        MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
        if (PlayMode ==2){
            COUNTER_CLEAR_CENTER++;
        }
        else{
            COUNTER_CLEAR_LEFT++;
        }

    }
    else if (stroke==8)
    {
        Serial.println("Clear Center shot speed command sent to motor");
        // Special trick to handle potential battery cut off
        /*MotorLeft.writeMicroseconds(1150);
        MotorRight.writeMicroseconds(1150);
        delay(200);
        MotorLeft.writeMicroseconds(1200);
        MotorRight.writeMicroseconds(1200);
        delay(200);
        MotorLeft.writeMicroseconds(1250);
        MotorRight.writeMicroseconds(1250);
        delay(200);
        MotorLeft.writeMicroseconds(1300);
        MotorRight.writeMicroseconds(1300);
        delay(200);
        */
        //End of special trick

        MotorLeft.writeMicroseconds(CLEAR_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_CENTER_SHOT_RIGHT_MOTOR);
        if (PlayMode ==2){
            COUNTER_CLEAR_RIGHT++;
        }
        else{
            COUNTER_CLEAR_CENTER++;
        }

    }
    else if (stroke==9)
    {
        Serial.println("Clear Center shot speed command sent to motor");
        MotorLeft.writeMicroseconds(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);
        COUNTER_CLEAR_RIGHT++;
        // Stroke not used in BADDY BUDDY mode
    }
    else if (stroke>10) // Only in BADDY BUDDY mode
    {
        if (stroke == 11)
        {
            COUNTER_DROP_LEFT++;
        }
        else if (stroke == 12)
        {
            COUNTER_DROP_CENTER++;
        }
        else if (stroke == 13)
        {
            COUNTER_DROP_RIGHT++;
        }
        else if (stroke == 14)
        {
            COUNTER_DRIVE_LEFT++;
        }
        else if (stroke == 15)
        {
            COUNTER_DRIVE_CENTER++;
        }
        else if (stroke == 16)
        {
            COUNTER_DRIVE_RIGHT++;
        }
        else if (stroke == 17)
        {
            COUNTER_CLEAR_LEFT++;
        }
        else if (stroke == 18)
        {
            COUNTER_CLEAR_CENTER++;
        }
        else if (stroke == 19)
        {
            COUNTER_CLEAR_RIGHT++;
        }
        Serial.println("Idle Speed set");
        MotorLeft.writeMicroseconds(900);
        MotorRight.writeMicroseconds(900);
    }
    return 1;
}

int warmup(int stroke)
{
    if (stroke==1)
    {
        Serial.println("Drop left shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DROP_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_LEFT_SHOT_RIGHT_MOTOR);
        if (PlayMode==2&&buddy_around) // BADDY BUDDY mode
        {
            COUNTER_DROP_CENTER++;
        }
        else
        {
            COUNTER_DROP_LEFT++;
        }
        
        delay(2000);

    }
    else if (stroke==2)
    {
        Serial.println("Drop Center shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DROP_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_CENTER_SHOT_RIGHT_MOTOR);

        if (PlayMode==2&&buddy_around) // BADDY BUDDY mode
        {
            COUNTER_DROP_RIGHT++;
        }
        else
        {
            COUNTER_DROP_CENTER++;
        }

        delay(2000);
    }
    else if (stroke==3)
    {
        Serial.println("Drop Right shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR);
        COUNTER_DROP_RIGHT++;
        delay(2000);
    }
    else if (stroke==4)
    {
        Serial.println("Drive Left shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_LEFT_SHOT_RIGHT_MOTOR);

        if (PlayMode==2&&buddy_around) // BADDY BUDDY mode
        {
            COUNTER_DRIVE_CENTER++;
        }
        else
        {
            COUNTER_DRIVE_LEFT++;
        }

        delay(2000);
    }
    else if (stroke==5)
    {
        Serial.println("Drive Center shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_CENTER_SHOT_RIGHT_MOTOR);

        if (PlayMode==2&&buddy_around) // BADDY BUDDY mode
        {
            COUNTER_DRIVE_RIGHT++;
        }
        else
        {
            COUNTER_DRIVE_CENTER++;
        }
        
        delay(2000);
    }
    else if (stroke==6)
    {
        Serial.println("Drive Right shot speed command sent to motor");
        MotorLeft.writeMicroseconds(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        COUNTER_DRIVE_RIGHT++;
        delay(2000);
    }
    else if (stroke==7)
    {
        Serial.println("Clear Left shot speed command sent to motor");
        MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_LEFT_SHOT_RIGHT_MOTOR);

        if (PlayMode==2&&buddy_around) // BADDY BUDDY mode
        {
            COUNTER_CLEAR_CENTER++;
        }
        else
        {
            COUNTER_CLEAR_LEFT++;
        }
        
        delay(2000);
    }
    else if (stroke==8)
    {
        Serial.println("Clear Center shot speed command sent to motor");

        // Special trick to handle potential battery cut off
        MotorLeft.writeMicroseconds(1150);
        MotorRight.writeMicroseconds(1150);
        delay(200);
        MotorLeft.writeMicroseconds(1200);
        MotorRight.writeMicroseconds(1200);
        delay(200);
        MotorLeft.writeMicroseconds(1250);
        MotorRight.writeMicroseconds(1250);
        delay(200);
        MotorLeft.writeMicroseconds(1300);
        MotorRight.writeMicroseconds(1300);
        delay(200);
        //End of special trick

        MotorLeft.writeMicroseconds(CLEAR_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_CENTER_SHOT_RIGHT_MOTOR);

        if (PlayMode==2&&buddy_around) // BADDY BUDDY mode
        {
            COUNTER_CLEAR_RIGHT++;
        }
        else
        {
            COUNTER_CLEAR_CENTER++;
        }
        
        delay(1200);
    }
    else if (stroke==9)
    {
        Serial.println("Clear Center shot speed command sent to motor");
        MotorLeft.writeMicroseconds(CLEAR_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(CLEAR_RIGHT_SHOT_RIGHT_MOTOR);
        COUNTER_CLEAR_RIGHT++;
        delay(2000);
    }
    else // in case we have BUDDY shot, we put idle speed, and of course wait for 2 sec to circumvent asynchronous BUDDY issues
    {
        Serial.println("BUDDY shot, so idle speed is set");
        MotorLeft.writeMicroseconds(DROP_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DROP_CENTER_SHOT_RIGHT_MOTOR);
        if (stroke==11)
        {
            COUNTER_DROP_LEFT++;
        }
        else if (stroke==12)
        {
            COUNTER_DROP_CENTER++;
        }
        else if (stroke==13)
        {
            COUNTER_DROP_RIGHT++;
        }
        else if (stroke==14)
        {
            COUNTER_DRIVE_LEFT++;
        }
        else if (stroke==15)
        {
            COUNTER_DRIVE_CENTER++;
        }
        else if (stroke==16)
        {
            COUNTER_DRIVE_RIGHT++;
        }
        else if (stroke==17)
        {
            COUNTER_CLEAR_LEFT++;
        }
        else if (stroke==18)
        {
            COUNTER_CLEAR_CENTER++;
        }
        else if (stroke==19)
        {
            COUNTER_CLEAR_RIGHT++;
        }

        delay(2000);
    }

    return 1;
}

int set_speed(int freq)
{

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

    return freq;
}

// End of functions for sequence Read

void break_motor_right(int outputspeed){

  //Break
  Serial.println("Break Right Motor");
  MotorRight.writeMicroseconds(STOP);
  delay(210);
  //Restart at idle low speed
  //Serial.println("Acceleration");
  MotorRight.writeMicroseconds(1700);
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
  MotorLeft.writeMicroseconds(1700);
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
  MotorLeft.writeMicroseconds(1300);
  MotorRight.writeMicroseconds(1300);
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

    if (next_type == 10){
        MotorRight.writeMicroseconds(860); // setting an idle speed
        MotorRight.writeMicroseconds(860); // setting an idle speed
        return 0;
    }

    // Here, we manage BADDY BUDDY time considerations between strokes - section kept out as i needed more tangible finetuning
    /*
    if ((next_type == 11)||(next_type ==12)||(next_type ==13)){
        return 0; //we do nothing
    }

    if ((next_type == 14)||(next_type ==16)){
        return 0; //we do nothing
    }

    if ((next_type == 17)||(next_type ==19)){
        return 0;
    }
    */

    // End of BADDY BUDDY timer considerations

    if ( ((type==1)||(type==2)||(type>10))&&((next_type == 3)) ){
        // Preferable to break in case if Drop Left and right sequence, to ensure excentricity
        break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
        //MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(1250); // Well, we need to accelerate a bit more with more heavy duty wheels
        delay(80);
        MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR);//Adjustement trick to faster make ESC get nominal speed
        return 480;
    }

    if ( ((type == 2)||(type == 3)||(type>10))&&((next_type == 1)) ){
        MotorLeft.writeMicroseconds(1250);// Well, we need more acceleration to accomodate heavy duty wheels
        delay(80);
        MotorLeft.writeMicroseconds(DROP_LEFT_SHOT_LEFT_MOTOR);//Adjustement trick to faster make ESC get nominal speed
        break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);
        //MotorRight.writeMicroseconds(DROP_LEFT_SHOT_RIGHT_MOTOR);
        return 480;
    }

    // We add this section because we lack to differentiate excentered drops from centered

    if ( ((type == 1)||(type == 3)||(type>10))&&((next_type == 2)) ){

            break_motor_all(DROP_CENTER_SHOT_RIGHT_MOTOR);
            return(480); // we deduce by the time needed to break

    }

    // New, manage transitions from drop to clear shot

    if ( ((type == 1)||(type == 2)||(type == 3)||(type>10))&&((next_type == 4)) ){

        MotorLeft.writeMicroseconds(DRIVE_LEFT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_LEFT_SHOT_RIGHT_MOTOR);
        return 0;
    }

    if ( ((type == 1)||(type == 2)||(type == 3)||(type>10))&&((next_type == 5)) ){
        MotorLeft.writeMicroseconds(DRIVE_CENTER_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_CENTER_SHOT_RIGHT_MOTOR);
        return 0;
    }

    if ( ((type == 1)||(type == 2)||(type == 3)||(type>10))&&((next_type == 6)) ){
        MotorLeft.writeMicroseconds(DRIVE_RIGHT_SHOT_LEFT_MOTOR);
        MotorRight.writeMicroseconds(DRIVE_RIGHT_SHOT_RIGHT_MOTOR);
        return 0;
    }

    if ( ((type == 1)||(type == 2)||(type == 3)||(type>10))&&((next_type == 7)) ){
            MotorLeft.writeMicroseconds(CLEAR_LEFT_SHOT_LEFT_MOTOR);
            MotorRight.writeMicroseconds(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
            return 0;
    }

    if ( ((type == 1)||(type == 2)||(type == 3)||(type>10))&&((next_type == 8)) ){

            // Special trick to handle potential battery cut off
            MotorLeft.writeMicroseconds(1150);
            MotorRight.writeMicroseconds(1150);
            delay(150);
            MotorLeft.writeMicroseconds(1200);
            MotorRight.writeMicroseconds(1200);
            delay(150);
            MotorLeft.writeMicroseconds(1250);
            MotorRight.writeMicroseconds(1250);
            delay(150);
            MotorLeft.writeMicroseconds(1300);
            MotorRight.writeMicroseconds(1300);
            delay(150);
            //End of special trick

            MotorLeft.writeMicroseconds(CLEAR_CENTER_SHOT_LEFT_MOTOR);
            MotorRight.writeMicroseconds(CLEAR_CENTER_SHOT_RIGHT_MOTOR);
            return 600;
    }

    if ( ((type == 1)||(type == 2)||(type == 3)||(type>10))&&((next_type == 9)) ){
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
                break_motor_left(DROP_LEFT_SHOT_LEFT_MOTOR+50);
                break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);// Trick to fetch motor's speed faster (accomodate ESC potential lack reactivity/wheels intertia)
                return(480); // we deduce by the time needed to break

            }
            if (type==8)
            {
                break_motor_left(DROP_LEFT_SHOT_LEFT_MOTOR+50);
                break_motor_right(DROP_LEFT_SHOT_RIGHT_MOTOR);
                return(480); // we deduce by the time needed to break
            }
            if (type==9);
            {
                break_motor_left(DROP_LEFT_SHOT_LEFT_MOTOR+50);
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
            if ((type == 7)||(type == 8)||(type == 9))
            {
                break_motor_all(DROP_CENTER_SHOT_RIGHT_MOTOR);
                return(480); // we deduce by the time needed to break
            }
        }
        else if (next_type == 3)
        {
            if (type==4)
            {
                break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
                MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR+50);
                return(480); // we deduce by the time needed to break
            }
            if (type==5)
            {
                MotorRight.writeMicroseconds(DROP_RIGHT_SHOT_RIGHT_MOTOR+50);
                break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
                return(480); // we deduce by the time needed to break
            }
            if (type==6)
            {
                MotorLeft.writeMicroseconds(DROP_RIGHT_SHOT_LEFT_MOTOR+50);
                break_motor_right(DROP_RIGHT_SHOT_RIGHT_MOTOR);
                return(480); // we deduce by the time needed to break
            }
            if (type==7)
            {
                break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR); // ESC trick
                break_motor_right(DROP_RIGHT_SHOT_RIGHT_MOTOR+50);
                return(480); // we deduce by the time needed to break
            }
            if (type==8)
            {
                break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
                break_motor_right(DROP_RIGHT_SHOT_RIGHT_MOTOR+50);
                return(480); // we deduce by the time needed to break
            }
            if (type==9);
            {
                break_motor_left(DROP_RIGHT_SHOT_LEFT_MOTOR);
                break_motor_right(DROP_RIGHT_SHOT_RIGHT_MOTOR+50);
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
                MotorRight.writeMicroseconds(CLEAR_LEFT_SHOT_RIGHT_MOTOR);
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
            if ((type==4)||(type==5)||(type==6)||(type==7)||(type==9))
            {
                // Special trick to handle potential battery cut off
                MotorLeft.writeMicroseconds(1150);
                MotorRight.writeMicroseconds(1150);
                delay(150);
                MotorLeft.writeMicroseconds(1200);
                MotorRight.writeMicroseconds(1200);
                delay(150);
                MotorLeft.writeMicroseconds(1250);
                MotorRight.writeMicroseconds(1250);
                delay(150);
                MotorLeft.writeMicroseconds(1300);
                MotorRight.writeMicroseconds(1300);
                delay(150);
                //End of special trick

                MotorLeft.writeMicroseconds(CLEAR_CENTER_SHOT_LEFT_MOTOR);
                MotorRight.writeMicroseconds(CLEAR_CENTER_SHOT_RIGHT_MOTOR);
                return(600); // we deduce by the time needed to accelerate
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

        else if (next_type == 11)
        {
                delay(480);
                return(480); // we deduce by the time needed to break
        }
        else if (next_type == 12)
        {
            if (type==11)
            {
                delay(560);
                return(560); // we deduce by the time needed to break
            }
            else if (type==13)
            {
                delay(560);
                return(560); // we deduce by the time needed to break
            }
            else if (type==14)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==15)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==16)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==17)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==18)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==19)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else
            {
                return(0); // we deduce by the time needed to break
            }

        }
        else if (next_type == 13)
        {

                delay(480);
                return(480); // we deduce by the time needed to break

        }
        else if (next_type == 14)
        {

            if (type==15)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }

            else if (type==17)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            
            else if (type==18)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else
            {
                return(0); // we deduce by the time needed to break
            }

        }
        else if (next_type == 15)
        {
        
            if (type==14)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==16)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==17)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==18)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==19)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else
            {
                return(0); // we deduce by the time needed to break
            }
            

        }
        else if (next_type == 16)
        {

            if (type == 15)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==18)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type==19)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else
            {
                return(0); // we deduce by the time needed to break
            }

        }
        else if (next_type == 17)
        {
                delay(600); // Avoid battery protect triggering, so delay is greater
                return(600); // we deduce by the time needed to break
        }
        else if (next_type == 18)
        {
            if (type == 19)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else if (type == 17)
            {
                delay(480);
                return(480); // we deduce by the time needed to break
            }
            else
            {
                return(0);
            }
                
        }

        else if (next_type == 19)
        {

                delay(600); // Avoid battery protect triggering, so delay is greater
                return(600); // we deduce by the time needed to break
        }



    }
    else
    {
        MotorLeft.writeMicroseconds(TRANSITION_SPEED);
        MotorRight.writeMicroseconds(TRANSITION_SPEED);
        return(0);
    }
}

// Functions for Sequence Read


void ReadSequence(){

    int i;
    int speed;
    //Detect Abort first
    Serial.print("type_seq value:");
    Serial.println(type_seq);
    Serial.print("stroke_seq value:");
    Serial.println(stroke_seq[0]);

    if (type_seq == "ABT")
    {
        Serial.println("************************ABORT Received. Stopping motors...**********************");
        MotorLeft.writeMicroseconds(STOP);
        MotorRight.writeMicroseconds(STOP);

        if ((PlayMode==2)&&(buddy_around)){

            buddy_send_abort();
            all_you_can_eat_flag = 0;

        }

        running_status=false;
        //update_status();
        flag_new_sequence = 0;
        return;
        // Place holder to send Json object to app with stats, battery level indicator and Status etc...)

        // End of place holder

    }
    // End Abort detection

    running_status=true; // motors are going to spin...

    // Checking loop_mode

    if (loop_mode_seq==0)
    {
        i=0;
        Serial.println("------------ No loop detected --------------");
        warmup(stroke_seq[0]);

        while(type_seq != "ABT")
        {

            if (stroke_seq[i]==0)
            {
                Serial.println("------------ End of sequence with no loop - shutting down motors--------------");
                MotorLeft.writeMicroseconds(STOP);
                MotorRight.writeMicroseconds(STOP);
                running_status=false;
                //update_status();
                flag_new_sequence = 0;
                all_you_can_eat_flag = 0;
                break;

            }

            if(flag_new_sequence)// avoid having one shot when ABORT received (may happen with Async server)
            {

                if (stroke_seq[i] < 10) // in case of BUDDY shot, no shot!
                {
                    servo_fire();
                    Serial.print("Shot id is: ");
                    Serial.println(i);
                    Serial.println("********* SHOT! **********");
                }
                else
                {
                    Serial.print("Shot id is: ");
                    Serial.println(i);
                    Serial.println("********* BUDDY SHOT! **********");

                    if (speed_seq[i] > 0) // condition for fast transitions in case period is 0
                    {
                        delay(RETAINER_TIMER*2 + SWITCH_TIMER*2);
                    }

                }

                speed = set_speed(speed_seq[i]); // convert speed id into milliseconds

                if (motor_speed_transition(stroke_seq[i],stroke_seq[i+1]) >0){
                    if (speed >1000){
                        delay(speed-(BREAK_TIMER+COMPUTATION_TIMER));
                    }
                }
                else{
                    if ((speed >1000)||(speed==1000)){
                        delay(speed-COMPUTATION_TIMER);
                    }
                }
            }

            i++;
            set_stroke(stroke_seq[i]);
        }
        Serial.print("New command received, EXIT ReadSequence");
        flag_new_sequence = 0;
        // To be sure...
        MotorLeft.writeMicroseconds(STOP);
        MotorRight.writeMicroseconds(STOP);

        if ((PlayMode==2)&&(buddy_around)){ // Transmit to BUDDY if PlayMode 2
            buddy_send_abort();
        }
        flag_print_squares = 1;
        return;

    }

    else if (loop_mode_seq==1)
    {
        Serial.println("------------ loop detected --------------");

        i=0;

        warmup(stroke_seq[0]);

        while(type_seq != "ABT"){

            if(flag_new_sequence)// avoid having one shot when ABORT received (may happen with Async server)
            {
                if (stroke_seq[i] < 10) // in case of BUDDY shot, no shot!
                {
                    servo_fire();
                    Serial.print("Shot id is: ");
                    Serial.println(i);
                    Serial.println("********* SHOT! **********");
                }
                else
                {
                    //We have to wait as if we make a servo_fire operation, otherwise disynchro will occur between BADDY and BUDDY
                    Serial.print("Shot id is: ");
                    Serial.println(i);
                    Serial.println("********* BUDDY SHOT! **********");

                    if (speed_seq[i] > 0) // condition for fast transitions in case period is 0
                    {
                        delay(RETAINER_TIMER*2 + SWITCH_TIMER*2);
                    }
            
                }

                speed = set_speed(speed_seq[i]); // convert speed id into milliseconds

                if((stroke_seq[i+1]==0)||(i+1==20)) // We need to set speed transition when at the last stroke of the sequence
                {
                    if (motor_speed_transition(stroke_seq[i],stroke_seq[0]) >0){
                        if (speed >1000){
                            delay(speed-(BREAK_TIMER+COMPUTATION_TIMER));
                        }
                    }
                    else{
                        if ((speed >1000)||(speed==1000)){
                            delay(speed-COMPUTATION_TIMER);
                        }
                    }
                }

                else{

                    if (motor_speed_transition(stroke_seq[i],stroke_seq[i+1]) >0){
                        if (speed >1000){
                            delay(speed-(BREAK_TIMER+COMPUTATION_TIMER));
                        }
                    }
                    else{
                        if ((speed >1000)||(speed==1000)){
                            delay(speed-COMPUTATION_TIMER);
                        }
                    }

                }
            }

            i++; // next stroke of the sequence
            if((stroke_seq[i]==0)||(i==20)){
                i=0; // End of the sequence, let's go back to the beginning
            }
            set_stroke(stroke_seq[i]);

       }

        Serial.print("New command received, EXIT ReadSequence");
        flag_new_sequence = 0;
        // To be sure...
        MotorLeft.writeMicroseconds(STOP);
        MotorRight.writeMicroseconds(STOP);
        if (PlayMode==2){ // Transmit to BUDDY if PlayMode 2
            buddy_send_abort();
        }
        flag_print_squares = 1;
        return;

    }
    else if (loop_mode_seq==2) // Note that this condition is not used in BUDDY mode. Random strokes' allocation is made at read sequence level in that case
    {
        Serial.println("------------ Random loop detected --------------");

        // count the number of strokes in the random sequence to feed the random function
        int flag_first_shot=1;
        int stroke_count=0;
        int random_id;
        int random_id_next;

        for (i=0;i<20;i++)
        {
            if (stroke_seq[i]!= 0)
            {
                stroke_count++;
            }
            else
            {
                break;
            }
        }

        Serial.print("TOTAL number of strokes: ");
        Serial.println(stroke_count);

        // End of counting strokes

        while (type_seq != "ABT"){

            if(flag_new_sequence)// avoid having one shot when ABORT received (may happen with Async server)
            {

                // Consider first shots a part, because warm up is taken into account for beg of sequence
                if (flag_first_shot){
                    random_id = random(0,stroke_count);
                    Serial.print("Random selected: ");
                    Serial.print(random_id);

                    random_id_next = random(0,stroke_count);
                    Serial.print("Next random selected: ");
                    Serial.print(random_id_next);

                    warmup(stroke_seq[random_id]);
                    //set_stroke(stroke_seq[random_id]);

                    servo_fire();
                    Serial.println("********* SHOT! **********");

                    speed = set_speed(speed_seq[random_id]);

                    if (motor_speed_transition(stroke_seq[random_id],stroke_seq[random_id_next]) >0){
                        if (speed >1000){
                            delay(speed-(BREAK_TIMER+COMPUTATION_TIMER));
                        }
                    }
                    else{
                        if ((speed >1000)||(speed==1000)){
                            delay(speed-COMPUTATION_TIMER);
                        }
                    }

                    flag_first_shot = 0; // Not a first shot anymore
                    random_id = random_id_next;

                }

                else{

                    Serial.print("Now shooting: ");
                    Serial.println(stroke_seq[random_id]);

                    servo_fire();
                    Serial.println("********* SHOT! **********");
                    speed = set_speed(speed_seq[random_id]);

                    // Computing next shot randomly
                    random_id_next = random(0,stroke_count);


                    if (motor_speed_transition(stroke_seq[random_id],stroke_seq[random_id_next]) >0){
                        if (speed >1000){
                            delay(speed-(BREAK_TIMER+COMPUTATION_TIMER));
                        }
                    }
                    else{
                        if ((speed >1000)||(speed==1000)){
                            delay(speed-COMPUTATION_TIMER);
                        }
                    }
                    set_stroke(stroke_seq[random_id_next]);
                    random_id = random_id_next;

                }
            }
        }

        Serial.print("New command received, EXIT ReadSequence");
        flag_new_sequence = 0;
        // To be sure...
        MotorLeft.writeMicroseconds(STOP);
        MotorRight.writeMicroseconds(STOP);
        if (PlayMode==2){ // Transmit to BUDDY if PlayMode 2
            buddy_send_abort();
        }
        flag_print_squares=1;
        return;
    }
    else
    {
        Serial.println("------------ Incorrect loop mode --------------");
        flag_print_frown = 1;
    }

}

String macToString(const unsigned char* mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
    Serial.println("Station connected: ");
    Serial.println(macToString(evt.mac));
    station_connected = true;
    flag_print_smiley = 1;
}

void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
    Serial.println("Station disconnected: ");
    Serial.println(macToString(evt.mac));
    flag_print_frown = 1;
    //printFrown();
}

int sequencefetch(String Json){

    int i=0;

    Serial.println("Getting data from wifi");

    // Parsing Json Object and extracting the values
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(Json);

    if (!root.success()){
        Serial.println("ERROR");
        return 0;
    }

    Serial.println("Parsing Json Object");
    const char* message_type = root["Type"];
    type_seq = message_type;
    Serial.print("Type: ");
    Serial.println(type_seq);

    // First of all, we detect ABORT messages
    if (type_seq == "ABT")
    {
        Serial.println("************************ABORT Received. Stopping motors...**********************");
        MotorLeft.writeMicroseconds(STOP);
        MotorRight.writeMicroseconds(STOP);

        if (PlayMode==2){ // Transmit to BUDDY if PlayMode 2

            buddy_send_abort();

            if (all_you_can_eat_flag) // Reset flag
            {
                all_you_can_eat_flag = 0;
            }
        }

        running_status=false;
        flag_new_sequence = 0;
        //update_status();
        flag_print_squares = 1;
    }

    else{

        // No Abort message, time to extract the other parameters of the Json sequence object
        JsonArray& stroke_sequence = root["Strokes"];
        JsonArray& speed_sequence = root["Speeds"];
        loop_mode_seq = root["LoopMode"];

        Serial.println("*********** Printing values of the Json Object ************");
        Serial.println("Type: ");
        Serial.println(message_type);

        Serial.println("Loop mode is: ");
        Serial.println(loop_mode_seq);

        Serial.println("Stroke Sequence: ");

        // We distinguish two cases, when one BADDY on court, or when BADDY with BUDDY on court

        if (PlayMode==2&&buddy_around) // BADDY BUDDY mode
        {
            stroke_count = 0; //init
            Serial.println("BADDY BUDDY mode here, dispatching stroke sequences!");
            // Preparing Json object
            DynamicJsonBuffer jsonBufferBuddy;
            JsonObject& SequenceBuddy = jsonBufferBuddy.createObject();

            SequenceBuddy["Type"] = message_type;
            JsonArray& Strokes_buddy = SequenceBuddy.createNestedArray("Strokes");
            JsonArray& Speeds_buddy = SequenceBuddy.createNestedArray("Speeds");
            SequenceBuddy["LoopMode"] = loop_mode_seq;

            for (i=0;i<20;i++)
            {
                if (stroke_sequence[i]!= 0)
                {
                stroke_count++;
                }
            }

            Serial.print("Total number of strokes: ");
            Serial.println(stroke_count);

            // Detect all you can eat situation

            if ((stroke_sequence[0] != 1)&&(stroke_sequence[0] != 3)&&(stroke_count == 2)&&(speed_sequence[0] == 0))
            {
                Serial.print("ALL YOU CAN EAT sequence!");
                all_you_can_eat_flag = 1;
            }

            // End of all you can eat detect

            if (loop_mode_seq == 2) // Well, we will rebuild playing sequence randomly in case of BUDDY mode
            {
                int random_sequence_buddy[20];
                int random_speed_buddy[20];

                for (i=0;i<20;i++)
                {
                    random_sequence_buddy[i] = stroke_sequence[random(0,stroke_count)];
                    
                    if (all_you_can_eat_flag)
                    {
                        random_speed_buddy[i] = 0;
                    } 
                    else
                    {
                        random_speed_buddy[i] = speed_sequence[random(0,stroke_count)];
                    } 
                }

                Serial.println("Sequence before BUDDY random remix: ");
                root.prettyPrintTo(Serial);

                for (i=0;i<20;i++)
                {
                    stroke_sequence[i] = random_sequence_buddy[i];
                    speed_sequence[i] = random_speed_buddy[i];
                }
                //Serial.println("New sequence after BUDDY random remix");
                //root.prettyPrintTo(Serial);

                loop_mode_seq = 1;
                SequenceBuddy["LoopMode"] = loop_mode_seq;
                Serial.print("Forcing loop mode to: ");
                Serial.println(loop_mode_seq);
            }

            Serial.println("Print of sequence object before dispatch: ");
            root.prettyPrintTo(Serial);

            // Time to dispatch strokes between BADDY and BUDDY
            bool my_turn = true;

            for (i=0;i<20;i++)
            {
                if (stroke_sequence[i] == 1){

                    stroke_seq[i] = 11;
                    Strokes_buddy.add(2);
                }
                else if (stroke_sequence[i] == 2){

                    if (my_turn){
                    stroke_seq[i] = 12;
                    Strokes_buddy.add(3);

                    my_turn = false;
                    }
                    else{
                        stroke_seq[i] = 1;
                        Strokes_buddy.add(12);
                        //stroke_seq_buddy[i] = 10;
                        my_turn = true;
                    }
                }
                else if (stroke_sequence[i] == 3){

                    stroke_seq[i] = 2;
                    Strokes_buddy.add(13);
                    //stroke_seq_buddy[i] = 10;
                }
                else if (stroke_sequence[i] == 4){

                    stroke_seq[i] = 14;
                    Strokes_buddy.add(5);
                    //stroke_seq_buddy[i] = 5;
                }
                else if (stroke_sequence[i] == 5){

                    if (my_turn){
                        stroke_seq[i] = 15;
                        Strokes_buddy.add(6);
                        //stroke_seq_buddy[i] = 6;
                        my_turn = false;
                    }
                    else{
                        stroke_seq[i] = 4;
                        Strokes_buddy.add(15);
                        //stroke_seq_buddy[i] = 10;
                        my_turn = true;
                    }
                }
                else if (stroke_sequence[i] == 6){

                    stroke_seq[i] = 5;
                    Strokes_buddy.add(16);
                    //stroke_seq_buddy[i] = 10;
                }
                else if (stroke_sequence[i] == 7){

                    stroke_seq[i] = 17;
                    Strokes_buddy.add(8);
                    //stroke_seq_buddy[i] = 8;
                }
                else if (stroke_sequence[i] == 8){
                    if (my_turn){
                        stroke_seq[i] = 18;
                        Strokes_buddy.add(9);
                        //stroke_seq_buddy[i] = 9;
                        my_turn = false;
                    }
                    else{
                        stroke_seq[i] = 7;
                        Strokes_buddy.add(18);
                        //stroke_seq_buddy[i] = 10;
                        my_turn = true;
                    }
                }
                else if (stroke_sequence[i] == 9){

                    stroke_seq[i] = 8;
                    Strokes_buddy.add(19);
                    //stroke_seq_buddy[i] = 10;
                }
                else if (stroke_sequence[i] == 0){

                    stroke_seq[i] = 0;
                    Strokes_buddy.add(0);
                    //stroke_seq_buddy[i] = 0;
                }
                else{
                    Serial.println("Wrong value in Sequence Data received");
                    return 0;
                }

                if ((all_you_can_eat_flag == 0)&&(speed_sequence[i] == 0))
                {
                    speed_seq[i] = 1; // set up the minimum period 
                }
                else
                {
                    speed_seq[i] = speed_sequence[i]; // Speeds remain the same!
                }

                Speeds_buddy.add(speed_seq[i]);

            }

            Serial.println("Dumping strokes of BADDY master: ");
/*
            for (i=0;i<20;i++)
            {
                Serial.print(stroke_seq[i]);
                Serial.print(", ");
            }
            Serial.println();

            Serial.print("Dumping speeds of BADDY master: ");

            for (i=0;i<20;i++)
            {
                Serial.print(speed_seq[i]);
                Serial.print(", ");
            }
            Serial.println();
*/
            Serial.println("Dumping Sequence object sent to BUDDY: ");
            //SequenceBuddy.prettyPrintTo(Serial);
/*
            if (!client_buddy.connect(host, port)) {
                Serial.println("connection failed");
                Serial.println("wait 5 sec...");
                delay(5000);
                return 0;
            }
*/
            // This will send the data to BADDY BUDDY server
            Serial.println("Sending data to BUDDY");
            String sequence_buddy;
            SequenceBuddy.printTo(sequence_buddy);
            //SequenceBuddy.prettyPrintTo(Serial);

            buddy_send_json(sequence_buddy);

            if (all_you_can_eat_flag)// enables to find shortest time period between 2 BADDY when BUDDY mode
            {
                delay(BUDDY_TIMER+SWITCH_TIMER+RETAINER_TIMER); // we wait until BUDDY processes the sequence
            }
            else
            {
                delay(BUDDY_TIMER);// we wait until BUDDY processes the sequence
            }
            
        }
        else // No BADDY BUDDY mode Here
        {
            Serial.println("NO BADDY BUDDY mode here...");
            for (i=0;i<20;i++)
            {
                //Store Json object values received in global variables that can be managed
                stroke_seq[i] = stroke_sequence[i];
                speed_seq[i] = speed_sequence[i];
                Serial.print("Stroke is: ");
                Serial.print(stroke_seq[i]);
                Serial.print(", Speed is: ");
                Serial.println(speed_seq[i]);
            }
        }

        Serial.println("*********** Printing values end ************");
    }

    printSquares();
    // End of parsing Json object and value extraction
    flag_new_sequence = true;

    return 1;

}


int set_play_mode_json(String Json)
{
    Serial.println("Getting data from wifi - set PlayMode endpoint triggered...");
    Serial.println(Json);

    // Parsing Json Object and extracting the values
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(Json);

    if (!root.success()){
        Serial.println("ERROR");
        return 0;
    }

    Serial.println("Parsing Json Object");

    PlayMode = root["PlayMode"]; // We change global PlayMode value
    Serial.println("PlayMode changed to value: ");
    Serial.println(PlayMode);

    // We fetch data from file system
    File file = SPIFFS.open("/BADDY_CONFIG_FILE", "r");

    if (!file){

        Serial.println("No BADDY config file exists");
        file.close();
        format_file_system_spifs();
        return false;

    }
    else {
        size_t size = file.size();
        if ( size == 0 ) {
            Serial.println("Config file empty !");
            file.close();
            format_file_system_spifs();
            return false;
        }
        else {

            std::unique_ptr<char[]> buf (new char[size]);
            file.readBytes(buf.get(), size);
            // Let's read BADDY configuration data form file system...
            // Create Json object to dump config file
            DynamicJsonBuffer jsonConfig;
            JsonObject& Config = jsonConfig.parseObject(buf.get());
            Serial.println("That's the config file dumped from Filesystem");
            // Config.prettyPrintTo(Serial); // For debug

            if (PlayMode == 0) {

                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedProfile"][17];

                Serial.println("Tripod Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);

            }
            else if (PlayMode == 1) {
                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedfloorProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedfloorProfile"][17];

                Serial.println("Floor Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);

            }
            else if (PlayMode == 2) {

                // DROP SHOTS
                DROP_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][0];
                DROP_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][1];
                DROP_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][2];
                DROP_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][3];
                DROP_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][4];
                DROP_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][5];
                //DRIVE SHOTS
                DRIVE_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][6];
                DRIVE_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][7];
                DRIVE_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][8];
                DRIVE_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][9];
                DRIVE_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][10];
                DRIVE_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][11];
                //CLEAR SHOTS
                CLEAR_LEFT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][12];
                CLEAR_LEFT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][13];
                CLEAR_CENTER_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][14];
                CLEAR_CENTER_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][15];
                CLEAR_RIGHT_SHOT_LEFT_MOTOR= Config["SpeedbuddyProfile"][16];
                CLEAR_RIGHT_SHOT_RIGHT_MOTOR= Config["SpeedbuddyProfile"][17];

                Serial.println("Buddy Speed profile loaded from file system");
                json_config = ""; // clean before print
                Config.printTo(json_config);
            }

            else{
                    Serial.println("ERROR: Wrong PlayMode value");
            }
        }
    }

    Serial.println("End of Playmode selection");
    file.close();
    dump_play_mode(); //Expose Playmode dump variable
    return 1;
}

void setup() {

    Serial.begin(9600);

    // Motor pins allocation
    MotorRight.attach(MOTOR_RIGHT_PIN);
    MotorRight.writeMicroseconds(STOP);
    MotorLeft.attach(MOTOR_LEFT_PIN);
    MotorLeft.writeMicroseconds(STOP);

    // Motor pins allocation end

    // Manage BADDY config file stored in File system
    SPIFFS.begin();
    //SPIFFS.format();
    if(!LoadConfig(0)){
        Serial.println("Abnormal configuration loading");
    }

    // Monitor station events
    stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
    // Call "onStationDisconnected" each time a station disconnects
    stationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);

    // End of BADDY config file management

    // Select if we have to set BADDY or BUDDY mode

    // TEST Wifi client mode

    Serial.println("Wifi client connecting...");
    Serial.print("ssid is: ");
    Serial.println(ssid);
    Serial.print("password is: ");
    Serial.println(password);

    //WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(),password.c_str());

    // add cors headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    int counter_connection=0;
    server.begin();
    Serial.println("HTTP server started BEFORE mDns assign");
    Serial.println("Server listening");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (counter_connection==30)
        {
            break;
        }
        counter_connection++;
    }

   if (counter_connection==30) // There is no hotspot around, we put BADDY in AP mode to be able to configure Hotspot
    {
        //WiFi.disconnect(true);
        Serial.println("No Hot spot to which BADDY can connect, so switching BADDY to AP mode");

        connect_mode = "AP";

        Serial.print("ssid init is: ");
        Serial.println(ssid_init);
        Serial.print("password is: ");
        Serial.println(password_init);

        // Sets wifi network in AP mode
        WiFi.softAPConfig(baddy_ip,baddy_gateway,baddy_subnet);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssid_init,password_init);

        //Serial.print("IP address is: ");
        //Serial.println(WiFi.localIP().toString());

        // We set DNS name to baddy.local
        Serial.println("Setting baddy.local as hostname");
        if (!MDNS.begin("baddy")) {             // Start the mDNS responder for baddy.local
            Serial.println("Error setting up MDNS responder!");
        }
        MDNS.addService("http", "tcp", 80);
        MDNS.update();
    }
    else
    {

        Serial.print("IP address is: ");
        Serial.println(WiFi.localIP().toString());

        connect_mode = "HotSpot";

        Serial.println("Starting mDNS to look up services...");
        if (!MDNS.begin("provisory",WiFi.localIP())) {             // Start the mDNS responder
            Serial.println("Error setting up MDNS responder!");
        }
        Serial.println("mDNS responder started");
        
        Serial.println("Sending mDNS query");

        int n = MDNS.queryService("baddy","tcp"); // Send out query for baddy tcp services
        Serial.println("mDNS query done");

        if (n==0){
            n = MDNS.queryService("baddy","tcp"); // check one more time
        }

        Serial.println();

        if (n == 0) {

            Serial.println("baddy not present, set baddy as service");
            
            Serial.println("mDNS responder started");
            MDNS.addService("http", "tcp", 80);
            //MDNS.addServiceTxt("baddy", "tcp", "0", "1");
            MDNS.addService("baddy","tcp", 80);

        } else {

            Serial.println("baddy already present, set buddy as service");
            Serial.println("Host: " + String(MDNS.hostname(0)));
            Serial.print("IP: ");
            Serial.println(MDNS.IP(0));
            BaddyMasterIP = MDNS.IP(0);

            MDNS.addService("http", "tcp", 80);
            //MDNS.addServiceTxt("buddy", "tcp", "0", "1");
            MDNS.addService("buddy", "tcp", 80);
            buddy.begin("http://"+BaddyMasterIP.toString()+"/buddy_is_here?buddy_ip="+ urlencode(WiFi.localIP().toString()));
            int http_code = buddy.GET();

            Serial.print("Http code value: ");
            Serial.println(http_code);

            String Payload = buddy.getString();
            Serial.println(Payload);

            buddy.end();

        }

        MDNS.update();

    }

    // Setting up servo motors after network declarations, because MDNS calls used to send dummy signals to the servo motors...
    ShuttleSwitch.attach(SHUTTLE_SWITCH_PIN);
    ShuttleSwitch.write(SWITCH_SHORT_POSITION);
    ShuttleRetainer.attach(SHUTTLE_RETAINER_PIN);
    ShuttleRetainer.write(RETAINER_DOWN_POSITION);
    // Servo motors declration and init end

// End of Test

////////////////////////////////////////////////////////////////////////////////
//////////////////////// SERVER END POINTS DECLARATION//////////////////////////
////////////////////////////////////////////////////////////////////////////////

    server.on("/buddy_is_here", HTTP_ANY, [](AsyncWebServerRequest *request){

        Serial.println("buddy_is_here end point called");
        buddy_ip_address = urldecode(request->arg("buddy_ip"));
        request->send(200,"text/plain","Buddy IP Address received");
        Serial.print("BUDDY IP Address is: ");
        Serial.println(buddy_ip_address);
        buddy_around=true;
        flag_print_smiley = true;

    });

    server.on("/network", HTTP_ANY, [](AsyncWebServerRequest *request){

        Serial.println("Network end point called");

        //List all parameters (Compatibility)
        String network = urldecode(request->arg("ssid"));
        String pass = urldecode(request->arg("pwd"));

        Serial.print("ssid is: ");
        Serial.println(network);
        Serial.print("pwd is: ");
        Serial.println(pass);       

        if (set_network(network.c_str(),pass.c_str()))
        {
            Serial.println("New Network parameters set!");
            flag_print_smiley = 1;
            request->send(200,"text/plain",json_config);
            //ESP.restart(); // let's connect to the new hotspot, removed the auto restart
        }
        else
        {
            Serial.println("Error in setting new network parameters:(((");
            flag_print_frown = 1;
            request->send(200,"application/json","{\"action\": \"set_network\", \"status\":\"error\",\"msg\": \"err in setting new network parameters\"}");
        }

    });

    server.on("/status", HTTP_ANY, [](AsyncWebServerRequest *request){

        DynamicJsonBuffer jsonBuffer;
        JsonObject& status = jsonBuffer.createObject();
        // check battery level

        int battery_level = analogRead(ANALOG_READ_BATTERY);

        // Debug battery level management
        Serial.print("Battery level read on A0: ");
        Serial.println(battery_level);
        // End debug

        if (battery_level > 195)
        {
            battery_level = 195;
        }
        if (battery_level < 135)
        {
            battery_level = 135;
        }

        battery_level = map(battery_level, 135, 195, 0, 100);


        status["Running"] = running_status;
        status["Battery"] = battery_level;
        status["Firmware_version"] = baddy_firmware_version;
        status["Connect_mode"] = connect_mode;
        JsonArray& Stats = status.createNestedArray("Stats");
        Stats.add(COUNTER_DROP_LEFT);
        Stats.add(COUNTER_DROP_CENTER);
        Stats.add(COUNTER_DROP_RIGHT);
        Stats.add(COUNTER_DRIVE_LEFT);
        Stats.add(COUNTER_DRIVE_CENTER);
        Stats.add(COUNTER_DRIVE_RIGHT);
        Stats.add(COUNTER_CLEAR_LEFT);
        Stats.add(COUNTER_CLEAR_CENTER);
        Stats.add(COUNTER_CLEAR_RIGHT);

        //Serial.println("Json status object dump");
        //status.prettyPrintTo(Serial);
        String json_status = ""; // clean before print
        status.printTo(json_status);
        //Serial.println(json_status);

        request->send(200,"text/plain",json_status);

    });

    server.on("/sequence", HTTP_ANY, [](AsyncWebServerRequest *request){

        AsyncWebParameter* p = request->getParam(0);
        JsonSequence = ""; // init first
        JsonSequence = p->value().c_str();

        DynamicJsonBuffer JsonBuffer;
        JsonObject& root = JsonBuffer.parseObject(JsonSequence);

        Serial.println("Parsing Json Object");
        const char* message_type = root["Type"];
        type_seq = message_type;
        Serial.println("Type: ");
        Serial.println(type_seq);

        // First of all, we detect ABORT messages
        if (type_seq == "ABT")
        {
            Serial.println("************************ABORT Received. Stopping motors...**********************");
            // Send statistics counters to GO BADDY app
            running_status=false;
            update_status();
            request->send(200,"text/plain",json_status);

            MotorLeft.writeMicroseconds(STOP);
            MotorRight.writeMicroseconds(STOP);

            Serial.println(JsonSequence);
            flag_new_sequence = false;
            all_you_can_eat_flag = 0; // release flag

        }
        else
        {
            request->send(200);
            //Serial.println(JsonSequence);
            flag_new_sequence = true;
        }
    });

    server.on("/config", HTTP_ANY, [](AsyncWebServerRequest *request){

        AsyncWebParameter* p = request->getParam(0);
        JsonSequence = ""; // init first
        JsonSequence = p->value().c_str();

        DynamicJsonBuffer JsonBuffer;
        JsonObject& root = JsonBuffer.parseObject(JsonSequence);

        Serial.println("Parsing Json Object");
        root.printTo(Serial);

        const char* message_type = root["Type"];
        type_seq = message_type;

        if (!root.success()){
            Serial.println("ERROR");
        }

        Serial.println("************************ Test motor speed request Received **********************");
        JsonArray& speed_sequence = root["testSpeeds"];
        int left_speed = speed_sequence[0];
        int right_speed = speed_sequence[1];
        Serial.print("Left motor speed: ");
        Serial.println(left_speed);
        Serial.print(", Right motor Speed is: ");
        Serial.println(right_speed);

        // Security
        if (right_speed > 1450)
        {
            right_speed = 1450;
        }
        if (left_speed > 1450)
        {
            left_speed = 1450;
        }

        MotorLeft.writeMicroseconds(left_speed);
        MotorRight.writeMicroseconds(right_speed);
        flag_test_shot = 1;

        request->send(200);

    });

    server.on("/set_speed", HTTP_ANY, [](AsyncWebServerRequest *request){

        AsyncWebParameter* p = request->getParam(0);
        JsonSequence = ""; // init first
        JsonSequence = p->value().c_str();

        if (set_speed_spifs(JsonSequence))
        {
            Serial.println("New Speeds uploaded!");
            flag_print_smiley = 1;
            request->send(200,"text/plain",json_playmode_dump);
        }
        else
        {
            Serial.println("Error in setting new speeds :(((");
            flag_print_frown=1;
            request->send(200,"application/json","{\"action\": \"set_speed\", \"status\":\"error\",\"msg\": \"err in setting new speeds\"}");
        }
    });

    server.on("/set_play_mode", HTTP_ANY, [](AsyncWebServerRequest *request){

        AsyncWebParameter* p = request->getParam(0);
        JsonSequence = ""; // init first
        JsonSequence = p->value().c_str();

        if (set_play_mode_json(JsonSequence))
        {
            Serial.println("New playmode Set!");
            flag_print_squares = 1;
        }
        else
        {
            Serial.println("Error in setting Playmode :((((");
            flag_print_frown = 1;
        }
        request->send(200,"text/plain",json_playmode_dump);
    });

    server.on("/format_file_system", HTTP_ANY, [](AsyncWebServerRequest *request){

        printSquares();

        if (format_file_system_spifs())
        {
            Serial.println("File system formatted");
            flag_print_smiley = 1;
        }
        else
        {
            Serial.println("Error in formating filesystem");
            flag_print_frown = 1;
        }
        request->send(200,"text/plain",json_config);
    });

    server.on("/playmodedump", HTTP_ANY, [](AsyncWebServerRequest *request){

        dump_play_mode();
        request->send(200,"text/plain",json_playmode_dump);
        flag_print_squares = 1;
    });

    server.on("/retainer_up", HTTP_ANY, [](AsyncWebServerRequest *request){

        if (set_retainer_up()){
            Serial.println("New retainer position set");
            flag_print_squares=1;
        }
        else
        {
            Serial.println("Error...");
            flag_print_frown = 1;
        }

        request->send(200,"text/plain",json_config);

    });

    server.on("/retainer_down", HTTP_ANY, [](AsyncWebServerRequest *request){

        if (set_retainer_down()){
            Serial.println("New retainer position set");
            flag_print_squares = 1;
        }
        else
        {
            Serial.println("Error...");
            flag_print_frown = 1;
        }

        request->send(200,"text/plain",json_config);

    });

    server.on("/switch_forward", HTTP_ANY, [](AsyncWebServerRequest *request){

        if (set_switch_forward()){
            Serial.println("New Switch position set");
            flag_print_smiley = 1;
        }
        else
        {
            Serial.println("Error...");
            flag_print_frown = 1;
        }

        request->send(200,"text/plain",json_config);
    });

    server.on("/switch_backward", HTTP_ANY, [](AsyncWebServerRequest *request){

        if (set_switch_backward()){
            Serial.println("New Switch position set");
            flag_print_smiley = 1;
        }
        else
        {
            Serial.println("Error...");
            flag_print_frown = 1;
        }

        request->send(200,"text/plain",json_config);
    });


///////////////////////////////////////////////////////////////////////////////
//////////////////// END OF SERVER END POINTS DECLARATION//////////////////////
///////////////////////////////////////////////////////////////////////////////


    //Wifi network config end

    // Led display Set up

    // Led set up and ready for pairing
    lc.shutdown(0,false);       //The MAX72XX is in power-saving mode on startup
    lc.setIntensity(0,2);      // Set the brightness to average value
    lc.clearDisplay(0);         // and clear the display
    printLevels();
    lc.clearDisplay(0);         // and clear the display

    // Led display set up end

    battery_level = analogRead(ANALOG_READ_BATTERY);
    Serial.print("Battery Level:");
    Serial.println(battery_level);
    //Serial.setDebugOutput(true);

    update_status();

    printSmiley(); // Hello BADDY

    //ArduinoOTA.setHostname(BADDY_ID);
    //ArduinoOTA.begin();

}

void loop() {

    //ArduinoOTA.handle();

    MDNS.update();

    if (flag_new_sequence)
    {
        sequencefetch(JsonSequence);
        ReadSequence();
    }

    if (flag_test_shot)
    {
        delay(1500);// wait till motors get their nominal speed
        Serial.println("Shooting shuttlecock...");
        servo_fire();
        flag_test_shot = 0;
        MotorRight.writeMicroseconds(STOP);
        MotorLeft.writeMicroseconds(STOP);
        Serial.println("Stop motors!");
    }

    if (flag_print_smiley)
    {
        printSmiley();
        flag_print_smiley = 0;
    }
    if (flag_print_frown)
    {
        printFrown();
        flag_print_frown = 0;
    }
    if (flag_print_squares)
    {
        printSquares();
        flag_print_squares = 0;
    }

}
