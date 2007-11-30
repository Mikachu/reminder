typedef union {
  GtkWidget *w;
  GtkWindow *d;
  GtkContainer *c;
  GObject *o;
} Gtkwindow;

typedef union {
  GtkPlug *p;
  GtkWindow *d;
  GtkContainer *c;
  GtkWidget *w;
  GObject *o;
} Plug;

typedef union {
  GtkListStore *l;
  GtkTreeModel *t;
  GObject *o;
} Liststore;

typedef union {
  GtkTreeView *t;
  GtkWidget *w;
  GObject *o;
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
  GtkNotebook *n;
  GtkContainer *c;
  GtkWidget *w;
  GObject *o;
} Notebook;

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
  GtkContainer *c;
  GObject *o;
} Button;

typedef union {
  GtkLabel *l;
  GtkWidget *w;
  GObject *o;
} Label;

typedef union {
  GtkEntry *e;
  GtkWidget *w;
  GObject *o;
} Entry;

typedef union {
  GtkImage *i;
  GtkWidget *w;
  GObject *o;
} Image;

typedef GtkCellRendererText* Cellrenderertext;
typedef GtkTreeIter Treeiter;
typedef GtkTreePath* Treepath;
typedef GtkWidget* Widget;
