LLVMFLAGS = $(shell llvm-config --cflags --ldflags --system-libs --libs all)
CFLAGS = -O3 -g -Wall -Wextra -Werror -pedantic -Isrc -I.

OBJS = lexer.o parser.o ast.o binaryops.o codegen.o stack.o llvmcontrol.o main.o
NAME = phi

VPATH = src

all: $(NAME)
.PHONY: all

.y.h:
	$(YACC) --defines=$@ $<

lexer.o : parser.h lexer.l

$(NAME): parser.h $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LLVMFLAGS)

clean:
	rm $(OBJS)
.PHONY: clean

distclean: clean
	rm $(NAME)
.PHONY: distclean
