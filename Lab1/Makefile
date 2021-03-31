CC=arm-unknown-linux-gnu-gcc
CC_X86=gcc
CFLAGS=-Wall

EXE_FILE=hello_world

all: 
	${CC} ${CFLAGS} -o ${EXE_FILE} hello_world.c

clean: 
	${RM} ${EXE_FILE}

x86:
	${CC_X86} ${CFLAGS} -o ${EXE_FILE} hello_world.c

