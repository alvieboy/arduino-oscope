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
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <glib.h>
#include <string.h>
#include "serial.h"
#include "../protocol.h"
static int fd = -1;

GIOChannel *channel;
gint watcher;

#ifdef STANDALONE
GMainLoop *loo;
#endif

static void (*sdata)(unsigned char *data);

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

	cksum^=(size>>8);
	cksum^=(size&0xff);

	sendchar(size&0xff);
	sendchar(size>>8);

	sendchar(command);
	cksum^=(unsigned char)command;


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
	SAMPLING
};

static enum mystate state = PING;

void process_packet(unsigned char command, unsigned char *buf, unsigned short size)
{
	//	printf("Got packet %d state %d\n",command,state);
	unsigned char trig = 0x80;

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
			send_packet(COMMAND_SET_TRIGGER,&trig,1);
			send_packet(COMMAND_START_SAMPLING,NULL,0);
			state=SAMPLING;
		} else {
			printf("Invalid packet %d\n",command);
		}
		break;
	case SAMPLING:

		sdata(buf);
		send_packet(COMMAND_START_SAMPLING,NULL,0);
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
		pSize = bIn;
		//printf("S0: %u\n",bIn);
		cksum = bIn;
		st = SIZE2;
		break;

	case SIZE2:
		pSize += (unsigned short)bIn<<8;
		//printf("S1: %u\n",bIn);
		pBufPtr = 0;
		st = COMMAND;
		//printf("S+: %u\n",pSize);
		break;

	case COMMAND:

		command = bIn;
		//printf("CM: %u\n",command);
		if (pSize>0)
			st = PAYLOAD;
		else
			st = CKSUM;
		break;
	case PAYLOAD:

		pBuf[pBufPtr++] = bIn;
		pSize--;
		if (pSize==0) {
			st = CKSUM;
		}
		break;

	case CKSUM:
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

void serial_set_trigger_level(unsigned char trig)
{
	send_packet(COMMAND_SET_TRIGGER,&trig,1);
}

void serial_set_holdoff(unsigned char holdoff)
{
	send_packet(COMMAND_SET_HOLDOFF,&holdoff,1);
}

int init(char *device)
{
	struct termios termset;
	GError *error = NULL;

	fd = open(device, O_RDWR);
	if (fd<0) {
		return -1;
	}

	tcgetattr(fd, &termset);
	cfmakeraw(&termset);
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
	return 0;

}
void serial_set_prescaler(unsigned char prescaler)
{
	send_packet(COMMAND_SET_PRESCALER,&prescaler,1);
}

void serial_set_vref(unsigned char vref)
{
	send_packet(COMMAND_SET_VREF,&vref,1);
}

int serial_run( void (*setdata)(unsigned char *data))
{
	sdata = setdata;
	send_packet(COMMAND_PING,(unsigned char*)"BABA",4);
	loop();
	return 0;
}
#ifdef STANDALONE
int main(int argc,char**argv)
{
	loo = g_main_loop_new(g_main_context_new(), TRUE);

	if (init(argv[1])<0)
		return -1;
	return serial_run();
}
#else
int serial_init(gchar*name)
{
	if (init(name)<0)
		return -1;
	return 0;
}
#endif
