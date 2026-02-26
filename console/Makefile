CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -I../include

LIBSC_DIR = ../mySimpleComputer
LIBTERM_DIR = ../myTerm

LIBSC = $(LIBSC_DIR)/libmySimpleComputer.a
LIBTERM = $(LIBTERM_DIR)/libmyTerm.a

PR01 = pr01
CONSOLE = console

PR01_SRCS = pr01.c
PR01_OBJS = $(PR01_SRCS:.c=.o)

CONSOLE_SRCS = main.c
CONSOLE_OBJS = $(CONSOLE_SRCS:.c=.o)

all: $(PR01) $(CONSOLE)

$(PR01): $(PR01_OBJS) $(LIBSC) $(LIBTERM)
	$(CC) $(CFLAGS) -o $@ $(PR01_OBJS) $(LIBSC) $(LIBTERM)

$(CONSOLE): $(CONSOLE_OBJS) $(LIBSC) $(LIBTERM)
	$(CC) $(CFLAGS) -o $@ $(CONSOLE_OBJS) $(LIBSC) $(LIBTERM)

$(LIBSC):
	$(MAKE) -C $(LIBSC_DIR)

$(LIBTERM):
	$(MAKE) -C $(LIBTERM_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PR01) $(CONSOLE) *.o
	$(MAKE) -C $(LIBSC_DIR) clean
	$(MAKE) -C $(LIBTERM_DIR) clean

.PHONY: all clean