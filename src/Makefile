CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -Wall
LDLIBS =
OBJECTS =

.PHONY: all clean

all:

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJECTS) $(LDLIBS)