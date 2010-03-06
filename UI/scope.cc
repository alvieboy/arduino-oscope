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
#include "channel.h"
#include "../protocol.h"

G_DEFINE_TYPE (ScopeDisplay, scope_display, GTK_TYPE_DRAWING_AREA);

static void scope_display_init (ScopeDisplay *scope)
{
	scope->zoom=1;
	scope->dbuf[0] = NULL;
	scope->dbuf[1] = NULL;
	scope->dbuf[2] = NULL;
	scope->dbuf[3] = NULL;

	scope->xy = FALSE;

	scope->analog_height = 255;
	scope->border = 30;
	scope->flags = 0;

#ifdef HAVE_DFT
	scope->mode = MODE_NORMAL;
	scope->dbuf_real = NULL;
	scope->dbuf_output = NULL;
#endif
	int i;
	for (i=0;i<4;i++)
		scope->chancfg[i].gain=1.0;
	scope->scope_xpos = 0;
	scope->scope_ypos = 0;
	scope->numSamples = 962;
}

GtkWidget *scope_display_new (void)
{
	return (GtkWidget*)g_object_new (SCOPE_DISPLAY_TYPE, NULL);
}

static void draw_background(cairo_t *cr,const GtkAllocation *allocation)
{
	/* This ought to fill all available area */

	cairo_set_source_rgb (cr, 0, 0, 0);

	//	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle(cr,
					0,//allocation->x,
					0,//allocation->y,
					allocation->width,
					allocation->height);
	cairo_fill(cr);
}

static void draw_grid(ScopeDisplay *scope, cairo_t *cr)
{
	cairo_set_source_rgb (cr, 0.4, 0.4, 0.4);
	int nxstep = 10;
	int nystep = 4;

	double step_x = (double)scope->numSamples / (double)nxstep;
	double step_y = (double)scope->analog_height / (double)nystep;
	double x,y;
	int xs,ys;

	for (x=0,xs=0; xs<=nxstep; x+=step_x,xs++) {
		cairo_move_to(cr, (double)scope->scope_xpos + x, (double)scope->scope_ypos);
		cairo_line_to(cr, (double)scope->scope_xpos + x, (double)scope->analog_height + (double)scope->scope_ypos);
		cairo_stroke( cr );
	}

	for (y=0,ys=0; ys<=nystep; y+=step_y,ys++) {
		cairo_move_to(cr, (double)scope->scope_xpos, (double)scope->scope_ypos + y);
		cairo_line_to(cr, (double)scope->scope_xpos + (double)scope->numSamples , (double)scope->scope_ypos + y);
		cairo_stroke( cr );
	}
}
typedef struct { double r,g,b; } color_type;

color_type colors[] = {
	{ 0.0,1.0,0.0 },    // Channel 0 - green
	{ 1.0,1.0,0.0 },    // Channel 1 - yellow
	{ 0.99,0.75,0.57 },    // Channel 2
	{ 0.89,0.73,0.86 }    // Channel 3
};

static void draw(GtkWidget *scope, cairo_t *cr)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	unsigned int i;
	cairo_text_extents_t te;
	cairo_font_extents_t fe;
	double vtextpos;
	gchar text[24];

	/* Compute positions */
	self->scope_xpos = (scope->allocation.width - self->numSamples) / 2;
	self->scope_ypos = (scope->allocation.height - self->analog_height) / 2;

	draw_background(cr, &scope->allocation);
	draw_grid(self, cr);

	cairo_set_source_rgb (cr, 0, 0, 1.0);
	cairo_fill_preserve (cr);

	int lx=self->scope_xpos;
	int ly=self->scope_ypos+self->analog_height;
	/* Draw trigger level */
    double clev = ( self->chancfg[0].ypos + self->tlevel * self->chancfg[0].gain);
	cairo_move_to(cr,lx,ly - clev );
	cairo_line_to(cr,lx + self->numSamples,ly - clev);
	/*	cairo_arc (cr, x, y, radius, 0, 2 * M_PI);
	 */
	cairo_stroke(cr);


	cairo_set_source_rgb( cr, 0, 255, 0);
	//cairo_set_line_width(cr,1.0);
	if (self->mode == MODE_DFT)
	{
#ifdef HAVE_DFT

		//double samplerate = self->freq;
		
		double dynamplituderatio=0.0;

		int v;
		if (NULL!=self->dbuf_output) {
			// AUTO PEAK
			static double max=0;
			for (i=0; i<(self->numSamples/self->zoom)/2; i++) {

				double iv = sqrt(self->dbuf_output[i] * self->dbuf_output[i]);
				if (iv>max)
					max=iv;
			}

			dynamplituderatio = 255.0/max;

			for (i=0; i<(self->numSamples/self->zoom)/2; i++) {

				double iv = sqrt(self->dbuf_output[i] * self->dbuf_output[i]);

				cairo_move_to(cr,lx,ly);
				v = (int)(iv * dynamplituderatio);
				if (v<0)
					v=0;
				if (v>255)
					v=255;
				lx=self->scope_xpos + i*self->zoom*2;
				ly=self->scope_ypos + self->analog_height - v;
				cairo_line_to(cr,lx,ly);
			}
			cairo_stroke (cr);
		}
#endif
	} else {
		if (NULL!=self->dbuf[0]) {

			if (self->xy && self->channels == 2) {

				lx=self->scope_xpos;
				ly=self->scope_ypos + self->analog_height;

				for (i=0; i<self->numSamples; i+=2) {
					cairo_move_to(cr,
								  lx + ((int)self->dbuf[i])-127 ,
								  ly + ((int)self->dbuf[i+1])-127
								 );
					lx+=i*self->zoom;
					ly-=self->dbuf[0][i];
					cairo_line_to(cr,lx,ly);
				}
				cairo_stroke (cr);

			} else {
				if (self->flags&CAPTURED_FRAME_FLAG_SEQUENTIAL_CHANNEL) {

					unsigned int start;

					for (start=0; start<self->channels; start++) {
						gboolean firstsample = true;

						cairo_set_source_rgb(cr, colors[start].r,colors[start].g,colors[start].b);

						lx=self->scope_xpos + start;
						ly=self->scope_ypos + self->analog_height;

						/* HACK - 1st sample of 2,3 and 4 channels is always wrong, because it
						 belongs to trigger channel (1)*/
						if (start>0) {
							fprintf(stderr,"Fixing 1st sample of %u to %u\n",
									self->dbuf[start][0] , self->dbuf[start][1]);

							self->dbuf[start][0] = self->dbuf[start][1];
						}

						for (i=0; i<self->numSamples/self->zoom; i++) {

							cairo_move_to(cr,lx,ly);

							lx=self->scope_xpos + i*self->zoom;
							ly=self->scope_ypos + self->analog_height - ((double)self->dbuf[start][i]*(double)self->chancfg[start].gain)
								- (double)self->chancfg[start].ypos;

							if (ly>(self->scope_ypos+self->analog_height-1))
								ly=self->scope_ypos+self->analog_height-1;

							if (ly<self->scope_ypos)
								ly=self->scope_ypos;

							if (!firstsample) {
								cairo_line_to(cr,lx,ly);
							} else {
								firstsample=false;
							}
						}
						cairo_stroke (cr);

					}



				} else {
					unsigned int start;
					for (start=0; start<self->channels; start++) {
						gboolean firstsample = true;
						cairo_set_source_rgb(cr, colors[start].r,colors[start].g,colors[start].b);

						lx=self->scope_xpos + start;
						ly=self->scope_ypos + self->analog_height;

						for (i=start; i<self->numSamples/self->zoom; i+=self->channels) {

							cairo_move_to(cr,lx,ly);

							lx=self->scope_xpos + i*self->zoom;
							ly=self->scope_ypos + self->analog_height - ((double)self->dbuf[0][i]*(double)self->chancfg[start].gain)
								- (double)self->chancfg[start].ypos;

							if (ly>(self->scope_ypos+self->analog_height-1))
								ly=self->scope_ypos+self->analog_height-1;

							if (ly<self->scope_ypos)
								ly=self->scope_ypos;

							if (!firstsample) {
								cairo_line_to(cr,lx,ly);
							} else {
								firstsample=false;
							}
						}
						cairo_stroke (cr);

					}
				}

			}
		}
	}

	cairo_set_font_size (cr, 12);
	cairo_set_source_rgb (cr, 0.5,1.0,1.0);
	cairo_select_font_face (cr, "Helvetica",
							CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

	double tdiv = (double)self->numSamples*1000.0/10.0 / self->freq;
	//sprintf(text,"tDiv: %.02fms", tdiv / (double)self->zoom);
	cairo_font_extents(cr, &fe);
	cairo_text_extents(cr, text, &te);

	vtextpos = scope->allocation.height - te.height;

	cairo_move_to(cr,
				  scope->allocation.width - te.width - 10,
				  vtextpos
				 );
	cairo_show_text(cr, text);


	if (self->channels>1 && !(self->flags&CAPTURED_FRAME_FLAG_SEQUENTIAL_CHANNEL)) {
		sprintf(text,"fMax/chan: %.02fHz", self->freq/(2*self->channels));
	} else {
		sprintf(text,"fMax: %.02fHz", self->freq/2);
	}

	fprintf(stderr,"%s\n",text);
	cairo_text_extents(cr, text, &te);

	vtextpos -= (te.height + 4);

	cairo_move_to(cr,
				  /*scope->allocation.x +*/ scope->allocation.width - te.width - 10,
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
	int i;
	for (i=0;i<4;i++) {
		if (self->dbuf[i])
			g_free(self->dbuf[i]);
		self->dbuf[i] = (unsigned char*)g_malloc(numSamples);
	}
	self->numSamples = numSamples;
#ifdef HAVE_DFT

	if(self->dbuf_real)
		g_free(self->dbuf_real);

	self->dbuf_real = (double*)g_malloc(numSamples*sizeof(double));

	if(self->dbuf_output)
		g_free(self->dbuf_output);

	self->dbuf_output = (double*)g_malloc(numSamples*sizeof(double));

	self->plan = fftw_plan_r2r_1d(numSamples, self->dbuf_real,
				      self->dbuf_output,
				      FFTW_R2HC, /*FFTW_FORWARD*/FFTW_ESTIMATE);
	if (NULL==self->plan) {
            fprintf(stderr,"Cannot create fftw plan!?\n");
	}
#endif


}
void scope_digital_set_data(GtkWidget *scope, unsigned char *data, size_t size)
{
}


void scope_display_set_data(GtkWidget *scope, unsigned char *data, size_t size)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	unsigned char *d = data;
	int storeChannel = 0;
	unsigned char flags;

#ifdef HAVE_DFT
	unsigned long sum=0;
	double dc;
#endif
	/* First value has number of channels */
	self->channels = (d[0] & 0x3) + 1;
	flags = d[1];

	if (flags&CAPTURED_FRAME_FLAG_SEQUENTIAL_CHANNEL) {
		// Sequential
		storeChannel = (d[0]>>2) & 0x03;
	}

	//fprintf(stderr,"Data: channels %u(%u), flags 0x%02x, total size %u\n", self->channels, storeChannel,flags, size);
	unsigned int i;
	d+=2; // Skip flags and channel info

	for (i=2; i<size && i<self->numSamples; i++) {
		self->dbuf[storeChannel][i-2] = *d;
#ifdef HAVE_DFT
		sum+=*d;
#endif
		d++;
	}
#ifdef HAVE_DFT
	d = data + 2;

	dc = (double)sum / (double)self->numSamples;

	for (i=0; i<size && i<self->numSamples; i++) {

		self->dbuf_real[i] = ((double)*d ) - dc;

		//self->dbuf_real[i]  = sin( (double)i / 2.0 ) * 127.0;

		d++;
	}

	if ( self->mode == MODE_DFT ) {
	    memset((void *)self->dbuf_output, 0, self->numSamples * sizeof(double));

	    if (self->plan) {
		fftw_execute(self->plan);
	    }
	}
#endif
	self->flags = flags;

	if (!(flags&CAPTURED_FRAME_FLAG_SEQUENTIAL_CHANNEL) || storeChannel==self->channels-1) {
		gtk_widget_queue_draw(scope);
	}
}

void scope_display_set_trigger_level(GtkWidget *scope, unsigned char level)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);

	self->tlevel=level;
	gtk_widget_queue_draw(scope);
}

void scope_display_set_channels(GtkWidget *scope, unsigned char channels)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);

	self->channels=channels;
	gtk_widget_queue_draw(scope);

}
void scope_set_mode(GtkWidget*scope, scope_mode_t mode)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);

	self->mode=mode;
	//gtk_widget_queue_draw(scope);

}
void scope_display_set_channel_config(GtkWidget *scope,
									  int channel,
									  int xpos,
									  int ypos,
                                      double gain)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	self->chancfg[channel].xpos=xpos;
	self->chancfg[channel].ypos=ypos;
	self->chancfg[channel].gain=gain;
	gtk_widget_queue_draw(scope);
}

static gboolean scope_display_expose(GtkWidget *scope, GdkEventExpose *event)
{
	cairo_t *cr;
	/* get a cairo_t */
	cr = gdk_cairo_create (scope->window);
    //fprintf(stderr,"Expose %d %d\n",event->area.width, event->area.height);
	cairo_rectangle (cr,
					 event->area.x, event->area.y,
					 event->area.width, event->area.height);
	cairo_clip (cr);

	draw (scope, cr);

	cairo_destroy (cr);
	return FALSE;
}

static void scope_size_request(GtkWidget *widget,GtkRequisition *requisition)
{
	ScopeDisplay *self = SCOPE_DISPLAY(widget);
	requisition->width = self->numSamples + self->border*2;
	requisition->height = self->analog_height + self->border*2;
}



static gboolean scope_motion_event(GtkWidget *scope, GdkEventMotion      *event)
{
	int x,y;

	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	x = (int)event->x - scope->allocation.x;
	y = (int)event->y - scope->allocation.y;
	//printf("%d %d\n",x,y);

	double correction = self->freq / self->numSamples;

	//printf("Freq: %f Hz (%f)\n", correction*x/2, self->freq/2);
	return TRUE;
}

static void scope_realize(GtkWidget*scope)
{
	GTK_WIDGET_CLASS (scope_display_parent_class)->realize (scope);
	gtk_widget_add_events(scope,GDK_POINTER_MOTION_MASK);
}

static void scope_display_class_init (ScopeDisplayClass *cl)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (cl);

	widget_class->expose_event = scope_display_expose;
	widget_class->size_request = scope_size_request;
	widget_class->motion_notify_event = scope_motion_event;
    widget_class->realize = scope_realize;
}

struct channelConfig *scope_display_get_config_for_channel(GtkWidget *scope, int chan)
{
	ScopeDisplay *self = SCOPE_DISPLAY(scope);
	return &(self->chancfg[chan]);
}
