include config.mk

SRC = lsc.c util.c
OBJ = ${SRC:.c=.o}

all: options lsc

options:
	@echo build flags:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c -ggdb ${CFLAGS} ${CPPFLAGS} $<

${OBJ}: config.mk

lsc: ${OBJ}
	${CC} -o $@ ${OBJ}

clean:
	rm -f lsc ${OBJ}

.PHONY: all options clean
