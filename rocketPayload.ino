/*
Payload Lauch Project Code
Spacecraft Club
Last Updated: May 15, 2018 by Wesley Stirk
*/

#include "quaternionFilters.h"
#include "MPU9250.h"

#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>
#include <stdint.h>


#define NUM_SENSORS  3

#define SerialDebug true

#define INTERRUPT_PIN 2 //This is the arduino pin that will have the interrupt from the IMU

#define pinsize  4 //size of fixed array of analog pins

#define MINUTES_TO_MEASURE 10
#define SECS_IN_MIN 60
#define MILLIS_IN_SEC 1000
#define TOTAL_MILLIS MINUTES_TO_MEASURE * SECS_IN_MIN * MILLIS_IN_SEC

MPU9250 myIMU;
File dataLog;
static int fileAttempt = 0;
static uint8_t pins[pinsize] = {23,24,25,26}; //numbers of analog pins



void setup() {
  //Initialize Pins
  Serial.begin(9600);
  
  pinMode(INTERRUPT_PIN, INPUT);
  fileAttempt = 0;
  setupIMU();
  setupSD();
  setupTemp();
  

}

void loop() {
  waitLaunch(); //It will wait here every time until another interrupt is received. 
  unsigned long startTime = millis();
  unsigned long currentTime = millis();
  setupSD();
  while ((currentTime - startTime) < TOTAL_MILLIS) //keep reading temperature sensors and recording them until the time is over. 
  {
    uint8_t readings[NUM_SENSORS+1]; //One element for each temp sensor and one 
                    //for time. We could add another for IMU 
                    //data if we wanted. 
    readTemp(readings);
    saveData(readings);
    currentTime = millis();
    //Should we add a delay here?
  }
  dataLog.close();
}


//Board team will write these

void isr()
{
  
}


/*Sets up everything needed for the IMU*/
void setupIMU()
{
  // Read the WHO_AM_I register, this is a good test of communication
  byte c = myIMU.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  if(SerialDebug)
  {
    Serial.print("MPU9250 "); Serial.print("I AM "); Serial.print(c, HEX);
    Serial.print(" I should be "); Serial.println(0x71, HEX);
  }

  if (c == 0x71) // WHO_AM_I should always be 0x68
  {
    
    myIMU.MPU9250SelfTest(myIMU.selfTest);
    if(SerialDebug)
    {
    Serial.println("MPU9250 is online...");

    // Start by performing self test and reporting values
      
      Serial.print("x-axis self test: acceleration trim within : ");
      Serial.print(myIMU.selfTest[0],1); Serial.println("% of factory value");
      Serial.print("y-axis self test: acceleration trim within : ");
      Serial.print(myIMU.selfTest[1],1); Serial.println("% of factory value");
      Serial.print("z-axis self test: acceleration trim within : ");
      Serial.print(myIMU.selfTest[2],1); Serial.println("% of factory value");
      Serial.print("x-axis self test: gyration trim within : ");
      Serial.print(myIMU.selfTest[3],1); Serial.println("% of factory value");
      Serial.print("y-axis self test: gyration trim within : ");
      Serial.print(myIMU.selfTest[4],1); Serial.println("% of factory value");
      Serial.print("z-axis self test: gyration trim within : ");
      Serial.print(myIMU.selfTest[5],1); Serial.println("% of factory value");
    }

    // Calibrate gyro and accelerometers, load biases in bias registers
    myIMU.calibrateMPU9250(myIMU.gyroBias, myIMU.accelBias);

    
     myIMU.initMPU9250();
    // Initialize device for active mode read of acclerometer, gyroscope, and
    // temperature
    if(SerialDebug)
    {
      Serial.println("MPU9250 initialized for active data mode....");
    }

    
  } // if (c == 0x71)
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
    while(1) ; // Loop forever if communication doesn't happen
  }


  //Now to put the IMU into wake-on-motion interrupt mode. 
  //Putting the IMU into interrupt mode. 
  uint8_t index[8];
  index[0] = 4;
  index[1] = 5;
  index[2] = 6;
  changeReg0(PWR_MGMT_1, index, 3);
  

  index[0] = 3;
  index[1] = 4;
  index[2] = 5;
  changeReg0(PWR_MGMT_2, index, 3);

  index[0] = 0;
  index[1] = 1;
  index[2] = 2;
  changeReg1(PWR_MGMT_2, index, 3);

  index[0] = 3; 
  index[1] = 0; 
  changeReg1(ACCEL_CONFIG2, index, 2);
  index[0] = 2;
  index[1] = 1;
  changeReg0(ACCEL_CONFIG2, index, 2);
  
  myIMU.writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x40);
  
  index[0] = 6;
  index[1] = 7;
  changeReg1(MOT_DETECT_CTRL, index, 2);
  
  myIMU.writeByte(MPU9250_ADDRESS, WOM_THR, 0xff); // this is the wake on motion threshold. I set it as high as possible. Not sure if that is best or not. 


  index[0] = 3;
  index[1] = 1;
  index[2] = 0;
  changeReg1(LP_ACCEL_ODR, index, 3); //this is the frequency of wake up. The instructions didnt' give much guidance on how to set this properly.

  index[0] = 5;
  changeReg0(INT_PIN_CFG, index, 1); //This insures that interrupt only last for 50us.
  
  index[0] = 5;
  changeReg1(PWR_MGMT_1, index, 1);  
  
}

/*Sets up everything needed for the SD memory*/
bool setupSD()
{
  if(SD.begin()) //This will default to the hardware SS pin 
          //unless we want to specify another pin. 
  {
    if(SerialDebug)
    {
      Serial.println("SD card initialized");
    }
  }
  else
  {
    if(SerialDebug)
    {
      Serial.println("SD card failed to initialize");
    }
   return false;
  }
  //String fileName = "datalog" + fileAttempt + ".csv";
  //dataLog = SD.open(fileName);
  dataLog.print("time,");
  int i = 0;
  for(i = 1; i < NUM_SENSORS; ++i)
  {
    dataLog.print("temp ");
    dataLog.print(i);
    dataLog.print(",");
  }
  dataLog.print("temp "); //outside of the loop to be the 
              //last column
  dataLog.println(i);
 return true;
}

/*Puts the board in low power mode and waits for a signal from the 
IMU that the rocket has launched*/
void waitLaunch()
{
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), isr, RISING);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  if(SerialDebug)
  {
    Serial.println("Entering Sleep mode....");
  }
  sleep_mode(); //It waits here until it receives an interrupt signal. Then it will move on. 
  sleep_disable();
  if(SerialDebug)
  {
      Serial.println("Arduino has woken up now.");
  }
  detachInterrupt(INTERRUPT_PIN);
 
}

//sets specific bits in registers to 0.
void changeReg0(uint8_t regName, uint8_t index[], uint8_t indSize)
{
  uint8_t reg = myIMU.readByte(MPU9250_ADDRESS, regName);
  for(uint8_t i = 0; i < indSize; ++i)
  {
    reg = reg & ~(0x01 << index[i]);
  }
  
  myIMU.writeByte(MPU9250_ADDRESS, regName, reg);

}

//sets specific bits in registers to 0.
void changeReg1(uint8_t regName, uint8_t index[], uint8_t indSize)
{
  uint8_t reg = myIMU.readByte(MPU9250_ADDRESS, regName);
  for(uint8_t i = 0; i < indSize; ++i)
  {
    reg = reg | (0x01 << index[i]);
  }
  
  myIMU.writeByte(MPU9250_ADDRESS, regName, reg);
}


/*Saves the data stored in the array to the SD card*/
//Currently set up to save the data in a csv file
//dataLog must be open before this function is called. 
void saveData(uint8_t reading[])
{
  for(uint8_t i = 0; i < NUM_SENSORS + 1; ++i)
  {
    if( i != NUM_SENSORS)
    {
      dataLog.print(reading[i]);
      dataLog.print(",");
    }
    else
    {
      dataLog.println(reading[i]);
    }
  }
}



//Temperature team will write these

/*Sets up everything needed for the temperature sensors when first powered on*/
void setupTemp()
{
    analogReference(INTERNAL); //set the max voltage to a lower value for more accuracy
}

/*Gets the data from the temperature sensors and saves it in the array*/
void readTemp(uint8_t readings[])
{
  for(uint8_t i; i < NUM_SENSORS; i++) //iterates through each of the sensors
  {
    readings[i] = analogRead(pins[i]); //reads from the analog pin and places in the proper place
    
    if(SerialDebugging)
    {
      Serial.print("analog voltage value: "); //outputs read value
      Serial.println(readings[i]);
    }
  }
}
