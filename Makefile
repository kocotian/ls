include config.mk

SRC = lsc.c util.c grammar.c
OBJ = ${SRC:.c=.o}

all: options lsc

options:
	@echo build flags:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c -ggdb ${CFLAGS} ${CPPFLAGS} $<

${OBJ}: config.mk tokentype.h tokentype.c

lsc: ${OBJ} lsc.h
	${CC} -o $@ ${OBJ}

clean:
	rm -f lsc ${OBJ}

grammar.c: tokentype.c

lsc.h: tokentype.h

tokentype.h: tokentypes
	./gentokentypes

tokentype.c: tokentypes
	./gentokentypes

.PHONY: all options clean
