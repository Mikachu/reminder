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
#include "Alert.xpm"
#include "Alert.xbm"
#include "Idle.xpm"
#include "Idle.xbm"
#include <X11/extensions/shape.h>

/* nicer gtk interface */
#include "gtkunion.h"

Gtkwindow dialog;
Plug dockchild;
Window dockapp;
GdkPixmap *alert_pixmap;
GdkBitmap *alert_mask;
Pixmap     alert_xmask;
GdkPixmap *idle_pixmap;
GdkBitmap *idle_mask;
Pixmap     idle_xmask;
Image image;

glong get_epochseconds()
{
  GTimeVal time;
  g_get_current_time(&time);
  return time.tv_sec;
}

const gchar *get_iso8601()
{
  GTimeVal time;
  g_get_current_time(&time);
  return g_time_val_to_iso8601(&time);
}

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

void cell_toggled(Cellrenderer renderer, const gchar *path_string,
                  Liststore liststore)
{
  Treeiter iter;
  Treepath path = gtk_tree_path_new_from_string(path_string);
  gboolean value;

  gtk_tree_model_get_iter(liststore.t, &iter, path);
  gtk_tree_model_get(liststore.t, &iter, 3, &value, -1);
  value = FALSE;
  gtk_list_store_set(liststore.l, &iter,
                     2, get_iso8601(),
                     3, FALSE,
                     -1);

  gtk_tree_path_free(path);
}

void cell_edited(Cellrenderer renderer, const gchar *path_string,
                 gchar *new_text, Liststore liststore)
{
  Treeiter iter;
  Treepath path = gtk_tree_path_new_from_string(path_string);
  gint column;
  GTimeVal check;

  column = GPOINTER_TO_INT(g_object_get_data(renderer.o, "column"));

  gtk_tree_model_get_iter(liststore.t, &iter, path);

  switch (column) {
    case 2: /* last done date */
      if (!g_time_val_from_iso8601(new_text, &check)) {
        Gtkwindow message;
        message.w = gtk_message_dialog_new(dialog.d, GTK_DIALOG_DESTROY_WITH_PARENT
                               | GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                               "The entered date is in an invalid format, not using.");
        g_signal_connect(message.o, "response", G_CALLBACK(gtk_widget_destroy), message.w);
        gtk_widget_show_all(message.w);
        break;
      } /* fall through */
    case 0: /* name */
      gtk_list_store_set(liststore.l, &iter, column, new_text, -1);
      break;
    case 1: /* interval */
      gtk_list_store_set(liststore.l, &iter, column, GINT_TO_POINTER(atoi(new_text)), -1);
      break;
  }
}

void new_action(Button button, Treeview treeview)
{
  Liststore liststore;
  Treeiter iter;
  Treeselection selection;
  Treepath path;

  selection.s = gtk_tree_view_get_selection(treeview.t);
  liststore.t = gtk_tree_view_get_model(treeview.t);
  if (gtk_tree_selection_get_selected(selection.s, NULL, &iter))
    gtk_list_store_insert_before(liststore.l, &iter, &iter);
  else
    gtk_list_store_insert_before(liststore.l, &iter, NULL);
  gtk_list_store_set(liststore.l, &iter,
                     0, "",
                     1, GINT_TO_POINTER(24),
                     2, get_iso8601(),
                     -1);
  path = gtk_tree_model_get_path(liststore.t, &iter);
  gtk_tree_selection_select_path(selection.s, path);
  gtk_tree_view_scroll_to_cell(treeview.t, path, NULL, FALSE, 0.0, 0.0);

  gtk_tree_path_free(path);
}

void delete_selected_action(Button button, Treeview treeview)
{
  Liststore liststore;
  Treeiter iter;
  Treeselection selection;

  selection.s = gtk_tree_view_get_selection(treeview.t);
  if (gtk_tree_selection_get_selected(selection.s, NULL, &iter)) {
    liststore.t = gtk_tree_view_get_model(treeview.t);
    gtk_list_store_remove(liststore.l, &iter);
  }
}

void selected_action(Treeselection selection, Button delete)
{
  gtk_widget_set_sensitive(delete.w, gtk_tree_selection_get_selected(selection.s, NULL, NULL));
}

void load_actions(Liststore liststore)
{
  gchar *config_file = g_build_filename(g_get_user_config_dir(), "reminder", "actions", NULL);
  GKeyFile *key_file = g_key_file_new();

  if (!g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_KEEP_COMMENTS, NULL))
    /* No config file */
    goto noconf;

  gchar **action_names;
  gchar **i;

  /* Load new list */
  action_names = g_key_file_get_groups(key_file, NULL);
  for (i = action_names; *i; i++) {
    Treeiter iter;
    const gchar *name = *i, *lastdone_iso;
    gint interval = g_key_file_get_integer(key_file, *i, "interval", NULL);
    lastdone_iso = g_key_file_get_string(key_file, *i, "lastdone", NULL);

    gtk_list_store_append(liststore.l, &iter);
    gtk_list_store_set(liststore.l, &iter,
                       0, name,
                       1, interval,
                       2, lastdone_iso,
                       3, FALSE,
                       -1);
  }
  /* The strings in this array are saved in the liststore so don't free them
   * with g_strfreev */
  g_free(action_names);
noconf:
  g_key_file_free(key_file);
  g_free(config_file);
}

void write_keyfile(GKeyFile *key_file, const gchar *config_file)
{
  char *contents = g_key_file_to_data(key_file, NULL, NULL);
  FILE *config_file_handle = fopen(config_file, "w");
  if (!config_file_handle) {
    perror("reminder: couldn't open config file for writing");
    exit(1);
  }

  fputs(contents, config_file_handle);
  fclose(config_file_handle);
  g_free(contents);
}

void save_actions(Button button, Treeview treeview)
{
  gchar *config_dir = g_build_filename(g_get_user_config_dir(), "reminder", NULL);
  gchar *config_file = g_build_filename(g_get_user_config_dir(), "reminder", "actions", NULL);
  GKeyFile *key_file = g_key_file_new();
  Liststore liststore;
  Treeiter iter;
  gboolean valid;

  if (g_mkdir_with_parents(config_dir, 0700) == -1) {
    perror("reminder: couldn't create dir");
    exit(1);
  }

  liststore.t = gtk_tree_view_get_model(treeview.t);
  valid = gtk_tree_model_get_iter_first(liststore.t, &iter);

  while (valid) {
    const gchar *lastdone_iso, *name;
    gint interval;
    gboolean expired;
    
    gtk_tree_model_get(liststore.t, &iter,
                       0, &name,
                       1, &interval,
                       2, &lastdone_iso,
                       3, &expired,
                       -1);
    if (*name) {
      g_key_file_set_integer(key_file, name, "interval", interval);
      g_key_file_set_string(key_file, name, "lastdone", lastdone_iso);
      g_key_file_set_integer(key_file, name, "expired", expired);
    }
    valid = gtk_tree_model_iter_next(liststore.t, &iter);
  }

  write_keyfile(key_file, config_file);
  g_key_file_free(key_file);
  g_free(config_dir);
  g_free(config_file);
}

Treeviewcolumn new_text_column(const gchar *name, Liststore store, gint c)
{
  Treeviewcolumn column;
  Cellrenderer renderer;

  renderer.r = gtk_cell_renderer_text_new();
  g_object_set(renderer.o, "editable", TRUE, NULL);
  g_object_set_data(renderer.o, "column", GINT_TO_POINTER(c));
  g_signal_connect(renderer.o, "edited", G_CALLBACK(cell_edited), store.t);

  column.c = gtk_tree_view_column_new_with_attributes(name, renderer.r, "text", c, NULL);
  g_object_set(column.o, "resizable", TRUE,
                         "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                         "expand", TRUE,
                         NULL);

  return column;
}

Treeviewcolumn new_check_column(const gchar *name, Liststore store, gint c)
{
  Treeviewcolumn column;
  Cellrenderer renderer;

  renderer.r = gtk_cell_renderer_toggle_new();
  g_object_set(renderer.o, "activatable", TRUE, NULL);
  g_object_set_data(renderer.o, "column", GINT_TO_POINTER(c));
  g_signal_connect(renderer.o, "toggled", G_CALLBACK(cell_toggled), store.t);

  column.c = gtk_tree_view_column_new_with_attributes(name, renderer.r, "active", c, NULL);
  g_object_set(column.o, "resizable", FALSE,
                         "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                         NULL);

  return column;
}

gboolean check_actions(Liststore liststore)
{
  Treeiter iter;
  gboolean valid;
  gboolean all_handled = TRUE;
  glong now = get_epochseconds();

  valid = gtk_tree_model_get_iter_first(liststore.t, &iter);
  while (valid) {
    const gchar *lastdone_iso, *name;
    gboolean expired;
    gint interval;
    GTimeVal lastdone;
    
    gtk_tree_model_get(liststore.t, &iter,
                       0, &name,
                       1, &interval,
                       2, &lastdone_iso,
                       3, &expired,
                       -1);
    g_time_val_from_iso8601(lastdone_iso, &lastdone);
    if (expired)
      all_handled = FALSE;
    else if ((now - lastdone.tv_sec)/(60*60) >= interval)
    {
      gtk_list_store_set(liststore.l, &iter, 3, TRUE, -1);
      set_icon_alert(TRUE);
      all_handled = FALSE;
    }
    valid = gtk_tree_model_iter_next(liststore.t, &iter);
  }
  if (all_handled)
    set_icon_alert(FALSE);
  return TRUE;
}

Widget create_settings()
{
  Vbox vbox; /* This contains all elements */
  Hbox hbox; /* This contains the buttons at the bottom */
  Scrolledwindow scroll;
  Treeview treeview;
  Liststore liststore;
  Treeiter iter;
  Treeselection selection;

  /* Create a new liststore and attach it to a treeview */
  liststore.l = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN);

  treeview.w = gtk_tree_view_new_with_model(liststore.t);
  gtk_tree_view_set_rules_hint(treeview.t, TRUE);
  gtk_tree_view_set_headers_visible(treeview.t, TRUE);  

  gtk_tree_view_insert_column(treeview.t, new_text_column("Task", liststore, 0).c, -1);
  gtk_tree_view_insert_column(treeview.t, new_text_column("Interval", liststore, 1).c, -1);
  gtk_tree_view_insert_column(treeview.t, new_text_column("Last Done", liststore, 2).c, -1);
  gtk_tree_view_insert_column(treeview.t, new_check_column("Expired", liststore, 3).c, -1);

  /* Load up our actions into the liststore */
  load_actions(liststore);

  g_timeout_add_seconds(1, (GSourceFunc)check_actions, liststore.t);

  /* Put everything in a vbox */
  vbox.w = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(vbox.c, 5);
  scroll.w = gtk_scrolled_window_new(NULL, NULL);

  gtk_scrolled_window_set_shadow_type(scroll.s, GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(scroll.s,
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  gtk_container_add(scroll.c, treeview.w);
  gtk_box_pack_start(vbox.b, scroll.w, TRUE, TRUE, 0);

  /* Some buttons too, use the same pointer for all */
  hbox.w = gtk_hbox_new(TRUE, 5);
  Button button;

  /* New button */
  button.w = gtk_button_new_with_mnemonic("_New");
  g_signal_connect(button.o, "clicked", G_CALLBACK(new_action), treeview.t);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  /* Delete button */
  button.w = gtk_button_new_with_mnemonic("_Delete");
  g_signal_connect(button.o, "clicked", G_CALLBACK(delete_selected_action), treeview.t);
  gtk_widget_set_sensitive(button.w, FALSE);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  /* Pass the delete button so we can toggle sensitivity when columns are selected */
  selection.s = gtk_tree_view_get_selection(treeview.t);
  g_signal_connect(selection.o, "changed", G_CALLBACK(selected_action), button.w);

  /* Save button */
  button.w = gtk_button_new_with_mnemonic("_Save");
  g_signal_connect(button.o, "clicked", G_CALLBACK(save_actions), treeview.t);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  /* Quit button */
  button.w = gtk_button_new_with_mnemonic("_Quit");
  g_signal_connect(button.o, "clicked", G_CALLBACK(gtk_main_quit), 0);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  gtk_box_pack_start(vbox.b, hbox.w, FALSE, FALSE, 0);

  return vbox.w;
}

Gtkwindow create_dialog()
{
  dialog.w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(dialog.d, "Reminder");
  gtk_window_set_default_size(dialog.d, 400, 600);
  gtk_window_set_position(dialog.d, GTK_WIN_POS_CENTER);
  g_signal_connect(dialog.o, "delete-event", G_CALLBACK(gtk_widget_hide), 0);

  gtk_container_add(dialog.c, create_settings());

  return dialog;
}

gboolean handle_dock_event(Window dockapp, GdkEventButton *event, Gtkwindow dialog)
{
  if (event->button == 1) {
    gtk_widget_show_all(dialog.w);
    return TRUE;
  }
  return FALSE;
}

/* I'm sorry if this code offends anyone :) */
void create_dockapp(Gtkwindow dialog, int argc, char *argv[])
{
  XWMHints *wm_hints;
  Pixmap xmask;

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

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  dialog = create_dialog();

  create_dockapp(dialog, argc, argv);

  gtk_main();

  return 0;
}

