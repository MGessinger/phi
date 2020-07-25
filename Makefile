NAME = fc

WARNINGS = -Wall -Wextra -pedantic
CFLAGS = -O2 -g $(WARNINGS)
SRCS = main.c lexer.c parser.c tokens.c
OBJS = $(subst .c,.o,$(SRCS))

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
