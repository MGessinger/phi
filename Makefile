LLVMFLAGS = $(shell llvm-config --cflags)
CFLAGS = -O3 -g -Wall -Wextra -Werror -pedantic
SRCS = main.c lexer.c parser.c ast.c
OBJS = $(subst .c,.o,$(SRCS))

VPATH = src

all: fc
.PHONY: all

fc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LLVMFLAGS)

clean:
	rm $(OBJS)
.PHONY: clean

distclean: clean
	rm fc
.PHONY: distclean
