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

#ifndef __KNOB_H__
#define __KNOB_H__

#include <gtk/gtk.h>
#include <cairo.h>

typedef struct _Knob Knob;
typedef struct _KnobClass       KnobClass;

struct _Knob
{
	GtkDrawingArea parent;
	/* private */
	GtkObject *adj;
	double rest_angle;
	gchar *label;
	gchar *display; // Text (value) to display.
	/* Computations we need */
	cairo_text_extents_t title_extents;
	cairo_text_extents_t value_extents;

	long (*validator)(long value,struct _Knob *);
	gchar *(*formatter)(long value, void *);
	
	void *formatter_data;

	unsigned divisions;
	GSList *divtitles;
	GSList *divtitles_extents;

	double knob_radius;
	gboolean snap_to_divisions;
	long reset_value;
};


struct _KnobClass
{
	GtkDrawingAreaClass parent_class;
	/* Callbacks */
	void (* value_changed)    (Knob     *knob);
};



#define KNOB_TYPE             (knob_get_type ())
#define KNOB(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), KNOB_TYPE, Knob))
#define KNOB_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), KNOB_TYPE,  KnobClass))
#define IS_KNOB(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KNOB_TYPE))
#define IS_KNOB_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), KNOB_TYPE))
#define KNOB_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), KNOB_TYPE, KnobClass))

GType knob_get_type(void) G_GNUC_CONST;

GtkWidget *knob_new (const gchar *label, GtkAdjustment *adj);
GtkWidget *knob_new_with_range(const gchar *label, double min, double max, double step, double page, double val);
double knob_get_value(Knob*);
void knob_set_value(Knob*,double);

void knob_set_validator(Knob *self, long (*validator)(long,Knob*));
void knob_set_divisions(Knob*self, long divisions);
long knob_set_divisions(Knob*self);
GSList *knob_change_division_labels(Knob*self,GSList*list);
void knob_set_formatter(Knob *self, gchar *(*formatter)(long value, void *), void*data);
void knob_set_reset_value(Knob *self, long value);
void knob_set_radius(Knob *self, double radius);

#endif
