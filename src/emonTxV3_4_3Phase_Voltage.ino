/*
emonTx V3/Shield CT1234 + 3-phase Voltage example

An example sketch for the emontx module for
3-phase electricity monitoring, with 4 current transformers

and 1 only voltage transformer.

Part of the openenergymonitor.org project
Licence: GNU GPL V3

Authors: Glyn Hudson, Trystan Lea
Builds upon JeeLabs RF12 library and Arduino
also "Solar PV power diversion with emonTx" by MartinR
Extended for 3-phase operation: Robert Wall
V0.1  7/11/2013    Derived from emonTx_CT123_3Phase_Voltage.ino
V0.2  28/1/2015    Altered to use low-pass filter and subtract the offset, to remove filter settling time.
V0.3  1/5/2015     Dependency on JeeLib removed, replaced by in-line code.
V0.4  16/6/2015    Array addressing bug solved.
V0.5  2/8/2015     Added option to select emonTx V3 / emonTx Shield.
V0.6  19/9/2015    Watchdog timer reinstated, transmitted Vrms scaled (x 100) for compatibility.
V0.7  17/11/2015   Comment added for #defines for RFu 328 (RFMSELPIN & RFMIRQPIN), RFM code separated out into "rfm.ino"
V0.8  8/5/2016     Added temperature measurement, returns "3000" for faulty sensor or "3020" for device error.
V0.9  5/11/2016    Added temperature measurement fault code "3010" for checksum error.
V0.10 31/1/2017    Added ESP  WiFi module support - Glyn Hudson. Temp fault codes renumbered: 3000 = Faulty sensor, sensor broken or disconnected.
                   3010 = Sensor has never been detected since power-up/reset. 3020	= Sensor returned an out-of-range value.
                   850 although in range, might indicate a wiring fault.
                   Pulse counting added. (RW)
V0.11 13/02/2017  Changed newline line endings for newline + carriage return line endings https://community.openenergymonitor.org/t/3-phase-serial-connection-emonhub-query/3099?u=pb66
V0.12 14/08/2017  Added serial direct connection and second nodeID with extended data

emonTx documentation: http://wiki.openenergymonitor.org/index.php/Main_Page#Monitoring_Nodes
emonTx firmware code explanation: http://openenergymonitor.org/emon/modules/emontx/firmware
emonTx / emonTx Shield calibration instructions: http://openenergymonitor.org/emon/buildingblocks/calibration

REQUIRES in the sketch directory
rfm.ino                                          // the low-level routines for the RF Modules

REQUIRES in [Arduino]/libraries
Arduino.h
WProgram.h
avr/wdt.h                                        // the UNO bootloader
OneWire.h                                        // http://www.pjrc.com/teensy/td_libs_OneWire.html
DallasTemperature.h                              // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip

Does NOT require JeeLib
Does NOT require EmonLib
=============================================================================================

Extended to allow the voltage measurement of a single phase to be used to generate approximate indications of
power (real and apparent) and phase angle for the other two phases of a 3-phase installation.

NOTE: This sketch is for a 4-wire connection at 50 Hz, measuring voltage Line-Neutral, and assuming CT1 - 3 current
measurements are on the incoming lines, and CT4 is on a load/infeed connected line-neutral.
A single AC-AC adapter IS REQUIRED and must be connected between L1 and N.
CT1 must be on L1, CT2 must be on L2 and CT3 must be on L3.
The phase rotation must be L1 - L2 - L3, though which physical phase is "L1" is arbitrary.

The measured voltage of phase one is used immediately for the calculations for its own phase, and recorded in an
array and retrieved later to be in the calculations for the remaining phases.

N.B. "Phase shifted" means a small adjustment by interpolation to the voltage waveform to accommodate small
( < 10 degrees) phase shifts in the transformers etc. "Delayed" means a delay of the voltage samples by
approx 1/3 or 2/3 cycle.
Without the 4th c.t. in use, this sketch records approx 24 sample sets per cycle at 50 Hz.

POSSIBLE SOURCES OF ERROR
This method is an approximation. It assumes that the voltages of the three phases remain identical and the angles
between the voltage vectors remain accurately 120 degrees apart. The lower the fault level of the supply (i.e. the
higher the impedance), the greater the change in the true voltage will be as a result of load changes, and therefore
the inaccuracies that result from these approximations will be greater also.
If the mains frequency changes, this will appear as a change in real power and power factor for L2 and more so for L3.

CALIBRATION
The fourth channel may be used, for example, for a PV input. Include the line " #define CT4LINE " if the fourth C.T.
is to be used. Adding or removing CT4 drastically changes the phase calibration for L2 and L3.

You need set only the calibration for the model of emonTx in use:
Adjust Vcal = 276.9 so that the correct voltage for L1 is displayed.
Adjust 1 = 90.9 so that the correct current for L1 is displayed.
Do the same for Ical2 & Ical3.
Connect a pure resistive load (e.g. a heater) to L1 and adjust Phasecal1 to display a power factor of 1.00. Phasecal1
should be within the range 0 - 2.
Do the same for L2 and L3. If it not possible to keep Phasecal2 and Phasecal3 within the range 0 - 1 , it will be
necessary to change "#define PHASE2 6" and/or "#define PHASE3 14". If either of these are changed, both Phasecal2
& Phasecal3 will need adjusting.

If CT 4 is in use, adjust Phasecal4 likewise. As CT4 has a different burden, it is to be expected that it will not
have the same Phasecal as the other CT on that phase.

[Note: It may be easier to calibrate on a single phase only: With all 3 (or 4) CTs on the same wire, set each Ical so that
all show the same correct value, then adjust Phasecal1 to display a power factor of 1.0, and Phasecal 2 & 3 to show
a power factor of exactly -0.5, and a real power exactly -0.5 times that of Line 1.]

RF OUTPUT POWER - RFPWR  (RFM69CW Only):

When powered via the AC adapter only, the r.f. output power is limited and depends on the minimum supply voltage.
The following is a rough guide. If correct operation is impossible (i.e. the emonTx continually resets) then
an external 5 V supply must be used.

The control range for transmitter power is 0x80 to 0x9F = -18 dBm to +13 dBm.
NOTE: Ensure a correctly matched antenna is used when operating at or near maximum power, otherwise the module may be damaged.

The RFM12B equivalent power is: 0x99 ( +7 dBm )
The maximum output power before the AC power supply might fail due to excessive current being drawn is:

Min supply     maximum power
 voltage
  235 V        -4 dBm (0x8E)
  230 V       -10 dBm (0x88)

Reducing the output power below -10 dBm has very little effect on the minimum supply voltage necessary.

"#define RFPWR" is at or near line 183 below.

emonhub.conf node decoder settings for this sketch:

[[11]]
    nodename = 3phase
    [[[rx]]]
       names = powerL1, powerL2, powerL3, power4, Vrms, temp1, temp2, temp3, temp4, temp5, temp6, pulse
       datacodes = h,h,h,h,h,h,h,h,h,h,h,L
       scales = 1,1,1,1,0.01,0.1,0.1,0.1,0.1,0.1,0.1,1
       units =W,W,W,W,V,C,C,C,C,C,C,p

[[15]]
     nodename = 3phase_extended
     [[[rx]]]
        names = apparentPowerL1, apparentPowerL2, apparentPowerL3, apparentPower4, Current1, Current2, Current3, Current4, powerFactor1, powerFactor2, powerFactor3, powerFactor4
        datacodes = h,h,h,h,h,h,h,h,h,h,h,h
        scales = 1,1,1,1,0.01,0.01,0.01,0.01,0.1,0.1,0.1,0.1
        units =W,W,W,W,A,A,A,A,C,C,C,C

*/
// #define DEBUGGING                             // enable this line to include debugging print statements
                                                 //  This is turned off when SERIALOUT or EMONESP (see below) is defined.

// #define SERIALPRINT                              // include 'human-friendly' print statement for commissioning - comment this line to exclude.
//#define EMONESP                                  //  This is turned off when SERIALOUT or EMONESP (see below) is defined.
#define DIRECTCONNECT

#define USEPULSECOUNT                            // include the ability to count pulses. Comment this line if pulse counting is not required.
#define PULSEINT 1                               // Interrupt no. for pulse counting: EmonTx V2 = 1, EmonTx V3.2(RFu) = 0, EmonTx V3.4 = 1, EmonTx Shield - see Wiki
#define PULSEPIN 3                               // Interrupt input pin: EmonTx V2 = 3, EmonTx V3.2(RFu) = 2, EmonTx V3.4 = 3, EmonTx Shield - see Wiki
                                                 //  Also, set the PulseMinPeriod (below) for contact debouncing.


// To enable 12-bit ADC resolution on Arduino Due,
// include the following line
//  #define ARDUINO_DUE
// otherwise will default to 10 bits, as in regular Arduino-based boards.

#if defined(__arm__)
#define ADC_BITS    12
#else
#define ADC_BITS    10
#endif

#define ADC_COUNTS  (1<<ADC_BITS)


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <OneWire.h>                             // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>                   // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip

#define EMONTX_V3                                // The type of emonTx - can be EMONTX_V3 (for V3.2 & V3.4) or EMONTX_SHIELD (all versions)
                                                 //   You must have one or the other.

#define RFM69CW                                  // The type of Radio Module, can be RFM69CW or RFM12B,
                                                 //   or SERIALOUT if a wired serial connection is used
                                                 //   or EMONESP if an ESP WiFi module is used
                                                 //     (see http://openenergymonitor.org/emonnode/3872)
                                                 //   or don't define anything if neither radio nor serial connection is required - in which case
                                                 //     the serial output will be for information and debugging only.
                                                 // The sketch will hang if the wrong radio module is specified, or if one is specified and not fitted.
#undef RF12_433MHZ
#undef RF12_868MHZ
#undef RF12_915MHZ                               // Should not be present, but can cause problems if they are.

#define RF12_433MHZ                              // Frequency of RFM module can be
                                                 //    RF12_433MHZ, RF12_868MHZ or RF12_915MHZ.
                                                 //  You should use the one matching the module you have.
                                                 //  (Note: this is different from the normal OEM definition.)

#define RFPWR 0x99                               // Transmitter power: 0x9F = +13 dBm (max) (see notes in comment above)

#define RFMSELPIN 10                             // Pins for the RFM Radio module
                                                 // EmonTx V2 = 10, EmonTx V3.2(RFu) = 4, EmonTx V3.4 = 10, EmonTx Shield = 5 or 10 (depending on jumper setting)
#define RFMIRQPIN 2                              // Pins for the RFM Radio module:
                                                 // EmonTx V2 = 2, EmonTx V3.2(RFu) = 3, EmonTx V3.4 = 2, EmonTx Shield = 2 or 3 (depending on jumper setting)

#define DS18B20_PWR 19                           // DS18B20 Power pin - all versions
#define ONE_WIRE_BUS 5                           // DS18B20 Data      - 5 for the V3.4; 4 for the Shield
#define MAXONEWIRE 6                             // Max number of temperature sensors
#define TEMPERATURE_PRECISION 11                 // 9 (93.8ms), 10 (187.5ms), 11 (375ms) or 12 (750ms) bits equal to resolution
                                                 //   of 0.5C, 0.25C, 0.125C and 0.0625C
#define ASYNC_DELAY 375                          // DS18B20 conversion delay - 9bit requires 95ms, 10bit 187ms, 11bit 375ms
                                                 //   and 12bit resolution takes 750ms

const int nodeID = 11;                           //  node ID for this emonTx. This sketch does NOT interrogate the DIP switch.
const int nodeID_extended = 15;

const int networkGroup = 210;                    //  wireless network group
                                                 //  - needs to be same as emonBase and emonGLCD. OEM default is 210


const int UNO = 1;                               // Set to 0 if you are not using the UNO bootloader
                                                 // (i.e using Duemilanove) - All Atmega's shipped from
                                                 // OpenEnergyMonitor come with Arduino Uno bootloader
const byte TIME_BETWEEN_READINGS = 5;           // Time between readings

#define CT4LINE 1                                // Set this to 1, 2, or 3 depending on the Line to which the CT4 load is connected.
                                                 //  The default is 1
                                                 // DO NOT DEFINE CT4LINE if the 4th CT is not to be used.
                                                 // The timing values "PHASE2", "PHASE3", "Phasecal2" & "Phasecal3" will be different
                                                 //  depending on whether CT 4 is used or not.

#define PHASE2 6                                 //  Number of samples delay for L2

#define PHASE3 12                                //  Number of samples delay for L3
                                                 //  These can be adjusted if the phase correction is not adequate
                                                 //  Suggested starting values for 3 ct's  [4 ct's]:
                                                 //    PHASE2                         7    [   6  ]
                                                 //    PHASE3                        17    [  14  ]
                                                 //    Phasecal2 =                 0.22    [ 0.60 ]
                                                 //    Phasecal3 =                 0.40    [ 0.08 ]
                                                 //  Suggested starting values for Phasecal4
                                                 //    On Line 1:  1.10,  Line 2: 0.09,  Line 3: 0.35


#ifdef EMONTX_V3

#define SUPPLYVOLTAGE 3300                       // Always 3.3 V for the emonTx V3

// Calibration coefficients for emonTx V3
// These need to be set in order to obtain accurate results
double Vcal = 249.5;                             // Calibration constant for voltage input
double Ical1 = 91.5;                             // Calibration constant for current transformer 1
double Ical2 = 91.5;                             // Calibration constant for current transformer 2
double Ical3 = 91.5;                             // Calibration constant for current transformer 3
double Ical4 = 17.0;                             // Calibration constant for current transformer 4
#endif

#ifdef EMONTX_SHIELD

#define SUPPLYVOLTAGE 5000                       // Normally 5.0 V for the emonTx Shield = operating voltage of Arduino

// Calibration coefficients for emonTx Shield
// These need to be set in order to obtain accurate results
double Vcal = 234.2;                             // Calibration constant for voltage input
double Ical1 = 60.6;                             // Calibration constant for current transformer 1
double Ical2 = 60.6;                             // Calibration constant for current transformer 2
double Ical3 = 60.6;                             // Calibration constant for current transformer 3
double Ical4 = 16.6;                             // Calibration constant for current transformer 4
#endif

// Calibration coefficients common to all versions - see comments above
double Phasecal1 = 1.00;                         // Calibration constant for phase shift L1
double Phasecal2 = 0.92;                         // Calibration constant for phase shift L2
double Phasecal3 = 0.95;                         // Calibration constant for phase shift L3
double Phasecal4 = 1.20;                         // Calibration constant for phase shift CT 4


#include <avr/wdt.h>                             // the UNO bootloader

#include <SPI.h>                                 // SPI bus for the RFM module
#include <util/crc16.h>                          // Checksum


#define BUFFERSIZE (PHASE3+2)                    // Store a little more than 240 degrees of voltage samples

// Set Voltage and current input pins
int inPinV  = 0;
int inPinI1 = 1;
int inPinI2 = 2;
int inPinI3 = 3;
int inPinI4 = 4;

// Detect CTs in use
bool CT1inUse = false;
bool CT2inUse = false;
bool CT3inUse = false;
bool CT4inUse = false;



//--------------------------------------------------------------------------------------
// Variable declaration for filters, phase shift, voltages, currents & powers
//--------------------------------------------------------------------------------------
double realPower1,                               // The final data
apparentPower1,
powerFactor1,
Irms1,
realPower2,
apparentPower2,
powerFactor2,
Irms2,
realPower3,
apparentPower3,
powerFactor3,
Irms3,
realPower4,
apparentPower4,
powerFactor4,
Irms4,
Vrms;


typedef struct { int power1, power2, power3, power4, Vrms, temp[MAXONEWIRE]; unsigned long pulseCount; } PayloadTX;
typedef struct { int apparentPower1, apparentPower2, apparentPower3, apparentPower4, Irms1, Irms2, Irms3, Irms4, powerFactor1, powerFactor2, powerFactor3, powerFactor4; } PayloadTXextended;  //for directconnect
                                                 // packaging data for RF comms
                                                 // Include all the variables that are desired, chosen from the list above;
                                                 // ensure the same struct or corresponding emonhub.conf node decoder settings are used to receive,
                                                 // The maximum size is 60 Bytes

PayloadTX emontx;                                // create an instance
PayloadTXextended emontx_extended;

# ifdef EMONTX_V3
const int LEDpin = 6;                            // On-board emonTx LED
#endif
# ifdef EMONTX_SHIELD
const int LEDpin = 9;                            // On-board emonTx LED
#endif

// Declare stuff for DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
byte allAddress [MAXONEWIRE][8];                 // 8 bytes per address
byte numSensors;

// Pulse counting
volatile byte pulses = 0;
unsigned long pulseTime = 0;                     // Record time of interrupt pulse
const byte PulseMinPeriod = 110;                 // minimum period between pulses (ms) - default pulse output meters = 100ms
                                                 //   Set to 0 for electronic sensor with solid-state output.

#ifdef EMONESP
#undef SERIALPRINT                               // Must not corrupt serial output to emonHub with 'human-friendly' printout
#undef SERIALOUT
#undef DEBUGGING
#endif

#if defined SERIALOUT
#undef EMONESP
#undef SERIALPRINT                               // Must not corrupt serial output to emonHub with 'human-friendly' printout
#undef DEBUGGING
#endif

#ifdef DIRECTCONNECT
#undef EMONESP
#undef SERIALPRINT                               // Must not corrupt serial output to emonHub with 'human-friendly' printout
#undef SERIALOUT
#undef DEBUGGING
#endif

void setup()
{
    Serial.begin(9600);
#if !defined SERIALOUT && !defined EMONESP && !defined DIRECTCONNECT
    Serial.print("Set baud=115200");
#endif
    Serial.end();
    Serial.begin(115200);

    wdt_enable(WDTO_8S);                     // Since we're not using JeeLib and will not sleep between samples
#if !defined SERIALOUT && !defined EMONESP && !defined DIRECTCONNECT
#ifdef EMONTX_V3
    Serial.println(F("emonTx V3.2 & V3.4 CT1234 Voltage 3 Phase example"));
#endif
#ifdef EMONTX_SHIELD
    Serial.println(F("emonTx Shield CT1234 Voltage 3 Phase example"));
#ifdef ARDUINO_DUE
    analogReadResolution(ADC_BITS);
    Serial.println(F("with Arduino Due"));
#endif
#endif
#ifdef RFM69CW && !defined DIRECTCONNECT
    Serial.println(F("Using RFM69CW Radio"));
#endif
#ifdef RFM12B
    Serial.println(F("Using RFM12B Radio"));
#endif
    #if !defined DIRECTCONNECT
      Serial.println(F("OpenEnergyMonitor.org"));
      Serial.print(F("Node: "));
      Serial.print(nodeID);

      Serial.print(F(" Freq: "));
      #ifdef RF12_868MHZ
          Serial.print(F("868MHz"));
      #elif defined RF12_915MHZ
          Serial.print(F("915MHz"))
      #else // default to 433 MHz
          Serial.print(F("433MHz"));
      #endif

      Serial.print(F(" Network: "));
      Serial.println(networkGroup);
    #endif
#endif  // #if !defined SERIALOUT && !defined EMONESP

#if defined RFM12B || defined RFM69CW
    rfm_init();
#endif

    DIDR0 |= 0x3F;                               // Disable digital input buffer on Analogue Inputs to save current
    DIDR1 |= 0x03;

    pinMode(LEDpin, OUTPUT);                     // Setup indicator LED
    digitalWrite(LEDpin, HIGH);

#ifdef USEPULSECOUNT
    pinMode(PULSEPIN, INPUT_PULLUP);             // Set interrupt pulse counting pin as input
#endif
    emontx.pulseCount=0;                                        // Make sure pulse count starts at zero

    if (analogRead(inPinI1) != 0)                // Detect CTs in use
        CT1inUse = true;
    if (analogRead(inPinI2) != 0)
        CT2inUse = true;
    if (analogRead(inPinI3) != 0)
        CT3inUse = true;
    if (analogRead(inPinI4) != 0)
        CT4inUse = true;
    digitalWrite(LEDpin, LOW);


    //################################################################################################################################
    //Setup and test for presence of DS18B20
    //################################################################################################################################
    pinMode(DS18B20_PWR, OUTPUT);
    digitalWrite(DS18B20_PWR, HIGH); delay(50);
    sensors.begin();
    sensors.setWaitForConversion(false);         // disable automatic temperature conversion to reduce time spent awake,
                                                 // conversion will be implemented manually in sleeping
                                                 //  http://harizanov.com/2013/07/optimizing-ds18b20-code-for-low-power-applications/
    numSensors=(sensors.getDeviceCount());
    if (numSensors > MAXONEWIRE)
      numSensors=MAXONEWIRE;                     // Limit number of sensors to max number of sensors

    byte j=0;                                    // search for one wire devices and
                                                     // copy to device address arrays.
    while ((j < numSensors) && (oneWire.search(allAddress[j])))
      j++;
    for(byte j=0; j<MAXONEWIRE; j++)             // Set up 'absent sensor' value
      emontx.temp[j] = 3010;

    digitalWrite(DS18B20_PWR, LOW);

#if !defined SERIALOUT && !defined EMONESP && !defined DIRECTCONNECT
    if (numSensors)
    {
        Serial.print("Detected Temp Sensors:  "); Serial.println(numSensors);
    }
    else
    {
        Serial.println("No temperature sensor");
    }
#endif  // #if !defined SERIALOUT && !defined EMONESP

#ifdef USEPULSECOUNT
    attachInterrupt(PULSEINT, onPulse, RISING);     // Attach pulse counting interrupt pulse counting
#endif

    delay(20);

}

//*********************************************************************************************************************
void loop()
{
    // Outer loop - Reads Voltages, Currents & temperatures - Sends results
    calcVI3Ph(11,2000);                              // Calculate all. No.of complete cycles, time-out

    if (numSensors)                                  // If we detected any temperature sensors
    {
        digitalWrite(DS18B20_PWR, HIGH);         // Power the sensors
        delay(50);
        for(int j=0;j<numSensors;j++)
            sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);
                                                 // and set the a to d conversion resolution of each.
        sensors.requestTemperatures();
        delay(ASYNC_DELAY);                      // Must wait for conversion to happen, since we use ASYNC mode
        for(byte j=0;j<numSensors;j++)
            emontx.temp[j]=get_temperature(j);

        digitalWrite(DS18B20_PWR, LOW);
    }



    #if defined SERIALOUT && !defined EMONESP
    Serial.print(nodeID);     Serial.print(' ');
    Serial.print((int)(realPower1+0.5)); Serial.print(' ');   // These for compatibility, but whatever you need if emonHub is configured to suit.
    Serial.print((int)(realPower2+0.5)); Serial.print(' ');
    Serial.print((int)(realPower3+0.5)); Serial.print(' ');
    Serial.print((int)(realPower4+0.5)); Serial.print(' ');
    Serial.print((int)(Vrms*100));

    if (numSensors)
    {
        for(byte j=0;j<numSensors;j++)
        {
            Serial.print(' ');
            Serial.print(emontx.temp[j]);
        }
    }
    Serial.println();

    #endif  // if defined SERIALOUT && !defined EMONESP

    #if defined EMONESP && !defined SERIALOUT
    Serial.print("ct1_power:"); Serial.print(realPower1);            // These for compatibility, but whatever you need if the receiver is configured to suit.
    Serial.print(",ct2_power:"); Serial.print(realPower2);
    Serial.print(",ct3_power:"); Serial.print(realPower3);
    Serial.print(",ct4_power:"); Serial.print(realPower4);
    Serial.print(",ct1_apparentPower:"); Serial.print(apparentPower1);            // These for compatibility, but whatever you need if the receiver is configured to suit.
    Serial.print(",ct2_apparentPower:"); Serial.print(apparentPower2);
    Serial.print(",ct3_apparentPower:"); Serial.print(apparentPower3);
    Serial.print(",ct4_apparentPower:"); Serial.print(apparentPower4);
    Serial.print(",ct1_Irms:"); Serial.print(Irms1);            // These for compatibility, but whatever you need if the receiver is configured to suit.
    Serial.print(",ct2_Irms:"); Serial.print(Irms2);
    Serial.print(",ct3_Irms:"); Serial.print(Irms3);
    Serial.print(",ct4_Irms:"); Serial.print(Irms4);
    Serial.print(",ct1_powerFactor:"); Serial.print(powerFactor1);            // These for compatibility, but whatever you need if the receiver is configured to suit.
    Serial.print(",ct2_powerFactor:"); Serial.print(powerFactor2);
    Serial.print(",ct3_powerFactor:"); Serial.print(powerFactor3);
    Serial.print(",ct4_powerFactor:"); Serial.print(powerFactor4);
    Serial.print(",vrms:"); Serial.print(Vrms);
    if (numSensors){
        for(byte j=0;j<numSensors;j++)
        {
            Serial.print(",t"); Serial.print(j); Serial.print(":");
            Serial.print(emontx.temp[j]);
        }
    }
    Serial.println();
    delay(50);
    #endif

    #if defined SERIALPRINT && !defined EMONESP

    Serial.print(F("Voltage: ")); Serial.println(Vrms);
    Serial.print(F(" Phase 1: ")); Serial.print(Irms1);
    Serial.print(F(" A, ")); Serial.print(realPower1);
    Serial.print(F(" W, ")); Serial.print(apparentPower1);
    Serial.print(F(" VA, PF=")); Serial.println(powerFactor1,3);

    Serial.print(F(" Phase 2: ")); Serial.print(Irms2);
    Serial.print(F(" A, ")); Serial.print(realPower2);
    Serial.print(F(" W, ")); Serial.print(apparentPower2);
    Serial.print(F(" VA, PF=")); Serial.println(powerFactor2,3);

    Serial.print(F(" Phase 3: ")); Serial.print(Irms3);
    Serial.print(F(" A, ")); Serial.print(realPower3);
    Serial.print(F(" W, ")); Serial.print(apparentPower3);
    Serial.print(F(" VA, PF=")); Serial.println(powerFactor3,3);

    #ifdef CT4LINE
    Serial.print(F(" Input 4: ")); Serial.print(Irms4);
    Serial.print(F(" A, ")); Serial.print(realPower4);
    Serial.print(F(" W, ")); Serial.print(apparentPower4);
    Serial.print(F(" VA, PF=")); Serial.println(powerFactor4,3);
    #endif // ifdef CT4

    if (numSensors)
    {
        Serial.print(" Temperatures: ");
        for(byte j=0;j<numSensors;j++)
        {
            Serial.print(emontx.temp[j]/10.0);
            Serial.print(' ');
        }
    }
    Serial.println();

    if (pulses)                                      // if the ISR has counted some pulses, update the total count
    {
        cli();                                       // Disable interrupt just in case pulse comes in while we are updating the count
        emontx.pulseCount += pulses;
        pulses = 0;
        sei();                                       // Re-enable interrupts
    }
    #ifdef USEPULSECOUNT
    Serial.print(F(" Pulses=")); Serial.println(emontx.pulseCount);
    #endif // ifdef USEPULSECOUNT


    Serial.println(); delay(100);

    #endif  //  if defined SERIALPRINT && !defined EMONESP

    emontx.power1 = realPower1;                      // Copy the desired variables ready for transmission
    emontx.power2 = realPower2;
    emontx.power3 = realPower3;
    emontx.power4 = realPower4;
    emontx.Vrms   = Vrms * 100;

    emontx_extended.apparentPower1 = apparentPower1;
    emontx_extended.apparentPower2 = apparentPower2;
    emontx_extended.apparentPower3 = apparentPower3;
    emontx_extended.apparentPower4 = apparentPower4;
    emontx_extended.Irms1 = Irms1*100;
    emontx_extended.Irms2 = Irms2*100;
    emontx_extended.Irms3 = Irms3*100;
    emontx_extended.Irms4 = Irms4*100;
    emontx_extended.powerFactor1 = powerFactor1*10;
    emontx_extended.powerFactor2 = powerFactor2*10;
    emontx_extended.powerFactor3 = powerFactor3*10;
    emontx_extended.powerFactor4 = powerFactor4*10;


    digitalWrite(LEDpin, HIGH); delay(2); digitalWrite(LEDpin, LOW);      // flash LED

#if defined RFM12B || defined RFM69CW
    rfm_send((byte *)&emontx, sizeof(emontx), networkGroup, nodeID);      // *SEND RF DATA*
    delay(50);
    rfm_send((byte *)&emontx_extended, sizeof(emontx_extended), networkGroup, nodeID_extended);
#endif

#if defined DIRECTCONNECT && !defined SERIALOUT
send_direct_serial();
send_direct_serial_extended();
#endif

    for (int i = TIME_BETWEEN_READINGS; i > 0; i--)  // because the maximum between watchdog resets is 8 s
    {
        delay(1000);
        wdt_reset();

    }

}

//*********************************************************************************************************************

void calcVI3Ph(int cycles, int timeout)
{
    //--------------------------------------------------------------------------------------
    // Variable declaration for filters, phase shift, voltages, currents & powers
    //--------------------------------------------------------------------------------------

    int lastSampleV,sampleV;                     // 'sample' holds the raw analogue read value, 'lastSample' holds the last sample
    int sampleI1;
    int sampleI2;
    int sampleI3;
    int sampleI4;


    double lastFilteredV,filteredV;              // 'Filtered' is the raw analogue value minus the DC offset
    double filteredI1;
    double filteredI2;
    double filteredI3;
    double filteredI4;

    static double offsetV  = ADC_COUNTS>>1;      // Low-pass filter output - start at half-rail, or zero if CT was not detected.
    static double offsetI1 = CT1inUse?ADC_COUNTS>>1 : 0;
    static double offsetI2 = CT2inUse?ADC_COUNTS>>1 : 0;
    static double offsetI3 = CT3inUse?ADC_COUNTS>>1 : 0;
    static double offsetI4 = CT4inUse?ADC_COUNTS>>1 : 0;


    double phaseShiftedV1;                       // Holds the calibrated delayed & phase shifted voltage.
    double phaseShiftedV2;
    double phaseShiftedV3;
    double phaseShiftedV4;

    double sumV,sumI1,sumI2,sumI3,sumI4;
    double sumP1,sumP2,sumP3,sumP4;              // running sum leading to the mean value

    int startV;                                  // Instantaneous voltage at start of sample window.

    int SupplyVoltage = SUPPLYVOLTAGE;           // Hardcode supply voltage
    int crossCount = -2;                         // Used to measure number of times threshold is crossed.
    int numberOfSamples = BUFFERSIZE;            // Total count - index into by circular array (Start at BUFFERSIZE to prevent
                                                 //  indexing outside of array on first cycle.
    int numberOfPowerSamples = 0;                // For averages - should be ~ 1.66 cycles less than numberOfSamples


    boolean lastVCross, checkVCross = false;     // Flags to determine which half-cycle we are in.
    double storedV[BUFFERSIZE];                  // Array to store >240 degrees of voltage samples


    //-------------------------------------------------------------------------------------------------------------------------
    // 1) Waits for the waveform to be close to 'zero' (1/2 scale adc) part in sin curve.
    //-------------------------------------------------------------------------------------------------------------------------
    boolean st=false;                            // an indicator to exit the while loop

    unsigned long start = millis();              // millis()-start makes sure it doesn't get stuck in the loop if there is an error.

    while(st==false)                             // Wait for first zero crossing...
    {
        startV = analogRead(inPinV);             // using the voltage waveform
        if ((startV < ((ADC_COUNTS>>1)+(ADC_COUNTS>>3))) && (startV > ((ADC_COUNTS>>1)-(ADC_COUNTS>>3)))) st=true;  //check it's within range
        if ((millis()-start)>timeout) st = true;
    }

    //-------------------------------------------------------------------------------------------------------------------------
    // 2) Main measurement loop
    //-------------------------------------------------------------------------------------------------------------------------
    start = millis();

    while ((crossCount < cycles * 2) && ((millis()-start)<timeout))
    {
        lastSampleV=sampleV;                     // Used for digital low pass filter - offset removal
        lastFilteredV = filteredV;

        //-----------------------------------------------------------------------------
        // A) Read in raw voltage and current samples
        //-----------------------------------------------------------------------------
        sampleV = analogRead(inPinV);            // Read in raw voltage signal
        sampleI1 = analogRead(inPinI1);          // Read in raw current signal
        sampleI2 = analogRead(inPinI2);          // Read in raw current signal
        sampleI3 = analogRead(inPinI3);          // Read in raw current signal

#ifdef CT4LINE
        sampleI4 = analogRead(inPinI4);          // Read in raw current signal
#endif

        // Apply digital low pass filter to the voltage input, then store it in a circular buffer
        offsetV = offsetV + ((sampleV-offsetV)/1024);
        filteredV = sampleV - offsetV;
        storedV[numberOfSamples%BUFFERSIZE] = filteredV;        // store this voltage sample in circular buffer

        // Count the number of zero crossings - the first cycle loads the buffer, so that we can look back for phases 2 & 3.

        lastVCross = checkVCross;
        checkVCross = (sampleV > startV) ? true : false;
        if (lastVCross != checkVCross)
        {
            if (crossCount == 0)                 // Started recording at -2 crossings so that one complete cycle
            {                                    //   has been stored before accumulating.
                sumV  = 0;
                sumI1 = 0;
                sumI2 = 0;
                sumI3 = 0;
                sumI4 = 0;
                sumP1 = 0;
                sumP2 = 0;
                sumP3 = 0;
                sumP4 = 0;
                numberOfPowerSamples = 0;
            }
            crossCount++;
        }



        //-----------------------------------------------------------------------------
        // B) Apply digital low pass filters to obtain 2.5V DC offset,
        //    then subtract it (to centre the 'filtered' wave on 0).
        //-----------------------------------------------------------------------------
        offsetI1 = offsetI1 + ((sampleI1-offsetI1)/1024);
        filteredI1 = sampleI1 - offsetI1;
        offsetI2 = offsetI2 + ((sampleI2-offsetI2)/1024);
        filteredI2 = sampleI2 - offsetI2;
        offsetI3 = offsetI3 + ((sampleI3-offsetI3)/1024);
        filteredI3 = sampleI3 - offsetI3;

#ifdef CT4LINE
        offsetI4 = offsetI4 + ((sampleI4-offsetI4)/1024);
        filteredI4 = sampleI4 - offsetI4;
#endif



        //-----------------------------------------------------------------------------
        // D) Root-mean-square method voltage
        //-----------------------------------------------------------------------------
        sumV += filteredV * filteredV;           // sum += squared voltage values

        //-----------------------------------------------------------------------------
        // E) Root-mean-square method current
        //-----------------------------------------------------------------------------
        sumI1 += filteredI1 * filteredI1;        // sum += squared current values
        sumI2 += filteredI2 * filteredI2;
        sumI3 += filteredI3 * filteredI3;

#ifdef CT4LINE
        sumI4 += filteredI4 * filteredI4;
#endif

        //-----------------------------------------------------------------------------
        // F) Phase calibration - for Phase 1: shifts V1 to correct transformer errors
        //    for phases 2 & 3 delays V1 by 120 degrees & 240 degrees respectively
        //    and shifts for fine adjustment and to correct transformer errors.
        //-----------------------------------------------------------------------------
        phaseShiftedV1 = lastFilteredV + Phasecal1 * (filteredV - lastFilteredV);

        phaseShiftedV2 = storedV[(numberOfSamples-PHASE2-1)%BUFFERSIZE]
            + Phasecal2 * (storedV[(numberOfSamples-PHASE2)%BUFFERSIZE]
                         - storedV[(numberOfSamples-PHASE2-1)%BUFFERSIZE]);

        phaseShiftedV3 = storedV[(numberOfSamples-PHASE3-1)%BUFFERSIZE]
            + Phasecal3 * (storedV[(numberOfSamples-PHASE3)%BUFFERSIZE]
                         - storedV[(numberOfSamples-PHASE3-1)%BUFFERSIZE]);        // must always read ahead of the current sample



#ifdef CT4LINE
        if (CT4LINE == 2)
        {
            phaseShiftedV4 = storedV[(numberOfSamples-PHASE2-1)%BUFFERSIZE]
                + Phasecal4 * (storedV[(numberOfSamples-PHASE2)%BUFFERSIZE]
                             - storedV[(numberOfSamples-PHASE2-1)%BUFFERSIZE]);

        }
        else if (CT4LINE == 3)
        {
            phaseShiftedV4 = storedV[(numberOfSamples-PHASE3-1)%BUFFERSIZE]
                + Phasecal4 * (storedV[(numberOfSamples-PHASE3)%BUFFERSIZE]
                             - storedV[(numberOfSamples-PHASE3-1)%BUFFERSIZE]);
        }
        else
            phaseShiftedV4 = lastFilteredV + Phasecal4 * (filteredV - lastFilteredV);
#endif
        //-----------------------------------------------------------------------------
        // G) Instantaneous power calc
        //-----------------------------------------------------------------------------
        sumP1 += phaseShiftedV1 * filteredI1;    // Sum += Instantaneous Power
        sumP2 += phaseShiftedV2 * filteredI2;
        sumP3 += phaseShiftedV3 * filteredI3;

#ifdef CT4LINE
        sumP4 += phaseShiftedV4 * filteredI4;
#endif

        numberOfPowerSamples++;                  // Count number of times looped for Power averages.
        numberOfSamples++;                       // Count number of times looped.

    }

    //-------------------------------------------------------------------------------------------------------------------------
    // 3) Post loop calculations
    //-------------------------------------------------------------------------------------------------------------------------
    // Calculation of the root of the mean of the voltage and current squared (rms)
    // Calibration coefficients applied.

    double V_Ratio = Vcal *((SupplyVoltage/1000.0) / 1023.0);
    Vrms = V_Ratio * sqrt(sumV / numberOfPowerSamples);

    double I_Ratio1 = Ical1 *((SupplyVoltage/1000.0) / 1023.0);
    Irms1 = I_Ratio1 * sqrt(sumI1 / numberOfPowerSamples);

    double I_Ratio2 = Ical2 *((SupplyVoltage/1000.0) / 1023.0);
    Irms2 = I_Ratio2 * sqrt(sumI2 / numberOfPowerSamples);

    double I_Ratio3 = Ical3 *((SupplyVoltage/1000.0) / 1023.0);
    Irms3 = I_Ratio3 * sqrt(sumI3 / numberOfPowerSamples);

#ifdef CT4LINE
    double I_Ratio4 = Ical4 *((SupplyVoltage/1000.0) / 1023.0);
    Irms4 = I_Ratio4 * sqrt(sumI4 / numberOfPowerSamples);
#endif

    // Calculation power values
    realPower1 = V_Ratio * I_Ratio1 * sumP1 / numberOfPowerSamples;
    apparentPower1 = Vrms * Irms1;
    powerFactor1 = realPower1 / apparentPower1;

    realPower2 = V_Ratio * I_Ratio2 * sumP2 / numberOfPowerSamples;
    apparentPower2 = Vrms * Irms2;
    powerFactor2 = realPower2 / apparentPower2;

    realPower3 = V_Ratio * I_Ratio3 * sumP3 / numberOfPowerSamples;
    apparentPower3 = Vrms * Irms3;
    powerFactor3 = realPower3 / apparentPower3;

#ifdef CT4LINE
    realPower4 = V_Ratio * I_Ratio4 * sumP4 / numberOfPowerSamples;
    apparentPower4 = Vrms * Irms4;
    powerFactor4 = realPower4 / apparentPower4;
#else
    realPower4 = 0.0;
    apparentPower4 = 0.0;
    powerFactor4 = 0.0;
#endif

    // Reset accumulators
    sumV = 0;
    sumI1 = 0;
    sumI2 = 0;
    sumI3 = 0;
    sumI4 = 0;
    sumP1 = 0;
    sumP2 = 0;
    sumP3 = 0;
    sumP4 = 0;
    //--------------------------------------------------------------------------------------

#ifdef DEBUGGING
    // Include these statements for development/debugging only

    Serial.print(F("Total Samples: ")); Serial.print(numberOfSamples);
    Serial.print(F(" Power Samples: ")); Serial.print(numberOfPowerSamples);
    Serial.print(F(" Time: ")); Serial.print(millis() - start);
    Serial.print(F(" Crossings: ")); Serial.println(crossCount);

    for (int j=0; j<BUFFERSIZE; j++)
    {
        Serial.print(storedV[j]); Serial.print(F(" "));
        Serial.println();
    }
#endif
}


int get_temperature(byte sensor)
{
  float temp=(sensors.getTempC(allAddress[sensor]));

  if (temp==-127)                                // if reading is -127, return value = 300 deg ('Checksum error')
    return 3000;
  else if (temp >= 125.0 || temp <= -55.0)       // return value = 302 deg ('Out of range')
    return 3020;
  else                                           // else reading is within range for the sensor, convert float to int ready to send via RF
    return(temp*10);
                                                 // N.B. array is prefilled with 301 deg ('absent sensor')
}

#ifdef USEPULSECOUNT
//-------------------------------------------------------------------------------------------------------------------------------------------
// The Interrupt Service Routine - runs each time a falling edge of a pulse is detected
//-------------------------------------------------------------------------------------------------------------------------------------------
void onPulse()
{
    if (PulseMinPeriod)
    {
      if ((millis() - pulseTime) > PulseMinPeriod) {              // Check that contact bounce has finished
        pulses++;
      }
      pulseTime=millis();                                         // No 'debounce' required - electronic switch presumed
    }
    else
        pulses++;
}

#endif

void send_direct_serial()
{
  //int serialnode = serialnode;
  //struct data = data;

  byte binarray[sizeof(emontx)];
  memcpy(binarray, &emontx, sizeof(emontx));

  Serial.print(F("OK "));
  Serial.print(nodeID);
  for (byte i = 0; i < sizeof(binarray); i++) {
    Serial.print(F(" "));
    Serial.print(binarray[i]);
  }
  Serial.print(F(" (-0)"));
  Serial.println();

  delay(10);
}

void send_direct_serial_extended()
{
  //int serialnode = serialnode;
  //struct data = data;

  byte binarray[sizeof(emontx_extended)];
  memcpy(binarray, &emontx_extended, sizeof(emontx_extended));

  Serial.print(F("OK "));
  Serial.print(nodeID_extended);
  for (byte i = 0; i < sizeof(binarray); i++) {
    Serial.print(F(" "));
    Serial.print(binarray[i]);
  }
  Serial.print(F(" (-0)"));
  Serial.println();

  delay(10);
}
