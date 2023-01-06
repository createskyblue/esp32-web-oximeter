#ifndef CAL_BPM_SPO2_H
#define CAL_BPM_SPO2_H
#include <Arduino.h>
#include "main.h"
#include <Wire.h>
#include "MAX30105.h" //sparkfun MAX3010X library

//CUSTOM DEFINITION
//#define MAX30105 //if you have Sparkfun's MAX30105 breakout board , try #define MAX30105

#define TIMETOBOOT 3000 // wait for this time(msec) to output SpO2
#define SCALE 88.0 //adjust to display heart beat and SpO2 in Arduino serial plotter at the same time
#define SAMPLING 1 //if you want to see heart beat more precisely , set SAMPLING to 1
#define FINGER_ON 50000 // if ir signal is lower than this , it indicates your finger is not on the sensor
#define MINIMUM_SPO2 80.0
#define MAX_SPO2 100.0
#define MIN_SPO2 80.0

#define SUM_CYCLE 100

#define FINGER_ON 50000 // if ir signal is lower than this , it indicates your finger is not on the sensor
#define LED_PERIOD 100 // light up LED for this period in msec when zero crossing is found for filtered IR signal
#define MAX_BPS 180
#define MIN_BPS 45

extern double eSpO2;
extern double Ebpm;

void sleepSensor();
void initSensor();
void cal_BPM_SpO2_task(void *pvParameters);
#endif
