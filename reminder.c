#include <stdio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>

/* nicer gtk interface */
#include "gtkunion.h"

#include "reminder.h"

/* #ifdef DOCKAPP */
#include "dockapp.h"

#include "pixmaps/icon.xpm"

#define PADDING 5

enum {
  COL_NAME,
  COL_INTERVAL,
  COL_DATESTRING,
  COL_EXPIRED,
  COL_POSTPONED,
  COL_DATEVAL,
  NUMBER_OF_COLUMNS
};

static gboolean check_actions(Liststore liststore);

#include "trivial.c"

static void set_save_sensitivity(GObject *object, gboolean sensitive)
{
  Button button;
  button.o = g_object_get_data(object, "savebutton");
  gtk_widget_set_sensitive(button.w, sensitive);
  g_object_set_data(object, "unsaved", GINT_TO_POINTER(sensitive));
}

/* This just saves making dialog a global variable. */
static Gtkwindow get_dialog(GObject *object)
{
  Gtkwindow dialog;
  dialog.o = g_object_get_data(object, "dialog");
  return dialog;
}

static void cell_toggled(Cellrenderer renderer, const gchar *path_string,
                         Liststore liststore)
{
  Treeiter iter;
  GTimeVal time;
  gint interval, lastdone, column;
  gboolean expired, postponed;

  gtk_tree_model_get_iter_from_string(liststore.t, &iter, path_string);
  gtk_tree_model_get(liststore.t, &iter,
                     COL_INTERVAL, &interval,
                     COL_DATEVAL,  &lastdone,
                     COL_EXPIRED,  &expired,
                     COL_POSTPONED,&postponed,
                     -1);

  column = GPOINTER_TO_INT(g_object_get_data(renderer.o, "column"));
  switch (column) {
  case COL_EXPIRED:
    g_get_current_time(&time);
    /* Automatically update the last done time to the current time, unless
     * the user has already updated the date and the task is currently expired. */
    if (!expired || (time.tv_sec - lastdone)/(60*60) >= interval)
      gtk_list_store_set(liststore.l, &iter,
                         COL_DATESTRING, g_time_val_to_iso8601(&time),
                         COL_DATEVAL,    time.tv_sec,
                         COL_EXPIRED,    FALSE,
                         COL_POSTPONED,  FALSE,
                         -1);
    else
      gtk_list_store_set(liststore.l, &iter,
                         COL_EXPIRED,    FALSE,
                         COL_POSTPONED,  FALSE,
                         -1);
    set_save_sensitivity(liststore.o, TRUE);
    break;
  case COL_POSTPONED:
    if (!expired)
      /* Can't postpone something that isn't due yet. */
      return;
    gtk_list_store_set(liststore.l, &iter,
                       COL_POSTPONED, !postponed,
                       -1);
  }
  check_actions(liststore);
}

static void cell_edited(Cellrenderer renderer, const gchar *path_string,
                        gchar *new_text, Liststore liststore)
{
  Treeiter iter;
  gint column;
  GTimeVal time;

  column = GPOINTER_TO_INT(g_object_get_data(renderer.o, "column"));

  gtk_tree_model_get_iter_from_string(liststore.t, &iter, path_string);

  switch (column) {
    case COL_DATESTRING:
      if (!g_time_val_from_iso8601(new_text, &time)) {
        Gtkwindow message;
        message.w = gtk_message_dialog_new(get_dialog(liststore.o).d, GTK_DIALOG_DESTROY_WITH_PARENT
                               | GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                               "The entered date is in an invalid format, not using.");
        g_signal_connect(message.o, "response", G_CALLBACK(gtk_widget_destroy), message.w);
        gtk_widget_show_all(message.w);
        break;
      }
      gtk_list_store_set(liststore.l, &iter,
                         COL_DATEVAL, time.tv_sec,
                         -1);
      check_actions(liststore);
      /* fall through */
    case COL_NAME:
      gtk_list_store_set(liststore.l, &iter,
                         column, new_text,
                         -1);
      set_save_sensitivity(liststore.o, TRUE);
      break;
    case COL_INTERVAL:
      gtk_list_store_set(liststore.l, &iter,
                         column, GINT_TO_POINTER(atoi(new_text)),
                         -1);
      check_actions(liststore);
      set_save_sensitivity(liststore.o, TRUE);
      break;
  }
}

static void new_action(Button button, Treeview treeview)
{
  Liststore liststore;
  Treeiter iter;
  Treeselection selection;
  Treepath path;
  GTimeVal time;

  g_get_current_time(&time);
  selection.s = gtk_tree_view_get_selection(treeview.t);
  liststore.t = gtk_tree_view_get_model(treeview.t);
  if (gtk_tree_selection_get_selected(selection.s, NULL, &iter))
    gtk_list_store_insert_before(liststore.l, &iter, &iter);
  else
    gtk_list_store_append(liststore.l, &iter);
  gtk_list_store_set(liststore.l, &iter,
                     COL_NAME,       "",
                     COL_INTERVAL,   GINT_TO_POINTER(24),
                     COL_DATESTRING, g_time_val_to_iso8601(&time),
                     COL_EXPIRED,    FALSE,
                     COL_DATEVAL,    time.tv_sec,
                     -1);
  path = gtk_tree_model_get_path(liststore.t, &iter);
  gtk_tree_selection_select_path(selection.s, path);
  gtk_tree_view_scroll_to_cell(treeview.t, path, NULL, FALSE, 0.0, 0.0);
  check_actions(liststore);
  set_save_sensitivity(liststore.o, TRUE);

  gtk_tree_path_free(path);
}

static void delete_response(Gtkwindow window, gint answer, Treeview treeview)
{
  if (answer == GTK_RESPONSE_YES) {
    Liststore liststore;
    Treeiter iter;

    if (gtk_tree_view_get_selected(treeview, &iter)) {
      liststore.t = gtk_tree_view_get_model(treeview.t);
      gtk_list_store_remove(liststore.l, &iter);
      check_actions(liststore);
      set_save_sensitivity(liststore.o, TRUE);
    }
  }

  gtk_widget_destroy(window.w);
}

static void confirm_delete_action(Button button, Treeview treeview)
{
  Treeiter iter;

  if (gtk_tree_view_get_selected(treeview, &iter)) {
    Gtkwindow confirm;
    Liststore liststore;
    const gchar *name;
    gchar *message;

    liststore.t = gtk_tree_view_get_model(treeview.t);
    gtk_tree_model_get(liststore.t, &iter,
                       COL_NAME, &name,
                       -1);
    message = g_strdup_printf("Are you sure you want to delete the action '%s'?", name);
    confirm.w = gtk_message_dialog_new(get_dialog(liststore.o).d, GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, message);
    g_signal_connect(confirm.o, "response", G_CALLBACK(delete_response), treeview.w);
    gtk_widget_show_all(confirm.w);
  }
}

static void load_actions(Liststore liststore)
{
  gchar *config_file = g_build_filename(g_get_user_config_dir(), "reminder", "actions", NULL);
  GKeyFile *key_file = g_key_file_new();

  gchar **action_names;
  gchar **i;

  if (!g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_KEEP_COMMENTS, NULL))
    /* No config file */
    goto noconf;

  /* Load new list */
  action_names = g_key_file_get_groups(key_file, NULL);
  for (i = action_names; *i; i++) {
    Treeiter iter;
    const gchar *name = *i;
    gint interval = g_key_file_get_integer(key_file, *i, "interval", NULL);
    gint lastdone = g_key_file_get_integer(key_file, *i, "lastdone", NULL);

    gtk_list_store_append(liststore.l, &iter);
    gtk_list_store_set(liststore.l, &iter,
                       COL_NAME,       name,
                       COL_INTERVAL,   interval,
                       COL_DATESTRING, tv_sec_to_iso8601(lastdone),
                       COL_EXPIRED,    FALSE,
                       COL_DATEVAL,    lastdone,
                       -1);
  }
  g_strfreev(action_names);
noconf:
  g_key_file_free(key_file);
  g_free(config_file);
}

static void write_keyfile(GKeyFile *key_file, const gchar *config_file)
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

static void save_actions(Button button, Treeview treeview)
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
    const gchar *name;
    gint interval, lastdone;
    gboolean expired;
    
    gtk_tree_model_get(liststore.t, &iter,
                       COL_NAME,     &name,
                       COL_INTERVAL, &interval,
                       COL_EXPIRED,  &expired,
                       COL_DATEVAL,  &lastdone,
                       -1);
    if (*name) {
      g_key_file_set_integer(key_file, name, "interval", interval);
      g_key_file_set_integer(key_file, name, "lastdone", lastdone);
      g_key_file_set_integer(key_file, name, "expired", expired);
    }
    valid = gtk_tree_model_iter_next(liststore.t, &iter);
  }
  set_save_sensitivity(liststore.o, FALSE);

  write_keyfile(key_file, config_file);
  g_key_file_free(key_file);
  g_free(config_dir);
  g_free(config_file);
}

static void confirm_quit(Button button, GObject *object)
{
  if (g_object_get_data(object, "unsaved")) {
    Gtkwindow confirm;
    confirm.w = gtk_message_dialog_new(get_dialog(object).d,
                                       GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
                                       "There are unsaved changes, quit anyway?");
    g_signal_connect(confirm.o, "response", G_CALLBACK(gtk_quit_on_yes), NULL);
    gtk_widget_show_all(confirm.w);
  } else
    gtk_main_quit();
}

static gboolean check_actions(Liststore liststore)
{
  Treeiter iter;
  gboolean valid;
  AlertLevel level = ALERT_IDLE;
  glong now = get_epochseconds();
  gint timepassed, nearesttimeout = 0;

  valid = gtk_tree_model_get_iter_first(liststore.t, &iter);
  while (valid) {
    gboolean expired, postponed;
    gint interval, lastdone;
    
    gtk_tree_model_get(liststore.t, &iter,
                       COL_INTERVAL, &interval,
                       COL_EXPIRED,  &expired,
                       COL_DATEVAL,  &lastdone,
                       COL_POSTPONED,&postponed,
                       -1);
    timepassed = now - lastdone;
    interval*=3600;
    if (expired && postponed) {
      if (level == ALERT_IDLE)
        level = ALERT_POSTPONED;
    } else if (expired) {
      level = ALERT_ALERT;
    } else if (timepassed >= interval) {
      gtk_list_store_set(liststore.l, &iter, COL_EXPIRED, TRUE, -1);
      level = ALERT_ALERT;
    } else if (!nearesttimeout || interval - timepassed < nearesttimeout)
      nearesttimeout = interval - timepassed;
    valid = gtk_tree_model_iter_next(liststore.t, &iter);
  }
  set_icon_alert(level);
  if (nearesttimeout)
    g_timeout_add_seconds(nearesttimeout, (GSourceFunc)check_actions, liststore.t);
  /* Don't repeat timeout */
  return FALSE;
}

static Treeviewcolumn new_column(const gchar *name, Liststore store, gint c, gboolean check)
{
  Treeviewcolumn column;
  Cellrenderer renderer;

  renderer.r = check ? gtk_cell_renderer_toggle_new() : gtk_cell_renderer_text_new();
  g_object_set(renderer.o, check ? "activatable" : "editable", TRUE, NULL);
  g_object_set_data(renderer.o, "column", GINT_TO_POINTER(c));
  g_signal_connect(renderer.o, check ? "toggled" : "edited",
                   check ? G_CALLBACK(cell_toggled) : G_CALLBACK(cell_edited), store.t);

  column.c = gtk_tree_view_column_new_with_attributes(name, renderer.r,
                                                      check ? "active" : "text", c, NULL);
  g_object_set(column.o, "resizable", TRUE,
                         "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                         check ? NULL : "expand", TRUE,
                         NULL);

  return column;
}

static Widget create_tasks_page(Gtkwindow dialog)
{
  Vbox vbox; /* This contains the liststore and the hbox */
  Hbox hbox; /* This contains the buttons at the bottom */
  Scrolledwindow scroll; /* Need to put the treeview in this for scrollbars */
  Treeview treeview;
  Liststore liststore;
  Treeselection selection;
  Button button;

  /* Create a new liststore and attach it to a treeview, the fifth column is
   * used for caching the last done date as an integer so we don't have to parse
   * the iso8601 representation so often. */
  liststore.l = gtk_list_store_new(NUMBER_OF_COLUMNS,
                                   G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING,
                                   G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT);
  g_object_set_data(liststore.o, "dialog", dialog.o);
  g_object_set_data(dialog.o, "liststore", liststore.o);

  treeview.w = gtk_tree_view_new_with_model(liststore.t);
  g_object_set_data(dialog.o, "treeview", treeview.o);
  /* this makes gtk+ draw rows in alternating colors which makes things easier to read */
  gtk_tree_view_set_rules_hint(treeview.t, TRUE);
  gtk_tree_view_set_headers_visible(treeview.t, TRUE);  

  gtk_tree_view_append_column(treeview.t, new_column("Task",      liststore, COL_NAME,       FALSE).c);
  gtk_tree_view_append_column(treeview.t, new_column("Interval",  liststore, COL_INTERVAL,   FALSE).c);
  gtk_tree_view_append_column(treeview.t, new_column("Last Done", liststore, COL_DATESTRING, FALSE).c);
  gtk_tree_view_append_column(treeview.t, new_column("Expired",   liststore, COL_EXPIRED,     TRUE).c);
  gtk_tree_view_append_column(treeview.t, new_column("Postponed", liststore, COL_POSTPONED,   TRUE).c);

  /* Load up our actions into the liststore */
  load_actions(liststore);

  /* Put everything in a vbox */
  vbox.w = gtk_vbox_new(FALSE, PADDING);
  gtk_container_set_border_width(vbox.c, PADDING);
  scroll.w = gtk_scrolled_window_new(NULL, NULL);

  gtk_scrolled_window_set_shadow_type(scroll.s, GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(scroll.s,
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  gtk_container_add(scroll.c, treeview.w);
  gtk_box_pack_start(vbox.b, scroll.w, TRUE, TRUE, 0);

  /* Some buttons too, use the same pointer for all */
  hbox.w = gtk_hbox_new(TRUE, PADDING);

  /* New button */
  button.w = gtk_button_new_with_mnemonic("_New");
  g_signal_connect(button.o, "clicked", G_CALLBACK(new_action), treeview.t);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  /* Delete button */
  button.w = gtk_button_new_with_mnemonic("_Delete");
  g_signal_connect(button.o, "clicked", G_CALLBACK(confirm_delete_action), treeview.t);
  gtk_widget_set_sensitive(button.w, FALSE);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);
  selection.s = gtk_tree_view_get_selection(treeview.t);
  g_signal_connect(selection.o, "changed", G_CALLBACK(gtk_sensitive_when_selected), button.w);

  /* Save button */
  button.w = gtk_button_new_with_mnemonic("_Save");
  g_signal_connect(button.o, "clicked", G_CALLBACK(save_actions), treeview.t);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  /* Put the save button in the treeview so we can set its sensitivity
   * when something changes */
  g_object_set_data(liststore.o, "savebutton", button.o);
  gtk_widget_set_sensitive(button.w, FALSE);
  g_object_set_data(liststore.o, "unsaved", GINT_TO_POINTER(FALSE));

  /* Close button */
  button.w = gtk_button_new_with_mnemonic("_Close");
  g_signal_connect(button.o, "clicked", G_CALLBACK(gtk_widget_hide_2nd_arg), dialog.o);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  /* Quit button */
  button.w = gtk_button_new_with_mnemonic("_Quit");
  g_signal_connect(button.o, "clicked", G_CALLBACK(confirm_quit), liststore.o);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

  gtk_box_pack_start(vbox.b, hbox.w, FALSE, FALSE, 0);

  return vbox.w;
}

static void update_details(Notebook note, GtkNotebookPage *page, guint page_num, Gtkwindow dialog)
{
  Treeview treeview;
  Liststore liststore;
  Treeiter iter;
  Entry Name, Interval, Lastdone;
  gchar *name = NULL;

  if (page_num != 1)
    return;

  treeview.o = g_object_get_data(dialog.o, "treeview");
  liststore.t = gtk_tree_view_get_model(treeview.t);
  Name.o = g_object_get_data(dialog.o, "Name");
  Interval.o = g_object_get_data(dialog.o, "Interval");
  Lastdone.o = g_object_get_data(dialog.o, "Last Done");

  if (gtk_tree_view_get_selected(treeview, &iter)) {
    gtk_tree_model_get(liststore.t, &iter,
                       COL_NAME, &name,
                       -1);
    gtk_entry_set_text(Name.e, name);
  } else
    gtk_entry_set_text(Name.e, "");
}

static Hbox create_label_and_button(gchar *name, Gtkwindow dialog)
{
  Label label;
  Entry entry;
  Hbox hbox;

  hbox.w = gtk_hbox_new(FALSE, PADDING);
  label.w = gtk_label_new(name);
  entry.w = gtk_entry_new();

  gtk_box_pack_start(hbox.b, label.w, TRUE, TRUE, 0);
  gtk_box_pack_start(hbox.b, entry.w, TRUE, TRUE, 0);
  g_object_set_data(dialog.o, name, entry.o);

  return hbox;
}

static Widget create_details_page(Gtkwindow dialog)
{
  Vbox vbox;

  vbox.w = gtk_vbox_new(TRUE, PADDING);
  gtk_box_pack_start(vbox.b, create_label_and_button("Name", dialog).w, TRUE, TRUE, 0);
  gtk_box_pack_start(vbox.b, create_label_and_button("Interval", dialog).w, TRUE, TRUE, 0);
  gtk_box_pack_start(vbox.b, create_label_and_button("Last Done", dialog).w, TRUE, TRUE, 0);

  return vbox.w;
}

static Gtkwindow create_dialog(void)
{
  Gtkwindow dialog;
  Notebook note;

  dialog.w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(dialog.d, "Reminder");
  gtk_window_set_default_size(dialog.d, 500, 300);
  gtk_window_set_position(dialog.d, GTK_WIN_POS_CENTER);
  g_signal_connect(dialog.o, "delete-event", G_CALLBACK(gtk_widget_hide), NULL);

  note.w = gtk_notebook_new();
  gtk_notebook_set_tab_pos(note.n, GTK_POS_TOP);
  gtk_container_add(dialog.c, note.w);
  gtk_notebook_append_page(note.n, create_tasks_page(dialog), gtk_label_new("Tasks"));
  gtk_notebook_append_page(note.n, create_details_page(dialog), gtk_label_new("Details"));
  g_signal_connect(note.o, "switch-page", G_CALLBACK(update_details), dialog.o);
  gtk_window_set_icon(dialog.d, gdk_pixbuf_new_from_xpm_data((const char **)&icon_xpm));

  return dialog;
}

int main(int argc, char *argv[])
{
  Gtkwindow dialog;
  Liststore liststore;

  gtk_init(&argc, &argv);

  dialog = create_dialog();

  create_icon(dialog, argc, argv);

  liststore.o = g_object_get_data(dialog.o, "liststore");
  check_actions(liststore);

  gtk_main();

  return 0;
}

