/*
Payload Lauch Project Code
Spacecraft Club
Last Updated: June 11, 2018 by Wesley Stirk
*/
#include "MPU9250.h"

#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>
#include <stdint.h>


#define NUM_SENSORS  2

//#define SerialDebug 

#define INTERRUPT_PIN 2 //This is the arduino pin that will have the interrupt from the IMU
#define LED_PIN 5
#define TEMP1_PIN 0 //FIXME
#define TEMP2_PIN 0

#define MINUTES_TO_MEASURE .25
#define SECS_IN_MIN 60L
#define MILLIS_IN_SEC 1000L
#define TOTAL_MILLIS MINUTES_TO_MEASURE * SECS_IN_MIN * MILLIS_IN_SEC
#define FILE_NAME "e.csv"


File dataLog;

void setup() {
  //Initialize Pins
#ifdef SerialDebug
  Serial.begin(9600);
#endif
  
  
  pinMode(INTERRUPT_PIN, INPUT);
  pinMode(TEMP1_PIN, INPUT);
  pinMode(TEMP2_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  TurnedOn(); //let the world now that it is on and functioning. 
  
  setupIMU();
  if(!setupSD()) //if the SD card doesn't work or the file doesn't open, there is a critical error and the program won't work. 
  {
    while(1)
    {
        ErrorMessage();
    }
  }
  setupTemp();
  

}

void loop() {
  waitLaunch(); //It will wait here every time until another interrupt is received. 
  //setupSD();
  dataLog = SD.open(F(FILE_NAME), FILE_WRITE);
  while(!dataLog) //if the dataLog doesn't open it's a critical error so keep trying to open until it works. 
  {
    ErrorMessage();
    dataLog = SD.open(F(FILE_NAME), FILE_WRITE); 
  }
  uint32_t startTime = millis();
  uint32_t currentTime = millis();
 
  while ((currentTime - startTime) < TOTAL_MILLIS) //keep reading temperature sensors and recording them until the time is over. 
  {
#ifdef SerialDebug
      Serial.println(F("in the loop"));
#endif
    uint16_t readings[NUM_SENSORS+1]; //One element for each temp sensor and one 
                    //for time. We could add another for IMU 
                    //data if we wanted. 
    readTemp(readings);
    saveData(readings);
    currentTime = millis();
    delay(250);//How long should the sample rate be?
  }
  
  dataLog.println(F("next sample"));
  dataLog.close();
}


//Board team will write these

void isr()
{
  
}

/*
 * Delivers a LED message saying that it is turned on.
 */
void TurnedOn()
{
  for(int i = 0; i < 12; ++i)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
  }
}

/*
 * Delivers an LED message that there was an error somewhere in the program. 
 */
void ErrorMessage()
{
  for(int i = 0; i < 3; ++i)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(50);
  }

  delay(250);
}

/*
 * Delivers and LED message that another valid check was achieved. 
 */
void GoodNews()
{
  for(int i = 0; i < 2; ++i)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(50);
  }
}
 

/*Sets up everything needed for the IMU*/
void setupIMU()
{
  MPU9250 myIMU;
  
#ifdef SerialDebug
  Serial.println(F("Starting register read/write"));
#endif
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


  index[0] = 1;
  index[1] = 0;
  //index[2] = 3;
  changeReg1(LP_ACCEL_ODR, index, 2); //this is the frequency of wake up. The instructions didnt' give much guidance on how to set this properly.

  index[0] = 5;
  changeReg0(INT_PIN_CFG, index, 1); //This insures that interrupt only last for 50us.
  
  index[0] = 5;
  changeReg1(PWR_MGMT_1, index, 1);  
  
}

/*Sets up everything needed for the SD memory*/
bool setupSD()
{
#ifdef SerialDebug
  Serial.println(F("Now setting up the SD card...."));
#endif
  pinMode(10,OUTPUT); //needed to make the software happy. 
  if(SD.begin()) //This will default to the hardware SS pin 
          //unless we want to specify another pin. 
  {
#ifdef SerialDebug
      Serial.println(F("SD card initialized"));
#endif
    GoodNews();
  }
  else
  {
#ifdef SerialDebug
      Serial.println(F("SD card failed to initialize"));
#endif
      ErrorMessage();
      return false;
  }

  dataLog = SD.open(F(FILE_NAME), FILE_WRITE);
  if(!dataLog)
  {
#ifdef SerialDebug
      Serial.println(F("error opening the file! It didn't work!!"));
#endif
    ErrorMessage();
    return false;
  }
  else
  {
#ifdef SerialDebug
      Serial.println(F("File Opened Successfully"));
#endif
  }
  
  dataLog.print(F("time,"));
  uint8_t i = 0;
  for(i = 1; i < NUM_SENSORS; ++i)
  {
    dataLog.print(F("temp "));
    dataLog.print(i);
    dataLog.print(",");
  }
  dataLog.print(F("temp ")); //outside of the loop to be the 
              //last column
  dataLog.println(i);
  dataLog.close(); //it will be opened again in the loop so we need to close it here. 
  GoodNews();
  return true;
}

/*Puts the board in low power mode and waits for a signal from the 
IMU that the rocket has launched*/
void waitLaunch()
{
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), isr, RISING);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
#ifdef SerialDebug
    Serial.println(F("Entering Sleep mode...."));
    Serial.flush();
#endif
  sleep_mode(); //It waits here until it receives an interrupt signal. Then it will move on. 
  sleep_disable();
#ifdef SerialDebug
      Serial.println(F("Arduino has woken up now."));
#endif
  detachInterrupt(INTERRUPT_PIN);
 
}

//sets specific bits in registers to 0.
void changeReg0(uint8_t regName, uint8_t index[], uint8_t indSize)
{
  MPU9250 myIMU;
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
  MPU9250 myIMU;
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
void saveData(uint16_t reading[])
{
  for(uint8_t i = 0; i < NUM_SENSORS + 1; ++i)
  {
    if( i != NUM_SENSORS)
    {
      dataLog.print(reading[i]);
      dataLog.print(F(","));
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
    if(analogRead(TEMP1_PIN) == 0 || analogRead(TEMP2_PIN) == 0) //error
    {
        if(analogRead(TEMP1_PIN) == 0 && analogRead(TEMP2_PIN) == 0) //critical error
        {
            while(1) //stop the program.
            {
              ErrorMessage(); 
            }
        }
    }
    
}

/*Gets the data from the temperature sensors and saves it in the array*/
void readTemp(uint16_t readings[])
{
  //randomSeed(millis());
  readings[0] = millis();
  readings[1] = analogRead(TEMP1_PIN);
  readings[2] = analogRead(TEMP2_PIN);
}
