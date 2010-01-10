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

digitalscope.o: digitalscope.h /usr/include/math.h /usr/include/features.h
digitalscope.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
digitalscope.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
digitalscope.o: /usr/include/gnu/stubs-32.h /usr/include/bits/huge_val.h
digitalscope.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
digitalscope.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
digitalscope.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
digitalscope.o: /usr/include/string.h /usr/include/xlocale.h
display.o: scope.h channel.h digitalscope.h serial.h /usr/include/math.h
display.o: /usr/include/features.h /usr/include/bits/predefs.h
display.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
display.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
display.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
display.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
display.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
display.o: /usr/include/bits/mathcalls.h /usr/include/stdlib.h
display.o: /usr/include/sys/types.h /usr/include/bits/types.h
display.o: /usr/include/bits/typesizes.h /usr/include/time.h
display.o: /usr/include/endian.h /usr/include/bits/endian.h
display.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
display.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
display.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
display.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
display.o: /usr/include/string.h /usr/include/xlocale.h ../protocol.h knob.h
knob.o: /usr/include/math.h /usr/include/features.h
knob.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
knob.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
knob.o: /usr/include/gnu/stubs-32.h /usr/include/bits/huge_val.h
knob.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
knob.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
knob.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h knob.h
knob.o: /usr/include/string.h /usr/include/xlocale.h
scope.o: scope.h channel.h /usr/include/math.h /usr/include/features.h
scope.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
scope.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
scope.o: /usr/include/gnu/stubs-32.h /usr/include/bits/huge_val.h
scope.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
scope.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
scope.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
scope.o: /usr/include/string.h /usr/include/xlocale.h
serial.o: /usr/include/termios.h /usr/include/features.h
serial.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
serial.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
serial.o: /usr/include/gnu/stubs-32.h /usr/include/bits/termios.h
serial.o: /usr/include/sys/ttydefaults.h /usr/include/unistd.h
serial.o: /usr/include/bits/posix_opt.h /usr/include/bits/types.h
serial.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
serial.o: /usr/include/getopt.h /usr/include/fcntl.h
serial.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
serial.o: /usr/include/time.h /usr/include/endian.h
serial.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
serial.o: /usr/include/sys/select.h /usr/include/bits/select.h
serial.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
serial.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
serial.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
serial.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
serial.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
serial.o: /usr/include/alloca.h /usr/include/string.h /usr/include/xlocale.h
serial.o: serial.h ../protocol.h ../SerPro.h /usr/include/stdint.h
serial.o: /usr/include/bits/wchar.h ../Packet.h /usr/include/inttypes.h
serial.o: ../SerProCommon.h ../preprocessor_table.h ../SerProHDLC.h
serial.o: ../crc16.h
../Packet.o: /usr/include/inttypes.h /usr/include/features.h
../Packet.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
../Packet.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
../Packet.o: /usr/include/gnu/stubs-32.h /usr/include/stdint.h
../Packet.o: /usr/include/bits/wchar.h
../SerPro.o: /usr/include/stdint.h /usr/include/features.h
../SerPro.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
../SerPro.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
../SerPro.o: /usr/include/gnu/stubs-32.h /usr/include/bits/wchar.h
../SerPro.o: /usr/include/string.h /usr/include/xlocale.h ../Packet.h
../SerPro.o: /usr/include/inttypes.h ../SerProCommon.h
../SerPro.o: ../preprocessor_table.h
../SerProHDLC.o: /usr/include/inttypes.h /usr/include/features.h
../SerProHDLC.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
../SerProHDLC.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
../SerProHDLC.o: /usr/include/gnu/stubs-32.h /usr/include/stdint.h
../SerProHDLC.o: /usr/include/bits/wchar.h ../crc16.h ../SerProCommon.h
../SerProHDLC.o: ../Packet.h /usr/include/stdio.h /usr/include/bits/types.h
../SerProHDLC.o: /usr/include/bits/typesizes.h /usr/include/libio.h
../SerProHDLC.o: /usr/include/_G_config.h /usr/include/wchar.h
../SerProHDLC.o: /usr/include/bits/stdio_lim.h
../SerProHDLC.o: /usr/include/bits/sys_errlist.h
../crc16.o: /usr/include/inttypes.h /usr/include/features.h
../crc16.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
../crc16.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
../crc16.o: /usr/include/gnu/stubs-32.h /usr/include/stdint.h
../crc16.o: /usr/include/bits/wchar.h
