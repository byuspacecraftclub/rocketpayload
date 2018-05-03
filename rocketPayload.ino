/*
Payload Lauch Project Code
Spacecraft Club
Last Updated: April 18, 2018 by Wesley Stirk
*/

#include "quaternionFilters.h"
#include "MPU9250.h"

#include <SPI.h>
#include <SD.h>

#define NUM_SENSORS	3

#define SerialDebug true

MPU9250 myIMU;
File dataLog;


void setup() {
  //Initialize Pins
  Serial.begin(9600);
  setupIMU();
  setupSD();
  setupTemp();
  waitLaunch();

}

void loop() {
	uint16_t readings[NUM_SENSORS+1]; //One element for each temp sensor and one 
									//for time. We could add another for IMU 
									//data if we wanted. 
	readTemp(readings);
	saveData(readings);
}

//Board team will write these

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
}

/*Sets up everything needed for the SD memory*/
void setupSD()
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
	}
	
	dataLog = SD.open("datalog.csv");
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
}

/*Puts the board in low power mode and waits for a signal from the 
IMU that the rocket has launched*/
void waitLaunch()
{
	//Putting the IMU into interrupt mode. 
	/*uint8_t fromIMU = myIMU.readByte(MPU9250_ADDRESS, PWR_MGMT_1	);
	uint8_t combo = 0x8f;
	uint8_t intoIMU = fromIMU & combo;
	myIMU.writeByte(MPU9250_ADDRESS, PWR_MGMT_1, intoIMU);
	
	fromIMU = myIMU.readByte(MPU9250_ADDRESS, PWR_MGMT_2);*/

	uint8_t index[8];
	index[0] = 4;
	index[1] = 5;
	index[2] = 6;
	changeReg0(PWR_MGMT_1, index, 3);

	index[0] = 3;
	index[1] = 4;
	index[2] = 5;
	changeReg0(PWR_MGMT_2, index, 2);

	index[0] = 0;
	index[1] = 1;
	index[2] = 2;
	changeReg1(PWR_MGMT_2, index, 2);

	index[0] = 3; //FIXME something was weird about the datasheet for this register. 
	index[1] = 0; //It didn't agree with the instructions. I did my best to guess. 
	changeReg1(ACCEL_CONFIG2, index, 2);
	index[0] = 2;
	index[1] = 1;
	changeReg0(ACCEL_CONFIG2, index, 2);
	
	myIMU.writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x40);
	
	index[0] = 6;
	index[1] = 7;
	changeReg1(MOT_DETECT_CTRL, index, 2);
	
	myIMU.writeByte(MPU9250_ADDRESS, WOM_THR, 0xff); // this is the wake on motion threshold. I set it as high as possible. Not sure if that is best or not. 
	
	myIMU.writeByte(MPU9250_ADDRESS, LP_ACCEL_ODR, 0x01);
	//this is the frequency of wake up. The instructions didnt' give much guidance on how to set this properly.
	
	index[0] = 5;
	changeReg1(PWR_MGMT_1, index, 1);
	
	//IMU setup for interrupt signal
	
	//put CPU to sleep 
	
	

 
}

//sets specific bits in registers to 0.
void changeReg0(uint8_t regName, uint8_t index[], int size)
{
	uint8_t fromIMU = myIMU.readByte(MPU9250_ADDRESS, regName);
	uint8_t combo = 0xff;
	for(uint8_t i = 0; i < size; ++i)
	{
		uint8_t subNum = 0x01;
		for(uint8_t j = 0; j < index[i]; ++j)
		{
			subNum == subNum << 1;
		}
		combo = combo - subNum;
	}
	uint8_t intoIMU = fromIMU & combo;
	myIMU.writeByte(MPU9250_ADDRESS, regName, intoIMU);
 //return true;
}

//sets specific bits in registers to 0.
void changeReg1(uint8_t regName, uint8_t index[], uint8_t size)
{
	uint8_t fromIMU = myIMU.readByte(MPU9250_ADDRESS, regName);
	uint8_t combo = 0;
	for(uint8_t i = 0; i < size; ++i)
	{
		uint8_t addNum = 0x01;
		for(uint8_t j = 0; j < index[i]; ++j)
		{
			addNum == addNum << 1;
		}
		combo = combo + addNum;
	}
	uint8_t intoIMU = fromIMU | combo;
	myIMU.writeByte(MPU9250_ADDRESS, regName, intoIMU);
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
	
}

/*Gets the data from the temperature sensors and saves it in the array*/
void readTemp(uint16_t readings[])
{
	
}
