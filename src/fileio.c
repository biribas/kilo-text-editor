#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <fileio.h>
#include <highlight.h>
#include <input.h>
#include <lines.h>
#include <output.h>
#include <terminal.h>

char *editorLinesToString(int *buflen) {
  int total_len = 0;
  for (int i = 0; i < E.numlines; i++)
    total_len += E.lines[i].length + 1;
  *buflen = total_len; 

  char *buf = malloc(total_len);
  char *p = buf;
  for (int i = 0; i < E.numlines; i++) {
    memcpy(p, E.lines[i].content, E.lines[i].length);
    p += E.lines[i].length;
    *p = '\n';
    p++;
  }

  return buf;
}

void editorOpen(char *filename) {
  E.filename = strdup(filename);

  editorSelectSyntaxHighlight();

  FILE *file = fopen(filename, "r");
  if (file == NULL)
    die("fopen");

  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;

  while ((length = getline(&line, &capacity, file)) != -1) {
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
      length--;
    editorInsertLine(E.numlines, line, length);
  }

  free(line);
  fclose(file);
}

void editorSave(void) {
  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as: %s", NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
    editorSelectSyntaxHighlight();
  }

  int len;
  char *buf = editorLinesToString(&len);

  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);

  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        editorSetStatusMessage("%d bytes written to disk", len);
        E.dirty = false;
        return;
      }
    }
    close(fd);
  }

  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
  free(buf);
}

