#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define MIN(a, b) (a < b ? a : b)
#define CLAMP(min, value, max) (value < min ? min : value > max ? max : value)
#define MOD(a, b) ((a % b + b) % b)
#define CTRL_KEY(k) ((k) & 0x1f)
#define BUFFER_INIT {NULL, 0}
#define KILO_VERSION "0.0.1"
#define TAB_SIZE 2
#define QUIT_TIMES 2

enum editorKeys {
  BACKSPACE = 127,
  ARROW_UP = 1000,
  ARROW_DOWN,
  ARROW_LEFT,
  ARROW_RIGHT,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

enum editorHighlight {
  HL_NORMAL = 0,
  HL_MATCH, 
  HL_CURRENT_MATCH,
  HL_NUMBER
};

typedef struct {
  char *content, *renderContent;
  int length, renderLength;
  unsigned char *highlight;
} editorLine;

typedef struct {
  char *content;
  int length;
} buffer;

struct editorConfig {
  int cursorX, cursorY;
  int savedLastY;
  int savedLastX;
  int highestLastX;
  int rCursorX;
  int screenRows, screenCols;
  int rowOffset, colOffset;
  int numlines;
  int dirty;
  editorLine *lines;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct termios original_state;
} E;

// Terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char *str);
int editorReadKey(void);
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);
// File i/o
char *editorLinesToString(int *buflen);
void editorOpen(char *filename);
void editorSave(void);
// Find
char *findLastOccurrence(char *lasMatch, char *query, char *lineContent);
int editorFindCallback(char *query, int key);
void editorFind(void);
// Syntax highlighting
bool isSeparator(int);
void editorUpdateHighlight(editorLine *line);
int editorSyntaxToColor(int);
// Line operations
int editorLineCxToRx(editorLine *line, int cursorX);
int editorLineRxToCx(editorLine *line, int rCursorX);
void editorUpdateLine(editorLine *);
void editorInsertLine(int at, char *line, size_t length);
void editorFreeLine(editorLine *line);
void editorDeleteLine(int at);
void editorLineInsertChar(editorLine *line, int at, int c);
void editorLineAppendString(editorLine *line, char *string, size_t len);
void editorLineDeleteChar(editorLine *line, int at);
void freeMemory(void);
// Editor operations
void editorInsertChar(int c);
void editorDeleteChar(void);
void editorInsertNewLine(void);
// Output
void editorScrollX(void);
void editorScrollY(void);
void editorDrawLines(buffer *);
void editorDrawStatusBar(buffer *);
void editorDrawMessageBar(buffer *);
void editorRefreshScreen(void);
void editorSetStatusMessage(const char *format, ...);
// Input
char *editorPrompt(char *, int (*callback)(char *, int));
void editorMoveCursor(int);
void editorProcessKeypress(void);
// Init
void initEditor(void);
// Buffer
void appendBuffer(buffer *, const char *string, int length);
void freeBuffer(buffer *);

/*** Init ***/

int main(int argc, char **argv) {
  enableRawMode();
  initEditor();

  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

void initEditor(void) {
  atexit(freeMemory);

  E.cursorX = 0;
  E.cursorY = 0;
  E.highestLastX = 0;
  E.rCursorX = 0;
  E.rowOffset = 0;
  E.colOffset = 0;
  E.numlines = 0;
  E.lines = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;

  if (getWindowSize(&E.screenRows, &E.screenCols) == -1)
    die("getWindowSize");

  E.screenRows -= 2;
}

/*** File i/o ***/

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
  free(E.filename);
  E.filename = strdup(filename);

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
        E.dirty = 0;
        return;
      }
    }
    close(fd);
  }

  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
  free(buf);
}

/*** Find ***/

char *findLastOccurrence(char *lasMatch, char *query, char *lineContent) {
  char *prev = NULL;
  char *cur = lineContent;

  while (1) {
    cur = strstr(cur, query);
    if (cur == lasMatch) break;
    prev = cur;
    cur++;
  }
  return prev;
}

void clearSearchHighlight(void) {  
  int cur = E.rowOffset;
  int last = MIN(cur + E.screenRows, E.numlines);
  while (cur < last) {
    editorUpdateHighlight(&E.lines[cur]);
    cur++; 
  }
}

int editorFindCallback(char *query, int key) {
  static int current;
  static int direction;
  static char *match = NULL;

  clearSearchHighlight();

  if (*query == '\0') {
    return 0;
  }
  else if (key == '\r' || key == '\x1b') {
    return -1;
  }

  if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1; 
  }
  else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1; 
  }
  else {
    current = E.savedLastY;
    direction = 1;
    match = E.lines[current].renderContent + E.savedLastX;
  }

  int found = 0;
  int lastMatchIndex;
  char *lastMatch;

  int limit = current;
  while (1) {
    editorLine *line = &E.lines[current];

    if (match) {
      match = direction == -1
        ? findLastOccurrence(match, query, line->renderContent)
        : strstr(match + 1, query);
    }

    if (!match) {
      int nextLine = MOD(current + direction, E.numlines);
      if (nextLine == limit) break;
      if (current + direction == limit) break;

      current = nextLine;
      line = &E.lines[current];

      match = direction == -1
        ? findLastOccurrence(match, query, line->renderContent)
        : strstr(line->renderContent, query);
    }

    if (match) {
      if (!found) {
        E.cursorY = current;
        E.cursorX = editorLineRxToCx(line, match - line->renderContent);

        E.rowOffset = E.screenRows <= E.cursorY
          ? E.cursorY - E.screenRows / 2
          : E.numlines;

        editorScrollY();

        found = 1;

        lastMatchIndex = current;
        lastMatch = match;
        direction = 1;

        current = E.rowOffset;
        limit = MIN(E.rowOffset + E.screenRows, E.numlines);

        match = E.lines[current].renderContent - 1;
      }
      else {
        memset(&line->highlight[match - line->renderContent], HL_MATCH, strlen(query));
      }
    }
  }

  if (found) {
    current = lastMatchIndex;
    match = lastMatch;

    editorLine *line = &E.lines[current];
    memset(&line->highlight[match - line->renderContent], HL_CURRENT_MATCH, strlen(query));
  }

  return found;
}

void editorFind(void) {
  char *query = editorPrompt("Search: %s", editorFindCallback);
  free(query);
}

/*** Syntax highlighting ***/

bool isSeparator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateHighlight(editorLine *line) {
  line->highlight = realloc(line->highlight, line->renderLength);
  memset(line->highlight, HL_NORMAL, line->renderLength);

  bool isPrevSep = true;

  int i = 0;
  while (i < line->renderLength) {
    char c = line->renderContent[i];
    unsigned char prevHL = (i > 0) ? line->highlight[i - 1] : HL_NORMAL;

    bool isInt = isdigit(c) && (isPrevSep || prevHL == HL_NUMBER);
    bool isFloat = c == '.' && prevHL == HL_NUMBER;

    if (isInt || isFloat) {
      line->highlight[i] = HL_NUMBER;
      isPrevSep = false;
    }

    isPrevSep = isSeparator(c);
    i++;
  }
}

int editorSyntaxToColor(int highlight) {
  switch (highlight) {
    case HL_MATCH: return 41;
    case HL_CURRENT_MATCH: return 7;
    case HL_NUMBER: return 32;
    default: return 39; 
  }
}

/*** Line operations ***/

int editorLineCxToRx(editorLine *line, int cursorX) {
  int rx = 0;
  for (int i = 0; i < cursorX; i++) {
    if (line->content[i] == '\t')
      rx += TAB_SIZE - (rx % TAB_SIZE);
    else
      rx++;
  }
  return rx;
}

int editorLineRxToCx(editorLine *line, int rCursorX) {
  int rx = 0;
  int cx;

  for (cx = 0; cx < line->length; cx++) {
    if (line->content[cx] == '\t')
      rx += TAB_SIZE - (rx % TAB_SIZE);
    else 
      rx++;

    if (rx > rCursorX)
      return cx;
  }

  return cx;
}

void editorUpdateLine(editorLine *line) {
  int tabs = 0;
  for (int i = 0; i < line->length; i++) {
    if (line->content[i] == '\t')
      tabs++;
  }
  
  free(line->renderContent);
  line->renderContent = malloc(line->length + (TAB_SIZE - 1) * tabs + 1);

  int index = 0;
  for (int i = 0; i < line->length; i++) {
    if (line->content[i] == '\t') {
      line->renderContent[index++] = ' ';
      while (index % TAB_SIZE != 0)
        line->renderContent[index++] = ' ';
    }
    else {
      line->renderContent[index++] = line->content[i];
    }
  }

  line->renderContent[index] = '\0';
  line->renderLength = index;

  editorUpdateHighlight(line);
}

void editorInsertLine(int at, char *line, size_t length) {
  if (at < 0 || at > E.numlines)
    return;

  E.lines = realloc(E.lines, sizeof(editorLine) * (E.numlines + 1));
  memmove(&E.lines[at + 1], &E.lines[at], sizeof(editorLine) * (E.numlines - at));

  E.lines[at].length = length;
  E.lines[at].content = malloc(length + 1);
  memcpy(E.lines[at].content, line, length);
  E.lines[at].content[length] = '\0';

  E.lines[at].highlight = NULL;
  E.lines[at].renderContent = NULL;
  E.lines[at].renderLength = 0;
  editorUpdateLine(&E.lines[at]);

  E.numlines++;
}

void editorFreeLine(editorLine *line) {
  free(line->content);
  free(line->renderContent);
  free(line->highlight);
}

void editorDeleteLine(int at) {
  if (at < 0 || at >= E.numlines)
    return;

  editorFreeLine(&E.lines[at]);
  memmove(&E.lines[at], &E.lines[at + 1], sizeof(editorLine) * (E.numlines - at - 1));
  E.numlines--;
}

void editorLineInsertChar(editorLine *line, int at, int c) {
  if (at < 0 || at > line->length) 
    at = line->length;

  line->content = realloc(line->content, line->length + 2);
  memmove(&line->content[at + 1], &line->content[at], line->length - at + 1);
  line->length++;
  line->content[at] = c;
  editorUpdateLine(line);
}

void editorLineAppendString(editorLine *line, char *s, size_t len) {
  line->content = realloc(line->content, line->length + len + 1);
  memcpy(&line->content[line->length], s, len);
  line->length += len;
  line->content[line->length] = '\0';
  editorUpdateLine(line);
}

void editorLineDeleteChar(editorLine *line, int at) {
  if (at < 0 || at > line->length)
    return;

  memmove(&line->content[at], &line->content[at + 1], line->length - at);
  line->length--; 
  editorUpdateLine(line);
}

void freeMemory(void) {
  free(E.filename);
  for (int i = 0; i < E.numlines; i++) {
    editorFreeLine(&E.lines[i]);
  }
  free(E.lines);
}

/** Editor operations **/
void editorInsertChar(int c) {
  if (E.cursorY == E.numlines)
    editorInsertLine(E.numlines, "", 0);

  editorLineInsertChar(&E.lines[E.cursorY], E.cursorX, c);

  E.cursorX++;
  E.highestLastX = E.cursorX;
  E.dirty++;
}

void editorDeleteChar(void) {
  if (E.cursorY == E.numlines) return;
  if (E.cursorX == 0 && E.cursorY == 0) return;

  editorLine *line = &E.lines[E.cursorY];
  if (E.cursorX > 0) {
    editorLineDeleteChar(line, --E.cursorX);
  }
  else {
    E.cursorX = E.lines[E.cursorY - 1].length;
    editorLineAppendString(&E.lines[E.cursorY - 1], line->content, line->length);
    editorDeleteLine(E.cursorY);
    E.cursorY--;
  }

  E.highestLastX = E.cursorX;
  E.dirty++;
}

void editorInsertNewLine(void) {
  if (E.cursorX == 0) {
    editorInsertLine(E.cursorY, "", 0);
  }
  else {
    editorLine *line = &E.lines[E.cursorY];
    editorInsertLine(E.cursorY + 1, &line->content[E.cursorX], line->length - E.cursorX);
 
    line = &E.lines[E.cursorY];
    line->length = E.cursorX; 
    line->content = realloc(line->content, line->length + 1);
    line->content[line->length] = '\0';

    editorUpdateLine(line);
    E.cursorX = 0;
  }

  E.highestLastX = E.cursorX;
  E.cursorY++;
  E.dirty++;
}

/*** Terminal ***/

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &E.original_state) == -1)
    die("tcgetattr");
  
  atexit(disableRawMode);

  struct termios raw = E.original_state;
  
  raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_state) == -1) 
    die("tcsetattr");
}

void die(const char *str) {
  write(STDOUT_FILENO, "\x1b[2J", 4); // Erase entire screen
  write(STDOUT_FILENO, "\x1b[H", 3);  // Moves cursor to home position (0, 0)

  perror(str);
  exit(1);
}

int editorReadKey(void) {
  int n;
  char c;
  while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
    if (n == -1 && errno != EAGAIN)
      die("read");
  }

  if (c == '\x1b') {
    char sequence[3];

    if (read(STDIN_FILENO, &sequence[0], 1) != 1) return c;
    if (read(STDIN_FILENO, &sequence[1], 1) != 1) return c;

    if (sequence[0] == '[') {
      if (sequence[1] >= '0' && sequence[1] <= '9') {
        if (read(STDIN_FILENO, &sequence[2], 1) != 1) return c;

        if (sequence[2] == '~') {
          switch (sequence[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      }
      else {
        switch (sequence[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        } 
      }
    }
    else if (sequence[0] == 'O') {
      switch (sequence[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }
  }

  return c;
}

int getCursorPosition(int *rows, int *cols) {
  // Request cursor position (reports as "\x1b[#;#R")
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) 
    return -1;

  char buffer[32];
  int i = 0;
 
  for (int n = sizeof(buffer) - 1; i < n; i++) {
    if (read(STDIN_FILENO, &buffer[i], 1) == -1) break;
    if (buffer[i] == 'R') break;
  }

  buffer[i] = '\0';

  if (buffer[0] != '\x1b' || buffer[1] != '[')
    return -1;

  if (sscanf(&buffer[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    // Moves cursor 999 spaces right and down, respectively
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) == -1)
      return -1;

    return getCursorPosition(rows, cols);
  }
  else {
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
  }
}

/*** Output ***/

void editorScrollX(void) {
  const int colOffsetGap = E.screenCols * 0.20;

  int gap = MIN(colOffsetGap, E.rCursorX);

  int min = E.rCursorX - E.screenCols + gap + 1;
  int max = E.rCursorX - gap;

  E.colOffset = CLAMP(min, E.colOffset, max);
}

void editorScrollY(void) {
  int gap;

  const int rowOffsetGap = E.screenRows * 0.25;

  if (E.cursorY < rowOffsetGap) {
    gap = E.cursorY;
  }
  else if (E.cursorY > E.numlines - rowOffsetGap) {
    gap = E.numlines - E.cursorY;
  }
  else {
    gap = rowOffsetGap;
  }

  int min = E.cursorY - E.screenRows + 1 + gap;
  int max = E.cursorY - gap;

  E.rowOffset = CLAMP(min, E.rowOffset, max);
}

void editorDrawLines(buffer *buff) {
  for (int i = 0; i < E.screenRows; i++) {
    int filerow = i + E.rowOffset;
    if (filerow >= E.numlines) {
      if (E.numlines == 0 && i == E.screenRows / 3) {
        char welcome[80];
        int length = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);

        length = MIN(length, E.screenCols);

        int padding = (E.screenCols - length) / 2;

        if (padding != 0) {
          appendBuffer(buff, "~", 1);
          padding--;
        }

        while (padding--)
          appendBuffer(buff, " ", 1);

        appendBuffer(buff, welcome, length);
      }
      else if (filerow != 0) { 
        appendBuffer(buff, "~", 1);
      }
    }
    else {
      int length = E.lines[filerow].renderLength - E.colOffset;
      length = CLAMP(0, length, E.screenCols);

      char *content = &E.lines[filerow].renderContent[E.colOffset];
      unsigned char *highlight = &E.lines[filerow].highlight[E.colOffset];
      
      int currentColor = -1;
      for (int j = 0; j < length; j++) {
        if (highlight[j] == HL_NORMAL) {
          if (currentColor != -1) {
            currentColor = -1;
            appendBuffer(buff, "\x1b[m", 4);
            appendBuffer(buff, "\x1b[39m", 5);
            appendBuffer(buff, "\x1b[49m", 5);
          }
        }
        else {
          int color = editorSyntaxToColor(highlight[j]);
          if (color != currentColor) {
            currentColor = color;
            char ansi[16];
            int len = snprintf(ansi, sizeof(ansi), "\x1b[%dm", color);
            appendBuffer(buff, ansi, len);
          }
        }
        appendBuffer(buff, &content[j], 1);
      }
      appendBuffer(buff, "\x1b[39m", 5);
    }

    appendBuffer(buff, "\x1b[K", 3); // Erase from cursor to end of line
    appendBuffer(buff, "\r\n", 2);
  }
}

void editorDrawStatusBar(buffer *buff) {
  appendBuffer(buff, "\x1b[7m", 4); // Inverted colors

  char info[80], position[80];

  const char *filename = E.filename ? E.filename : "[No name]";
  const char *modified = E.dirty ? "(modified)" : "";
  int infoLen = snprintf(info, sizeof(info), " %.20s %s", filename, modified);

  char percent[80];
  if (E.numlines == 0 || E.cursorY == 0) {
    sprintf(percent, "Top");
  }
  else if (E.cursorY >= E.numlines - 1) {
    sprintf(percent, "Bot");
  }
  else {
    sprintf(percent, "%d%%", 100 * (E.cursorY + 1) / E.numlines);
  }

  int posLen = snprintf(position, sizeof(position), " %d:%d   %s ", E.cursorY + 1, E.rCursorX + 1, percent);

  infoLen = MIN(infoLen, E.screenCols);

  appendBuffer(buff, info, infoLen);

  for (; infoLen < E.screenCols; infoLen++) {
    if (E.screenCols - infoLen == posLen) {
      appendBuffer(buff, position, posLen);
      break;
    }
    appendBuffer(buff, " ", 1);
  }

  appendBuffer(buff, "\x1b[m", 3); // Reset normal color
  appendBuffer(buff, "\r\n", 2);
}

void editorDrawMessageBar(buffer *buff) {
  appendBuffer(buff, "\x1b[K", 3); // Erase from cursor to end of line
  int len = strlen(E.statusmsg);
  len = MIN(len, E.screenCols);

  if (len && time(NULL) - E.statusmsg_time < 5)
    appendBuffer(buff, E.statusmsg, len);
}

void editorRefreshScreen(void) {
  E.rCursorX = E.cursorY < E.numlines ? editorLineCxToRx(&E.lines[E.cursorY], E.cursorX) : 0;

  editorScrollX();
  editorScrollY();

  buffer buff = BUFFER_INIT;

  appendBuffer(&buff, "\x1b[?25l", 6); // Make cursor invisible
  appendBuffer(&buff, "\x1b[H", 3); // Moves cursor to home position (0, 0)

  editorDrawLines(&buff);
  editorDrawStatusBar(&buff);
  editorDrawMessageBar(&buff);

  char temp[32];
  snprintf(temp, sizeof(temp), "\x1b[%d;%dH", (E.cursorY - E.rowOffset) + 1, (E.rCursorX - E.colOffset) + 1);
  appendBuffer(&buff, temp, strlen(temp));

  appendBuffer(&buff, "\x1b[?25h", 6); // Make cursor visible

  write(STDOUT_FILENO, buff.content, buff.length);
  freeBuffer(&buff);
}

void editorSetStatusMessage(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), format, args);
  va_end(args);
  E.statusmsg_time = time(NULL);
}

/*** Input ***/

char *editorPrompt(char *prompt, int (*callback)(char *, int)) {
  // Used only in search
  E.savedLastX = E.cursorX;
  E.savedLastY = E.cursorY;
  int saved_rowOff = E.rowOffset;
  int saved_colOff = E.colOffset;

  size_t bufsize = 128;
  char *buf = malloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';

  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();

    int c = editorReadKey();

    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen == 0) continue;
      buf[--buflen] = '\0';
    }
    else if (c == '\x1b') {
      if (callback) {
        callback(buf, c);
        E.cursorX = E.savedLastX;
        E.cursorY = E.savedLastY;
        E.rowOffset = saved_rowOff;
        E.colOffset = saved_colOff;
      }

      editorSetStatusMessage("");
      free(buf);
      return NULL;
    }
    else if (c == '\r') {
      if (buflen == 0) continue;
      if (callback) callback(buf, c);
      editorSetStatusMessage("");
      return buf;
    }
    else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback && !callback(buf, c)) {
      E.cursorX = E.savedLastX;
      E.cursorY = E.savedLastY;
      E.rowOffset = saved_rowOff;
      E.colOffset = saved_colOff;
    }
  }
}

void editorMoveCursor(int key) {
  editorLine *currentLine = &E.lines[E.cursorY];

  switch (key) {
    case ARROW_UP:
      if (E.cursorY != 0) {
        E.cursorY--;
        E.cursorX = E.highestLastX;
      }
      break;

    case ARROW_DOWN:
      if (E.cursorY + 1 < E.numlines) {
        E.cursorY++;
        E.cursorX = E.highestLastX;
      }
      break;

    case ARROW_LEFT:
      if (E.cursorX != 0) {
        E.cursorX--;
      }
      else if (E.cursorY > 0) {
        E.cursorY--;
        E.cursorX = E.lines[E.cursorY].length;
      }
      E.highestLastX = E.cursorX;
      break;

    case ARROW_RIGHT:
      if (E.cursorX < currentLine->length) {
        E.cursorX++;
      }
      else if (E.cursorY + 1 < E.numlines) {
        E.cursorY++;
        E.cursorX = 0;
      }
      E.highestLastX = E.cursorX;
      break;
  }

  currentLine = &E.lines[E.cursorY];
  
  if (E.cursorX > currentLine->length)
    E.cursorX = currentLine->length;
}

void editorProcessKeypress(void) {
  static int quit_times = 2;

  int c = editorReadKey();

  switch (c) {
    case '\r':
      editorInsertNewLine();
      break;
    
    case CTRL_KEY('q'):
      quit_times--;
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("File has unsaved changes. Press Ctrl-Q again to quit");
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4); // Erase entire screen
      write(STDOUT_FILENO, "\x1b[H", 3);  // Moves cursor to home position (0, 0)
      exit(0);
      break;

    case CTRL_KEY('s'):
      editorSave();
      break;

    case CTRL_KEY('f'):
      editorFind();
      break;

    case HOME_KEY:
      E.cursorX = 0;
      break;

    case END_KEY:
      if (E.cursorY < E.numlines)
        E.cursorX = E.lines[E.cursorY].length;
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      editorDeleteChar();
      break;

    case PAGE_UP:
    case PAGE_DOWN: {
      if (c == PAGE_UP) {
        E.cursorY = E.rowOffset;
      }
      else {
        E.cursorY = MIN(E.rowOffset + E.screenRows, E.numlines) - 1;
      }

      int times = E.screenRows;
      int direction = c == PAGE_UP ? ARROW_UP : ARROW_DOWN;
      while (times--)
        editorMoveCursor(direction);
      break;
    }

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;
    
    default:
      editorInsertChar(c);
      break;
  }

  if (quit_times < QUIT_TIMES) {
    editorSetStatusMessage("");
    quit_times = QUIT_TIMES;
  }
}

/*** Buffer ***/

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

