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
GtkWidget *window;
GdkPixbuf *pixbuf;
GtkWidget *image;
GtkWidget *vbox;
GtkWidget *hbox;
cairo_surface_t *surface;
cairo_t *cr;

void win_destroy_callback()
{
	gtk_main_quit();
}

unsigned char current_trigger_level = 0;

void mysetdata(unsigned char *data)
{
	scope_display_set_data(image,data,512);

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

int main(int argc,char **argv)
{
	gtk_init(&argc,&argv);
	unsigned char data[512];
	serial_init(argv[1]);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);


	gtk_widget_show(window);

	g_signal_connect(G_OBJECT(window),"destroy",G_CALLBACK(win_destroy_callback),NULL);


	vbox = gtk_vbox_new(FALSE,0);

	gtk_container_add(GTK_CONTAINER(window),vbox);

	image = scope_display_new();

	gtk_box_pack_start(GTK_BOX(vbox),image,TRUE,TRUE,0);

	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Trigger level:"),TRUE,TRUE,0);
	GtkWidget *scale=gtk_hscale_new_with_range(0,255,1);
	gtk_box_pack_start(GTK_BOX(hbox),scale,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(scale),"value-changed",G_CALLBACK(&trigger_level_changed),NULL);

	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Holdoff samples:"),TRUE,TRUE,0);
	scale=gtk_hscale_new_with_range(0,255,1);
	gtk_box_pack_start(GTK_BOX(hbox),scale,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(scale),"value-changed",G_CALLBACK(&holdoff_level_changed),NULL);



	scope_display_set_data(image,data,sizeof(data));

	gtk_widget_show_all(vbox);
	gtk_widget_set_size_request(image,512,256);

	serial_run( &mysetdata );

	gtk_main();

	return 0;
}
