CC=gcc
SPARSE=sparse

all:
	$(CC) `pkg-config x11 gtk+-2.0 --cflags --libs` reminder.c dockapp.c -o reminder $(CFLAGS)

clean:
	rm -f reminder

check:
	$(SPARSE) `pkg-config gtk+-2.0 --cflags` dockapp.c $(WFLAGS)
	$(SPARSE) `pkg-config gtk+-2.0 --cflags` reminder.c $(WFLAGS)
