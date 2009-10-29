#include <gtk/gtk.h>
#include "scope.h"

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

extern int serial_init(gchar*name);
extern int serial_run( void (*setdata)(unsigned char *data));
extern void serial_set_trigger_level(unsigned char trig);


void mysetdata(unsigned char *data)
{
	scope_display_set_data(image,data,512);

}

gboolean scale_changed(GtkWidget *widget)
{
	int l = (int)gtk_range_get_value(GTK_RANGE(widget));
	serial_set_trigger_level(l & 0xff);

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
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Ai"),TRUE,TRUE,0);

	GtkWidget *scale=gtk_hscale_new_with_range(0,255,1);

	gtk_box_pack_start(GTK_BOX(hbox),scale,TRUE,TRUE,0);

	g_signal_connect(G_OBJECT(scale),"value-changed",G_CALLBACK(&scale_changed),NULL);

	scope_display_set_data(image,data,sizeof(data));

	gtk_widget_show_all(vbox);
	gtk_widget_set_size_request(image,512,256);

	serial_run( &mysetdata );

	gtk_main();

	return 0;
}
