// Random patch to fix Android studio linking bug (???)
#include <stddef.h>
extern size_t strlen (const char *);
extern void *memcpy (void *, const void *, size_t);
char *stpcpy (char *dst, const char *src) {
  const size_t len = strlen (src);
  return (char *) memcpy (dst, src, len + 1) + len;
}
