all:
	gcc `pkg-config gtk+-2.0 --cflags --libs` reminder.c -o reminder $(CFLAGS)
