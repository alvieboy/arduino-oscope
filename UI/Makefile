all: oscope

CXX=g++-4.3

CXXFLAGS=-D_REENTRANT -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/directfb -I/usr/include/libpng12 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include   -D_REENTRANT -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/directfb -I/usr/include/libpng12    

LIBS=-lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lgio-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lglib-2.0   -lcairo -lX11   -lfftw3 -lm  

OBJECTS= display.o scope.o serial.o knob.o ../crc16.o

oscope: $(OBJECTS)
	$(CXX) -o oscope $+ $(LIBS)

clean:
	rm -f *.o oscope $(OBJECTS) 
	
