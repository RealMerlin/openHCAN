TARGET = hcanaddressd
SRC = main.c socket.c

CXX = gcc
CFLAGS = -O2 -I. -Wall -g -Wno-unused-result #-DDEBUG0

all:
	$(CXX) $(CFLAGS) -o $(TARGET) $(SRC)
	
clean:
	rm -f $(TARGET)
	rm -fR ./scanBuild

install:
	strip $(TARGET)
	sudo install $(TARGET) /usr/sbin
