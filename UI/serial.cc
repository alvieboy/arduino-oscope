/*
 * Copyright (c) 2009 Alvaro Lopes <alvieboy@alvie.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include "serial.h"
#include "../protocol.h"
#include "../SerPro.h"
#include "../SerProHDLC.h"

static int fd = -1;

static GIOChannel *channel;
static gint watcher;
static gboolean in_request;
static gboolean freeze = FALSE;
static gboolean delay_request = FALSE;
static gboolean is_trigger_invert;

static void (*sdata)(unsigned char *data,size_t size);
static void (*sdigdata)(unsigned char *data,size_t size);
static void (*oneshot_cb)(void*) = NULL;

void *oneshot_cb_data;

class SerialWrapper
{
public:
	static void write(uint8_t v) {
		GError *error = NULL;
		gsize written;
		g_io_channel_write_chars(channel,(const gchar*)&v,sizeof(v),&written,&error);
		fprintf(stderr,"> %u\n",v);
	}

	static void write(const unsigned char *buf, unsigned int size) {
		GError *error = NULL;
		gsize written;
		g_io_channel_write_chars(channel,(const gchar*)buf,size,&written,&error);
	}
	static void flush() {
		GError *error = NULL;
		g_io_channel_flush(channel,&error);
	}
};

class GLIBTimer
{
public:
	typedef guint timer_t;
	static timer_t addTimer( int (*cb)(void*), int milisseconds, void *data=0)
	{
		return g_timeout_add(milisseconds,cb,data);
	}
	static timer_t cancelTimer(const timer_t &t) {
		g_source_remove(t);
		return 0;
	}
	static inline bool defined(const timer_t &t) {
		return t != 0;
	}
};

struct SerProConfig {
	static unsigned int const maxFunctions = 128;
	static unsigned int const maxPacketSize = 1024;
	static unsigned int const stationId = 0xFF; /* Only for HDLC */
	static SerProImplementationType const implementationType = Master;
};

DECLARE_SERPRO_WITH_TIMER( SerProConfig, SerialWrapper, SerProHDLC, GLIBTimer, SerPro);




extern void scope_got_parameters(unsigned char triggerLevel,
								 unsigned char holdoffSamples,
								 unsigned char adcref,
								 unsigned char prescale,
								 unsigned short numSamples,
								 unsigned char flags,
								 unsigned char numChannels);

void sendchar(int i) {
	char t = i &0xff;
	gsize written;
	GError *error = NULL;
	fprintf(stderr,"Send %d\n",i);
	g_io_channel_write_chars(channel,&t,sizeof(t),&written,&error);
	if (NULL!=error) {
		fprintf(stderr,"Cannot write ?!?!?!? %s\n", error->message);
		return;
	}
}

enum mystate {
	PING,
	GETVERSION,
	GETPARAMETERS,
	SAMPLING
};

static enum mystate state = PING;


DECLARE_FUNCTION(COMMAND_PARAMETERS_REPLY)(const parameters_t *p)
{
	is_trigger_invert = p->flags & FLAG_INVERT_TRIGGER;

	unsigned char *r = (unsigned char*)p;
	unsigned i;
	for (i=0;i<sizeof(parameters_t);i++) {
		printf("%02x ",(unsigned)r[i]);
	}
	printf("\n");

	printf("Num samples: %d\n", p->numSamples);
	printf("Channels: %d \n",p->channels);

	scope_got_parameters(p->triggerLevel,
						 p->holdoffSamples,
						 p->adcref,
						 p->prescale,
						 p->numSamples,
						 p->flags,
						 p->channels
						);

	if (state==GETPARAMETERS) {
		in_request=TRUE;
        printf("Requesting samples\n");
		SerPro::sendPacket(COMMAND_START_SAMPLING);
		state = SAMPLING;
	} else {
		printf("Not requesting samples\n");
	}
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_PONG)(const SerPro::RawBuffer &b)
{
	printf("Got ping reply\n");
	/* Request version */
	SerPro::sendPacket(COMMAND_GET_VERSION);
	state = GETVERSION;
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_VERSION_REPLY)(uint8_t major,uint8_t minor) {
	printf("Got version: OSCOPE %d.%d\n", major,minor);
	SerPro::sendPacket(COMMAND_GET_PARAMETERS);
	state=GETPARAMETERS;
}
END_FUNCTION


DECLARE_FUNCTION(COMMAND_BUFFER_SEG)(const SerPro::RawBuffer &b)
{
	fprintf(stderr,"Got analog data, %d samples\n",b.size);
	sdata(b.buffer, b.size);
	if ( oneshot_cb && ! delay_request) {
		in_request=FALSE;
		oneshot_cb(oneshot_cb_data);
	} else{
		if (!freeze) {
			printf("Requesting more samples\n");
			SerPro::sendPacket(COMMAND_START_SAMPLING);
			in_request=TRUE;
		}
	}
	delay_request=FALSE;
	state=SAMPLING;
}
END_FUNCTION

template<>
void handleEvent<LINK_UP>() {
	fprintf(stderr,"Pinging device...\n");
	SerPro::sendPacket<uint8_t,uint8_t,uint8_t,uint8_t>(COMMAND_PING,1,2,3,4);
}


/*
DECLARE_FUNCTION(COMMAND_DIG_BUFFER_SEG)(const SerPro::RawBuffer &b) {
	fprintf(stderr,"Got digital data, %d samples\n",b.size);
	sdigdata(b.buffer,b.size);
}
END_FUNCTION
*/

IMPLEMENT_SERPRO(128,SerPro,SerProHDLC);

gboolean serial_data_ready(GIOChannel *source,
						   GIOCondition condition,
						   gpointer data)
{
	gsize r;
	gchar c;
	GError *error = NULL;

	g_io_channel_read_chars(source,&c,1,&r,&error);
	if (NULL==error && r==1)  {
		SerPro::processData((unsigned char)c);
	}
	return TRUE;
}

void loop()
{
#ifdef STANDALONE
	g_main_loop_run(loo);
#endif
}

void serial_reset_target()
{
}

void serial_set_trigger_level(uint8_t trig)
{
	SerPro::sendPacket(COMMAND_SET_TRIGGER,trig);
}

void serial_set_holdoff(unsigned char holdoff)
{
	SerPro::sendPacket(COMMAND_SET_HOLDOFF,holdoff);
}

int real_serial_init(char *device)
{
	struct termios termset;
	GError *error = NULL;

	fd = open(device, O_RDWR);
	if (fd<0) {
		perror("open");
		return -1;
	}

	fprintf(stderr,"Opened device '%s'\n", device);

	tcgetattr(fd, &termset);

	termset.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
						 | INLCR | IGNCR | ICRNL | IXON);
	termset.c_oflag &= ~OPOST;
	termset.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termset.c_cflag &= ~(CSIZE | PARENB| HUPCL);
	termset.c_cflag |= CS8;

	cfsetospeed(&termset,B115200);
	cfsetispeed(&termset,B115200);

	tcsetattr(fd,TCSANOW,&termset);

	channel =  g_io_channel_unix_new(fd);

	if (NULL==channel) {
		fprintf(stderr,"Cannot open channel\n");
		return -1;
	}
	g_io_channel_set_encoding (channel, NULL, &error);
	if (error) {
		fprintf(stderr,"Cannot set encoding: %s\n", error->message);
	}
	error = NULL;
    g_io_channel_set_buffered(channel, false);
	

	g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &error);
	if (error) {
		fprintf(stderr,"Cannot set flags: %s\n", error->message);
	}
	g_io_channel_set_close_on_unref (channel, TRUE);
	if ((watcher=g_io_add_watch(channel, G_IO_IN, &serial_data_ready, NULL))<0) {
		fprintf(stderr,"Cannot add watch\n");
	}

	fprintf(stderr,"Channel set up OK\n");

	in_request = FALSE;
	delay_request = FALSE;

	return 0;

}
void serial_set_prescaler(unsigned char prescaler)
{
	SerPro::sendPacket<uint8_t>(COMMAND_SET_PRESCALER,prescaler);
}

void serial_set_vref(unsigned char vref)
{
	SerPro::sendPacket<uint8_t>(COMMAND_SET_VREF,vref);
}

static void set_flags()
{
	unsigned char c=0;
	if (is_trigger_invert)
		c|=FLAG_INVERT_TRIGGER;
	SerPro::sendPacket<uint8_t>(COMMAND_SET_FLAGS,c);
}


void serial_set_trigger_invert(gboolean active)
{
	is_trigger_invert = active;
	set_flags();
}
void serial_set_channels(int channels)
{
	if (channels<1 || channels>4)
		return;
	unsigned char c = channels;
	SerPro::sendPacket<uint8_t>(COMMAND_SET_CHANNELS, c);
}

int serial_run( void (*setdata)(unsigned char *data,size_t size), void (*setdigdata)(unsigned char *data,size_t size) )
{
	sdata = setdata;
	sdigdata = setdigdata;
	SerPro::connect();
	loop();
	return 0;
}

void serial_freeze_unfreeze(gboolean freeze)
{
}


void serial_set_oneshot( void(*callback)(void*), void*data )
{
	unsigned char tvalue;
	oneshot_cb = callback;
    oneshot_cb_data = data;
	if (oneshot_cb) {
		tvalue=0;
	} else {
		tvalue=100;
	}

	SerPro::sendPacket<uint8_t>(COMMAND_SET_AUTOTRIG,tvalue);

	if (NULL==oneshot_cb && !in_request) {
		SerPro::sendPacket(COMMAND_START_SAMPLING);
	} else {
		if (in_request)
			delay_request=TRUE;
		else
			SerPro::sendPacket(COMMAND_START_SAMPLING);
	}
}

gboolean serial_in_request()
{
    return in_request;
}

double get_sample_frequency(unsigned long freq, unsigned long prescaler)
{
	unsigned long adc_clock = freq / prescaler;
	double fsample = (double)adc_clock / 13;
	return fsample;
}

int serial_init(gchar*name)
{
	if (real_serial_init(name)<0)
		return -1;
	return 0;
}
