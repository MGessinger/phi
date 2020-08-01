LLVMFLAGS = $(shell llvm-config --cflags --ldflags --system-libs --libs core)
CFLAGS = -O3 -g -Wall -Wextra -Werror -pedantic -Isrc -I.

OBJS = lexer.o parser.o ast.o codegen.o stack.o main.o
NAME = phi

VPATH = src

all: $(NAME)
.PHONY: all

.y.h:
	$(YACC) --defines=$@ $<

$(NAME): parser.h $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LLVMFLAGS)

clean:
	rm $(OBJS)
.PHONY: clean

distclean: clean
	rm $(NAME)
.PHONY: distclean
