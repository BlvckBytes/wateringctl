#include "util/strfmt.h"

bool vstrfmt(char **buf, size_t *offs, const char *fmt, va_list ap)
{
  va_list ap2;
  va_copy(ap2, ap);

  // Calculate available remaining buffer size
  mman_meta_t *buf_meta = mman_fetch_meta(*buf);
  size_t offs_v = offs ? *offs : 0;
  size_t buf_len = buf_meta->num_blocks;
  size_t buf_avail = buf_len - (offs_v + 1);

  // Calculate how many chars are needed
  size_t needed = vsnprintf(NULL, 0, fmt, ap) + 1;

  // Buffer too small
  if (buf_avail < needed)
  {
    size_t diff = needed - buf_avail;

    // Extend by the difference
    if (!mman_realloc((void **) buf, sizeof(char), buf_len + diff)) return false;
    buf_avail += diff;
  }

  // Write into buffer and update outside offset, if applicable
  int written = vsnprintf(&((*buf)[offs_v]), buf_avail, fmt, ap2);
  if (offs) *offs += written;
  va_end(ap2);
  return true;

  va_end(ap2);
}

bool strfmt(char **buf, size_t *offs, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  bool res = vstrfmt(buf, offs, fmt, ap);

  va_end(ap);
  return res;
}

char *strfmt_direct(const char *fmt, ...)
{
  scptr char *buf = (char *) mman_alloc(sizeof(char), 128, NULL);

  va_list ap;
  va_start(ap, fmt);

  size_t offs = 0;
  bool res = vstrfmt(&buf, &offs, fmt, ap);

  va_end(ap);

  if (!res) return NULL;
  return (char *) mman_ref(buf);
}
