#ifndef ROBOT_GUARD
#define ROBOT_GUARD

#include <iostream>
#include <thread>
#include "ev3dev.h"
#include <unistd.h>
#include <exception>
#include <chrono>
#include <ctime>

#endif // ROBOT_GUARD

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

bool motorsActive = false;
int rotationDirection = 1;
time_t lastRotationSwitch;
time_t currTime;
int ultrasonicMeasurement = 0;
int lastUltrasonicMeasurement = ARENA_DIAMETER + 10;

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

void drive(int speed) {
	motorLeft.set_duty_cycle_sp(-speed).run_forever();
	motorRight.set_duty_cycle_sp(-speed).run_forever();

}

void drive(int speedL, int speedR) {
	motorLeft.set_duty_cycle_sp(-speedL).run_forever();
	motorRight.set_duty_cycle_sp(-speedR).run_forever();
	
}

void stop() {
	motorLeft.stop();
	motorRight.stop();
	
}

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

bool isOnWhite() {
	if(colorSensor.value() > LIGHT_THRESHOLD) {	
		return true;
	} else {
		return false;
	}
}


bool towardsTarget() {
	
	ultrasonicMeasurement = ultrasonicSensor.value();
	
	if(ultrasonicSensor.value() < ARENA_DIAMETER && lastUltrasonicMeasurement < ARENA_DIAMETER) {
		lastUltrasonicMeasurement = ultrasonicMeasurement;
		return true;
	}else{
		lastUltrasonicMeasurement = ultrasonicMeasurement;
		return false;
	}
}

bool onTarget() {
	if(ultrasonicSensor.value() < CLOSE_UP_TARGET)
		return true;
	else
		return false;
	
}

void turnTowardsTarget() {
	
	try{
		
		drive(SPEED_SEARCH*rotationDirection, -SPEED_SEARCH*rotationDirection);
		
		while(!towardsTarget()) {
			this_thread::sleep_for(chrono::milliseconds(25));
		}
		
		this_thread::sleep_for(chrono::milliseconds(50));
		
		stop();
		
		time(&currTime);
		if(difftime(currTime,lastRotationSwitch) > 0.3) {
			rotationDirection *= -1;
			lastRotationSwitch = currTime;
			sound::beep();
			
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

void findRobot() {
	
	//Set starttime for rotation switch monotoring
	time(&lastRotationSwitch);
	
	
	while(true) {
		
		try{
			if(towardsTarget()){
				if(!motorsActive){				
					drive(100);
				}
			}else {
				if(!motorsActive){
					motorsActive = true;
					
					turnTowardsTarget();
					
					motorsActive = false;
				}
			}
			this_thread::sleep_for(chrono::milliseconds(25));
			
		} catch (exception& e) {
			cout << e.what() << endl;
			
			sound::beep();
			this_thread::sleep_for(chrono::milliseconds(200));
			sound::beep();
		}	
	}
	
	
}

void findEdge() {
	
	while(true) {
		try{
			//If the robot doesn't have another task right now
			if(!motorsActive){
				motorsActive = true;
				
				// If the robot is on the black line
				if(!isOnWhite()) {
					
					//If the robot is touching the other robot (beeing pushed)
					if(touchLeft.value() || touchRight.value()) {
						//right behind
						if(touchRight.value() && touchLeft.value()) {
							//Search for white to the right and continue the move if found.
							//Else try the other direction
							drive(-SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR, -SPEED_PANIC_TURN);
							this_thread::sleep_for(chrono::milliseconds(TIME_LINE_CHECKER));
							if(isOnWhite()) {
								drive(-SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR, -SPEED_PANIC_TURN);
								this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
								
							} else {
								drive(-SPEED_PANIC_TURN, -SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR);
								this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN + TIME_LINE_CHECKER));
								
							}
							
							
						//Right
						}else if(touchRight.value()) {
							drive(-SPEED_PANIC_TURN, -SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						//Left
						}else {
							drive(-SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR, -SPEED_PANIC_TURN);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						}

					//The robot ran into the edge by itself (probably)
					} else {
						drive(-100);
						this_thread::sleep_for(chrono::milliseconds(250));
						turnTowardsTarget();
						stop();

					}
				
				//If it is in the white area
				} else {
					//If the robot is touching the other robot (beeing pushed)
					if(touchLeft.value() || touchRight.value()) {
						//To the right or right behind
						if(touchRight.value()) {
							
							drive(SPEED_PANIC_TURN, -SPEED_PANIC_TURN);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						//Left
						}else if(touchLeft.value()) {
							drive(-SPEED_PANIC_TURN, -SPEED_PANIC_TURN*PANIC_ROTATION_FACTOR);
							this_thread::sleep_for(chrono::milliseconds(TIME_PANIC_TURN));
							
						}
						
					}
					
				}
				
				motorsActive = false;
			
			}
			
			this_thread::sleep_for(chrono::milliseconds(25));
		}catch(exception& e){
			cout << e.what() << endl;
			
			sound::beep();
		}
	}
	
}

int main(){
	
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
		
	}else {
		
		while(!button::enter.pressed()){
			this_thread::sleep_for(chrono::milliseconds(20));
			
		}
		
		thread findRobotThread (findRobot);
		thread findEdgeThread(findEdge);

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
