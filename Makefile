CC=gcc

all:
	$(CC) `pkg-config gtk+-2.0 --cflags --libs` reminder.c dockapp.c -o reminder $(CFLAGS)

clean:
	rm -f reminder
