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


/* Serial commands we support */
#define COMMAND_PING           0x3E
#define COMMAND_GET_VERSION    0x40
#define COMMAND_START_SAMPLING 0x41
#define COMMAND_SET_TRIGGER    0x42
#define COMMAND_SET_HOLDOFF    0x43
#define COMMAND_SET_TRIGINVERT 0x44
#define COMMAND_SET_VREF       0x45
#define COMMAND_SET_PRESCALER  0x46
#define COMMAND_GET_PARAMETERS 0x47
#define COMMAND_SET_SAMPLES    0x48
#define COMMAND_SET_AUTOTRIG   0x49
#define COMMAND_VERSION_REPLY  0x80
#define COMMAND_BUFFER_SEG     0x81
#define COMMAND_PARAMETERS_REPLY 0x87
#define COMMAND_PONG           0xE3
#define COMMAND_ERROR          0xFF

#endif
