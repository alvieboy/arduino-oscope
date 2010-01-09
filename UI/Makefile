all: oscope

CXXFLAGS=$(shell pkg-config --cflags gtk+-2.0 cairo-xlib) -I.. -Wall 

#-DHAVE_DFT $(shell pkg-config --cflags fftw3)
LIBS=$(shell pkg-config --libs gtk+-2.0) 
#$(shell pkg-config --libs fftw3)


oscope: display.o scope.o serial.o knob.o ../crc16.o
	$(CXX) -o oscope $+ $(LIBS)

clean:
	rm -f *.o oscope serial
	
# DO NOT DELETE
