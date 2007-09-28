#include <stdio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>

#include "gtkunion.h"
#include "reminder.h"

GSList *actions = NULL;
void save_actions();

void cell_edited(GtkCellRendererText *cell, const gchar *path_string,
                 gchar *new_text, gpointer data)
{
  /* Update stuff from here */
}

Treeviewcolumn new_column(const gchar *name, Liststore store, gint c)
{
  Treeviewcolumn column;
  Cellrenderer renderer;

  renderer.r = gtk_cell_renderer_text_new();
  g_object_set(renderer.o, "editable", TRUE, NULL);
  g_object_set_data(renderer.o, "column", GINT_TO_POINTER(0));
  g_signal_connect(renderer.o, "edited", G_CALLBACK(cell_edited), store.t);

  column.c = gtk_tree_view_column_new_with_attributes(name, renderer.r, "text", c, NULL);
  g_object_set(column.o, "resizable", TRUE,
                         "sizing", GTK_TREE_VIEW_COLUMN_FIXED,
                         "min-width", 50,
                         NULL);

  return column;
}

GtkWidget *create_settings()
{
  Vbox vbox; /* This contains all elements */
  Hbox hbox; /* This contains the buttons at the bottom */
  Scrolledwindow scroll;
  Treeview treeview;
  Liststore liststore;
  const GSList *i = actions;
  GtkTreeIter iter;

  /* Create a new liststore and attach it to a treeview */
  liststore.l = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);

  treeview.w = gtk_tree_view_new_with_model(liststore.t);
  gtk_tree_view_set_rules_hint(treeview.t, TRUE);
  gtk_tree_view_set_headers_visible(treeview.t, TRUE);  

  gtk_tree_view_insert_column(treeview.t, new_column("Task", liststore, 0).c, -1);
  gtk_tree_view_insert_column(treeview.t, new_column("Interval", liststore, 1).c, -1);
  gtk_tree_view_insert_column(treeview.t, new_column("Last Done", liststore, 2).c, -1);

  /* Load up our actions into the liststore */
  while (i) {
    Action *a = (Action *) (i->data);

    gtk_list_store_append(liststore.l, &iter);
    gtk_list_store_set(liststore.l, &iter, 0, a->name, 1, a->interval, 2, a->lastdone, -1);
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

  /* Some buttons too */
  hbox.w = gtk_hbox_new(TRUE, 5);
  Button button;
  button.w = gtk_button_new_with_mnemonic("_Save");
  g_signal_connect(button.o, "clicked", G_CALLBACK(save_actions), NULL);
  gtk_box_pack_start(hbox.b, button.w, TRUE, TRUE, 0);
  gtk_box_pack_start(vbox.b, hbox.w, FALSE, FALSE, 0);

  return vbox.w;
}

Window create_dialog()
{
  Window dialog;

  dialog.d = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(dialog.w, "Reminder");
  g_signal_connect(dialog.d, "delete-event", G_CALLBACK(exit /* Some function that asks for saving any changes before closing */), 0);

  gtk_container_add(dialog.c, create_settings());

  return dialog;
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
  gsize n;

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

    a = (Action *) malloc(sizeof(Action));
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

void save_actions()
{
  gchar *config_dir = g_build_filename(g_get_user_config_dir(), "reminder", NULL);
  gchar *config_file = g_build_filename(g_get_user_config_dir(), "reminder", "actions", NULL);
  GSList *j = actions;
  GKeyFile *key_file = g_key_file_new();

  if (g_mkdir_with_parents(config_dir, 0700) == -1) {
    perror("reminder: couldn't create dir");
    exit(1);
  }

  while (j) {
    Action *a = (Action *) (j->data);

    g_key_file_set_integer(key_file, a->name, "interval", a->interval);
    g_key_file_set_integer(key_file, a->name, "lastdone", a->lastdone);
    j = g_slist_next(j);
  }

  write_keyfile(key_file, config_file);
  g_key_file_free(key_file);
  g_free(config_dir);
  g_free(config_file);
}

int main(int argc, char *argv[])
{
  Window dialog;

  gtk_init(&argc, &argv);

  load_actions();

  dialog = create_dialog();

  gtk_widget_show_all(dialog.d);
  gtk_main();

  return 0;
}

