#include <buffer.h>

void appendBuffer(buffer *buff, const char *string, int length) {
  char *new = realloc(buff->content, buff->length + length);

  if (new == NULL)
    return;

  memcpy(&new[buff->length], string, length);
  buff->content = new;
  buff->length += length;
}

void freeBuffer(buffer *buff) {
  free(buff->content);
}

