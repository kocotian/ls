/* See licenses/LIBSL file for copyright and license details. */
/* Extended by kocotian */

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))

void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);
ssize_t nextline(int fd, char *buf, size_t size);
void errwarn(const char *fmt, int iserror, ...);
