CFLAGS = -O2 -g -Wall -Wextra -pedantic
SRCS = main.c lexer.c parser.c
OBJS = $(subst .c,.o,$(SRCS))

VPATH = src

all: fc
.PHONY: all

fc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(OBJS)
.PHONY: clean

distclean: clean
	rm fc
.PHONY: distclean
