typedef union {
  GtkWidget *w;
  GtkWindow *d;
  GtkContainer *c;
  GObject *o;
} Window;

typedef union {
  GtkListStore *l;
  GtkTreeModel *t;
} Liststore;

typedef union {
  GtkTreeView *t;
  GtkWidget *w;
} Treeview;

typedef union {
  GtkTreeViewColumn *c;
  GObject *o;
} Treeviewcolumn;

typedef union {
  GtkTreeSelection *s;
  GObject *o;
} Treeselection;

typedef union {
  GtkCellRenderer *r;
  GtkCellRendererText *t;
  GtkCellRendererToggle *c;
  GObject *o;
} Cellrenderer;

typedef union {
  GtkHBox *v;
  GtkContainer *c;
  GtkBox *b;
  GtkWidget *w;
} Hbox;

typedef union {
  GtkVBox *v;
  GtkContainer *c;
  GtkBox *b;
  GtkWidget *w;
} Vbox;

typedef union {
  GtkScrolledWindow *s;
  GtkWidget *w;
  GtkContainer *c;
} Scrolledwindow;

typedef union {
  GtkButton *b;
  GtkWidget *w;
  GObject *o;
} Button;

typedef GtkCellRendererText* Cellrenderertext;
typedef GtkTreeIter Treeiter;
typedef GtkTreePath* Treepath;
typedef GtkWidget* Widget;
