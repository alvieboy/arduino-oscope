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

#undef TEST_CHANNELS

/* Baud rate, for communication with PC */
#define BAUD_RATE 115200
static const uint32_t freq = 16000000;
static const uint16_t avcc = 5000; // 5v
static const uint16_t vref = 5000; // 5v
#undef FASTISR

struct SerialWrapper
{
public:
	static inline void write(uint8_t v) {
		Serial.write(v);
	}
	static inline void write(const uint8_t *buf,int size) {
		Serial.write(buf,size);
	}
	static void flush() {
	}
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

/* Data buffer, where we store our samples. Allocated dinamically */
static unsigned char *dataBuffer;

/* Current buffer positions, used to store data on buffer */

static unsigned char *storePtr,*endPtr;

/* Auto-trigger samples. If we don't trigger and we reach this number of
 samples without triggerting, then we trigger */
static unsigned char autoTrigSamples;
static unsigned char autoTrigCount;

static unsigned char currentChannel=0;

/* To be copied from params when we can change channel numbers */

static unsigned char maxChannels=0;

static parameters_t params;

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

static void stop_adc()
{
	ADCSRA = BIT(ADSC)|BIT(ADATE)|params.prescale; 
}

static void start_adc()
{
	if (params.prescale<=2) {
		ADCSRA = BIT(ADEN)|BIT(ADSC)|BIT(ADATE)|params.prescale; // Do not enable interrupts here.
	} else {
		ADCSRA = BIT(ADIE)|BIT(ADEN)|BIT(ADSC)|BIT(ADATE)|params.prescale;
	}
}

static void setup_adc()
{
	ADCSRA = 0;
	ADCSRB = 0; // Free-running mode
	// DIDR0 = ~1; // Enable only first analog input
	ADMUX = 0x20; // left-aligned, channel 0
	ADMUX |= (params.adcref<<REFS0); // internal 1.1v reference, left-aligned, channel 0

	//	PRR &= ~BIT(PRADC); /* Disable ADC power reduction */
	sei();
	start_adc();
}

static void start_sampling()
{
	cli();
	if (params.prescale<=2) {
		/* Fast sampling loop */
		storePtr=dataBuffer;
		ADCSRA = BIT(ADEN)|BIT(ADSC)|BIT(ADATE)|params.prescale; // Start conversion, enable autotrigger
		do {
			while (!ADCSRA&BIT(ADIF));
			ADCSRA&=~BIT(ADIF);
			*storePtr++=ADCH;
		} while (storePtr!=endPtr);
		storePtr=dataBuffer;

		sei();

		SerPro::send(COMMAND_BUFFER_SEG, maxChannels, VariableBuffer(dataBuffer, params.numSamples) );
	}   
	else {
		params.flags &= ~BYTE_FLAG_CONVERSIONDONE;
		params.flags |= BYTE_FLAG_STARTCONVERSION;
		storePtr=dataBuffer;
		ADCSRA |= BIT(ADEN);
		sei();
	}
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

	params.numSamples  = num;
	dataBuffer = (unsigned char*)malloc(params.numSamples);
	storePtr = dataBuffer;
	endPtr = dataBuffer+params.numSamples+1;
	sei();
}

void setup()
{
	params.prescale = BIT(ADPS0)|BIT(ADPS1)|BIT(ADPS2);
	params.adcref = 0x0; // Default
	dataBuffer=NULL;
	params.triggerLevel=0;
	autoTrigSamples = 255;
	autoTrigCount = 0;
	params.holdoffSamples = 0;
	params.channels = 0;
	maxChannels = 0;
	currentChannel=0;

	Serial.begin(BAUD_RATE);
	pinMode(ledPin,OUTPUT);
	pinMode(sampleFreqPin,OUTPUT);
	setup_adc();

	/* Simple test for PWM output */
	TCCR0B = TCCR0B & 0b11111000 | 0x02; // 62.5KHz
	TCCR1B = TCCR1B & 0b11111000 | 0x02; // 62.5KHz
	TCCR2B = TCCR2B & 0b11111000 | 0x02; // 62.5KHz
	analogWrite(pwmPin,127);

	set_num_samples(962);
}

static void send_parameters()
{
	SerPro::send(COMMAND_PARAMETERS_REPLY,&params);
}


void loop() {
	int bIn;
	if (Serial.available()>0) {
		bIn =  Serial.read();
		SerPro::processData(bIn & 0xff);
	} else if (params.flags & BYTE_FLAG_CONVERSIONDONE) {
		cli();
		params.flags &= ~ BYTE_FLAG_CONVERSIONDONE;
		sei();
		stop_adc();
		SerPro::send(COMMAND_BUFFER_SEG, maxChannels, VariableBuffer(dataBuffer, params.numSamples) );
		cli();
		maxChannels = params.channels;
		currentChannel=0;
		sei();
		start_adc();
	} 
}

#define TRIGGER_NOISE_LEVEL 1

uint8_t last = 0;
uint8_t holdoff;

#ifdef TEST_CHANNELS
uint8_t admux_dly;
#endif

#ifndef FASTISR

ISR(ADC_vect)
{
	register byte flags = params.flags;
	register byte sampled = ADCH;
	register byte inadmux = ADMUX;

	if (flags&BYTE_FLAG_TRIGGERED)
		goto is_trigger; /* Fast path */

	if (params.triggerLevel>0) {

		if (holdoff>0) {
			holdoff--;
			return;
		}

		if (autoTrigCount>0 && autoTrigCount >= autoTrigSamples ) {
			flags |= BYTE_FLAG_TRIGGERED;
			goto do_switch_channel;
		} else {
			if (flags&BYTE_FLAG_INVERTTRIGGER) {
				if (sampled>=params.triggerLevel && last<params.triggerLevel) {
					flags|= BYTE_FLAG_TRIGGERED;
					goto do_switch_channel;
				}
			} else {
				if (sampled<=params.triggerLevel && last>params.triggerLevel) {
					flags|= BYTE_FLAG_TRIGGERED;
					goto do_switch_channel;
				}
			}
			if (autoTrigSamples>0)
				autoTrigCount++;
		}
	} else {
		flags |= BYTE_FLAG_TRIGGERED;
		goto do_switch_channel;
	}

	if (flags & BYTE_FLAG_TRIGGERED) {
		is_trigger:

		if (flags & BYTE_FLAG_STARTCONVERSION) {
			flags |= BYTE_FLAG_STOREDATA;
			goto do_store;
		}

		if (flags & BYTE_FLAG_STOREDATA) {
		do_store:
#ifdef TEST_CHANNELS
			*storePtr++ = 10+((admux_dly&0x7)<<4);
            //*storePtr++ = currentChannel<<6;
#else
			*storePtr++ = sampled;
#endif
			// Switch channel.
		do_switch_channel:
			if (currentChannel<maxChannels)
				currentChannel++;
			else
				currentChannel=0;
#ifdef TEST_CHANNELS
			admux_dly = inadmux;
#endif
			inadmux &= 0xf8;
			inadmux |= currentChannel;
		}

		if (storePtr == endPtr) {

			if (flags & BYTE_FLAG_STOREDATA) {
				flags |= BYTE_FLAG_CONVERSIONDONE;
				flags &= ~BYTE_FLAG_STARTCONVERSION;
			}

			flags &= ~BYTE_FLAG_STOREDATA;
			flags &= ~BYTE_FLAG_TRIGGERED;
			// Reset muxer
			inadmux &= 0xf8;
			holdoff=params.holdoffSamples;
			autoTrigCount=0;
			currentChannel=0;
			last=0;
			if (!(flags&BYTE_FLAG_INVERTTRIGGER))
				last--;
		}
		ADMUX=inadmux;
	} else {
		last=sampled;
	}
	params.flags=flags;
}

#else // FASTISR

#if 1
ISR(ADC_vect)
{
	register byte flags = params.flags;
	register byte sampled = ADCH;

	if (flags & BYTE_FLAG_STARTCONVERSION) {
		flags |= BYTE_FLAG_STOREDATA;
		flags &= ~BYTE_FLAG_STARTCONVERSION;
		goto do_store;
	}

	if (flags & BYTE_FLAG_STOREDATA) {
	do_store:
		*storePtr++ = sampled;


		if (storePtr == endPtr) {

			flags |= BYTE_FLAG_CONVERSIONDONE;
			flags &= ~BYTE_FLAG_STOREDATA;

			storePtr = dataBuffer;
		}
	}

	params.flags=flags;
}
#else
ISR(ADC_vect,ISR_NAKED)
{
	push __zero_reg__
	push r0
	in r0,__SREG__
	push r0
	clr __zero_reg__
	push r18
	push r19
	push r20
	push r24
	push r25
	push r30
	push r31
	lds r20,_ZL6params+6
	lds r25,121
	sbrs r20,6
	ori r20,lo8(16)
	andi r20,lo8(-65)

.STOREDATA:
	lds r30,storePtr
	lds r31,(storePtr)+1
	st Z+,r25
	sts (storePtr)+1,r31
	sts storePtr,r30
	lds r24,endPtr
	lds r25,(endPtr)+1
	cp r30,r24
	cpc r31,r25
	breq .ENDCONVERSION

.EXITISR:
	sts _ZL6params+6,r20
/* epilogue start */
	pop r31
	pop r30
	pop r25
	pop r24
	pop r20
	pop r19
	pop r18
	pop r0
	out __SREG__,r0
	pop r0
	pop __zero_reg__
	reti
.L4:
	sbrs r20,4
	rjmp .EXITISR
	rjmp .STOREDATA

.ENDCONVERSION:

	ori r20,lo8(32)
	andi r20,lo8(-17)
	lds r24,_ZL10dataBuffer
	lds r25,(_ZL10dataBuffer)+1
	sts (storePtr)+1,r25
	sts storePtr,r24
	rjmp .EXITISR

}
#endif
#endif


template<> void handleEvent<START_FRAME>()
{
	digitalWrite(ledPin,1);
}

template<> void handleEvent<END_FRAME>()
{
	digitalWrite(ledPin,0);
}

DECLARE_FUNCTION(COMMAND_GET_VERSION)(void) {
	SerPro::send<uint8_t,uint8_t>(COMMAND_VERSION_REPLY,PROTOCOL_VERSION_HIGH,PROTOCOL_VERSION_LOW);
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_START_SAMPLING)(void) {
	start_sampling();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_TRIGGER)(uint8_t val) {
	params.triggerLevel = val;
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_HOLDOFF)(uint8_t val) {
	params.holdoffSamples = val;
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_VREF)(uint8_t val) {
	params.adcref = val & 0x3;
	setup_adc();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_PRESCALER)(uint8_t val) {
	params.prescale = val & 0x7;
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

DECLARE_FUNCTION(COMMAND_SET_CHANNELS)(uint8_t val) {
	params.channels = val - 1;
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_GET_PARAMETERS)(void) {
	send_parameters();
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_GET_CONSTANTS)(void) {
	SerPro::send(freq, avcc, vref);
}
END_FUNCTION

DECLARE_FUNCTION(COMMAND_SET_FLAGS)(uint8_t val) {
	cli();
	params.flags &= ~(BYTE_FLAG_INVERTTRIGGER);
	val &= BYTE_FLAG_INVERTTRIGGER;
	params.flags |= val;
	sei();
	send_parameters();
}
END_FUNCTION

IMPLEMENT_SERPRO(16,SerPro,SerProHDLC);
