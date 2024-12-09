#include <Arduino.h>
//Pinout
#define LED_MOTOR_ON_PIN        14
#define LED_DRY_RUN_CUTOFF_PIN  16
#define LED_LOW_HIGH_CUTOFF_PIN 12

#define RELAY_MOTOR_PIN         13
#define RELAY_STARTER_PIN       9

#define WATERLEVEL_LOW_PIN      4
#define WATERLEVEL_FULL_PIN     2
#define WATERLEVEL_DRYRUN_PIN   5

#define MANUAL_START_BUTTON_PIN  9
#define SETUP_BUTTON_PIN         10

#define VOLTAGE_SENSOR_PIN      A0
//constants
unsigned long DRYRUN_TIMOUT = 60; //seconds
const int MOTOR_START_MIN_VOLTAGE_CUTOFF = 185;
const int MOTOR_START_MAX_VOLTAGE_CUTOFF = 245;
const int MOTOR_RUN_MIN_VOLTAGE_CUTOFF = 165;
const int VOLTAGE_CUTOFF_RETRY_TIME = 600;   // to be edited
const int STARTER_SWITCH_DURATION = 5000; //duration in which the starter button is pressed, give 0 to disable

//variables
bool motor_running = false; //global variable to check the motor if it is currently pumping or not
int motor_running_status = 0; // 0 for not running, 1 for just started( in this case dryrun is not checked), 2 running phase (check for dryrun)
unsigned long starting_millis;
unsigned long voltage_cutoff_millis;
bool dryrun_cutoff_status = false;
bool voltage_cutoff_status = false;
bool starter_status = false;

void setup(){
  //setting up Pin Modes
  pinMode(LED_MOTOR_ON_PIN, OUTPUT);
  pinMode(LED_DRY_RUN_CUTOFF_PIN, OUTPUT);
  pinMode(LED_LOW_HIGH_CUTOFF_PIN, OUTPUT);
  pinMode(RELAY_MOTOR_PIN, OUTPUT);
  pinMode(RELAY_STARTER_PIN, OUTPUT);

  pinMode(WATERLEVEL_LOW_PIN, INPUT);
  pinMode(WATERLEVEL_FULL_PIN, INPUT);
  pinMode(WATERLEVEL_DRYRUN_PIN, INPUT);

  pinMode(MANUAL_START_BUTTON_PIN, INPUT);
  pinMode(SETUP_BUTTON_PIN, INPUT);

  //ensuring all pins are low
  digitalWrite(RELAY_MOTOR_PIN, LOW);
  digitalWrite(RELAY_STARTER_PIN, LOW);
  digitalWrite(LED_MOTOR_ON_PIN, LOW);
  digitalWrite(LED_DRY_RUN_CUTOFF_PIN, LOW);
  digitalWrite(LED_LOW_HIGH_CUTOFF_PIN, LOW);
}
int check_voltage(){
  return analogRead(VOLTAGE_SENSOR_PIN);
}
void start_motor(){
  if (dryrun_cutoff_status) return;
  if (voltage_cutoff_status) {
    if(millis() - voltage_cutoff_millis < VOLTAGE_CUTOFF_RETRY_TIME){
      return;
    }
    else {
      voltage_cutoff_status = false;
    }
  }
  if (check_voltage() >= MOTOR_START_MIN_VOLTAGE_CUTOFF && check_voltage() <= MOTOR_START_MAX_VOLTAGE_CUTOFF){
    motor_running = true;
    motor_running_status = 1;
    starting_millis = millis();
    digitalWrite(RELAY_MOTOR_PIN,HIGH);
    digitalWrite(LED_MOTOR_ON_PIN, HIGH);
    digitalWrite(LED_LOW_HIGH_CUTOFF_PIN,LOW);
    if (STARTER_SWITCH_DURATION != 0){
      digitalWrite(RELAY_STARTER_PIN,HIGH);
      starter_status = true;
    }
    
  }
  else {
    digitalWrite(LED_LOW_HIGH_CUTOFF_PIN,HIGH);
  }
}
void stop_motor(){
  motor_running = false;
  motor_running_status = 0;
  digitalWrite(LED_MOTOR_ON_PIN, LOW);
  digitalWrite(RELAY_MOTOR_PIN,LOW);
  digitalWrite(RELAY_STARTER_PIN,LOW);
  starter_status = false;
}
void loop(){
  bool water_low = digitalRead(WATERLEVEL_LOW_PIN);
  bool water_full = digitalRead(WATERLEVEL_FULL_PIN);
  bool motor_dry_run = digitalRead(WATERLEVEL_DRYRUN_PIN);


  if (!motor_running){
    if (!water_low && !water_full){
      start_motor();
    }
    else if (!water_low && water_full){
      //error condition
    }
  }
  else {  //motor is running
    if (water_full){
      stop_motor();
    }
    else if (check_voltage() < MOTOR_RUN_MIN_VOLTAGE_CUTOFF){
      digitalWrite(LED_LOW_HIGH_CUTOFF_PIN,HIGH);
      voltage_cutoff_millis = millis();
      voltage_cutoff_status = true;
      stop_motor();
    }
    else if (motor_running_status == 1) {
      if (millis() - starting_millis >= DRYRUN_TIMOUT){
        motor_running_status = 2;
      }
    }
    else if (motor_running_status == 2){
      if (!motor_dry_run){
        stop_motor();
        digitalWrite(LED_DRY_RUN_CUTOFF_PIN,HIGH);
        dryrun_cutoff_status = true;
      }
    }
    if (starter_status && (millis() - starting_millis >= STARTER_SWITCH_DURATION)){
      starter_status = false;
      digitalWrite(RELAY_STARTER_PIN,LOW);
    }
  }

  delay(300);
  if (!motor_running && digitalRead(MANUAL_START_BUTTON_PIN)){
    voltage_cutoff_status = false;
    dryrun_cutoff_status = false;
    start_motor();
  }
  

}