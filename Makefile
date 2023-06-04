# Define required macros here
SHELL = /bin/sh

#OBJS =  cstep.o
CFLAGS = -Wall -g
CC = gcc

LIBS = `pkg-config --libs gtk+-3.0 alsa fluidsynth` 
CFLAGS = `pkg-config --cflags gtk+-3.0 alsa fluidsynth`  
INCLUDES = 
# `pkg-config --libs gtk+-3.0 ` 

cstep: main.o sequencer.o launchpad.o config.o
	${CC} ${CFLAGS} ${INCLUDES}  main.o  launchpad.o config.o sequencer.o  -o $@   ${LIBS}
#sequencer.o launchpad.o config.o 
#main.o: main.c 
#	${CC} ${CFLAGS} ${INCLUDES} -c main.c -o main.o
 
launchpad.o: launchpad.c
	${CC} ${CFLAGS} ${INCLUDES} -c launchpad.c -o launchpad.o

sequencer.o: sequencer.c 
	${CC} ${CFLAGS} ${INCLUDES} -c sequencer.c -o sequencer.o

config.o: cstep_config.c
	${CC} ${CFLAGS} ${INCLUDES} -c cstep_config.c -o config.o


clean:
	rm -rf *.o cstep
	echo "deleted"
 
all: clean cstep
 	
 
#.cpp.o:
#   ${CC} ${CFLAGS} ${INCLUDES} -c $<

#.c.o:
#	${CC} ${CFLAGS} ${INCLUDES} -c $<