#!/usr/bin/env python

import time

LED_PATH = "/sys/class/leds/ev3:%(color)s:%(direction)s/brightness"

def do_up_down(f1, f2, f3, f4):
	for i in range(0, 256):
		val1 = str(i)
		val2 = str(255-i)
		if f1:
			f1.write(val1)
		if f2:
			f2.write(val2)
		if f3:
			f3.write(val2)
		if f4:
			f4.write(val1)
		time.sleep(0.03)

def main():
	f1 = open(LED_PATH % { "color": "green", "direction": "left" }, "w", 0)
	f2 = open(LED_PATH % { "color": "red", "direction": "left" }, "w", 0)
	f3 = open(LED_PATH % { "color": "green", "direction": "right" }, "w", 0)
	f4 = open(LED_PATH % { "color": "red", "direction": "right" }, "w", 0)
	# assumes starting with green LEDs illuminated
	# dims green LEDs only to off
	do_up_down(None, f1, f3, None)
	time.sleep(0.5)
	# brings red LEDs only to full
	do_up_down(f2, None, None, f4)
	time.sleep(0.1)
	# fades left LED from red to green and right LED from green to red
	do_up_down(f1, f2, f3, f4)
	# fades left LED from green to red and right LED from red to green
	do_up_down(f2, f1, f4, f3)
	# return to full green only state
	f1.write("255")
	f2.write("0")
	f3.write("255")
	f4.write("0")

if __name__ == "__main__":
	main()