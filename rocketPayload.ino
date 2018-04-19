/*
Skeleton Code for the Payload Lauch Project
Spacecraft Club
Last Updated: April 2, 2018 by Wesley Stirk
*/

#define NUM_SENSORS	3

void setup() {
  //Initialize Pins
  
  setupIMU();
  setupSD();
  setupTemp();
  waitLaunch();

}

void loop() {
	float readings[NUM_SENSORS+1]; //One element for each temp sensor and one 
									//for time. We could add another for IMU 
									//data if we wanted. 
	readTemp(readings);
	saveData(readings);
}

//Board team will write these

/*Sets up everything needed for the IMU*/
void setupIMU()
{
	
}

/*Sets up everything needed for the SD memory*/
void setupSD()
{
	
}

/*Puts the board in low power mode and waits for a signal from the 
IMU that the rocket has launched*/
void waitLaunch()
{
	
}

/*Saves the data stored in the array to the SD card*/
void saveData(float reading[])
{
	
}



//Temperature team will write these

/*Sets up everything needed for the temperature sensors when first powered on*/
void setupTem()
{
	
}

/*Gets the data from the temperature sensors and saves it in the array*/
void readTemp(float readings[])
{
	
}