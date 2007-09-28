#include <stdio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>

#include "gtkunion.h"
#include "reminder.h"

GSList *actions = NULL;

glong get_epochseconds()
{
  GTimeVal time;
  g_get_current_time(&time);
  return time.tv_sec;
}

/* Oh dear god please die gtk+, why don't you pass the new state here? */
void cell_toggled(Cellrenderer renderer, const gchar *path_string,
                  Liststore liststore)
{
  Treeiter iter;
  Treepath path = gtk_tree_path_new_from_string(path_string);
  gint column;
  gboolean value;

  column = GPOINTER_TO_INT(g_object_get_data(renderer.o, "column"));

  gtk_tree_model_get_iter(liststore.t, &iter, path);
  gtk_tree_model_get(liststore.t, &iter, column, &value, -1);
  value = !value;
  gtk_list_store_set(liststore.l, &iter, column, value, -1);

  gtk_tree_path_free(path);
}

void cell_edited(Cellrenderer renderer, const gchar *path_string,
                 gchar *new_text, Liststore liststore)
{
  Treeiter iter;
  Treepath path = gtk_tree_path_new_from_string(path_string);
  gint column;

  column = GPOINTER_TO_INT(g_object_get_data(renderer.o, "column"));

  gtk_tree_model_get_iter(liststore.t, &iter, path);

  switch (column) {
    case 0: /* name */
    case 2: /* last done date */
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
                     2, GINT_TO_POINTER(get_epochseconds),
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

void load_actions()
{
  gchar *config_file = g_build_filename(g_get_user_config_dir(), "reminder", "actions", NULL);
  GKeyFile *key_file = g_key_file_new();

  if (!g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_KEEP_COMMENTS, NULL))
    /* No config file */
    goto noconf;

  gchar **action_names;
  gchar **i;
  GSList *j;

  j = actions;
  /* Clear old list */
  while (j) {
    Action *action = (Action *) (j->data);

    free(action->name);
    j = g_slist_next(j);
  }
  g_slist_free(actions);
  actions = NULL;

  /* Load new list */
  action_names = g_key_file_get_groups(key_file, NULL);
  for (i = action_names; *i; i++) {
    Action *a;

    a = malloc(sizeof(Action));
    a->name = *i;
    a->interval = g_key_file_get_integer(key_file, *i, "interval", NULL);
    a->lastdone = g_key_file_get_integer(key_file, *i, "lastdone", NULL);
    actions = g_slist_append(actions, a);
  }
  /* The strings in this array are saved in a->name so don't free them
   * with g_strfreev */
  g_free(action_names);
  g_key_file_free(key_file);
noconf:
  g_free(config_file);
}

void write_keyfile(GKeyFile *key_file, const gchar *config_file)
{
  char *contents = g_key_file_to_data(key_file, NULL, NULL);
  FILE *config_file_handle = fopen(config_file, "w");
  if (!config_file_handle) {
    perror("reminder");
    exit(1);
  }

  fputs(contents, config_file_handle);
  fclose(config_file_handle);
  g_free(contents);
}

/* This function first clears the action list
 * then fills it in from the list store
 * then stores the list in the keyfile */
void save_actions(Button button, Treeview treeview)
{
  gchar *config_dir = g_build_filename(g_get_user_config_dir(), "reminder", NULL);
  gchar *config_file = g_build_filename(g_get_user_config_dir(), "reminder", "actions", NULL);
  GSList *j;
  GKeyFile *key_file = g_key_file_new();
  Liststore liststore;
  Treeiter iter;
  gboolean valid;

  if (g_mkdir_with_parents(config_dir, 0700) == -1) {
    perror("reminder: couldn't create dir");
    exit(1);
  }

  for (j = actions; j; j = g_slist_next(j)) {
    Action *a = (Action *) (j->data);
    free(a->name);
  }
  g_slist_free(actions);
  actions = NULL;

  liststore.t = gtk_tree_view_get_model(treeview.t);
  valid = gtk_tree_model_get_iter_first(liststore.t, &iter);

  while (valid) {
    GTimeVal time;
    Action *a;
    const gchar *iso;
    
    a = malloc(sizeof(Action));
    gtk_tree_model_get(liststore.t, &iter,
                       0, &a->name,
                       1, &a->interval,
                       2, &iso,
                       3, &a->expired,
                       -1);
    g_time_val_from_iso8601(iso, &time);
    a->lastdone = time.tv_sec;
    actions = g_slist_append(actions, a);
    valid = gtk_tree_model_iter_next(liststore.t, &iter);
  }

  for (j = actions; j; j = g_slist_next(j)) {
    Action *a = (Action *) (j->data);

    g_key_file_set_integer(key_file, a->name, "interval", a->interval);
    g_key_file_set_integer(key_file, a->name, "lastdone", a->lastdone);
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
                         "sizing", GTK_TREE_VIEW_COLUMN_FIXED,
                         "min-width", 50,
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
  g_object_set(column.o, "resizable", TRUE,
                         "sizing", GTK_TREE_VIEW_COLUMN_FIXED,
                         "min-width", 10,
                         NULL);

  return column;
}

Widget create_settings()
{
  Vbox vbox; /* This contains all elements */
  Hbox hbox; /* This contains the buttons at the bottom */
  Scrolledwindow scroll;
  Treeview treeview;
  Liststore liststore;
  const GSList *i = actions;
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
  while (i) {
    Action *a = (Action *) (i->data);
    GTimeVal time;
    time.tv_sec = a->lastdone;
    time.tv_usec = 0;

    gtk_list_store_append(liststore.l, &iter);
    gtk_list_store_set(liststore.l, &iter,
                       0, a->name,
                       1, a->interval,
                       2, g_time_val_to_iso8601(&time),
                       -1);
    i = g_slist_next(i);
  }

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

  /* Save button */
  button.w = gtk_button_new_with_mnemonic("_Save");
  g_signal_connect(button.o, "clicked", G_CALLBACK(save_actions), treeview.t);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);

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

  gtk_box_pack_start(vbox.b, hbox.w, FALSE, FALSE, 0);

  return vbox.w;
}

gboolean handle_reply(Window confirm, gint response, Window dialog)
{
  if (response == GTK_RESPONSE_YES)
    gtk_widget_hide(dialog.w);

  gtk_widget_destroy(confirm.w);
  
  return TRUE;
}

gboolean confirm_close(Window dialog, gpointer event, gpointer data)
{
  Window confirm;

  confirm.w =
    gtk_message_dialog_new(dialog.d, GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                           GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                           "There may be unsaved changes, close window anyway?");
  g_signal_connect(confirm.o, "response", G_CALLBACK(handle_reply), dialog.w);
  gtk_widget_show_all(confirm.w);
  return TRUE;
}

Window create_dialog()
{
  Window dialog;

  dialog.w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(dialog.d, "Reminder");
  gtk_window_set_default_size(dialog.d, 400, 600);
  gtk_window_set_position(dialog.d, GTK_WIN_POS_CENTER);
  g_signal_connect(dialog.w, "delete-event", G_CALLBACK(confirm_close), 0);

  gtk_container_add(dialog.c, create_settings());

  return dialog;
}

void notify_expired(gchar *s)
{
  printf("do something clever %s\n", s);
}

gboolean check_actions(gpointer data)
{
  GSList *j;
  glong now = get_epochseconds();

  for (j = actions; j; j = g_slist_next(j)) {
    Action *a = j->data;
    if (!a->expired && ((now - a->lastdone)/(60*24) > a->interval)) {
      a->expired = TRUE;
      notify_expired(a->name);
    }
  }
  return TRUE;
}

int main(int argc, char *argv[])
{
  Window dialog;

  gtk_init(&argc, &argv);

  load_actions();

  dialog = create_dialog();

  g_timeout_add_seconds(1, check_actions, NULL);

  gtk_widget_show_all(dialog.w);
  gtk_main();

//  printf("%li\n", get_epochseconds());

  return 0;
}

