CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2

TARGET  := test
OBJS    := test.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

test.o: test.c
	$(CC) $(CFLAGS) -c test.c -o test.o

clean:
	rm -f $(TARGET) $(OBJS)
