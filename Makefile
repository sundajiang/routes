LD_LIBRARY_PATH="../app_interface/"
CC = arm-linux-gcc
#CC = gcc

objects = route.o fhr.o common.o

routes:$(objects)
	$(CC) -g -o routes $(objects) -lpthread -lm -L $(LD_LIBRARY_PATH) -lnetif
route.o:route.c
	$(CC) -c -g route.c -o route.o
fhr.o:fhr.c
	$(CC) -c -g fhr.c -o fhr.o
common.o:common.c
	$(CC) -c -g common.c -o common.o

clean:
	rm *.o routes
