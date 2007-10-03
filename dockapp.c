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
#include "pixmaps/Alert.xpm"
#include "pixmaps/Alert.xbm"
#include "pixmaps/Idle.xpm"
#include "pixmaps/Idle.xbm"

/* nicer gtk interface */
#include "gtkunion.h"
/* prototypes */
#include "dockapp.h"

static Plug dockchild;
static Window dockapp;
static Image image;
static GdkPixmap *alert_pixmap, *idle_pixmap;
static GdkBitmap *alert_mask, *idle_mask;
static Pixmap alert_xmask, idle_xmask;

static gboolean handle_dock_event(Window dockapp, GdkEventButton *event, Gtkwindow dialog);

void set_icon_alert(gboolean alert)
{
  if (alert) {
    gtk_image_set_from_pixmap(image.i, alert_pixmap, alert_mask);
    XShapeCombineMask(GDK_DISPLAY(), dockapp, ShapeBounding, 0, 0, alert_xmask, ShapeSet);
  } else {
    gtk_image_set_from_pixmap(image.i, idle_pixmap, idle_mask);
    XShapeCombineMask(GDK_DISPLAY(), dockapp, ShapeBounding, 0, 0, idle_xmask, ShapeSet);
  }
}

static gboolean handle_dock_event(Window dockapp, GdkEventButton *event, Gtkwindow dialog)
{
  if (event->button == 1) {
    gtk_widget_show_all(dialog.w);
    return TRUE;
  }
  return FALSE;
}

/* I'm sorry if this code offends anyone :) */
void create_icon(Gtkwindow dialog, int argc, char *argv[])
{
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

  dockchild.w = gtk_plug_new(0);
  gtk_widget_add_events(dockchild.w, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(dockchild.w, "button-release-event", G_CALLBACK(handle_dock_event), dialog.o);
  gtk_window_set_default_size(dockchild.d, 64, 24);

  gtk_widget_realize(dockchild.w);

  alert_mask = gdk_bitmap_create_from_data(dockchild.w->window,
                                          Alert_bits, Alert_width, Alert_height);
  alert_pixmap = gdk_pixmap_create_from_xpm_d(dockchild.w->window, NULL, NULL, Alert_xpm);
  alert_xmask = XCreateBitmapFromData(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
                                Alert_bits, Alert_width, Alert_height);

  idle_mask = gdk_bitmap_create_from_data(dockchild.w->window,
                                          Idle_bits, Idle_width, Idle_height);
  idle_pixmap = gdk_pixmap_create_from_xpm_d(dockchild.w->window, NULL, NULL, Idle_xpm);
  idle_xmask = XCreateBitmapFromData(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
                                Idle_bits, Idle_width, Idle_height);

  image.w = gtk_image_new();
  set_icon_alert(FALSE);
  gtk_container_add(dockchild.c, image.w);

  gtk_widget_show_all(dockchild.w);

  XReparentWindow(GDK_DISPLAY(), GDK_WINDOW_XID(dockchild.w->window), dockapp, 0, 0);
  XMapWindow(GDK_DISPLAY(), GDK_WINDOW_XID(dockchild.w->window));
  XMapWindow(GDK_DISPLAY(), dockapp);
}

