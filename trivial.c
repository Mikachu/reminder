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

static gint64 get_epochseconds(void)
{
  gint64 time;
  time = g_get_real_time();
  return time / G_USEC_PER_SEC;
}

static const gchar *unixtime_to_iso8601(gint64 sec)
{
  static gchar buf[64];
  GDateTime *dt = g_date_time_new_from_unix_local(sec);
  if (!dt)
    *buf = '\0';
  else {
    gchar *s = g_date_time_format_iso8601(dt);
    g_date_time_unref(dt);
    g_strlcpy(buf, s, sizeof(buf));
    g_free(s);
  }
  return buf;
}

static gboolean unixtime_from_iso8601(const gchar *iso, gint64 *time)
{
  GDateTime *dt = g_date_time_new_from_iso8601(iso, NULL);
  if (!dt)
    return FALSE;
  *time = g_date_time_to_unix(dt);
  g_date_time_unref(dt);
  return TRUE;
}
