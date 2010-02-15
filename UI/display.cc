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
#include "digitalscope.h"
#include "serial.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../protocol.h"
#include "channel.h"
#include "knob.h"


GtkWidget *window;
GdkPixbuf *pixbuf;
GtkWidget *image;
GtkWidget *digital_image;
GtkWidget *vbox;
GtkWidget *hbox;
cairo_surface_t *surface;
cairo_t *cr;

GtkWidget *knob_trigger;
GtkWidget *knob_holdoff;
GtkWidget *combo_prescaler;
GtkWidget *combo_vref;
//GtkWidget *combo_channels;
GtkWidget *knob_channels;
GtkWidget *shot_button;
GtkWidget *freeze_button;
GtkStyle *style;

unsigned short numSamples;
static gboolean frozen=FALSE;

const unsigned long arduino_freq = 16000000; // 16 MHz

void win_destroy_callback()
{
	gtk_main_quit();
}

unsigned char current_trigger_level = 0;

void mysetdata(unsigned char *data,size_t size)
{
	scope_display_set_data(image,data,size);
}

void mydigsetdata(unsigned char *data,size_t size)
{
	//digital_scope_display_set_data(digital_image,data,size);
}

void scope_got_parameters(unsigned char triggerLevel,
						  unsigned char holdoffSamples,
						  unsigned char adcref,
						  unsigned char prescale,
						  unsigned short numS,
						  unsigned char flags,
						  unsigned char num_channels)
{
	numSamples=numS;
	gtk_widget_set_size_request(image,numS,256 + 80);
	gtk_widget_set_size_request(digital_image,numS,128);
	scope_display_set_samples(image,numS);
	scope_display_set_channels(image,num_channels);

	knob_set_value(KNOB(knob_trigger),triggerLevel);
	knob_set_value(KNOB(knob_holdoff),holdoffSamples);

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
	//gtk_combo_box_set_active(GTK_COMBO_BOX(combo_channels),num_channels-1);
	knob_set_value( KNOB(knob_channels), num_channels );
}


gboolean trigger_level_changed(GtkWidget *widget)
{
	int l = (int)knob_get_value(KNOB(widget));
	serial_set_trigger_level(l & 0xff);
	current_trigger_level=l&0xff;
	scope_display_set_trigger_level(image,l&0xff);
	return TRUE;
}

gboolean holdoff_level_changed(GtkWidget *widget)
{
	int l = (int)knob_get_value(KNOB(widget));
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
	if(!c)
        return FALSE;
	int base = (int)log2((double)atoi(c));
	printf("Prescale: 0x%x\n",base);
	serial_set_prescaler(base);

	double fsample = get_sample_frequency(arduino_freq, atol(c));
	printf("Fsample: %F Hz\n", fsample);

	// 512 samples.

	// Ts = 1/freq.
	// Full scope time: 512*1/freq.
	// Each slot: 512/freq/10
    // In ms * 1000.0

	printf("Tdiv: %F ms\n", (double)numSamples*100.0 / fsample );
	printf("Test with freq %F Hz\n", fsample/((double)numSamples/10.0));
	scope_display_set_sample_freq(image, fsample);
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

void trigger_toggle_changed(GtkWidget *widget)
{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	serial_set_trigger_invert(active);
}

void channels_changed(GtkWidget *widget)
{
	long v = knob_get_value(KNOB(widget));
	//char *active_s = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	//if (active_s)
	//	serial_set_channels(atoi(active_s));
	serial_set_channels(v);
}

void cancel_trigger(GtkDialog *widget)
{
	gtk_widget_destroy(GTK_WIDGET(widget));
}

void close_trigger(GtkWidget *widget)
{
	printf("Done\n");
}

void done_trigger(void*data)
{
	GtkWidget*dialog=GTK_WIDGET(data);
	gtk_widget_destroy(dialog);
}

void set_frozen(gboolean yes)
{
	frozen=yes;
	if (frozen) {
		gtk_button_set_label(GTK_BUTTON(freeze_button),"Unfreeze");
	}
	else
		gtk_button_set_label(GTK_BUTTON(freeze_button),"Freeze");
}

void freeze_unfreeze(GtkWidget *widget)
{
	serial_freeze_unfreeze(!frozen);
	if (frozen) {
		serial_set_oneshot( NULL, NULL );
		set_frozen(FALSE);
	} else {
		set_frozen(TRUE);
	}
}

gboolean trigger_single_shot(GtkWidget *widget)
{
	GtkWidget *dialog= gtk_dialog_new_with_buttons("Waiting for trigger...",
												   GTK_WINDOW(window),

												   (GtkDialogFlags)(GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT),
												   GTK_STOCK_CANCEL,
												   GTK_RESPONSE_REJECT,
												   NULL);

	GtkWidget *l = gtk_label_new("Waiting for trigger....");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),l,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(dialog),"close",G_CALLBACK(&close_trigger),NULL);
	g_signal_connect(G_OBJECT(dialog),"response",G_CALLBACK(&cancel_trigger),NULL);

	gtk_widget_show_all(dialog);

	set_frozen(TRUE);
	serial_set_oneshot( done_trigger, dialog );

	return TRUE;
}

int help(char*cmd)
{
	printf("Usage: %s serialport\n\n",cmd);
	printf("  example: %s /dev/ttyUSB0\n\n",cmd);
	return -1;
}

void channel_changed_ypos(GtkWidget *knob, struct channelConfig *conf)
{
	conf->ypos = (int)knob_get_value(KNOB(knob));
}

void channel_changed_gain(GtkWidget *knob, struct channelConfig *conf)
{
	conf->gain = knob_get_value(KNOB(knob))/100.0;
}

GtkWidget *create_channel(const gchar *name, struct channelConfig *chanConfig)
{
	GtkWidget *frame = gtk_frame_new(name);

	GtkWidget *hbox = gtk_hbox_new(FALSE,4);

	/*
	 GtkWidget *pos=knob_new_with_range("X-POS",-255,255,1,10,0);
	 gtk_box_pack_start(GTK_BOX(hbox),pos,TRUE,TRUE,0);
	 gtk_widget_set_size_request(pos,60,60);
	 */

	GtkWidget *pos=knob_new_with_range("Y-POS",-255,255,1,10,0);
	gtk_box_pack_start(GTK_BOX(hbox),pos,TRUE,TRUE,0);
//	gtk_widget_set_size_request(pos,60,60);
	g_signal_connect(G_OBJECT(pos),"value-changed",G_CALLBACK(&channel_changed_ypos),chanConfig);

	pos=knob_new_with_range("GAIN",0,200,5,25,100);
	gtk_box_pack_start(GTK_BOX(hbox),pos,TRUE,TRUE,0);
//	gtk_widget_set_size_request(pos,60,60);
	g_signal_connect(G_OBJECT(pos),"value-changed",G_CALLBACK(&channel_changed_gain),chanConfig);

	gtk_container_add(GTK_CONTAINER(frame), hbox);
	return frame;
}

void win_expose_callback(GtkWidget *w)
{

	//style = gtk_style_attach(style,w->window);
}

void channel_changed(GtkWidget *w, void *data)
{
	int i = (int)data;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
		fprintf(stderr,"I: %d\n",i);
	}
}


int main(int argc,char **argv)
{
	GtkWidget*scale_zoom;
	

	gtk_init(&argc,&argv);

	if (argc<2)
		return help(argv[0]);

	if (serial_init(argv[1])<0)
		return -1;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	style = gtk_widget_get_style(window);

	g_signal_connect(G_OBJECT(window),"destroy",G_CALLBACK(win_destroy_callback),NULL);
	//g_signal_connect(G_OBJECT(window),"expose-event",G_CALLBACK(win_expose_callback),NULL);

	GdkColor color;

	gdk_color_parse ("black", &color);

	//gtk_widget_modify_bg (window, GTK_STATE_NORMAL, &color);

    /*
	GtkRcStyle * rc = gtk_widget_get_modifier_style(window);
	gtk_rc_parse_string("style \"oscope\" { bg[NORMAL]=\"black\" fg[NORMAL]=\"white\" }");
	gtk_rc_parse_string("widget \"*\" style \"oscope\"\n");
	gtk_widget_modify_style(window,rc);
    */
	hbox = gtk_hbox_new(FALSE,0);


	vbox = gtk_vbox_new(FALSE,0);

	gtk_container_add(GTK_CONTAINER(window),hbox);
	gtk_box_pack_start(GTK_BOX(hbox),vbox,TRUE,TRUE,0);
	scale_zoom=gtk_vscale_new_with_range(1,64,1);
	gtk_box_pack_start(GTK_BOX(hbox),scale_zoom,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(scale_zoom),"value-changed",G_CALLBACK(&zoom_changed),NULL);

	GtkWidget *chan1 = gtk_radio_button_new_with_label(NULL,"1");
	GtkWidget *chan2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(chan1),"2");
	GtkWidget *chan3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(chan1),"3");
	GtkWidget *chan4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(chan1),"4");

	hbox = gtk_hbox_new(FALSE,0);

	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Channels:"),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox),chan1,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox),chan2,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox),chan3,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox),chan4,FALSE,FALSE,0);

	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new(""),TRUE,TRUE,0);

	g_signal_connect(G_OBJECT(chan1),"toggled",G_CALLBACK(&channel_changed),(void*)1);
	g_signal_connect(G_OBJECT(chan2),"toggled",G_CALLBACK(&channel_changed),(void*)2);
	g_signal_connect(G_OBJECT(chan3),"toggled",G_CALLBACK(&channel_changed),(void*)3);
	g_signal_connect(G_OBJECT(chan4),"toggled",G_CALLBACK(&channel_changed),(void*)4);


	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	image = scope_display_new();

	//digital_image = digital_scope_display_new();
	gtk_box_pack_start(GTK_BOX(vbox),image,TRUE,TRUE,0);
	//gtk_box_pack_start(GTK_BOX(vbox),digital_image,TRUE,TRUE,0);

   // gtk_widget_set_size_request(digital_image,512,128);

	//hbox = gtk_hbox_new(FALSE,4);
	//gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	//gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Trigger level:"),TRUE,TRUE,0);

	// TESTING

	GtkWidget *frame = gtk_frame_new("Trigger");
	gtk_box_pack_start(GTK_BOX(vbox),frame,TRUE,TRUE,0);

	hbox = gtk_hbox_new(FALSE,4);

	knob_trigger=knob_new_with_range("TRIGGER",0,255,1,10,0);
	gtk_box_pack_start(GTK_BOX(hbox),knob_trigger,TRUE,TRUE,0);
//    gtk_widget_set_size_request(knob_trigger,60,60);
	g_signal_connect(G_OBJECT(knob_trigger),"value-changed",G_CALLBACK(&trigger_level_changed),NULL);

	knob_holdoff= knob_new_with_range("HOLD",0,255,1,10,0);
	gtk_box_pack_start(GTK_BOX(hbox),knob_holdoff,TRUE,TRUE,0);
//	gtk_widget_set_size_request(knob_holdoff,60,60);
	g_signal_connect(G_OBJECT(knob_holdoff),"value-changed",G_CALLBACK(&holdoff_level_changed),NULL);

	gtk_container_add(GTK_CONTAINER(frame), hbox);

    /*
	hbox = gtk_hbox_new(FALSE,4);

	gtk_box_pack_start(GTK_BOX(hbox),create_channel("Channel A",scope_display_get_config_for_channel(image,0)),TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),create_channel("Channel B",scope_display_get_config_for_channel(image,1)),TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),create_channel("Channel C",scope_display_get_config_for_channel(image,2)),TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),create_channel("Channel D",scope_display_get_config_for_channel(image,3)),TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
    */

/*	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Holdoff samples:"),TRUE,TRUE,0);
	scale_holdoff=gtk_hscale_new_with_range(0,255,1);
	gtk_box_pack_start(GTK_BOX(hbox),scale_holdoff,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(scale_holdoff),"value-changed",G_CALLBACK(&holdoff_level_changed),NULL);
  */
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


	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

    /*
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Channels:"),TRUE,TRUE,0);
	combo_channels = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(hbox),combo_channels,TRUE,TRUE,0);

	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"1");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"2");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"3");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"4");
	g_signal_connect(G_OBJECT(combo_channels),"changed",G_CALLBACK(&channels_changed),NULL);
    */
	//gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Channels:"),TRUE,TRUE,0);
	knob_channels = knob_new_with_range("CHANS",1,4,1,1,1);
	knob_set_divisions( KNOB(knob_channels), 4 );
	gtk_box_pack_start(GTK_BOX(hbox),knob_channels,TRUE,TRUE,0);
	//gtk_widget_set_size_request(knob_channels,60,60);

	/*
	 gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"1");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"2");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"3");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_channels),"4");
	*/

	g_signal_connect(G_OBJECT(knob_channels),"changed",G_CALLBACK(&channels_changed),NULL);

	hbox = gtk_hbox_new(FALSE,4);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

	shot_button = gtk_button_new_with_label("Single shot");
	g_signal_connect(G_OBJECT(shot_button),"clicked",G_CALLBACK(&trigger_single_shot),NULL);
	gtk_box_pack_start(GTK_BOX(hbox),shot_button,TRUE,TRUE,0);

	freeze_button = gtk_button_new_with_label("Freeze");
	g_signal_connect(G_OBJECT(freeze_button),"clicked",G_CALLBACK(&freeze_unfreeze),NULL);
	gtk_box_pack_start(GTK_BOX(hbox),freeze_button,TRUE,TRUE,0);

	GtkWidget *tog = gtk_check_button_new_with_label("Invert trigger");
	gtk_box_pack_start(GTK_BOX(hbox),tog,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(tog),"toggled",G_CALLBACK(&trigger_toggle_changed),NULL);

	gtk_widget_show_all(window);

	serial_run( &mysetdata, &mydigsetdata );

	gtk_main();

	return 0;
}
