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

#include "scope.h"
#include <cairo.h>
#include <math.h>

G_DEFINE_TYPE (ScopeDisplay, scope_display, GTK_TYPE_DRAWING_AREA);

static void scope_display_init (ScopeDisplay *scope)
{
	scope->zoom=1;
}

GtkWidget *scope_display_new (void)
{
	return g_object_new (SCOPE_DISPLAY_TYPE, NULL);
}



static void draw(GtkWidget *scope, cairo_t *cr)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	int i;
	int lx=scope->allocation.x;
	int ly=scope->allocation.y+scope->allocation.height;

	cairo_set_source_rgb (cr, 0, 0, 0xff);

	cairo_move_to(cr,lx,ly - self->tlevel);
    cairo_line_to(cr,lx + scope->allocation.width,ly - self->tlevel);
	/*	cairo_arc (cr, x, y, radius, 0, 2 * M_PI);
	 */
	cairo_stroke(cr);

	cairo_set_source_rgb (cr, 0, 0, 0);
//	cairo_fill_preserve (cr);
	//	cairo_set_source_rgb (cr, 0, 0, 0);


	for (i=0; i<512/self->zoom; i++) {
		cairo_move_to(cr,lx,ly);
		lx=scope->allocation.x + i*self->zoom;
		ly=scope->allocation.y+scope->allocation.height - self->dbuf[i];
		cairo_line_to(cr,lx,ly);

	}
	cairo_stroke (cr);
}

void scope_display_set_zoom(GtkWidget *scope, unsigned int zoom)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	self->zoom=zoom;
	gtk_widget_queue_draw(scope);
}

void scope_display_set_data(GtkWidget *scope, unsigned char *data, size_t size)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	
    int i;
	for (i=0; i<size && i<512; i++) {
		self->dbuf[i] = *data;
		data++;
	}
	gtk_widget_queue_draw(scope);
}

void scope_display_set_trigger_level(GtkWidget *scope, unsigned char level)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);

	self->tlevel=level;
	gtk_widget_queue_draw(scope);
}

static gboolean scope_display_expose(GtkWidget *scope, GdkEventExpose *event)
{
	cairo_t *cr;
	/* get a cairo_t */
	cr = gdk_cairo_create (scope->window);

	cairo_rectangle (cr,
					 event->area.x, event->area.y,
					 event->area.width, event->area.height);
	cairo_clip (cr);

	draw (scope, cr);

	cairo_destroy (cr);
	return FALSE;
}



static void scope_display_class_init (ScopeDisplayClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (class);

	widget_class->expose_event = scope_display_expose;
}

