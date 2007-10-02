glong get_epochseconds(void);
const gchar *get_iso8601(void);
void cell_toggled(Cellrenderer renderer, const gchar *path_string,
                  Liststore liststore);
void cell_edited(Cellrenderer renderer, const gchar *path_string,
                 gchar *new_text, Liststore liststore);
void new_action(Button button, Treeview treeview);
void delete_selected_action(Button button, Treeview treeview);
void selected_action(Treeselection selection, Button delete);
void load_actions(Liststore liststore);
void write_keyfile(GKeyFile *key_file, const gchar *config_file);
void save_actions(Button button, Treeview treeview);
Treeviewcolumn new_text_column(const gchar *name, Liststore store, gint c);
Treeviewcolumn new_check_column(const gchar *name, Liststore store, gint c);
gboolean check_actions(Liststore liststore);
Widget create_settings(void);
Gtkwindow create_dialog(void);

