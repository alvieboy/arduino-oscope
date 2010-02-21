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

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <gtk/gtk.h>

int serial_init(gchar*name);
int serial_run( void (*setdata)(unsigned char *data,size_t size),
			   void (*setdigdata)(unsigned char *data,size_t size) );
void serial_set_trigger_level(unsigned char trig);
void serial_set_holdoff(unsigned char holdoff);
void serial_set_prescaler(unsigned char prescaler);
void serial_set_vref(unsigned char vref);
void serial_set_trigger_invert(gboolean active);
void serial_set_channels(int channels);
void serial_set_sequential_channel(gboolean isseq);

double get_sample_frequency(unsigned long freq, unsigned long prescaler);
void serial_set_oneshot( void(*callback)(void*) , void *data);
void serial_freeze_unfreeze( gboolean freeze );
gboolean serial_in_request();


#endif
