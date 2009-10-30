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

#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include "protocol.h"

#define BAUD_RATE 115200

/* Our version */
#define VERSION_HIGH 0x01
#define VERSION_LOW 0x01

/* Serial processor state */
enum state {
	SIZE,
	SIZE2,
	COMMAND,
	PAYLOAD,
	CKSUM
};

/* Maximum packet size we can receive from serial. We can however transmit more than this */
#define MAX_PACKET_SIZE 6
#define NUM_SAMPLES 512

unsigned char dataBuffer[NUM_SAMPLES];
unsigned short dataBufferPtr;
unsigned char triggerLevel=0;
unsigned char triggerFlags=0;
unsigned char autoTrigSamples = 100;
unsigned char holdoffSamples = 0;
boolean triggered;
boolean startConversion=false;
boolean conversionDone=false;

#define BIT(x) (1<<x)

const int ledPin = 13;


void setup_adc()
{
	ADCSRA = 0;
	ADCSRB = 0; // Free-running mode
	// DIDR0 = ~1; // Enable only first analog input
	ADMUX = 0xF0; // internal 1.1v reference, left-aligned, channel 0
	PRR &= ~BIT(PRADC);
	ADCSRA = BIT(ADIE)|BIT(ADEN)|BIT(ADSC)|BIT(ADATE)|/*BIT(ADPS0)|*/BIT(ADPS1)|BIT(ADPS2); // Start conversion, enable autotrigger

}

void start_sampling()
{
	sei();
	conversionDone=false;
	startConversion=true;
}

void adc_set_frequency(unsigned char divider)
{
	ADCSRA &= ~0x7;
	ADCSRA |= divider  & 0x7;
}

void setup()
{
	Serial.begin(BAUD_RATE);
	pinMode(ledPin,OUTPUT);
	setup_adc();
	sei();
}

void send_packet(unsigned char command, unsigned char *buf, unsigned short size)
{
	unsigned char cksum=command;
	unsigned short i;

	cksum^= (size>>8);
	cksum^= (size&0xff);

	Serial.write(size&0xff);
	Serial.write((size>>8)&0xff);

	Serial.write(command);
	for (i=0;i<size;i++) {
		cksum^=buf[i];
		Serial.write(buf[i]);
	}
	Serial.write(cksum);
}


void process_packet(unsigned char command, unsigned char *buf, unsigned short size)
{
	switch (command) {
	case COMMAND_PING:
		send_packet(COMMAND_PONG, buf, size);
		break;
	case COMMAND_GET_VERSION:
		buf[0] = VERSION_HIGH;
		buf[1] = VERSION_LOW;
		send_packet(COMMAND_VERSION_REPLY, buf, 2);
		break;
	case COMMAND_START_SAMPLING:
		//adc_set_frequency(0x7);
		start_sampling();
		break;
	case COMMAND_SET_TRIGGER:
		triggerLevel = buf[0];
		break;

	case COMMAND_SET_HOLDOFF:
		holdoffSamples = buf[0];
		break;
	default:
		send_packet(COMMAND_ERROR,NULL,0);
		break;
	}
}


void process(unsigned char bIn)
{
	static unsigned char pBuf[MAX_PACKET_SIZE];
	static unsigned char cksum;
	static unsigned int pBufPtr;
	static unsigned short pSize;
	static unsigned char command;

	static enum state st = SIZE;

	cksum^=bIn;
	//Serial.write(cksum);

	switch(st) {
	case SIZE:
		pSize = bIn;
		cksum = bIn;
		st = SIZE2;
		break;

	case SIZE2:
		pSize += ((unsigned short)bIn<<16);
		if (bIn>MAX_PACKET_SIZE)
			break;
		pBufPtr = 0;
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
		}

		st = SIZE;
	}
}



void loop() {
	int bIn;
	if (Serial.available()>0) {
		bIn =  Serial.read();
		process(bIn & 0xff);
	} else if (conversionDone) {
		conversionDone=false;
		send_packet(COMMAND_BUFFER_SEG, dataBuffer, 512);
	} else {
	}
}

#define TRIGGER_NOISE_LEVEL 1


ISR(ADC_vect)
{
	static unsigned char last=0;
	static unsigned char autoTrigCount=0;
	static boolean do_store_data;
	static unsigned char holdoff;

	if (holdoff>0) {
		holdoff--;
		return;
	}

	if (triggered==0 && triggerLevel>0) {

#ifdef AUTOTRIGGER
		if (autoTrigCount >= autoTrigSamples ) {
			triggered=1;
		} else {
#endif
			//if (last<=triggerLevel && ADCH>last && (ADCH>triggerLevel) && ( (ADCH-last) > TRIGGER_NOISE_LEVEL)) {
			if (ADCH>=triggerLevel) {
				triggered=1;
			} else {
				autoTrigCount++;
			}
#ifdef AUTOTRIGGER
		}
#endif
	} else {
		triggered=1;
	}

	last=ADCH;

	if (triggered) {

		if (startConversion && dataBufferPtr==0) {
			do_store_data=true;
			digitalWrite(ledPin,1);

		}

		if (do_store_data) {
			dataBuffer[dataBufferPtr] = ADCH;
		}
		dataBufferPtr++;

		if (dataBufferPtr>sizeof(dataBuffer)) {

			/* End of this conversion. Perform holdoff if needed */
			if (do_store_data) {
				conversionDone = true;
				startConversion = false;
			}

			if (conversionDone)
				digitalWrite(ledPin,0);

			do_store_data=false;

			triggered=0;
			holdoff=holdoffSamples;
			autoTrigCount=0;
			dataBufferPtr=0;
		}
	}
}

