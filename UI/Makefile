all: oscope

CFLAGS=$(shell pkg-config --cflags gtk+-2.0 cairo-xlib) -Wall -Werror -std=c99
#-DHAVE_DFT $(shell pkg-config --cflags fftw3)
LIBS=$(shell pkg-config --libs gtk+-2.0) 
#$(shell pkg-config --libs fftw3)


serial:  serial.o 
	$(CC) -o serial $+ $(LIBS)

oscope: display.o scope.o serial.o
	$(CC) -o oscope $+ $(LIBS)

clean:
	rm -f *.o oscope serial
	
# DO NOT DELETE
