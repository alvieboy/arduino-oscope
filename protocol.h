/*
 * Copyright (c) 2007 Alvaro Lopes <alvieboy@alvie.com>
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

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <inttypes.h>

/* Our version */
#define PROTOCOL_VERSION_HIGH 0x04
#define PROTOCOL_VERSION_LOW  0x02

/* Serial commands we support */
#define COMMAND_GET_VERSION    0x00
#define COMMAND_START_SAMPLING 0x02
#define COMMAND_SET_TRIGGER    0x03
#define COMMAND_SET_HOLDOFF    0x04
#define COMMAND_SET_VREF       0x05
#define COMMAND_SET_PRESCALER  0x06
#define COMMAND_GET_PARAMETERS 0x07
#define COMMAND_SET_SAMPLES    0x08
#define COMMAND_SET_AUTOTRIG   0x09
#define COMMAND_SET_FLAGS      0x0A
#define COMMAND_SET_CHANNELS   0x0B
#define COMMAND_VERSION_REPLY  0x00
#define COMMAND_BUFFER_SEG     0x01
#define COMMAND_PARAMETERS_REPLY 0x02
#define COMMAND_PONG           0x03
#define COMMAND_ERROR          0xFF

#define FLAG_INVERT_TRIGGER  (1<<0)

typedef struct {
	uint8_t triggerLevel;
	uint8_t holdoffSamples;
	uint8_t adcref;
	uint8_t prescale;
	uint16_t numSamples;
	uint8_t flags;
	uint8_t channels;
} __attribute__((packed)) parameters_t;


#endif
