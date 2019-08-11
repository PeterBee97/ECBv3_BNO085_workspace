/* ========================================
 *
 * Copyright CMU BIOROBOTICS LAB,2019
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF CMU BIORROBOTICS LAB
 *
 * ========================================
*/

#include "project.h"
#include "include/IMU_BNO085.h"
#include <stdio.h> 

/**********CONSTANT VALUES AND FLAGS*********/
#define USBUART_MODE                         //Debugging does not work with USBUART enabled. Disable for debugging and performance
#define DATA_OUTPUT_MODE                   //When debugging, comment out DATA_OUTPUT_MODE to be able to print strings. But for procedure use we just want serial output of the float data.
//#define CALIBRATE_MAG_MODE


#define BUZZER_TIMER_CLK 8000000
#define USBFS_DEVICE    (0u)

/* The buffer size is equal to the maximum packet size of the IN and OUT bulk
* endpoints.
*/
#define USBUART_BUFFER_SIZE (64u)
#define LINE_STR_LENGTH     (20u)

#define INVALID_INPUT -1
#define NO_CMD_FOUND  -2
#define OUT_OF_CHARS  -3
#define CMD_SIZE 3

//GLOBALS
uint16 buzzer_freq = 20;
uint16 buzzer_clock_div = 2;
uint16 buzzer_timer_peorid = 1;
uint16 buzzer_timer_cmp1 = 0;
uint16 buzzer_chirp_dir = 1;
uint8_t count;                     //Used in DATATRANSM2
uint8 buffer[USBUART_BUFFER_SIZE]; //used for USBUART in DATATRANSM2
sh2_Hal_t *pSh2Hal;
sh2_ProductIds_t prodIds;           //Used for getting product ID in Sh2_getProdID().    
sh2_SensorValue_t value;
sh2_SensorEvent_t eventsBuff[200];
int eventsBuffindex = 0;

//Debugging constants
int interruptval = 1;
volatile int status;                //Every sh2 function returns an int. Values specified in Sh2_err.h
static char str [128];
volatile uint32_t time1 = 0;
volatile uint32_t time2 = 0;
float transmitBuf[10];  
bool got_accel = 0, got_gyro = 0, got_rot = 0, got_mag; //Flags to detect when sensor_event activates a certain sensor to receive data from
uint8_t gameAccuracy = 0, gyroAccuracy = 0, accelAccuracy = 0, magAccuracy = 0;
uint32_t accCount = 0;
volatile uint32_t tdf;
bool cal = false;

//FLAGS
uint8_t IMU_READY = 0; //Startup IMU not ready

//STATIC VARS 
//Def: Static variables retains value even when declared out of scope... essentially a global variable with a few caveats.
//Def: Statif functions are functions reserved for that particular c file. Non static functions can be called outside the file 
//it was declared in to be used in other files.

static sh2_AsyncEvent_t async_event; //for helper function eventHandler()
static uint8_t send_time = 0u;
static uint8_t new_imu_data = 0;        //Used only in DATATRANSM2
static sh2_SensorEvent_t sensor_event;  //Used in start_reports()

/****HELPER FUNCTIONS*****/

//Utility function for debugging
void printOut(char s[64])           
{ 
    #ifdef USBUART_MODE
    #ifndef DATA_OUTPUT_MODE
    //Print Statement. Each time you print, use both CDCIsReady() and PutString(). 
    while(0 == USBUART_CDCIsReady())
    {
        //This function returns a nonzero value if the Component is ready to send more data to the PC; 
        //otherwise, it returns zero.
    }
    USBUART_PutString(s);
    CyDelay(1); //delay so statement can fully print
    //End Print Statement
    #endif
    #endif //USBUART_MODE
}


//Utility function for debugging
char * f2cstring(char s[32],float f) //Converts floats to a string for printOut
{
    char *tmpSign = (f < 0) ? "-" : "";
    float tmpVal = (f < 0) ? -f : f;            
                                           //Example: 678.0123
    int tmpInt1 = tmpVal;                  // Get the integer (678).
    float tmpFrac = tmpVal - tmpInt1;      // Get fraction (0.0123).
    int tmpInt2 = trunc(tmpFrac * 1000000);  // Turn into integer (123). match zeroes with precision 6
    // Print as parts, note that you need 0-padding for fractional bit.
    sprintf (s, "%s%d.%06d", tmpSign, tmpInt1, tmpInt2); //6 places after the decimal precision. Will just append zeroes if float is not specified to have that precision
    return s;
}

//Utility function for debugging
void testPrintFloats()
{
    char s[32];
    char s2[32];
    float f = 3.14159;
    float f2 = -3.1415999;
    sprintf(str,"pi: %s, neg pi: %s\r\n",f2cstring(s,f),f2cstring(s2,f2));
    printOut(str);
}

uint32_t getTdiff()
{
    uint32_t tdiff = 0;
    time2 = get_timestamp();
    
    tdiff = time2 - time1;
    time1 = time2;
    return tdiff;
}

//Utility function for debugging

#ifdef CALIBRATE_MAG_MODE
void print10(float fa[10], uint8_t gameAcc, uint8_t accelAcc, uint8_t gyroAcc, uint8_t magAcc)
#endif
#ifndef CALIBRATE_MAG_MODE
void print10(float fa[10], uint8_t gameAcc, uint8_t accelAcc, uint8_t gyroAcc)
#endif
{
    float a,b,c,d,e,f,g,h,i,j;
    char s0[32];
    char s1[32];
    char s2[32];
    char s3[32];
    char s4[32];
    char s5[32];
    char s6[32];
    char s7[32];
    char s8[32];
    char s9[32];
    
    sprintf(str,"i:%s\tj:%s\tk:%s\tr:%s\tx:%s\ty:%s\tz:%s\twx:%s\twy:%s\twz:%s\r\n",
    f2cstring(s0,fa[0]),f2cstring(s1,fa[1]),
    f2cstring(s2,fa[2]),f2cstring(s3,fa[3]), 
    f2cstring(s4,fa[4]),f2cstring(s5,fa[5]),
    f2cstring(s6,fa[6]),f2cstring(s7,fa[7]),
    f2cstring(s8,fa[8]),f2cstring(s9,fa[9])
    );
    printOut(str);
    
    //Printing for Arduino serial plotter
//    sprintf(str,"%s\t%s\t%s\t%s\r\n",
//    f2cstring(s0,fa[0]*10),f2cstring(s1,fa[1]),
//    f2cstring(s2,fa[2]*10),f2cstring(s3,fa[3])
//    );
//    printOut(str);
    //End printing for Arduino serial plotter.
    
//    #ifdef CALIBRATE_MAG_MODE
//    sprintf(str," game:%u accel:%u gyro:%u mag:%u accCount:%u\r\n", gameAcc, accelAcc, gyroAcc, magAcc,accCount);
//    #endif
//    #ifndef CALIBRATE_MAG_MODE
//    sprintf(str," game:%u accel:%u gyro:%u mag:%u accCount:%u\r\n", gameAcc, accelAcc, gyroAcc,accCount);    
//    #endif
//    printOut(str); //second printout of sensor accuracy bits and Count.
}

static int start_reports()
{
    static sh2_SensorConfig_t config;
    int status;
    int sensorID;
    
    #ifndef CALIBRATE_MAG_MODE
    static const int enabledSensors[] = {SH2_GAME_ROTATION_VECTOR, SH2_ACCELEROMETER, SH2_GYROSCOPE_CALIBRATED};
    #endif
    #ifdef CALIBRATE_MAG_MODE
    static const int enabledSensors[] = {SH2_GAME_ROTATION_VECTOR, SH2_ACCELEROMETER, SH2_GYROSCOPE_CALIBRATED, SH2_MAGNETIC_FIELD_CALIBRATED};
    #endif
    config.changeSensitivityEnabled = false;
    config.changeSensitivityEnabled = false;
    config.wakeupEnabled = false;
    config.changeSensitivityRelative = false;
    config.alwaysOnEnabled = false;
    config.changeSensitivity = 0;
    config.batchInterval_us = 0;
    config.sensorSpecific = 0;

    // Select a report interval.
    //config.reportInterval_us = 10000;  // microseconds (100Hz)
    //config.reportInterval_us = 2500;   // microseconds (400Hz)
    config.reportInterval_us = 1000;   // microseconds (1000Hz)

    for (unsigned int n = 0; n < ARRAY_LEN(enabledSensors); n++)
    {
        // Configure the sensor hub to produce these reports
        sensorID = enabledSensors[n];
        status = sh2_setSensorConfig(sensorID, &config);
        if (status != 0) 
        {
            return status;
        }
    }
    //return status;
}



static void sensorHandler(void *cookie, sh2_SensorEvent_t *pEvent){
    eventsBuff[eventsBuffindex] = *pEvent;
    eventsBuffindex++;
    return;
}

static void eventHandler(void *cookie, sh2_AsyncEvent_t *pEvent){
    for(int i = 0; i < 5; i++){}
    async_event = *pEvent;
    return;
}

int process_commands(uint8_t *buffer, uint8_t count)
{
    //Avoid undefined behavior with checks for improper inputs
    if(buffer == NULL | count == 0 | count >= USBUART_BUFFER_SIZE) return INVALID_INPUT;
    
    //Search for the start of a command, marked by the character 'C'
    uint8_t ind, num_ex = 0;
    while(ind < count){
        //Search the buffer for the start character
        uint8_t cmd_start = 0;
        while(ind < count && !cmd_start){
            cmd_start = buffer[ind] == 'C';
            ind += 1;
        }
        if(!cmd_start) return NO_CMD_FOUND;
        
        //If we find the start character, ensure that we do not run out of characters
        if(ind + CMD_SIZE > count) return OUT_OF_CHARS;
        num_ex++; //If valid length, add to counter of commands executed
        char cmd = buffer[ind];
        switch(cmd){
            //Set indicator LEDs command
            //case 'S': set_LEDs(buffer[ind+2]); break;
            //case 'G': send_Time(); break;
            default: 
                num_ex--; //If invalid command, don't count it
        }
        ind += CMD_SIZE;
    }
    return num_ex;
}

// Read product ids with version info from sensor hub and print them
static int reportProdIds(void)
{
    int status;
    
    memset(&prodIds, 0, sizeof(prodIds));
    status = sh2_getProdIds(&prodIds);
    
    if (status < 0) {
        sprintf(str,"Error from sh2_getProdIds: %d\r\n",status);
        printOut(str);
        return status;
    }

    // Report the results
    for (int n = 0; n < prodIds.numEntries; n++) {
        sprintf(str,"Part %d : Version %d.%d.%d Build %d\r\n",
               prodIds.entry[n].swPartNumber,
               prodIds.entry[n].swVersionMajor, prodIds.entry[n].swVersionMinor, 
               prodIds.entry[n].swVersionPatch, prodIds.entry[n].swBuildNumber);
        printOut(str);
        // Wait a bit so we don't overflow the console output.
        CyDelayUs(10000);
    }
    return status;
}

static int enableCal(bool calAccel, bool calGyro, bool calMag)
{
    volatile int status;
    volatile int statusReturn;
    uint8_t enabled = 0x0;
    uint8_t a = 0, g = 0, m = 0;
    if (calAccel)
        a = SH2_CAL_ACCEL;
    if (calGyro)
        g = SH2_CAL_GYRO;
    if(calMag)
        m = SH2_CAL_MAG;    
    status = sh2_setCalConfig(a | g | m);
    statusReturn = sh2_getCalConfig(&enabled);
    return status & statusReturn;
}

//Phase functions:
/* Order of Operations for IMU to work
    1. If USBUART enabled, start our USBUART
    2. Initialize non IMU associated hardware (LEDs, Buzzer)
    3. Initialize HAL object and functions
    4. Open our Sh2 object which sets up parameters from HAL to Sh2, and SHTP. 
    5. Enable our sensors and configure them for communication through reports. (with setSensorCallback, and start_reports)
       Now we can use SH2 functions to communicate with IMU. (functs such as Sh2_getProdID())
    6. Repeatedly call Sh2_service which calls decode_Sensor_Event to repeatedly obtain data from IMU.
       Embedded system unable to print floats so need to convert them into s strings.
*/

void USBUART_setup()
{
    #ifdef USBUART_MODE
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);
    while(0 == USBUART_GetConfiguration())  //wait for enumeration
    {
        //If stuck here, USB has not enumerated properly. 
        //Check Hardware connection, USBUART CDC settings: (clock, driver update,endpoints)
    } 
    USBUART_CDC_Init(); //Initialize PC to receive data from USBUART
    while (0u == USBUART_GetConfiguration())
    {
        //This is to see if device has been enumerated properly
    }
    CyDelay(2000); //Henry: Need a delay here to delay the main loop for any init debug msgs
    //end USBUART Start process
    #endif //USBUART_MODE
}

void IMU_setup()
{
    //Note: Initialization process involves sh2_hal_init AND sh2_open functions. These setup firmware for reading from IMU.
    pSh2Hal = sh2_hal_init(); //Within here we setup all functions dealing with IMU including open(), close(), read(), write().
    //Those IMU functions will set up IMU Interrupt, IMU I2C comms, and timer for timestamps.
    status = sh2_open(pSh2Hal, eventHandler, NULL); //sh2_open calls sh2_i2c_hal_open() in sh2_hal_psoc5.c
    sprintf(str,"sh2_open returned with status: %d\r\n",status);
    //printOut(str);
    status = sh2_setSensorCallback(sensorHandler, NULL); 
}

int main(void)
{
    CyGlobalIntEnable; // Enable global interrupts.
   
    /****SYSTEM INITS****/
    
    USBUART_setup();             //Start USBUART start process
    PWM_LED_Start();             //Init debug LEDs and Buzzer... no audio though
    PWM_EN_Start();
    PWM_BUZZER_Start();
    
    TIMER_FIRING_PIN_Write(1);
    CyDelay(10);
    TIMER_FIRING_PIN_Write(0);
    CyDelay(20);
    TIMER_FIRING_PIN_Write(1);
    CyDelay(10);
    TIMER_FIRING_PIN_Write(0);
    CyDelay(20);
    TIMER_FIRING_PIN_Write(1);
    //PWM_TIMER_FIRE_Start();
    
    printOut("****INITIALIZATION START****\r\n");
    IMU_setup();                 //Initialize IMU using SH2 HAL
    start_reports();
    //status = reportProdIds();  //Simple Sh2 function to get a product ID. Used in debugging to verify HAL read and write.
	
    //PWM_TIMER_FIRE_Start();
    bool calAccel = 1, calGyro = 1, calMag = 1;
    status = enableCal(calAccel, calGyro, calMag);
    while(status != 0)
    {
        
    }   
    //PWM_TIMER_FIRE_Start();
    /****LOOP****/
    for(;;)
    {
        if(accCount >= 1000 && cal != true)
        {
            cal = true;
            LED_R_Write(1);
            printOut("IMU calibrated\r\n");
            sh2_saveDcdNow();  
        }
        sh2_service();
        
        for(int i = 0; i < eventsBuffindex; i++)
        {
            sensor_event = eventsBuff[i];
            status = sh2_decodeSensorEvent(&value, &sensor_event); //sensor_event fluctuates type of data it is outputting randomly. Use flags to detect when certain sensor is selected.
            switch(sensor_event.reportId)
            {
                case SH2_GAME_ROTATION_VECTOR:                  //detection of the event being for a certain sensor.
                {
                    got_rot = 1;
                    transmitBuf[0] = value.un.gameRotationVector.i;
                    transmitBuf[1] = value.un.gameRotationVector.j;
                    transmitBuf[2] = value.un.gameRotationVector.k;
                    transmitBuf[3] = value.un.gameRotationVector.real;
                    gameAccuracy = (sensor_event.report[2] & 0x03);
                    break;
                }
                case SH2_ACCELEROMETER:
                {
                    got_accel = 1;
                    transmitBuf[4] = value.un.accelerometer.x; 
                    transmitBuf[5] = value.un.accelerometer.y; 
                    transmitBuf[6] = value.un.accelerometer.z;
                    accelAccuracy = (sensor_event.report[2] & 0x03);
                    break;
                }
                case SH2_GYROSCOPE_CALIBRATED:
                {
                    got_gyro = 1;
                    transmitBuf[7] = value.un.gyroscope.x; 
                    transmitBuf[8] = value.un.gyroscope.y; 
                    transmitBuf[9] = value.un.gyroscope.z;
                    gyroAccuracy = (sensor_event.report[2] & 0x03);   
                    break;
                }
                case SH2_MAGNETIC_FIELD_CALIBRATED:
                {
                    got_mag = 1;
                    magAccuracy = (sensor_event.report[2] & 0x03);
                }
            }
            #ifdef CALIBRATE_MAG_MODE
            if (got_accel && got_gyro && got_rot && got_mag)
            {
                #ifdef  USBUART_MODE
                #ifdef DATA_OUTPUT_MODE    
                USBUART_PutData(( void *)transmitBuf,40);
                #endif
                print10(transmitBuf,gameAccuracy, accelAccuracy, gyroAccuracy, magAccuracy);
                if(gameAccuracy >= 2 && gyroAccuracy >= 2 && accelAccuracy >= 2 && magAccuracy >=2)
                    accCount ++;    
                else
                {
                    cal = false;
                    accCount = 0;
                    LED_R_Write(0);
                }
                
                #endif
                got_accel = 0;
                got_gyro = 0;
                got_rot = 0;
                got_mag = 0;
            }
            #endif
            
            #ifndef CALIBRATE_MAG_MODE
            if (got_accel && got_gyro && got_rot)
            {         
                TIMER_FIRING_PIN_Write(0);
                #ifdef  USBUART_MODE
                #ifdef DATA_OUTPUT_MODE    
                USBUART_PutData(( void *)transmitBuf,40);
               
                #endif
                print10(transmitBuf,gameAccuracy, accelAccuracy, gyroAccuracy);
                if(gameAccuracy >= 2 && gyroAccuracy >= 2 && accelAccuracy >= 2)
                    accCount ++;    
                else
                {
                    cal = false;
                    accCount = 0;
                    LED_R_Write(0);
                }
                #endif
                TIMER_FIRING_PIN_Write(1);
                tdf = getTdiff();
                got_accel = 0;
                got_gyro = 0;
                got_rot = 0;
            }
            #endif   
            
        } //end printing events for loop
        eventsBuffindex = 0; //reset our pointer for eventsBuffer so we print new values.
        
    } //End For loop
} //end Main

    
/* [] END OF FILE */
