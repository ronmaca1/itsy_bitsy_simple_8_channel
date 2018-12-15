//<debuuging defines>
#define _DEBUG_ // for serial debugging
//#undef    _DEBUG_
//</debuging defines>

#define REFMV HIGH
#define REFVCC LOW
#define SCALEMV 1500
#define SCALEVCC 5000

//<pin defines>

//digital
#define HEAT_1 5
#define HEAT_2 7
#define HEAT_3 9
#define HEAT_4 10
#define REFSWITCH 12
//analog
#define B1OXYGEN A0
#define B2OXYGEN A1
#define B3OXYGEN A2
#define B4OXYGEN A3
#define TPOS A4
#define REF_5V_2 A5 // map/maf
#define REF_5V_3 A6
#define REF_5V_4 A7
//</pin defines>

#include <Wire.h>
#include <stdio.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

unsigned long startmillis = 0;
unsigned long currentmillis = 0;

/*
long mymap(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
}
*/
RTC_PCF8523 rtc;
void setup()
{
    // put your setup code here, to run once:
    //
    // set analog reference first
    analogReference(EXTERNAL);
    // disable input buffers on ADC pins,
    // per datasheet page 43
    DIDR0 |= _BV(ADC0D) | _BV(ADC1D) | _BV(ADC2D) | _BV(ADC3D),
        _BV(ADC4D) | _BV(ADC5D) | _BV(ADC6D) | _BV(ADC7D);

    // <pins in use>
    digitalWrite(REFSWITCH, LOW);
    pinMode(REFSWITCH, OUTPUT);
    pinMode(HEAT_1, INPUT);
    pinMode(HEAT_2, INPUT);
    pinMode(HEAT_3, INPUT);
    pinMode(HEAT_4, INPUT);
    // </pins in use>

    // real time clock check
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");

        while (1) // no clock, die here.
            ;
    }

//delay(20); // a short delay to let things stabilize
#ifdef _DEBUG_

    Serial.begin(57600);
    Serial.print("Initializing SD card...");
#endif
    // see if the card is present and can be initialized:
    if (!SD.begin(10))
    {
#ifdef _DEBUG_
        Serial.println("Card failed, or not present");
// don't do anything more:
#endif

        while (1)
            ; // hang till power down and card inserted
    }
#ifdef _DEBUG_
    Serial.println("card initialized.");
#endif
    String StartString = "";
    StartString += "Startup";

    startmillis = millis();
}

void loop()
{
    // put your main code here, to run repeatedly:

    int ref_5v_2, tpos, b1oxygen, b2oxygen, b3oxygen, b4oxygen, temp;

    String dataString = "";
    currentmillis = millis();
    /*    
#ifdef _TIMESTAMP_PER_MINUTE_

    DateTime now = rtc.now();
    dataString += now.unixtime();
    dataString += ",";

#endif
#ifdef _TIMESTAMP_PER_POWERUP_
    powerup = 0;
    DateTime now = rtc.now();
    dataString += now.unixtime();
    dataString += ",";

#endif */

    dataString += String(millis() - startmillis);
    dataString += String(",");

    // <get us some heater info>
    dataString += String(digitalRead(HEAT_1));
    dataString += String(",");
    dataString += String(digitalRead(HEAT_2));
    dataString += String(",");
    dataString += String(digitalRead(HEAT_3));
    dataString += String(",");
    dataString += String(digitalRead(HEAT_4));
    dataString += String(",");

    // </get us some heater info>

    //<get us some o2 info>
    // get 4 samples and then average them
    dataString += String(adcaverage(B1OXYGEN, SCALEMV, REFMV));
    dataString += String(",");
    //</get us some o2 info>

    //<get us some o2 info>
    // get 4 samples and then average them
    dataString += String(adcaverage(B2OXYGEN, SCALEMV, REFMV));
    dataString += String(",");
    //</get us some o2 info>

    //<get us some o2 info>
    // get 4 samples and then average them
    dataString += String(adcaverage(B3OXYGEN, SCALEMV, REFMV));
    dataString += String(",");
    //</get us some o2 info>

    //<get us some o2 info (gain of 2(4))>
    // get 4 samples and then average them
    dataString += String(adcaverage(B4OXYGEN, SCALEMV, REFMV));
    dataString += String(",");
    //</get us some o2 info (gain of 2(4))>

    // <get us some throttle info>
    // get 4 samples and then average them
    dataString += String(adcaverage(TPOS, SCALEVCC, REFVCC));
    dataString += String(",");
    // </get us some throttle info>

    // <get us some REF_5V_2 channel>
    // get 4 samples and then average them
    dataString += String(adcaverage(REF_5V_2, SCALEVCC, REFVCC));
    dataString += String(",");
    // </get us some unused channel>

    // <get us some REF_5V_3 channel>
    // get 4 samples and then average them
    dataString += String(adcaverage(REF_5V_3, SCALEVCC, REFVCC));
    dataString += String(",");
    // </get us some unused channel)>

    // <get us some REF_5V_4 channel>
    // get 4 samples and then average them
    dataString += String(adcaverage(REF_5V_4, SCALEVCC, REFVCC)); // last channel no comma appended
    // </get us some unused channel>

    // <SD card setup>

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SD.open("datalog.csv", FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile)
    {
        dataFile.println(dataString);
        dataFile.close();
#ifdef _DEBUG_
        // print to the serial port too:
        Serial.println(dataString);
#endif
    }
    // if the file isn't open, pop up an error:
    else
    {
#ifdef _DEBUG_
        Serial.println("error opening datalog.csv");
#endif
    }

    while (millis() - currentmillis < 100)
        ; // do every 100 millis aka 10 sample / sec.
#ifdef _TIMESTAMP_PER_MINUTE_
    if (loopcount <= 600)
    {
        loopcount++;
    }
    else
    {
        loopcount = 0; //reset each minute
        // see beginning of loop for the usage of this}
    }
#endif

}

int adcaverage(int channel, int scale, int reference)
{ // first, set aref
    digitalWrite(REFSWITCH, reference);
    // sum 4 samples, divide by 4 giving average
    int temp = 0;
    // get 4 samples and then average them
    temp += analogRead(channel);
    temp += analogRead(channel);
    temp += analogRead(channel);
    temp += analogRead(channel);
    temp = temp >> 2;
    return map(temp, 0, 1023, 0, scale);
}
