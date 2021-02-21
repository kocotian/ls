#include <unistd.h>

/* temporarily */
#include "arg.h"
#include "util.h"

#define BUFSIZ 8192

static void usage(void);

char *argv0;

static void
usage(void)
{
	die("usage: %s", argv0);
}

int
main(int argc, char *argv[])
{
	char buffer[BUFSIZ];
	ssize_t rb;
	int line;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	for (rb = line = 0; (rb = nextline(0, buffer, BUFSIZ)) > 0; ++line) {
		write(1, buffer, rb);
	}
}
