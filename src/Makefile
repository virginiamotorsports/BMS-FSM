CC = clang
CFLAGS = -Wall -g -v
LDFLAGS = -Wall
LDLIBS =
OBJECTS = fsm FSM.o

.PHONY: all clean

all: fsm

fsm : FSM.o

FSM.o : FSM.cpp FSM.h

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJECTS) $(LDLIBS)