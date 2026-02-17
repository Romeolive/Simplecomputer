CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -I../include

TARGET = pr01
SRCS = pr01.c
OBJS = $(SRCS:.c=.o)

LIBDIR = ../mySimpleComputer
LIB = $(LIBDIR)/libmySimpleComputer.a

all: $(TARGET)

$(TARGET): $(OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB)

$(LIB):
	$(MAKE) -C $(LIBDIR)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
	$(MAKE) -C $(LIBDIR) clean

.PHONY: all clean