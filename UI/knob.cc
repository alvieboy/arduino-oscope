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

#define __USE_GNU
#include <math.h>

#include "knob.h"
#include <cairo.h>
#include <string.h>
#include <gtk/gtkmarshal.h>
#include <gdk/gdkkeysyms.h>

enum {
	VALUE_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (Knob, knob, GTK_TYPE_DRAWING_AREA);

static gchar *knob_formatter(long value, void *data)
{
    gchar *ret = NULL;
	asprintf(&ret,"%ld",value);
	return ret;
}

static void knob_init (Knob *knob)
{
	// Defaults
	knob->rest_angle = G_PI_4;
	knob->validator = NULL;
	knob->divisions = 13;
	knob->divtitles = NULL;
	knob->divtitles = NULL;
	knob->divtitles_extents = NULL;
	knob->knob_radius = 10.0;
	knob->snap_to_divisions = FALSE;
	knob->formatter = &knob_formatter;
	knob->reset_value = 0;
}


void knob_set_value(Knob *self,double value)
{
	double newvalue = value;
	GtkAdjustment *adj = GTK_ADJUSTMENT(self->adj);

	if (self->snap_to_divisions) {
		double delta = ( adj->upper - adj->lower ) / (self->divisions-1);
		double dval = value - adj->lower;
		int steps = (dval/delta);
		newvalue = (steps * delta) + adj->lower;
	}

	gtk_adjustment_set_value(adj,newvalue);
}

GtkWidget *knob_new_with_range(const gchar *label, double min, double max, double step,double page, double def)
{
	GtkObject *adj =  gtk_adjustment_new(def,min,max,step,page,0);
	return knob_new(label,GTK_ADJUSTMENT(adj));
}


void knob_adjustment_changed(GtkWidget*adj, Knob *self)
{
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

void knob_adjustment_value_changed(GtkWidget*adj, Knob *self)
{
	gtk_widget_queue_draw(GTK_WIDGET(self));
	g_signal_emit (self, signals[VALUE_CHANGED], 0);
}

GtkWidget *knob_new(const gchar *label, GtkAdjustment *adj)
{
	GtkWidget *w = (GtkWidget*)g_object_new (KNOB_TYPE, NULL);
	Knob *self = KNOB(w);
	self->adj = GTK_OBJECT(adj);
	self->label = g_strdup(label);

	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_FOCUS);

	g_signal_connect (adj, "changed",
					  G_CALLBACK (knob_adjustment_changed),
					  self);
	g_signal_connect (adj, "value-changed",
					  G_CALLBACK (knob_adjustment_value_changed),
					  self);
	return w;
}

void knob_set_validator(Knob *self, long (*validator)(long,Knob*))
{
	self->validator = validator;
}

void knob_set_divisions(Knob*self, long divisions)
{
	self->divisions = divisions;
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void compute_extents(GtkWidget *knob, cairo_t *cr)
{
	//cairo_t *cr = gdk_cairo_create (knob->window);
	Knob *self = KNOB(knob);

	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
							CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, 8.0);

	cairo_text_extents (cr, self->display, &self->value_extents);
	cairo_text_extents (cr, self->label, &self->title_extents);

	//cairo_destroy (cr);
}


static void draw(GtkWidget *knob, cairo_t *cr)
{
	Knob *self = KNOB(knob);
	GtkAdjustment *adj =GTK_ADJUSTMENT(self->adj);

	double value = (adj->value - adj->lower) / (adj->upper-adj->lower);

	double angle = G_PI - G_PI_4 + value*(2 * G_PI - self->rest_angle - G_PI_4);

	int x=knob->allocation.x;
	int y=knob->allocation.y;

	int w=knob->allocation.width;
	int h=knob->allocation.height;

	if (GTK_WIDGET_HAS_FOCUS(knob)) {
		cairo_set_source_rgb( cr, 0.7,0.7,1.0);
	} else {
		cairo_set_source_rgb( cr, 0.7,0.7,0.7);
	}
	cairo_arc( cr, w/2,h/2, self->knob_radius, 0, 2.0 * G_PI);
	cairo_fill(cr);

	cairo_set_source_rgb( cr, 0,0,0);
	cairo_arc( cr, w/2,h/2, self->knob_radius, 0, 2.0 * G_PI);
	cairo_stroke(cr);

	// Indicator line

	cairo_set_source_rgb(cr,1.0,0,0);
	cairo_move_to(cr, w/2.0 + 0.9*self->knob_radius*cos(angle) ,h/2 + 0.9*self->knob_radius*sin(angle));
	cairo_line_to(cr, w/2.0 + 0.1*self->knob_radius*cos(angle) ,h/2 + 0.1*self->knob_radius*sin(angle));
	cairo_stroke(cr);

	// Range

	cairo_set_source_rgb( cr, 0,0,0);
	cairo_set_line_width (cr, 1.0);

	cairo_arc( cr, w/2,h/2, self->knob_radius+4.0, self->rest_angle + G_PI_2, 2.0 * G_PI - self->rest_angle + G_PI_2);
	cairo_stroke(cr);

	// Draw metering lines

	double i;
	unsigned int z;
	double delta = (G_PI*2.0 - 2.0*self->rest_angle)/(double)(self->divisions-1);

	/* labels */
	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
							CAIRO_FONT_WEIGHT_BOLD);
	double tick_labels_font_size = 7.0;

	cairo_set_font_size (cr, tick_labels_font_size);
	/* end labels */

	GSList *divtitle = self->divtitles;

	if (NULL!=divtitle && self->divtitles_extents==NULL) {
		//fprintf(stderr,"Need to allocate extents\n");

		/* Compute extents */
		GSList *iter;
		for (iter=divtitle; NULL!=iter; iter=g_slist_next(iter)) {

			//fprintf(stderr,"Allocate extents for '%s'\n", (gchar*) iter->data);
			cairo_text_extents_t *extents = g_new(cairo_text_extents_t,1);

			self->divtitles_extents = g_slist_append(self->divtitles_extents, extents);

			//cairo_set_source_rgb( cr, 0.2,0.2,0.2);
			cairo_text_extents(cr, (gchar*)iter->data, extents);
		}
        //fprintf(stderr,"Extents computed\n");
	}

	GSList *divtitle_extents = self->divtitles_extents;

	for (z=0, i=self->rest_angle + G_PI_2;
		 z<self->divisions;
		 i+=/*self->rest_angle/2.0*/delta,z++) {

		cairo_set_source_rgb( cr, 0,0,0);

		cairo_move_to(cr, w/2.0 + (4.0+self->knob_radius)*cos(i), h/2.0+(4.0+self->knob_radius)*sin(i));
		cairo_line_to(cr, w/2.0 + (8.0+self->knob_radius)*cos(i), h/2.0+(8.0+self->knob_radius)*sin(i));

		cairo_stroke(cr);

		if (NULL!=divtitle) {

			cairo_set_source_rgb( cr, 0.4,0.4,0.4 );

			double DTEXTLEN = 10.0 + self->knob_radius;

			/* Align left/right or center */
			cairo_text_extents_t *extents = (cairo_text_extents_t*)divtitle_extents->data;

			if ( z<((self->divisions-1)/2) ) {
				cairo_move_to(cr, w/2.0 + DTEXTLEN*cos(i) - extents->width,
							  h/2.0+DTEXTLEN*sin(i) + extents->height/2.0);
			} else if ( z==(self->divisions-1)/2 ) {
				cairo_move_to(cr, w/2.0 + DTEXTLEN*cos(i) - extents->width/2,
							  h/2.0+DTEXTLEN*sin(i));

			} else {
				cairo_move_to(cr, w/2.0 + DTEXTLEN*cos(i),
							  h/2.0+DTEXTLEN*sin(i) + extents->height/2.0);

			}


			cairo_show_text (cr, (gchar*)divtitle->data);

			divtitle = g_slist_next(divtitle);
            divtitle_extents = g_slist_next(divtitle_extents);
		}

	}


	self->display = self->formatter((long)GTK_ADJUSTMENT(self->adj)->value,self);

	compute_extents(knob, cr);

	cairo_set_source_rgb( cr, 0.2,0.2,0.2);
	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
							CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, 8.0);


	double px = (w/2.0)-(self->value_extents.width/2 + self->value_extents.x_bearing);
	double py = (h/2.0 + (8.0+self->knob_radius))-(self->value_extents.height/2 + self->value_extents.y_bearing);

	if (self->divtitles) {
		py += tick_labels_font_size;
	}
	cairo_move_to (cr, px,py);
	cairo_show_text (cr, self->display);

	cairo_set_font_size (cr, 8.0);

	px = (w/2.0)-(self->title_extents.width/2 + self->title_extents.x_bearing);
	py = (h/2.0 - (12.0+self->knob_radius))-(self->title_extents.height + self->title_extents.y_bearing);

	if (self->divtitles) {
		py -= tick_labels_font_size;
	}

	cairo_move_to (cr, px, py);
	cairo_show_text (cr, self->label);

	if (self->display) {
		g_free(self->display);
		self->display=NULL;
	}
}

#if 0
static void knob_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	//printf("Request %d %d\n",requisition->width,requisition->height);
	gtk_widget_size_request(widget,requisition);

}

static void knob_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	//printf("Allocate %d %d\n",allocation->width,allocation->height);
}
#endif

static gboolean knob_expose(GtkWidget *knob, GdkEventExpose *event)
{
	cairo_t *cr;
	cr = gdk_cairo_create (knob->window);
	cairo_rectangle (cr,
					 event->area.x, event->area.y,
					 event->area.width, event->area.height);
	cairo_clip (cr);
	draw (knob, cr);
	cairo_destroy (cr);
	return FALSE;
}

static void knob_realize(GtkWidget*knob)
{
	GTK_WIDGET_CLASS (knob_parent_class)->realize (knob);
	gtk_widget_add_events(knob,
						  GDK_FOCUS_CHANGE_MASK
						  |GDK_BUTTON_MOTION_MASK
						  |GDK_BUTTON_PRESS_MASK);
}

static gboolean knob_scroll_event(GtkWidget*knob,GdkEventScroll*event)
{
	Knob *self = KNOB(knob);
	GtkAdjustment *adj = GTK_ADJUSTMENT(self->adj);

	double diff=5*adj->step_increment;
	double newvalue;

	switch (event->state)
	{
	case GDK_MOD1_MASK:
		diff=1.0;    // Need moer info here
		break;
	case GDK_CONTROL_MASK:
		diff=adj->page_increment;
		break;
	default:
		break;
	}

	switch(event->direction) {
	case GDK_SCROLL_UP:
		newvalue = adj->value + diff;
		if (newvalue>adj->upper)
			newvalue = adj->upper;
		knob_set_value(self,newvalue);
		gtk_adjustment_value_changed(adj);
		break;
	case GDK_SCROLL_DOWN:
		newvalue = adj->value - diff;
		if (newvalue<adj->lower)
			newvalue = adj->lower;
		knob_set_value(self,newvalue);
		gtk_adjustment_value_changed(adj);

		break;
	default:
		//printf("???\n");
		return FALSE;
	};

	return TRUE;
}

static void knob_set_value_by_vector(Knob *self,double dx, double dy)
{
	GtkAdjustment *adj = GTK_ADJUSTMENT(self->adj);

	// Compute angle
	double angle;
	double arange = 2*G_PI-(2*self->rest_angle); // Angle range
	long new_value;

	angle = atan2(dx,dy);
	if (angle<0)
		angle+=2*G_PI;

	/*printf("Angle: %f %f  = %f (rad %f, corrected %f)\n", dx, dy, 360.0*angle/(2*G_PI),angle,
		   angle - self->rest_angle);

	printf("Range %f -> %f %%\n",arange,(angle - self->rest_angle)/arange);

	printf("Value: %f\n",adj->lower + (adj->upper-adj->lower) * (angle-self->rest_angle)/arange);
    */
	if (angle<self->rest_angle){
		new_value = adj->lower;
	} else if (angle > 2*G_PI-self->rest_angle) {
		new_value = adj->upper;
	} else {
		new_value = adj->lower + (adj->upper-adj->lower) * (angle-self->rest_angle)/arange;
	}

	if (NULL!=self->validator) {
		new_value = self->validator(new_value,self);
	}

	knob_set_value(self, new_value);

}

static gboolean knob_button_press_event(GtkWidget*knob,GdkEventButton*event)
{
	Knob *self = KNOB(knob);
	//printf("press!!! %f %f \n",event->x,event->y);

	if (!GTK_WIDGET_HAS_FOCUS(knob))
		gtk_widget_grab_focus (knob);

	double dx = -1.0*(event->x - knob->allocation.width/2);
	double dy = event->y - knob->allocation.height/2;

	if (fabs(dx)<self->knob_radius && fabs(dy)<self->knob_radius)
		return TRUE;

	knob_set_value_by_vector(self,dx,dy);
	return TRUE;
}

static gboolean knob_key_press_event(GtkWidget*knob,GdkEventKey*event)
{
	Knob *self = KNOB(knob);
	GtkAdjustment *adj = GTK_ADJUSTMENT(self->adj);

	double diff;

	switch (event->keyval) {
	case GDK_KP_Up:
	case GDK_Up:
		diff = adj->step_increment;
		break;
	case GDK_KP_Down:
	case GDK_Down:
		diff = -1.0*adj->step_increment;
		break;
	case GDK_KP_Left:
	case GDK_Left:
		diff = -1.0*adj->step_increment;
		break;
	case GDK_KP_Right:
	case GDK_Right:
		diff = 1.0*adj->step_increment;
		break;
	case GDK_KP_Home:
	case GDK_Home:
		diff = adj->lower - adj->value;
		break;
	case GDK_KP_End:
	case GDK_End:
		diff = adj->upper - adj->value;
		break;
	case GDK_KP_Page_Up:
	case GDK_Page_Up:
		diff = adj->page_increment;
		break;
	case GDK_KP_Page_Down:
	case GDK_Page_Down:
		diff = -1.0 * adj->page_increment;
		break;
	case GDK_R:
	case GDK_r:
        diff = self->reset_value - adj->value;
        break;
	default:
		return FALSE;
	}

	double newvalue = adj->value + diff;
	if (newvalue<adj->lower)
		newvalue=adj->lower;
    if (newvalue>adj->upper)
		newvalue=adj->upper;

	knob_set_value(self,newvalue);

	return TRUE;
}

static gboolean knob_focus_in_event(GtkWidget*knob,GdkEventFocus*event)
{
	//printf("focusin %p!!!\n",knob);
	gtk_widget_queue_draw(knob);
	return TRUE;
}

static gboolean knob_focus_out_event(GtkWidget*knob,GdkEventFocus*event)
{
	//printf("focusout %p!!!\n",knob);
	gtk_widget_queue_draw(knob);
	return TRUE;
}

static void knob_size_request(GtkWidget *widget,GtkRequisition *requisition)
{
	Knob *self = KNOB(widget);
	requisition->width = self->knob_radius * 2 + 43;
	requisition->height = self->knob_radius * 2 + 43;

	if (self->divtitles) {
		requisition->height += 2* 7.0;
	}
}


static void knob_class_init (KnobClass *cl)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (cl);

	signals[VALUE_CHANGED] =
		g_signal_new ("value-changed",
					  G_TYPE_FROM_CLASS (widget_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (KnobClass, value_changed),
					  NULL, NULL,
					  gtk_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);


	widget_class->expose_event = knob_expose;
	widget_class->size_request = knob_size_request;
	widget_class->realize = knob_realize;
	widget_class->scroll_event = knob_scroll_event;
	widget_class->button_press_event = knob_button_press_event;
	widget_class->focus_in_event = knob_focus_in_event;
	widget_class->focus_out_event = knob_focus_out_event;
	widget_class->key_press_event = knob_key_press_event;
}

double knob_get_value(Knob *self)
{
	return gtk_adjustment_get_value(GTK_ADJUSTMENT(self->adj));
}

GSList *knob_change_division_labels(Knob*self,GSList*list)
{
	GSList *old = self->divtitles;
	GSList *iter;

	int z;

	if (self->divtitles_extents) {
		g_slist_free(self->divtitles_extents);
	}

	self->divtitles = list;
	self->divtitles_extents = NULL;

	if (NULL==list)
		return old;

	

	//fprintf(stderr,"Have new division labels: %p\n",list);
	gtk_widget_queue_draw(GTK_WIDGET(self));

	return old;
}

void knob_set_radius(Knob *self, double radius)
{
	self->knob_radius=radius;
	gtk_widget_queue_draw(GTK_WIDGET(self));
}
void knob_set_formatter(Knob *self, gchar *(*formatter)(long value, void *), void*data)
{
	self->formatter = formatter;
    self->formatter_data = data;
}

void knob_set_reset_value(Knob *self, long value)
{
	self->reset_value = value;
}
