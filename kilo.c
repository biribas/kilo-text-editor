#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define BUFFER_INIT {NULL, 0}
#define KILO_VERSION "0.0.1"
#define TAB_SIZE 8

enum editorKeys {
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

typedef struct {
  char *content, *renderContent;
  int length, renderLength;
} editorLine;

typedef struct {
  char *content;
  int length;
} buffer;

struct editorConfig {
  int cursorX, cursorY;
  int rCursorX;
  int rows, cols;
  int rowOffset, colOffset;
  int numlines;
  editorLine *lines;
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
void editorOpen(char *filename);
// Line operations
int editorConvertCursorX(editorLine *line, int cursorX);
void editorUpdateLine(editorLine *);
void editorAppendLine(char *line, size_t length);
// Output
void editorScroll(void);
void editorDrawLines(buffer *buff);
void editorRefreshScreen(void);
// Input
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

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

void initEditor(void) {
  E.cursorX = 0;
  E.cursorY = 0;
  E.rCursorX = 0;
  E.rowOffset = 0;
  E.colOffset = 0;
  E.numlines = 0;
  E.lines = NULL;

  if (getWindowSize(&E.rows, &E.cols) == -1)
    die("getWindowSize");
}

/*** File i/o ***/

void editorOpen(char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    die("fopen");

  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;

  while ((length = getline(&line, &capacity, file)) != -1) {
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
      length--;
    editorAppendLine(line, length);
  }

  free(line);
  fclose(file);
}

/*** Line operations ***/

int editorConvertCursorX(editorLine *line, int cursorX) {
  int rx = 0;
  for (int i = 0; i < cursorX; i++) {
    if (line->content[i] == '\t')
      rx += TAB_SIZE - (rx % TAB_SIZE);
    else
      rx++;
  }
  return rx;
}

void editorUpdateLine(editorLine *line) {
  int tabs = 0;
  for (int i = 0; i < line->length; i++) {
    if (line->content[i] == '\t')
      tabs++;
  }
  
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
}

void editorAppendLine(char *line, size_t length) {
  E.lines = realloc(E.lines, sizeof(editorLine) * (E.numlines + 1));
  int at = E.numlines;

  E.lines[at].length = length;
  E.lines[at].content = malloc(length + 1);
  memcpy(E.lines[at].content, line, length);
  E.lines[at].content[length] = '\0';

  E.lines[at].renderContent = NULL;
  E.lines[at].renderLength = 0;
  editorUpdateLine(&E.lines[at]);

  E.numlines++;
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
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

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

void editorScroll(void) {
  if (E.cursorY < E.rowOffset) {
    E.rowOffset = E.cursorY;
  }
  else if (E.cursorY >= E.rowOffset + E.rows) {
    E.rowOffset = E.cursorY - E.rows + 1;
  }

  if (E.rCursorX < E.colOffset) {
    E.colOffset = E.rCursorX;
  }
  else if (E.rCursorX >= E.colOffset + E.cols) {
    E.colOffset = E.rCursorX - E.cols + 1;
  }
}

void editorDrawLines(buffer *buff) {
  for (int i = 0; i < E.rows; i++) {
    int filerow = i + E.rowOffset;
    if (filerow >= E.numlines) {
      if (E.numlines == 0 && i == E.rows / 3) {
        char welcome[80];
        int length = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);

        if (length > E.cols)
          length = E.cols;

        int padding = (E.cols - length) / 2;

        if (padding != 0) {
          appendBuffer(buff, "~", 1);
          padding--;
        }

        while (padding--)
          appendBuffer(buff, " ", 1);

        appendBuffer(buff, welcome, length);
      }
      else { 
        appendBuffer(buff, "~", 1);
      }
    }
    else {
      int length = E.lines[filerow].renderLength - E.colOffset;
      if (length < 0)
        length = 0;
      if (length > E.cols)
        length = E.cols;

      appendBuffer(buff, &E.lines[filerow].renderContent[E.colOffset], length);
    }

    appendBuffer(buff, "\x1b[K", 3);

    if (i < E.rows - 1)
      appendBuffer(buff, "\r\n", 2);
  }
}

void editorRefreshScreen(void) {
  E.rCursorX = E.cursorY < E.numlines ? editorConvertCursorX(&E.lines[E.cursorY], E.cursorX) : 0;

  editorScroll();

  buffer buff = BUFFER_INIT;

  appendBuffer(&buff, "\x1b[?25l", 6);
  appendBuffer(&buff, "\x1b[H", 3);

  editorDrawLines(&buff);

  char temp[32];
  snprintf(temp, sizeof(temp), "\x1b[%d;%dH", (E.cursorY - E.rowOffset) + 1, (E.rCursorX - E.colOffset) + 1);
  appendBuffer(&buff, temp, strlen(temp));

  appendBuffer(&buff, "\x1b[?25h", 6);

  write(STDOUT_FILENO, buff.content, buff.length);
  freeBuffer(&buff);
}

/*** Input ***/

void editorMoveCursor(int key) {
  editorLine *currentLine = (E.cursorY >= E.numlines) ? NULL : &E.lines[E.cursorY];

  switch (key) {
    case ARROW_UP:
      if (E.cursorY != 0)
        E.cursorY--;
      break;
    case ARROW_DOWN:
      if (E.cursorY < E.numlines)
        E.cursorY++;
      break;
    case ARROW_LEFT:
      if (E.cursorX != 0) {
        E.cursorX--;
      }
      else if (E.cursorY > 0) {
        E.cursorY--;
        E.cursorX = E.lines[E.cursorY].length;
      }
      break;
    case ARROW_RIGHT:
      if (!currentLine) break;

      if (E.cursorX < currentLine->length) {
        E.cursorX++;
      }
      else {
        E.cursorY++;
        E.cursorX = 0;
      }
      break;
  }

  currentLine = (E.cursorY >= E.numlines) ? NULL : &E.lines[E.cursorY];
  int length = currentLine ? currentLine->length : 0;
  
  if (E.cursorX > length)
    E.cursorX = length;
}

void editorProcessKeypress(void) {
  int c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case HOME_KEY:
      E.cursorX = 0;
      break;

    case END_KEY:
      if (E.cursorY < E.numlines)
        E.cursorX = E.lines[E.cursorY].length;
      break;

    case PAGE_UP:
    case PAGE_DOWN: {
      if (c == PAGE_UP) {
        E.cursorY = E.rowOffset;
      }
      else {
        E.cursorY = E.rowOffset + E.rows - 1;  
        if (E.cursorY > E.numlines)
          E.cursorY = E.numlines;
      } 

      int times = E.rows;
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

