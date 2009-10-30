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

#include <gtk/gtk.h>
#include "scope.h"
#include "serial.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

GtkWidget *window;
GdkPixbuf *pixbuf;
GtkWidget *image;
GtkWidget *vbox;
GtkWidget *hbox;
cairo_surface_t *surface;
cairo_t *cr;

GtkWidget *scale_trigger;
GtkWidget *scale_holdoff;
GtkWidget *combo_prescaler;
GtkWidget *combo_vref;

void win_destroy_callback()
{
	gtk_main_quit();
}

unsigned char current_trigger_level = 0;

void mysetdata(unsigned char *data)
{
	scope_display_set_data(image,data,512);
}

void scope_got_parameters(unsigned char triggerLevel,
						  unsigned char holdoffSamples,
						  unsigned char adcref,
						  unsigned char prescale)
{
	gtk_range_set_value(GTK_RANGE(scale_trigger),triggerLevel);
	gtk_range_set_value(GTK_RANGE(scale_holdoff),holdoffSamples);

	switch(adcref) {
	case 0:
	case 1:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_vref),adcref);
		break;
	case 3:
	default:
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_vref),2);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_prescaler),7-prescale);
}


gboolean trigger_level_changed(GtkWidget *widget)
{
	int l = (int)gtk_range_get_value(GTK_RANGE(widget));
	serial_set_trigger_level(l & 0xff);
	current_trigger_level=l&0xff;
	scope_display_set_trigger_level(image,l&0xff);
	return TRUE;
}

gboolean holdoff_level_changed(GtkWidget *widget)
{
	int l = (int)gtk_range_get_value(GTK_RANGE(widget));
	serial_set_holdoff(l & 0xff);

	return TRUE;
}

gboolean zoom_changed(GtkWidget *widget)
{
	int l = (int)gtk_range_get_value(GTK_RANGE(widget));
	scope_display_set_zoom(image,l);

	return TRUE;
}

gboolean prescaler_changed(GtkWidget *widget)
{
	gchar *c = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));

	int base = (int)log2((double)atoi(c));
	//printf("Prescale: 0x%x\n",base);
	serial_set_prescaler(base);
	return TRUE;
}
gboolean vref_changed(GtkWidget *widget)
{
	unsigned char base;
	gchar *c = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));

	/* Ugly :) */
	if (!strcmp(c,"AREF")) {
		base=0;
	} else if (!strcmp(c,"AVcc")) {
		base=1;
	} else {
		base = 3;
	}
	serial_set_vref(base);
	return TRUE;
}

int help(char*cmd)
{
	printf("Usage: %s serialport\n\n",cmd);
	printf("  example: %s /dev/ttyUSB0\n\n",cmd);
	return -1;
}

int main(int argc,char **argv)
{
	GtkWidget*scale_zoom;
	gtk_init(&argc,&argv);
	unsigned char data[512];

	if (argc<2)
		return help(argv[0]);

	if (serial_init(argv[1])<0)
		return -1;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);


	g_signal_connect(G_OBJECT(window),"destroy",G_CALLBACK(win_destroy_callback),NULL);

	hbox = gtk_hbox_new(FALSE,0);


	vbox = gtk_vbox_new(FALSE,0);

	gtk_container_add(GTK_CONTAINER(window),hbox);
	gtk_box_pack_start(GTK_BOX(hbox),vbox,TRUE,TRUE,0);
	scale_zoom=gtk_vscale_new_with_range(1,64,1);
	gtk_box_pack_start(GTK_BOX(hbox),scale_zoom,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(scale_zoom),"value-changed",G_CALLBACK(&zoom_changed),NULL);

	image = scope_display_new();

	gtk_box_pack_start(GTK_BOX(vbox),image,TRUE,TRUE,0);

	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Trigger level:"),TRUE,TRUE,0);
	scale_trigger=gtk_hscale_new_with_range(0,255,1);
	gtk_box_pack_start(GTK_BOX(hbox),scale_trigger,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(scale_trigger),"value-changed",G_CALLBACK(&trigger_level_changed),NULL);

	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Holdoff samples:"),TRUE,TRUE,0);
	scale_holdoff=gtk_hscale_new_with_range(0,255,1);
	gtk_box_pack_start(GTK_BOX(hbox),scale_holdoff,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(scale_holdoff),"value-changed",G_CALLBACK(&holdoff_level_changed),NULL);

	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Prescaler:"),TRUE,TRUE,0);
	combo_prescaler=gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(hbox),combo_prescaler,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(combo_prescaler),"changed",G_CALLBACK(&prescaler_changed),NULL);

	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_prescaler),"128");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_prescaler),"64");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_prescaler),"32");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_prescaler),"16");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_prescaler),"8");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_prescaler),"4");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_prescaler),"2");

	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("VRef source:"),TRUE,TRUE,0);
	combo_vref=gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(hbox),combo_vref,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(combo_vref),"changed",G_CALLBACK(&vref_changed),NULL);

	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_vref),"AREF");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_vref),"AVcc");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_vref),"Internal 1.1V");

	scope_display_set_data(image,data,sizeof(data));

	gtk_widget_show_all(window);
	gtk_widget_set_size_request(image,512,256);

	serial_run( &mysetdata );

	gtk_main();

	return 0;
}
