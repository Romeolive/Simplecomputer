CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic
TARGET = main
SRCS = $(wildcard *.c)

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
