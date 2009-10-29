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

static int fd = -1;

/* Serial commands we support */
#define COMMAND_PING 0x3E
#define COMMAND_GET_VERSION   0x40
#define COMMAND_START_SAMPLING   0x41
#define COMMAND_SET_TRIGGER   0x42
#define COMMAND_SET_HOLDOFF  0x43
#define COMMAND_VERSION_REPLY 0x80
#define COMMAND_BUFFER_SEG1   0x81
#define COMMAND_BUFFER_SEG2   0x82
#define COMMAND_PONG 0xE3
#define COMMAND_ERROR 0xFF
#define COMMAND_BUFFER_SEG4   0x84

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

void dumpcks(unsigned char c)
{
	//unsigned char r;
	//read(fd,&r,1);
	//printf("CKS: sent %u recv %u\n",c,r);
}
void send_packet(unsigned char command, unsigned char *buf, unsigned char size)
{
	unsigned char cksum=0;
	unsigned char i;
	GError *error = NULL;

	sendchar(size);
	cksum^=(unsigned char)size;
	dumpcks(cksum);

	sendchar(command);
	cksum^=(unsigned char)command;
	dumpcks(cksum);


	for (i=0;i<size;i++) {
		cksum^=buf[i];
		sendchar(buf[i]);
		dumpcks(cksum);
	}

	//printf("Sending cksum %u\n",cksum);
	sendchar(cksum);
	dumpcks(cksum);
	g_io_channel_flush(channel,&error);
}

enum packetstate {
	SIZE,
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

static int boff=0;
static unsigned char outbuf[512];

void process_packet(unsigned char command, unsigned char *buf, unsigned char size)
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
		{
			int i;
			for (i=0;i<size;i++) {
				outbuf[boff++] = buf[i];
			 //   printf("%02x",buf[i]);
			}
           // printf("\n");
		}
		if (command== COMMAND_BUFFER_SEG2) {
			//memset(outbuf,0,sizeof(outbuf));
			sdata(outbuf);
			send_packet(COMMAND_START_SAMPLING,NULL,0);
			boff=0;
			state=SAMPLING;
		}
        break;
	default:
		printf("Invalid packet %d\n",command);
	}
}

void process(unsigned char bIn)
{
	static unsigned char pBuf[255];
	static unsigned char cksum;
	static unsigned int pBufPtr;
	static unsigned char pSize;
	static unsigned char command;

	static enum packetstate st = SIZE;

	cksum^=bIn;

	switch(st) {
	case SIZE:
		pSize = bIn;
		pBufPtr = 0;
		cksum = bIn;
  //  	printf("Size: %u\n",pSize);
		st = COMMAND;
		break;
	case COMMAND:

		command = bIn;
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
