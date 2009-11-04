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
#include <string.h>

G_DEFINE_TYPE (ScopeDisplay, scope_display, GTK_TYPE_DRAWING_AREA);

static void scope_display_init (ScopeDisplay *scope)
{
	scope->zoom=1;
	scope->dbuf = NULL;
	scope->dual = FALSE;
#ifdef HAVE_DFT
	scope->mode = MODE_NORMAL;
	scope->dbuf_real = NULL;
	scope->dbuf_output = NULL;
#endif
}

GtkWidget *scope_display_new (void)
{
	return g_object_new (SCOPE_DISPLAY_TYPE, NULL);
}

static void draw_background(cairo_t *cr,const GtkAllocation *allocation)
{
	cairo_set_source_rgb (cr, 0, 0, 0);

	//	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle(cr,
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);
	cairo_fill(cr);
}

static void draw_grid(cairo_t *cr,const GtkAllocation *allocation)
{
	cairo_set_source_rgb (cr, 0.4, 0.4, 0.4);

	double step_x = (double)allocation->width / 10.0;
	double step_y = (double)allocation->height / 4.0;
	double x,y;

	for (x=0; x<(double)allocation->width; x+=step_x) {
		cairo_move_to(cr, (double)allocation->x + x, (double)allocation->y);
		cairo_line_to(cr, (double)allocation->x + x, (double)(allocation->y+allocation->height));
		cairo_stroke( cr );
	}
	for (y=0; y<(double)allocation->height; y+=step_y) {
		cairo_move_to(cr, (double)allocation->x, (double)allocation->y + y);
		cairo_line_to(cr, (double)allocation->x + (double)(allocation->x+allocation->width) , (double)allocation->y + y);
		cairo_stroke( cr );
	}
}

static void draw(GtkWidget *scope, cairo_t *cr)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	int i;
	int lx=scope->allocation.x;
	int ly=scope->allocation.y+scope->allocation.height;
	cairo_text_extents_t te;
	cairo_font_extents_t fe;
	double vtextpos;
	gchar text[24];

	draw_background(cr, &scope->allocation);
	draw_grid(cr, &scope->allocation);

	cairo_set_source_rgb (cr, 0, 0, 1.0);
	cairo_fill_preserve (cr);
	cairo_move_to(cr,lx,ly - self->tlevel);
	cairo_line_to(cr,lx + scope->allocation.width,ly - self->tlevel);
	/*	cairo_arc (cr, x, y, radius, 0, 2 * M_PI);
	 */
	cairo_stroke(cr);


	cairo_set_source_rgb( cr, 0, 255, 0);

#ifdef HAVE_DFT

	int v;
	if (NULL!=self->dbuf_output) {
		for (i=0; i<self->numSamples/self->zoom; i++) {
			cairo_move_to(cr,lx,ly);
			v = self->dbuf_output[i]/16;
			if (v<0)
				v=0;
			if (v>255)
				v=255;
			lx=scope->allocation.x + i*self->zoom;
			ly=scope->allocation.y+scope->allocation.height - v;
			cairo_line_to(cr,lx,ly);
		}
		cairo_stroke (cr);
	}

#else
	if (NULL!=self->dbuf) {

		if (!self->dual) {
			for (i=0; i<self->numSamples/self->zoom; i++) {
				cairo_move_to(cr,lx,ly);
				lx=scope->allocation.x + i*self->zoom;
				ly=scope->allocation.y+scope->allocation.height - self->dbuf[i];
				cairo_line_to(cr,lx,ly);
			}
			cairo_stroke (cr);
		} else {
			for (i=0; i<self->numSamples/self->zoom; i+=2) {
				cairo_move_to(cr,lx,ly);
				lx=scope->allocation.x + i*self->zoom;
				ly=scope->allocation.y+scope->allocation.height - self->dbuf[i];
				cairo_line_to(cr,lx,ly);
			}
			cairo_stroke (cr);
			cairo_set_source_rgb(cr,255,255,0);
			lx=scope->allocation.x+1;
			ly=scope->allocation.y+scope->allocation.height;

			for (i=1; i<self->numSamples/self->zoom; i+=2) {
				cairo_move_to(cr,lx,ly);
				lx=scope->allocation.x + i*self->zoom;
				ly=scope->allocation.y+scope->allocation.height - self->dbuf[i];
				cairo_line_to(cr,lx,ly);
			}
			cairo_stroke (cr);
			
		}
	}

#endif

		cairo_set_font_size (cr, 12);
		cairo_set_source_rgb (cr, 0.5,1.0,1.0);
		cairo_select_font_face (cr, "Helvetica",
								CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

		double tdiv = (double)self->numSamples*100.0 / self->freq;
		sprintf(text,"tDiv: %.02fms", tdiv / (double)self->zoom);
		cairo_font_extents(cr, &fe);
		cairo_text_extents(cr, text, &te);

		vtextpos = scope->allocation.y + scope->allocation.height - te.height;

		cairo_move_to(cr,
					  scope->allocation.x + scope->allocation.width - te.width - 10,
					  vtextpos
					 );
		cairo_show_text(cr, text);

		if (self->dual) {
			sprintf(text,"fMax/chan: %.02fHz", self->freq/4);
		} else {
			sprintf(text,"fMax: %.02fHz", self->freq/2);
		}
		cairo_text_extents(cr, text, &te);

		vtextpos -= (te.height + 4);

		cairo_move_to(cr,
					  scope->allocation.x + scope->allocation.width - te.width - 10,
					  vtextpos
					 );
		cairo_show_text(cr, text);
	}

void scope_display_set_zoom(GtkWidget *scope, unsigned int zoom)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	self->zoom=zoom;
	gtk_widget_queue_draw(scope);
}
void scope_display_set_sample_freq(GtkWidget *scope, double freq)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	self->freq=freq;
}

void scope_display_set_samples(GtkWidget *scope, unsigned short numSamples)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	if (self->dbuf)
		g_free(self->dbuf);
	self->dbuf = (unsigned char*)g_malloc(numSamples);
	self->numSamples = numSamples;
#ifdef HAVE_DFT
	if(self->dbuf_real)
		g_free(self->dbuf_real);
	self->dbuf_real = g_malloc(numSamples*sizeof(double));

	if(self->dbuf_output)
		g_free(self->dbuf_output);
	self->dbuf_output = g_malloc(numSamples*sizeof(double));

	self->plan = fftw_plan_r2r_1d(numSamples, self->dbuf_real, self->dbuf_output,
								  FFTW_REDFT01, 0);

#endif


}

void scope_display_set_data(GtkWidget *scope, unsigned char *data, size_t size)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	unsigned char *d = data;
#ifdef HAVE_DFT
	unsigned long sum=0;
	double dc;
#endif

	int i;
	for (i=0; i<size && i<self->numSamples; i++) {
		self->dbuf[i] = *d;
#ifdef HAVE_DFT
		sum+=*d;
#endif
		d++;
	}
#ifdef HAVE_DFT
	d = data;
	dc = (double)sum / (double)self->numSamples;

	for (i=0; i<size && i<self->numSamples; i++) {
		self->dbuf_real[i] = ((double)*d ) - dc;
		d++;
	}
	fftw_execute(self->plan);
#endif
	gtk_widget_queue_draw(scope);
}

void scope_display_set_trigger_level(GtkWidget *scope, unsigned char level)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);

	self->tlevel=level;
	gtk_widget_queue_draw(scope);
}

void scope_display_set_dual(GtkWidget *scope, gboolean value)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);

	self->dual=value;
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

