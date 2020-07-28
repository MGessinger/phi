LLVMFLAGS = $(shell llvm-config --cflags --ldflags --system-libs --libs core)
CFLAGS = -O3 -g -Wall -Wextra -pedantic -Isrc
LEX = flex

OBJS = main.o lexer.o parser.o ast.o codegen.o stack.o
NAME = phi

VPATH = src

all: $(NAME)
.PHONY: all

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LLVMFLAGS)

clean:
	rm $(OBJS)
.PHONY: clean

distclean: clean
	rm $(NAME)
.PHONY: distclean
