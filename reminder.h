typedef struct {
  gchar *name;
  gint interval; /* hours */
  gint lastdone; /* epochseconds maybe? */
  gboolean expired; /* Time to perform this action */
} Action;
