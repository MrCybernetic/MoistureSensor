/*
  __  __       _     _                    _____           _ _           _              __      _____  
 |  \/  |     (_)   | |                  |_   _|         | (_)         | |             \ \    / / _ \ 
 | \  / | ___  _ ___| |_ _   _ _ __ ___    | |  _ __   __| |_  ___ __ _| |_ ___  _ __   \ \  / / | | |
 | |\/| |/ _ \| / __| __| | | | '__/ _ \   | | | '_ \ / _` | |/ __/ _` | __/ _ \| '__|   \ \/ /| | | |
 | |  | | (_) | \__ \ |_| |_| | | |  __/  _| |_| | | | (_| | | (_| (_| | || (_) | |       \  / | |_| |
 |_|  |_|\___/|_|___/\__|\__,_|_|  \___| |_____|_| |_|\__,_|_|\___\__,_|\__\___/|_|        \/   \___/ 

  By Florian LOBERT, 2019

  Reducing power consumption with the help of this website: https://www.re-innovation.co.uk/docs/sleep-modes-on-attiny85/
 */
#define F_CPU 8000000  // This is used by delay.h library
#include <stdlib.h>
#include <EEPROM.h> 
#include <avr/io.h>        // Adds useful constants
#include <util/delay.h>    // Adds delay_ms and delay_us functions
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <Adafruit_NeoPixel.h>

// Routines to set and clear bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// Pins
const int analogPin = A2;
const int rgbPin = 2;
const int MoistureSensorVoltagePin = 1;

// Variables
float moistureValue = 0; //0-1000
int red=0; //0-255
int green=0; //0-255

// "Strip"
Adafruit_NeoPixel myLed = Adafruit_NeoPixel(1, rgbPin, NEO_GRB + NEO_KHZ800);

//************ USER PARAMETERS***********************
//******************MODE*****************************
//const int deviceType = 85;  // 45 = ATTiny45, NO serial output, 85 = AtTiny85, with serial output
  

 
// Variables for the Sleep/power down modes:
volatile boolean f_wdt = 1;

void setup() {
  pinMode(rgbPin, OUTPUT);
  pinMode(MoistureSensorVoltagePin, OUTPUT);
  pinMode(analogPin, INPUT);
  myLed.begin();
  myLed.show(); // Initialize to 'off'

  setup_watchdog(9); 
}

void loop() {
  if (f_wdt==1) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
    f_wdt=0;       // reset flag
    
    // Read the moisture value and calculate green and red value
    digitalWrite(MoistureSensorVoltagePin,HIGH);
    _delay_ms(10);
    moistureValue = analogRead(analogPin);
    green = int((255.0*moistureValue/1000.0)*0.5);
    red = int((255.0-255.0*moistureValue/1000.0)*0.5);
    _delay_ms(10);
    digitalWrite(MoistureSensorVoltagePin,LOW);
    // Set the color of the RGB Led for 1s
    myLed.setPixelColor(0, myLed.Color(red, green ,0));
    myLed.show();
    delay(1000);
    // Turn off the led
    myLed.setPixelColor(0, myLed.Color(0, 0, 0));
    myLed.show();
    // Set the ports to be inputs - saves more power
    pinMode(rgbPin, INPUT);
    pinMode(MoistureSensorVoltagePin, INPUT);
    system_sleep();  // Send the unit to sleep
    // Set the ports to be output again
    pinMode(rgbPin, OUTPUT);
    pinMode(MoistureSensorVoltagePin, OUTPUT);  
  }   
}

// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
 
  sleep_mode();                        // System actually sleeps here
 
  sleep_disable();                     // System continues execution here when watchdog timed out 
  
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
  
}
 
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
 
  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;
 
  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}
  
// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  f_wdt=1;  // set global flag
}
