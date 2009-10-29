all: oscope

CFLAGS=$(shell pkg-config --cflags gtk+-2.0 cairo-xlib) -Wall -Werror
LIBS=$(shell pkg-config --libs gtk+-2.0)


serial:  serial.o 
	$(CC) -o serial $+ $(LIBS)

oscope: display.o scope.o serial.o
	$(CC) -o oscope $+ $(LIBS)

clean:
	rm -f *.o oscope serial
	
# DO NOT DELETE

display.o: scope.h
scope.o: scope.h /usr/include/math.h /usr/include/features.h
scope.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
scope.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
scope.o: /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h
scope.o: /usr/include/bits/mathcalls.h
serial-2.o: /usr/include/termios.h /usr/include/features.h
serial-2.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
serial-2.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
serial-2.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
serial-2.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
serial-2.o: /usr/include/sys/types.h /usr/include/bits/types.h
serial-2.o: /usr/include/bits/typesizes.h /usr/include/time.h
serial-2.o: /usr/include/endian.h /usr/include/bits/endian.h
serial-2.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
serial-2.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
serial-2.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
serial-2.o: /usr/include/bits/pthreadtypes.h /usr/include/stdio.h
serial-2.o: /usr/include/libio.h /usr/include/_G_config.h
serial-2.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
serial-2.o: /usr/include/bits/sys_errlist.h /usr/include/malloc.h
