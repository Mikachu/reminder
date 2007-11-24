static gboolean gtk_tree_view_get_selected(Treeview treeview, Treeiter *iter)
{
  Treeselection selection;

  selection.s = gtk_tree_view_get_selection(treeview.t);
  return gtk_tree_selection_get_selected(selection.s, NULL, iter);
}

static void gtk_widget_hide_2nd_arg(Widget widget1, Widget widget2)
{
  gtk_widget_hide(widget2);
}

static void gtk_quit_on_yes(Gtkwindow window, gint answer, void *unused)
{
  if (answer == GTK_RESPONSE_YES)
    gtk_main_quit();
  gtk_widget_destroy(window.w);
}

static void gtk_sensitive_when_selected(Treeselection selection, Widget widget)
{
  gtk_widget_set_sensitive(widget, gtk_tree_selection_get_selected(selection.s, NULL, NULL));
}

static const gchar *tv_sec_to_iso8601(gint tv_sec)
{
  GTimeVal time;
  time.tv_sec = tv_sec;
  time.tv_usec = 0;
  return g_time_val_to_iso8601(&time);
}

static glong get_epochseconds(void)
{
  GTimeVal time;
  g_get_current_time(&time);
  return time.tv_sec;
}
