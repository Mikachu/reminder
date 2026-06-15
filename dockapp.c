#include <stdio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>

/* dockapp stuff */
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include "pixmaps/alert.xpm"
#include "pixmaps/postponed.xpm"
#include "pixmaps/idle.xpm"

/* nicer gtk interface */
#include "gtkunion.h"
/* prototypes */
#include "reminder.h"
#include "dockapp.h"

static Plug dockchild;
static GdkWindow *gdkdockapp;
static Image image;
static GdkPixmap *pixmap[NUM_ALERT];
static GdkBitmap *bitmap[NUM_ALERT];

static gboolean handle_dock_event(Plug dockchild, GdkEventButton *event, Gtkwindow dialog);

void set_icon_alert(AlertLevel level)
{
  gtk_image_set_from_pixmap(image.i, pixmap[level], NULL);
  gdk_window_shape_combine_mask(gdkdockapp, bitmap[level], 0, 0);
}

static gboolean handle_dock_event(Plug dockchild, GdkEventButton *event, Gtkwindow dialog)
{
  gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(g_object_get_data(dialog.o, "treeview")));
  if (event->button == 1) {
    gtk_widget_show_all(dialog.w);
    if (!gtk_window_is_active(dialog.d)) {
      gtk_widget_hide(dialog.w);
      gtk_window_deiconify(dialog.d);
      gtk_window_present_with_time(dialog.d, event->time);
    } else {
      gtk_widget_hide(dialog.w);
    }
    return TRUE;
  }
  return FALSE;
}

/* I'm sorry if this code offends anyone :) */
void create_icon(Gtkwindow dialog, int argc, char *argv[])
{
  Window dockapp;
  XWMHints *wm_hints;

  dockapp = XCreateSimpleWindow(GDK_DISPLAY(), GDK_ROOT_WINDOW(), 0, 0, 64, 24, 0, 0, 0);
  wm_hints = XAllocWMHints();
  wm_hints->initial_state = WithdrawnState;
  wm_hints->icon_window = dockapp;
  wm_hints->window_group = dockapp;
  wm_hints->flags = StateHint | IconWindowHint;
  XSetWMHints(GDK_DISPLAY(), dockapp, wm_hints);
  XFree(wm_hints);
  XSetCommand(GDK_DISPLAY(), dockapp, argv, argc);

  gdkdockapp = gdk_window_foreign_new(dockapp);

  dockchild.w = gtk_plug_new(0);
  gtk_widget_add_events(dockchild.w, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(dockchild.w, "button-release-event", G_CALLBACK(handle_dock_event), dialog.o);
  gtk_window_set_default_size(dockchild.d, 64, 24);

  gtk_widget_realize(dockchild.w);

  pixmap[ALERT_IDLE] = gdk_pixmap_create_from_xpm_d(dockchild.w->window, &bitmap[ALERT_IDLE], NULL, idle_xpm);
  pixmap[ALERT_POSTPONED] = gdk_pixmap_create_from_xpm_d(dockchild.w->window, &bitmap[ALERT_POSTPONED], NULL, postponed_xpm);
  pixmap[ALERT_ALERT] = gdk_pixmap_create_from_xpm_d(dockchild.w->window, &bitmap[ALERT_ALERT], NULL, alert_xpm);

  image.w = gtk_image_new();
  set_icon_alert(ALERT_IDLE);
  gtk_container_add(dockchild.c, image.w);

  gtk_widget_show_all(dockchild.w);

  gdk_window_reparent(dockchild.w->window, gdkdockapp, 0, 0);
  gdk_window_show_unraised(dockchild.w->window);
  /* can't use gdk for this final map, or it will clear my carefully set dockapp hints */
  XMapWindow(GDK_DISPLAY(), dockapp);
}

