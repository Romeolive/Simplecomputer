CC = gcc
AR = ar
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -I../include

LIB = libmyTerm.a

SRCS = mt_clrscr.c
OBJS = $(SRCS:.c=.o)

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $(LIB) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
