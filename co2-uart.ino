#include <KashiwaGeeks.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "cactus_io_BME280_I2C.h"

#define ECHO true

#define LoRa_Port_NORM  12
#define LoRa_Port_COMP  13

ADB922S LoRa;

// Settings
// int use_lora = 1; // 0 = disabled, 1 = enabled
int data_interval_minutes = 30; // job interval


SoftwareSerial co2Serial(A0, A1); // define MH-Z19 RX TX
BME280_I2C bme(0x76); // I2C using address 0x76


//================================
//          Initialize Device Function
//================================
#define BPS_9600       9600
#define BPS_19200     19200
#define BPS_57600     57600
#define BPS_115200   115200


void start(void)
{
    ConsoleBegin(BPS_9600);
    EnableInt0();
    co2Serial.begin(BPS_9600);

    // initCO2(); // Please enable when you are outside

    bme.begin();
    bme.setTempCal(-1); // Temp was reading high so subtract 1 degree

    ConsolePrint(F("********* Sensor Starting *******\n"));

    //DisableConsole();
    //DisableDebug();

    /*
     * Enable Interrupt 0 & 1  Uncomment the following two  lines.
     */
    EnableInt0();
    //EnableInt1();  // For ADB922S, CUT the pin3 of the Sheild.


    ConsolePrint(F("\n**** LoRa Starting *****\n"));

    /*  setup Power save Devices */
    //power_adc_disable();          // ADC converter
    //power_spi_disable();          // SPI
    //power_timer1_disable();   // Timer1
    //power_timer2_disable();   // Timer2, tone()
    //power_twi_disable();         // I2C

    /*  setup ADB922S  */
    if ( LoRa.begin(BPS_9600, DR3) == false )
    {
        while(true)
        {
            LedOn();
            delay(1000);
            LedOff();
            delay(1000);
        }
    }


    /*  join LoRaWAN */
    LoRa.join();
    LedOff();

    //DisableConsole();
    //DisableDebug();
}


//================================
//          Intrrupt callbaks
//================================
void int0D2(void) {
    ConsolePrint(F("********* INT0 *******\n"));
    mainFunction();
}


//================================
//          Power save functions
//================================
void sleep(void)
{
    LoRa.sleep();
    DebugPrint(F("LoRa sleep.\n"));

    //
    //  ToDo: Set Device to Power saving mode
    //
}

void wakeup(void)
{
    LoRa.wakeup();
    DebugPrint(F("LoRa wakeup.\n"));
    //
    //  ToDo: Set Device to Power On mode
    //
}

//================================
//          Tasks
//================================

int cnt = 0;

void mainFunction() {
    LedOn();
    sendMQTT();
    LedOff();

}


void sendMQTT(void)
{
    int16_t temp = getTemp();
    uint16_t humi = getHumidity();
    uint16_t pressure = getPressure_MB();
    uint16_t co_two = getCO2();

    char s[16];
    ConsolePrint(F("\n  Send Data\n\n"));
    ConsolePrint(F("Temperature:  %d degrees C\n"), temp);
    ConsolePrint(F("Humidity: %d %%\n"), humi);
    ConsolePrint(F("Pressure: %d Pa\n"), pressure);
    ConsolePrint(F("CO2: %d ppm\n"), co_two);


    int rc = LoRa.sendData(LoRa_Port_NORM, ECHO, F("%04X%04X%04X%04X"), temp, humi, pressure, co_two);
    // checkResult(rc);
}

void func2()
{
    // ConsolePrint("func2:  Count=%d\n", cnt++);
}

void initCO2()  // Zero Calibration
{
    byte cmd[9] = {0xFF,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78};
    co2Serial.listen();  // MANDATORY switch multiple SoftwareSerial object
    co2Serial.write(cmd, 9); //request ZERO POINT CALIBRATION
}

int getCO2()
{
    byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
    unsigned char response[9]; // for answer
    co2Serial.listen();  // MANDATORY switch multiple SoftwareSerial object
    co2Serial.write(cmd, 9); //request PPM CO2
    co2Serial.readBytes(response, 9);
    if (response[0] != 0xFF){
        return 0;
    }
    if (response[1] != 0x86){
        return 0;
    }
    unsigned int byte2 = (unsigned int) response[2];
    unsigned int byte3 = (unsigned int) response[3];
    unsigned int byte4 = (unsigned int) response[4];
    int ppm = (256 * byte2) + byte3;
    int temp = byte4 - 40;
    // ConsolePrint("byte2:  %d\n", byte2);
    // ConsolePrint("byte3:  %d\n", byte3);
    ConsolePrint("CO2:  %d\n", ppm);
    return ppm;
}

int getTemp()
{
    bme.readSensor();
    int16_t temp =   bme.getTemperature_C();
    ConsolePrint("Temp:  %d\n", temp);
    return temp;
}

int getHumidity()
{
    bme.readSensor();
    uint16_t humidity =   bme.getHumidity();
    ConsolePrint("Humidity:  %d\n", humidity);
    return humidity;
}

int getPressure_MB() {
    bme.readSensor();
    uint32_t pressure =   bme.getPressure_MB();;
    ConsolePrint("Pressure:  %d\n", pressure);
    return pressure;
}

//===============================
//            Execution interval
//     TASK( function, initial offset, interval by minute )
//===============================
TASK_LIST = {
//  TASK(getCO2, 3, 1),
        TASK(mainFunction, 0, data_interval_minutes),
        END_OF_TASK_LIST
};