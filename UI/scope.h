#ifndef __SCOPE_H__
#define __SCOPE_H__

#include <gtk/gtk.h>

typedef struct _ScopeDisplay ScopeDisplay;
typedef struct _ScopeDisplayClass       ScopeDisplayClass;

struct _ScopeDisplay
{
	GtkDrawingArea parent;
	/* private */
	unsigned char dbuf[512];
	unsigned char tlevel;
};

struct _ScopeDisplayClass
{
	GtkDrawingAreaClass parent_class;
};

#define SCOPE_DISPLAY_TYPE             (scope_display_get_type ())
#define SCOPE_DISPLAY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCOPE_DISPLAY_TYPE, ScopeDisplay))
#define SCOPE_DISPLAY_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), SCOPE_DISPLAY_TYPE,  ScopeDisplayClass))
#define SCOPE_IS_DISPLAY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCOPE_DISPLAY_TYPE))
#define SCOPE_IS_DISPLAY_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), SCOPE_DISPLAY_TYPE))
#define SCOPE_DISPLAY_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), SCOPE_DISPLAY_TYPE, ScopeDisplayClass))

GtkWidget *scope_display_new (void);
void scope_display_set_data(GtkWidget *scope, unsigned char *data, size_t size);
void scope_display_set_trigger_level(GtkWidget *scope, unsigned char level);

#endif
