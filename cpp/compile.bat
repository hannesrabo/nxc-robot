arm-none-linux-gnueabi-g++ -Wall -mcpu=arm9 -O -std=c++11 -Wall -g -mcpu=arm9 -lpthread -c main.cpp
arm-none-linux-gnueabi-g++ -lpthread -o out.exe ev3dev.o main.o
