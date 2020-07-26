LLVMFLAGS = $(shell llvm-config --cflags --ldflags --system-libs --libs core)
CFLAGS = -O3 -g -Wall -Wextra -Werror -pedantic $(LLVMFLAGS)
SRCS = main.c lexer.c parser.c ast.c codegen.o
OBJS = $(subst .c,.o,$(SRCS))
NAME = phi

VPATH = src

all: $(NAME)
.PHONY: all

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(OBJS)
.PHONY: clean

distclean: clean
	rm $(NAME)
.PHONY: distclean
