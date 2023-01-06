//https://github.com/coniferconifer/ESP32_MAX30102_simple-SpO2_plotter
//Apache License 2.0
#include "cal_BPM_SpO2.h"
extern MAX30105 particleSensor;

double aveRed = 0;//DC component of RED signal
double aveIr = 0;//DC component of IR signal
double sumIrRMS = 0; //sum of IR square
double sumRedRMS = 0; // sum of RED square

int Num = SUM_CYCLE; //calculate SpO2 by this sampling interval
double eSpO2 = 95.0;//initial value of estimated SpO2
double fSpO2 = 0.7; //filter factor for estimated SpO2
double fRate = 0.95; //low pass filter for IR/red LED value to eliminate AC component

double SpO2 = 0; //raw SpO2 before low pass filtered
double Ebpm;//estimated Heart Rate (bpm)

uint32_t ir, red;//raw data
float ir_forWeb = 0;
float red_forWeb = 0;

void initSensor() {
    //Setup to sense a nice looking saw tooth on the plotter
    uint8_t ledBrightness = 0x7F; //Options: 0=Off to 255=50mA
    uint8_t sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
    uint8_t ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    //Options: 1 = IR only, 2 = Red + IR on MH-ET LIVE MAX30102 board
    int sampleRate = 200; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411; //Options: 69, 118, 215, 411
    int adcRange = 16384; //Options: 2048, 4096, 8192, 16384
    // Set up the wanted parameters
    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
}

//
// Heart Rate Monitor by interval of zero crossing at falling edge
// max 180bpm - min 45bpm
double HRM_estimator(double fir, double aveIr)
{
    static double fbpmrate = 0.95; // low pass filter coefficient for HRM in bpm
    static uint32_t crosstime = 0; //falling edge , zero crossing time in msec
    static uint32_t crosstime_prev = 0;//previous falling edge , zero crossing time in msec
    static double bpm = 60.0;
    static double ebpm = 60.0;
    static double eir = 0.0; //estimated lowpass filtered IR signal to find falling edge without notch
    static double firrate = 0.85; //IR filter coefficient to remove notch , should be smaller than fRate
    static double eir_prev = 0.0;

    // Heart Rate Monitor by finding falling edge
    eir = eir * firrate + fir * (1.0 - firrate); //estimated IR : low pass filtered IR signal
    if (((eir - aveIr) * (eir_prev - aveIr) < 0) && ((eir - aveIr) < 0.0)) { //find zero cross at falling edge
        crosstime = millis();//system time in msec of falling edge
        //Serial.print(crosstime); Serial.print(","); Serial.println(crosstime_prev);
        if (((crosstime - crosstime_prev) > (60 * 1000 / MAX_BPS)) && ((crosstime - crosstime_prev) < (60 * 1000 / MIN_BPS))) {
            bpm = 60.0 * 1000.0 / (double)(crosstime - crosstime_prev); //get bpm
            //   Serial.println("crossed");
            ebpm = ebpm * fbpmrate + (1.0 - fbpmrate) * bpm;//estimated bpm by low pass filtered
        }
        else {
            //Serial.println("faild to find falling edge");
        }
        crosstime_prev = crosstime;
    }
    eir_prev = eir;
    return (ebpm);
}

void cal_BPM_SpO2_task(void* pvParameters) {
    unsigned int loopCnt = 0;
    log_i("Initializing MAX3010X...");
    // Initialize sensor
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
    {
        log_e("MAX3010X was not found.");
#ifdef MAX30105
        log_e("Please check wiring/power at MAX30105 board.");
#else
        log_e("Please check wiring/power/solder jumper at MH-ET LIVE MAX30102 board. ");
#endif
        log_e("restarting...");
        abort();
    }
    //初始化传感器
    initSensor();
    //事件循环
    while (1) {
        double fred, fir; //floating point RED ana IR raw values
        particleSensor.check(); //Check the sensor, read up to 3 samples
        while (particleSensor.available()) {//do we have new data

#ifdef MAX30105
            red = particleSensor.getFIFORed(); //Sparkfun's MAX30105
            ir = particleSensor.getFIFOIR();  //Sparkfun's MAX30105
#else
            red = particleSensor.getFIFOIR(); //why getFOFOIR output Red data by MAX30102 on MH-ET LIVE breakout board
            ir = particleSensor.getFIFORed(); //why getFIFORed output IR data by MAX30102 on MH-ET LIVE breakout board
#endif
            loopCnt++;
            fred = (double)red;
            fir = (double)ir;
            aveRed = aveRed * fRate + (double)red * (1.0 - fRate);//average red level by low pass filter
            aveIr = aveIr * fRate + (double)ir * (1.0 - fRate); //average IR level by low pass filter
            sumRedRMS += (fred - aveRed) * (fred - aveRed); //square sum of alternate component of red level
            sumIrRMS += (fir - aveIr) * (fir - aveIr);//square sum of alternate component of IR level

            Ebpm = HRM_estimator(fir, aveIr); //Ebpm is estimated BPM

            if ((loopCnt % SAMPLING) == 0) {//slow down graph plotting speed for arduino Serial plotter by decimation
                if (millis() > TIMETOBOOT) {
                    //        float ir_forGraph = (2.0 * fir - aveIr) / aveIr * SCALE;
                    //        float red_forGraph = (2.0 * fred - aveRed) / aveRed * SCALE;
                    float ir_forGraph = 2.0 * (fir - aveIr) / aveIr * SCALE + (MIN_SPO2 + MAX_SPO2) / 2.0;
                    float red_forGraph = 2.0 * (fred - aveRed) / aveRed * SCALE + (MIN_SPO2 + MAX_SPO2) / 2.0;
                    //trancation to avoid Serial plotter's autoscaling
                    if (ir_forGraph > MAX_SPO2) ir_forGraph = MAX_SPO2;
                    if (ir_forGraph < MIN_SPO2) ir_forGraph = MIN_SPO2;
                    if (red_forGraph > MAX_SPO2) red_forGraph = MAX_SPO2;
                    if (red_forGraph < MIN_SPO2) red_forGraph = MIN_SPO2;
                    //        Serial.print(red); Serial.print(","); Serial.print(ir);Serial.print(".");
                    if (ir < FINGER_ON) eSpO2 = MINIMUM_SPO2; //indicator for finger detached
                    //更新给网页端共享的数值
                    ir_forWeb = ir_forGraph;
                    red_forWeb = red_forGraph;
// #define PRINT
// #ifdef PRINT
//         //Serial.print(bpm);// raw Heart Rate Monitor in bpm
//         //Serial.print(",");
//                     Serial.print(Ebpm);// estimated Heart Rate Monitor in bpm
//                     Serial.print(",");
//                     //        Serial.print(Eir - aveIr);
//                     //        Serial.print(",");
//                     Serial.print(ir_forGraph); // to display pulse wave at the same time with SpO2 data
//                     Serial.print(","); Serial.print(red_forGraph); // to display pulse wave at the same time with SpO2 data
//                     Serial.print(",");
//                     Serial.print(eSpO2); //low pass filtered SpO2
//                     Serial.print(","); Serial.print(85.0); //reference SpO2 line
//                     Serial.print(","); Serial.print(90.0); //warning SpO2 line
//                     Serial.print(","); Serial.print(95.0); //safe SpO2 line
//                     Serial.print(","); Serial.println(100.0); //max SpO2 line

// #else
//                     Serial.print(fred); Serial.print(",");
//                     Serial.print(aveRed); Serial.println();
//                     //    Serial.print(fir);Serial.print(",");
//                     //   Serial.print(aveIr);Serial.println();
// #endif
                }
            }
            if ((loopCnt % Num) == 0) {
                double R = (sqrt(sumRedRMS) / aveRed) / (sqrt(sumIrRMS) / aveIr);
                // Serial.println(R);
                //#define MAXIMREFDESIGN
#ifdef MAXIMREFDESIGN
      //https://github.com/MaximIntegratedRefDesTeam/RD117_ARDUINO/blob/master/algorithm.h
      //uch_spo2_table is approximated as  -45.060*ratioAverage* ratioAverage + 30.354 *ratioAverage + 94.845 ;
                SpO2 = -45.060 * R * R + 30.354 * R + 94.845;
                //      SpO2 = 104.0 - 17.0*R; //from MAXIM Integrated https://pdfserv.maximintegrated.com/en/an/AN6409.pdf
#else
#define OFFSET 0.0
                SpO2 = -23.3 * (R - 0.4) + 100 - OFFSET; //http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
                if (SpO2 > 100.0) SpO2 = 100.0;
#endif
                eSpO2 = fSpO2 * eSpO2 + (1.0 - fSpO2) * SpO2;//low pass filter
                //  Serial.print(SpO2);Serial.print(",");Serial.println(eSpO2);
                sumRedRMS = 0.0; sumIrRMS = 0.0; loopCnt = 0;//reset mean square at every interval
                break;
            }
            particleSensor.nextSample(); //We're finished with this sample so move to next sample
        }
    }
}