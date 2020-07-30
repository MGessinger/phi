LLVMFLAGS = $(shell llvm-config --cflags --ldflags --system-libs --libs core)
CFLAGS = -O3 -g -Wall -Wextra -Werror -pedantic -Isrc -I.

OBJS = parser.h lexer.o parser.o ast.o codegen.o stack.o main.o
NAME = phi

VPATH = src

all: $(NAME)
.PHONY: all

.y.h:
	$(YACC) --defines=$@ $<

%.o : parser.h %.c

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LLVMFLAGS)

clean:
	rm $(OBJS)
.PHONY: clean

distclean: clean
	rm $(NAME)
.PHONY: distclean
