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

static int fd = -1;

static GIOChannel *channel;
static gint watcher;
static gboolean in_request;
static gboolean freeze = FALSE;
static gboolean delay_request = FALSE;
static gboolean is_trigger_invert;

#ifdef STANDALONE
GMainLoop *loo;
#endif

static void (*sdata)(unsigned char *data,size_t size);
static void (*oneshot_cb)(void*) = NULL;
void *oneshot_cb_data;

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
	g_io_channel_write_chars(channel,&t,sizeof(t),&written,&error);
}

void send_packet(unsigned char command, unsigned char *buf, unsigned short size)
{
	unsigned char cksum=0;
	unsigned char i;
	GError *error = NULL;

	size++;
	if (size>127) {
		size |= 0x8000; // Set MSBit on MSB
		cksum^= (size>>8);
		sendchar((size>>8)&0xff);
		size &= 0x7FFF;
	}
	cksum^=(size&0xff);
	sendchar(size&0xff);

	sendchar(command);
	cksum^=(unsigned char)command;

	size--;

	for (i=0;i<size;i++) {
		cksum^=buf[i];
		sendchar(buf[i]);
	}

	//printf("Sending cksum %u\n",cksum);
	sendchar(cksum);
	g_io_channel_flush(channel,&error);
}

enum packetstate {
	SIZE,
	SIZE2,
	COMMAND,
	PAYLOAD,
	CKSUM
};


enum mystate {
	PING,
	GETVERSION,
	GETPARAMETERS,
	SAMPLING
};

static enum mystate state = PING;

void process_packet(unsigned char command, unsigned char *buf, unsigned short size)
{
	unsigned short ns;

	if (command==COMMAND_PARAMETERS_REPLY) {
		ns = buf[4] << 8;
		ns += buf[5];

		is_trigger_invert = buf[6] & FLAG_INVERT_TRIGGER;

		scope_got_parameters(buf[0],buf[1],buf[2],buf[3],ns,buf[6],buf[7]);
		printf("Num samples: %d %d %d \n", ns, buf[4],buf[5]);
		printf("Channels: %d \n",buf[7]);
	}

	switch(state) {

	case PING:
		if (command==COMMAND_PONG) {
			printf("Got ping reply\n");
			/* Request version */
			send_packet(COMMAND_GET_VERSION,NULL,0);
            state = GETVERSION;
		}
		break;
	case GETVERSION:
		if (command==COMMAND_VERSION_REPLY) {
			printf("Got version: OSCOPE %d.%d\n", buf[0],buf[1]);
			send_packet(COMMAND_GET_PARAMETERS,NULL,0);
			state=GETPARAMETERS;
		} else {
			printf("Invalid packet %d\n",command);
		}
		break;

	case GETPARAMETERS:
		in_request=TRUE;
		send_packet(COMMAND_START_SAMPLING,NULL,0);

		state = SAMPLING;
		break;

	case SAMPLING:
		sdata(buf, size);
		if ( oneshot_cb && ! delay_request) {
			in_request=FALSE;
			oneshot_cb(oneshot_cb_data);
		} else{
			if (!freeze) {
				send_packet(COMMAND_START_SAMPLING,NULL,0);
				in_request=TRUE;
			}
		} 
		delay_request=FALSE;
		state=SAMPLING;

		break;
	default:
		printf("Invalid packet %d\n",command);
	}
}

void process(unsigned char bIn)
{
	static unsigned char pBuf[1024];
	static unsigned char cksum;
	static unsigned int pBufPtr;
	static unsigned short pSize;
	static unsigned char command;

	static enum packetstate st = SIZE;

	cksum^=bIn;

	switch(st) {
	case SIZE:
		cksum = bIn;
		if (bIn==0)
			break; // Reset procedure.
		if (bIn & 0x80) {
			pSize =((unsigned short)(bIn&0x7F)<<8);
			st = SIZE2;
		} else {
			pSize = bIn;
			pBufPtr = 0;
			st = COMMAND;
		}
		break;

	case SIZE2:
		pSize += bIn;
		//printf("S1: %u\n",bIn);
		//printf("Large packet %u\n",pSize);
		pBufPtr = 0;
		st = COMMAND;
		//printf("S+: %u\n",pSize);
		break;

	case COMMAND:

		command = bIn;
		//printf("CM: %u\n",command);
		pSize--;
		if (pSize>0)
			st = PAYLOAD;
		else
			st = CKSUM;
		break;
	case PAYLOAD:

		//printf("< %u %02x\n",pBufPtr,(unsigned) bIn);
		pBuf[pBufPtr++] = bIn;
		pSize--;
		if (pSize==0) {
			st = CKSUM;
		}
		break;

	case CKSUM:
		//printf("< SUM %02x\n",(unsigned) bIn);
		if (cksum==0) {
			process_packet(command,pBuf,pBufPtr);
		} else {
			printf("Packet fails checksum check\n");
		}
		st = SIZE;
	}
}

gboolean serial_data_ready(GIOChannel *source,
						   GIOCondition condition,
						   gpointer data)
{
	gsize r;
	gchar c;
	GError *error = NULL;

	g_io_channel_read_chars(source,&c,1,&r,&error);
	if (NULL==error && r==1)  {
		process((unsigned char)c);
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
	int i;
	for (i=0; i<512; i++) {
		sendchar(0x00);
	}
}

void serial_set_trigger_level(unsigned char trig)
{
	send_packet(COMMAND_SET_TRIGGER,&trig,1);
}

void serial_set_holdoff(unsigned char holdoff)
{
	send_packet(COMMAND_SET_HOLDOFF,&holdoff,1);
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
	termset.c_cflag &= ~(CSIZE | PARENB);
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
	// printf("Setting PRESCALE %d\n", prescaler);
	send_packet(COMMAND_SET_PRESCALER,&prescaler,1);
}

void serial_set_vref(unsigned char vref)
{
	//	printf("Setting VREF %d\n", vref);
	send_packet(COMMAND_SET_VREF,&vref,1);
}

static void set_flags()
{
	unsigned char c=0;
	if (is_trigger_invert)
		c|=FLAG_INVERT_TRIGGER;
	send_packet(COMMAND_SET_FLAGS,&c,1);
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
	send_packet(COMMAND_SET_CHANNELS, &c, 1);
}

int serial_run( void (*setdata)(unsigned char *data,size_t size))
{
	sdata = setdata;
	serial_reset_target();
	fprintf(stderr,"Pinging device...\n");
	send_packet(COMMAND_PING,(unsigned char*)"BABA",4);
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

	send_packet(COMMAND_SET_AUTOTRIG,&tvalue,1);

	if (NULL==oneshot_cb && !in_request) {
		send_packet(COMMAND_START_SAMPLING,NULL,0);
	} else {
		if (in_request)
			delay_request=TRUE;
		else
			send_packet(COMMAND_START_SAMPLING,NULL,0);
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
