TARGET = hcan4mqttpc


SRC = $(wildcard *.c)
SRC += $(wildcard ./MQTTPacket/src/*.c)

CXX = gcc

CFLAGS  = -I. -Wall -g -Os
CFLAGS += -DDEBUG

all:
	$(CXX) $(CFLAGS) -o $(TARGET) $(SRC)
	
clean:
	rm -f $(TARGET)
	rm -fR ./scanBuild

install:
	strip $(TARGET)
	sudo install $(TARGET) /usr/sbin
	
run: 
	$(TARGET) -D
