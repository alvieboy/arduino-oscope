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

#include <inttypes.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include "protocol.h"
#include "SerPro.h"
#include "SerProHDLC.h"

/* Baud rate, for communication with PC */
#define BAUD_RATE 115200



struct SerialWrapper
{
public:
	static inline void write(uint8_t v){
		Serial.write(v);
	}
	static inline void write(const uint8_t *buf,int size){
		Serial.write(buf,size);
	}
        static void flush() {}
};

// 4 functions
// 32 bytes maximum receive buffer size

struct SerProConfig {
	static unsigned int const maxFunctions = 16;
	static unsigned int const maxPacketSize = 16;
	static unsigned int const stationId = 3;
	static SerProImplementationType const implementationType = Slave;
};

// SerialWrapper is class to handle tx
// Name of class is SerPro
// Protocol type is SerProPacket or SerProHDLC

DECLARE_SERPRO(SerProConfig,SerialWrapper,SerProHDLC,SerPro);

/* Maximum packet size we can receive from serial. We can however
 transmit more than this */
#define MAX_PACKET_SIZE 7

/* Number of samples we support, i.e., dataBuffer size */
static unsigned short numSamples;

/* Data buffer, where we store our samples. Allocated dinamically */
static unsigned char *dataBuffer;

/* Current buffer position, used to store data on buffer */
static unsigned short dataBufferPtr;

/* Current trigger level. 0 means no trigger */
static unsigned char triggerLevel;

/* Auto-trigger samples. If we don't trigger and we reach this number of
 samples without triggerting, then we trigger */
static unsigned char autoTrigSamples;
static unsigned char autoTrigCount;

/* Number of holdoff samples. This is the number of samples we ignore
 when we finish filling a buffer and before starting to look for trigger
 again */
static unsigned char holdoffSamples;

/* Current prescaler value */
static unsigned char prescale;

/* Current ACD reference */
static unsigned char adcref;

/* Current flags. See defines below */
static byte gflags = 0;

/*static unsigned char pBuf[MAX_PACKET_SIZE];
static unsigned char cksum;
static unsigned int pBufPtr;
static unsigned short pSize;
static unsigned char command;
static enum state st;
*/

#define BYTE_FLAG_TRIGGERED       (1<<7) /* Signal is triggered */
#define BYTE_FLAG_STARTCONVERSION (1<<6) /* Request conversion to start */
#define BYTE_FLAG_CONVERSIONDONE  (1<<5) /* Conversion done flag */
#define BYTE_FLAG_STOREDATA       (1<<4) /* Internal flag - store data in buffer */

#define BYTE_FLAG_DUALCHANNEL     FLAG_DUAL_CHANNEL /* Dual-channel enabled */
#define BYTE_FLAG_INVERTTRIGGER   FLAG_INVERT_TRIGGER /* Trigger is inverted (negative edge) */

#define BIT(x) (1<<x)

const int ledPin = 13;
const int sampleFreqPin = 2;
const int pwmPin = 9;

static void setup_adc()
{
	ADCSRA = 0;
	ADCSRB = 0; // Free-running mode
	// DIDR0 = ~1; // Enable only first analog input
	ADMUX = 0x20; // left-aligned, channel 0
	ADMUX |= (adcref<<REFS0); // internal 1.1v reference, left-aligned, channel 0

	//	PRR &= ~BIT(PRADC); /* Disable ADC power reduction */
	ADCSRA = BIT(ADIE)|BIT(ADEN)|BIT(ADSC)|BIT(ADATE)|prescale; // Start conversion, enable autotrigger
}

static void start_sampling()
{
	cli();
	gflags &= ~BYTE_FLAG_CONVERSIONDONE;
	gflags |= BYTE_FLAG_STARTCONVERSION;
	sei();
}

static void adc_set_frequency(unsigned char divider)
{
	ADCSRA &= ~0x7;
	ADCSRA |= (divider & 0x7);
}

static void set_num_samples(unsigned short num)
{
	if (num>1024)
		return;

	if (NULL!=dataBuffer)
		free(dataBuffer);

	/* NOTE - we must not change this while sampling!!! */
	cli();

	numSamples  = num;
	dataBuffer = (unsigned char*)malloc(numSamples);

	sei();
}

void setup()
{
	prescale = BIT(ADPS0)|BIT(ADPS1)|BIT(ADPS2);
	adcref = 0x0; // Default
	dataBuffer=NULL;
	triggerLevel=0;
	autoTrigSamples = 255;
	autoTrigCount = 0;
	holdoffSamples = 0;

	Serial.begin(BAUD_RATE);
	pinMode(ledPin,OUTPUT);
	pinMode(sampleFreqPin,OUTPUT);
	setup_adc();

	/* Simple test for PWM output */
	analogWrite(pwmPin,127);

	set_num_samples(962);
}

static void send_parameters()
{
	SerPro::send(COMMAND_PARAMETERS_REPLY,
				 triggerLevel,holdoffSamples,adcref,
				 prescale,numSamples,gflags);
}


void loop() {
	int bIn;
	if (Serial.available()>0) {
		bIn =  Serial.read();
		SerPro::processData(bIn & 0xff);
	} else if (gflags & BYTE_FLAG_CONVERSIONDONE) {
		cli();
		gflags &= ~ BYTE_FLAG_CONVERSIONDONE;
		sei();
		SerPro::send<SerPro::VariableBuffer>(COMMAND_BUFFER_SEG, SerPro::VariableBuffer(dataBuffer, numSamples) );
	} 
}

#define TRIGGER_NOISE_LEVEL 1

#if 1

ISR(ADC_vect)
{
	static unsigned char last=0;
	static unsigned char holdoff;
	register byte flags = gflags;

	if (holdoff>0) {
		holdoff--;
		return;
	}

	if (!(flags & BYTE_FLAG_TRIGGERED) && triggerLevel>0) {

		if (autoTrigCount>0 && autoTrigCount >= autoTrigSamples ) {
			flags |= BYTE_FLAG_TRIGGERED;
		} else {

			if ( !(flags&BYTE_FLAG_INVERTTRIGGER) && ADCH>=triggerLevel && last<triggerLevel) {
				flags |= BYTE_FLAG_TRIGGERED;
			} else if ( flags&BYTE_FLAG_INVERTTRIGGER && ADCH<=triggerLevel && last>triggerLevel) {
				flags |= BYTE_FLAG_TRIGGERED;
			} else {
				if (autoTrigSamples>0)
					autoTrigCount++;
			}
		}
	} else {
		flags |= BYTE_FLAG_TRIGGERED;
	}

	last=ADCH;

	if (flags & BYTE_FLAG_TRIGGERED) {

		if (flags & BYTE_FLAG_STARTCONVERSION && dataBufferPtr==0) {
			flags |= BYTE_FLAG_STOREDATA;
		}

		if (flags & BYTE_FLAG_STOREDATA) {

			 // Switch channel.
			if (flags & BYTE_FLAG_DUALCHANNEL)
				ADMUX = ADMUX ^ 1;
			else
				ADMUX &= 0xfe;

			dataBuffer[dataBufferPtr] = ADCH;
		}
		dataBufferPtr++;

		if (dataBufferPtr>numSamples) {

			/* End of this conversion. Perform holdoff if needed */
			if (flags & BYTE_FLAG_STOREDATA) {
				flags |= BYTE_FLAG_CONVERSIONDONE;
				flags &= ~BYTE_FLAG_STARTCONVERSION;
			}

			flags &= ~BYTE_FLAG_STOREDATA;
			flags &= ~BYTE_FLAG_TRIGGERED;
            // Reset muxer
			ADMUX &= 0xfe;
			holdoff=holdoffSamples;
			autoTrigCount=0;
			dataBufferPtr=0;
			if (flags&BYTE_FLAG_INVERTTRIGGER)
				last=0;
			else
				last=255;
		}
	}
	gflags=flags;
}

#else

ISR(ADC_vect,ISR_NAKED)
{
	reti();
}

#endif



DECLARE_FUNCTION(COMMAND_PING)(const SerPro::RawBuffer &buf) {
	SerPro::send<SerPro::RawBuffer>(COMMAND_PONG, buf);
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_GET_VERSION)(void) {
	SerPro::send<uint8_t,uint8_t>(COMMAND_VERSION_REPLY,PROTOCOL_VERSION_HIGH,PROTOCOL_VERSION_LOW);
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_START_SAMPLING)(void) {
	start_sampling();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_TRIGGER)(uint8_t val) {
	triggerLevel = val;
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_HOLDOFF)(uint8_t val) {
	holdoffSamples = val;
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_VREF)(uint8_t val) {
	adcref = val & 0x3;
	setup_adc();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_PRESCALER)(uint8_t val) {
		prescale = val & 0x7;
		setup_adc();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_AUTOTRIG)(uint8_t val) {
	autoTrigSamples = val;
	autoTrigCount = 0; // Reset.
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_SAMPLES)(uint16_t val) {
	set_num_samples(val);
	send_parameters();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_GET_PARAMETERS)(void) {
	send_parameters();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_FLAGS)(uint8_t val) {
	cli();
	gflags &= ~(BYTE_FLAG_INVERTTRIGGER|BYTE_FLAG_DUALCHANNEL);
	val &= BYTE_FLAG_INVERTTRIGGER|BYTE_FLAG_DUALCHANNEL;
	gflags |= val;
	sei();
	send_parameters();
}
END_FUNCTION

IMPLEMENT_SERPRO(16,SerPro,SerProHDLC);
