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

#include "digitalscope.h"
#include <cairo.h>
#include <math.h>
#include <string.h>

G_DEFINE_TYPE (DigitalScopeDisplay, digital_scope_display, GTK_TYPE_DRAWING_AREA);

static void digital_scope_display_init (DigitalScopeDisplay *scope)
{
	scope->dbuf = NULL;
}

GtkWidget *digital_scope_display_new (void)
{
	return (GtkWidget*)g_object_new (DIGITAL_SCOPE_DISPLAY_TYPE, NULL);
}

static void draw_background(cairo_t *cr,const GtkAllocation *allocation)
{
	cairo_set_source_rgb (cr, 0, 0, 0);

	//	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle(cr,
					0,//allocation->x,
					0,//allocation->y,
					allocation->width,
					allocation->height);
	cairo_fill(cr);
}

static void draw_grid(cairo_t *cr,const GtkAllocation *allocation)
{
	cairo_set_source_rgb (cr, 0.4, 0.4, 0.4);

	double step_x = (double)allocation->width / 10.0;
	double step_y = (double)allocation->height / 8.0;
	double x,y;

	for (x=0; x<(double)allocation->width; x+=step_x) {
		cairo_move_to(cr, x, 0);
		cairo_line_to(cr, x, (double)allocation->height);
		cairo_stroke( cr );
	}
	for (y=0; y<(double)allocation->height; y+=step_y) {
		cairo_move_to(cr, 0, (double)y);
		cairo_line_to(cr, (double)allocation->width, (double)y);
		cairo_stroke( cr );
	}
}

static struct { double r,g,b; } colors[] = {
	{ 0.0,1.0,0.0 },    // Channel 0 - green
	{ 1.0,1.0,0.0 },    // Channel 1 - yellow
	{ 0.99,0.75,0.57 },    // Channel 2
	{ 0.89,0.73,0.86 }    // Channel 3
};

static void draw(GtkWidget *scope, cairo_t *cr)
{
	DigitalScopeDisplay *self = DIGITAL_SCOPE_DISPLAY(scope);

	int i;
	int z;

	draw_background(cr, &scope->allocation);

	draw_grid(cr, &scope->allocation);

	if (NULL==self->dbuf)
		return;

	int dheight = 8;
	int margin = 4;

	for (z=0; z<8; z++) {
		double base = z*(dheight+2*margin)+margin;
		cairo_set_source_rgb( cr, 0, 255, 0);
		cairo_move_to(cr,0,base);

		for (i=0; i<self->numSamples;i++) {
			if (self->dbuf[i] & (1<<z)) {
				cairo_line_to(cr,i,base+dheight );
			} else {
				cairo_line_to(cr,i,base );
			}
		}
	    cairo_stroke(cr);
	}
	/*
	 cairo_set_source_rgb (cr, 0, 0, 1.0);
	 cairo_fill_preserve (cr);
	 cairo_move_to(cr,lx,ly - self->tlevel);
	 cairo_line_to(cr,lx + scope->allocation.width,ly - self->tlevel);
	 cairo_stroke(cr);

	 */
	return;
	cairo_set_source_rgb( cr, 0, 255, 0);

}

void digital_scope_display_set_data(GtkWidget *scope, unsigned char *data, size_t size)
{
	DigitalScopeDisplay *self = DIGITAL_SCOPE_DISPLAY(scope);
	unsigned char *d = data;

	if (NULL==self->dbuf)
		self->dbuf=(unsigned char*)g_malloc(size);
    self->numSamples=size;

	int i;
	for (i=0; i<size && i<self->numSamples; i++) {
		self->dbuf[i] = *d;
		d++;
	}
	gtk_widget_queue_draw(scope);
}


static gboolean digital_scope_display_expose(GtkWidget *scope, GdkEventExpose *event)
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



static void digital_scope_display_class_init (DigitalScopeDisplayClass *cl)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (cl);

	widget_class->expose_event = digital_scope_display_expose;
}

