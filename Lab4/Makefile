CC=arm-linux-gnueabihf-gcc

obj-m := mydev.o

all:
		make -C ../ M=$(PWD) modules
		$(CC) writer.cpp -o writer --static
		$(CC) reader.cpp -o reader --static
clean:
		make -C ../ M=$(PWD) clean
		rm writer reader
