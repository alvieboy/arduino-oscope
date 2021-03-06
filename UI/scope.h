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

#ifndef __SCOPE_H__
#define __SCOPE_H__

#include <gtk/gtk.h>

#ifdef HAVE_DFT
#include <fftw3.h>
#endif

typedef struct _ScopeDisplay ScopeDisplay;
typedef struct _ScopeDisplayClass       ScopeDisplayClass;

struct _ScopeDisplay
{
	GtkDrawingArea parent;
	/* private */
	unsigned char *dbuf;
	unsigned short numSamples;
	unsigned char tlevel;
	unsigned int zoom;
	unsigned char channels;
	gboolean xy;
	double freq;
#ifdef HAVE_DFT
	double *dbuf_real;
	double *dbuf_output;
	enum { MODE_NORMAL,MODE_DFT } mode;
	fftw_plan plan;
#endif

};

struct _ScopeDisplayClass
{
	GtkDrawingAreaClass parent_class;
};

#define SCOPE_DISPLAY_TYPE             (scope_display_get_type ())
#define SCOPE_DISPLAY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCOPE_DISPLAY_TYPE, ScopeDisplay))
#define SCOPE_DISPLAY_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), SCOPE_DISPLAY_TYPE,  ScopeDisplayClass))
#define SCOPE_IS_DISPLAY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCOPE_DISPLAY_TYPE))
#define SCOPE_IS_DISPLAY_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), SCOPE_DISPLAY_TYPE))
#define SCOPE_DISPLAY_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), SCOPE_DISPLAY_TYPE, ScopeDisplayClass))

GtkWidget *scope_display_new (void);
void scope_display_set_data(GtkWidget *scope, unsigned char *data, size_t size);
void scope_display_set_trigger_level(GtkWidget *scope, unsigned char level);
void scope_display_set_zoom(GtkWidget *scope, unsigned int zoom);
void scope_display_set_samples(GtkWidget *scope, unsigned short numSamples);
void scope_display_set_sample_freq(GtkWidget *scope, double freq);
void scope_display_set_channels(GtkWidget *scope, unsigned char);

#endif
