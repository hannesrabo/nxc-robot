#ifndef ROBOT_GUARD
#define ROBOT_GUARD

//Include libraries
#include <iostream>
#include <thread>
#include "ev3dev.h"
#include <unistd.h>
#include <exception>
#include <chrono>
#include <ctime>

#endif // ROBOT_GUARD

//defines constants in program
#define SPEED_ROTATE 100
#define SPEED_SEARCH 100
#define LIGHT_THRESHOLD 25
#define ARENA_DIAMETER 850
#define CLOSE_UP_TARGET 100
#define SPEED_PANIC_TURN 100
#define TIME_PANIC_TURN 2500
#define TIME_PANIC_FREE 1500
#define TIME_LINE_CHECKER 200
#define PANIC_ROTATION_FACTOR 0.5

using namespace ev3dev;
using namespace std;

//Define sensors and motors
large_motor motorLeft(OUTPUT_D);
large_motor motorRight(OUTPUT_A);

touch_sensor touchLeft(INPUT_1);
touch_sensor touchRight(INPUT_4);
ultrasonic_sensor ultrasonicSensor(INPUT_2);
color_sensor colorSensor(INPUT_3);

//internal variables
bool motorsActive = false;								//Block motors from beeing used by multiple threads

int rotationDirection = 1;								//Not used in this version
time_t lastRotationSwitch;		
time_t currTime;

int ultrasonicMeasurement = 0;							//Stor ultrasonic measurement from this and last time.
int lastUltrasonicMeasurement = ARENA_DIAMETER + 10;

//Check sensors and mototrs connected
bool initialized() {
	return (
		motorLeft.connected() 	&&
		motorRight.connected() 	&&
		touchLeft.connected() 	&&
		touchRight.connected() 	&&
		ultrasonicSensor.connected() &&
		colorSensor.connected()
	);
	
}

//drive straight
void drive(int speed) {
	motorLeft.set_duty_cycle_sp(-speed).run_forever();
	motorRight.set_duty_cycle_sp(-speed).run_forever();

}

//controll both motors individually (Can be used to turn)
void drive(int speedL, int speedR) {
	motorLeft.set_duty_cycle_sp(-speedL).run_forever();
	motorRight.set_duty_cycle_sp(-speedR).run_forever();
	
}

//syncronized stop of both motors
void stop() {
	motorLeft.stop();
	motorRight.stop();
	
}

//Rotate a number of degrees (not in use in this version)
void rotate(int direction) {
	stop();
	
	motorLeft.set_position_sp(direction).set_duty_cycle_sp(SPEED_ROTATE).run_to_rel_pos();
	motorRight.set_position_sp(-direction).set_duty_cycle_sp(SPEED_ROTATE).run_to_rel_pos();
	
	while(motorLeft.state().count("running") || motorRight.state().count("running"))
		usleep(10000);
	
	stop();
	motorLeft.reset();
	motorRight.reset();
	
}

//check if on white/black
bool isOnWhite() {
	if(colorSensor.value() > LIGHT_THRESHOLD) {	
		return true;
	} else {
		return false;
	}
}

//If the robot is towards the target
bool towardsTarget() {
	
	ultrasonicMeasurement = ultrasonicSensor.value();
	
	//If the value is less than the size of the arena (the blocking object must be the other robot if it is in the arena).
	if(ultrasonicSensor.value() < ARENA_DIAMETER && lastUltrasonicMeasurement < ARENA_DIAMETER) {
		lastUltrasonicMeasurement = ultrasonicMeasurement; //Store mesurement to next time 
		return true;
	}else{
		lastUltrasonicMeasurement = ultrasonicMeasurement;
		return false;
	}
}

//if close to target (not in use)
bool onTarget() {
	if(ultrasonicSensor.value() < CLOSE_UP_TARGET)
		return true;
	else
		return false;
	
}

//find target
void turnTowardsTarget() {
	
	try{
		
		drive(SPEED_SEARCH*rotationDirection, -SPEED_SEARCH*rotationDirection);
		
		//Turn until the other robot is detected
		while(!towardsTarget()) {
			this_thread::sleep_for(chrono::milliseconds(25));
		}
				
		stop();
		
	} catch(exception& e) {
		cout << e.what() << endl;
		
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(200));
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(200));
		sound::beep();
	}
	
}

//one of the main threads - finds the other robot
void findRobot() {
	
	time(&lastRotationSwitch);		//Set starttime for rotation switch monotoring (not in use)
	
	while(true) {
		
		try{
			if(towardsTarget()){
				if(!motorsActive){				
					drive(100);				//Drive forward if the robot is in from of it
				}
			}else {
				if(!motorsActive){
					//Block motors
					motorsActive = true;
					
					turnTowardsTarget();	//Find the robot if the robot is not in front of it
					
					motorsActive = false;
				}
			}
			
			this_thread::sleep_for(chrono::milliseconds(25));			//Sleep to limit the use of the processor.
		
		//catch exception from system controll	
		} catch (exception& e) {
			cout << e.what() << endl;
			
			sound::beep();
			this_thread::sleep_for(chrono::milliseconds(200));
			sound::beep();
		}	
	}
	
	
}

//main thread - find edge
void findEdge() {
	
	while(true) {
		try{
			//If the robot doesn't have another task right now
			if(!motorsActive){
				
				//Block motors
				motorsActive = true;
				
				// If the robot is on the black line
				if(!isOnWhite()) {
					
					//If the robot is touching the other robot (either sensor is pushed)
					if(touchLeft.value() || touchRight.value()) {
						
						//right behind
						if(touchRight.value() && touchLeft.value()) {
							
							//Search for white to the left and continue the move if found.
							//Else use the other direction
							drive(-SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR, -SPEED_PANIC_TURN);
							this_thread::sleep_for(chrono::milliseconds(TIME_LINE_CHECKER));
							
							if(isOnWhite()) {
								drive(-SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR, -SPEED_PANIC_TURN);
								this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
								
							} else {
								drive(-SPEED_PANIC_TURN, -SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR);
								this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN + TIME_LINE_CHECKER));
								
							}
							
							
						//only Right
						}else if(touchRight.value()) {
							drive(-SPEED_PANIC_TURN, -SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						//only Left
						}else {
							drive(-SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR, -SPEED_PANIC_TURN);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						}

					//Nothing pressed : The robot ran into the edge by itself (Easier escape)
					} else {
						
						//Go backwards a small distance, then find the target.
						drive(-100);
						this_thread::sleep_for(chrono::milliseconds(250));
						turnTowardsTarget();
						stop();

					}
				
				//If it is in the white area (not on the black line)
				} else {
					//If the robot is touching the other robot (beeing pushed on either sensor)
					if(touchLeft.value() || touchRight.value()) {
						
						//To the right or behind
						if(touchRight.value()) {
							
							//Turn to the right backwards
							drive(SPEED_PANIC_TURN, -SPEED_PANIC_TURN);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						//Left
						}else if(touchLeft.value()) {
							
							//Turn to the left backwards
							drive(-SPEED_PANIC_TURN, -SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						}
						
					}
					
				}
				
				//Release motors
				motorsActive = false;
			
			}
			
			this_thread::sleep_for(chrono::milliseconds(25));
			
		//if anything goes wrong in the main : catch and start over in the main loop
		}catch(exception& e){
			cout << e.what() << endl;
			
			sound::beep();
		}
	}
	
}

int main(){
	
	//Check if everything is connected
	if(!initialized()) {
		
		cout << "Cables missing" << endl;
		
		cout << "Light sensor: " << colorSensor.connected() << endl;
		cout << "Left bumper: " << touchLeft.connected() << endl;
		cout << "Right bumper: " << touchRight.connected() << endl;
		cout << "Ultrasonic: " << ultrasonicSensor.connected() << endl;
		
		cout << "Right motor: " << motorRight.connected() << endl;
		cout << "Left motor: " << motorLeft.connected() << endl;
		
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(250));
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(250));
		sound::beep();
		
		this_thread::sleep_for(chrono::milliseconds(10000));
		
	//Everyting is connected: Run programs
	}else {
		
		//Wait until the enter button is pressed. (Makes for a quicker start)
		while(!button::enter.pressed()){
			this_thread::sleep_for(chrono::milliseconds(20));
			
		}
		
		//Start threads
		thread findRobotThread (findRobot);
		thread findEdgeThread(findEdge);
		
		//Wait until all threds have finished (Should not happen)
		findRobotThread.join();
		findEdgeThread.join();
		
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(100));
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(100));
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(100));
		sound::beep();
		this_thread::sleep_for(chrono::milliseconds(100));
		sound::beep();		
		
		this_thread::sleep_for(chrono::milliseconds(20000));

	}
    
	return 0;
}
