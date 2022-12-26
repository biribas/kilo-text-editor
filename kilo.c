#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define BUFFER_INIT {NULL, 0}
#define KILO_VERSION "0.0.1"

enum editorKeys {
  ARROW_UP = 1000,
  ARROW_DOWN,
  ARROW_LEFT,
  ARROW_RIGHT,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

struct {
  int cursorX, cursorY;
  int rows, cols;
  struct termios original_state;
} editorConfig;

typedef struct {
  char *content; 
  int length;
} buffer;

// Terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char *str);
int editorReadKey(void);
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);
// Output
void editorDrawRows(buffer *buff);
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

int main(void) {
  enableRawMode();
  initEditor();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

void initEditor(void) {
  editorConfig.cursorX = 0;
  editorConfig.cursorY = 0;

  if (getWindowSize(&editorConfig.rows, &editorConfig.cols) == -1)
    die("getWindowSize");
}

/*** Terminal ***/

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &editorConfig.original_state) == -1)
    die("tcgetattr");
  
  atexit(disableRawMode);

  struct termios raw = editorConfig.original_state;
  
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
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editorConfig.original_state) == -1) 
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

void editorDrawRows(buffer *buff) {
  for (int i = 0; i < editorConfig.rows; i++) {
    if (i == editorConfig.rows / 3) {
      char welcome[80];
      int length = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);

      if (length > editorConfig.cols)
        length = editorConfig.cols;

      int padding = (editorConfig.cols - length) / 2;

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

    appendBuffer(buff, "\x1b[K", 3);

    if (i < editorConfig.rows - 1)
      appendBuffer(buff, "\r\n", 2);
  }
}

void editorRefreshScreen(void) {
  buffer buff = BUFFER_INIT;

  appendBuffer(&buff, "\x1b[?25l", 6);
  appendBuffer(&buff, "\x1b[H", 3);

  editorDrawRows(&buff);

  char temp[32];
  snprintf(temp, sizeof(temp), "\x1b[%d;%dH", editorConfig.cursorY + 1, editorConfig.cursorX + 1);
  appendBuffer(&buff, temp, strlen(temp));

  appendBuffer(&buff, "\x1b[?25h", 6);

  write(STDOUT_FILENO, buff.content, buff.length);
  freeBuffer(&buff);
}

/*** Input ***/

void editorMoveCursor(int key) {
  switch (key) {
    case ARROW_UP:
      if (editorConfig.cursorY != 0)
        editorConfig.cursorY--;
      break;
    case ARROW_DOWN:
      if (editorConfig.cursorY != editorConfig.rows - 1)
        editorConfig.cursorY++;
      break;
    case ARROW_LEFT:
      if (editorConfig.cursorX != 0)
        editorConfig.cursorX--;
      break;
    case ARROW_RIGHT:
      if (editorConfig.cursorX != editorConfig.cols - 1)
        editorConfig.cursorX++;
      break;
  }
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
      editorConfig.cursorX = 0;
      break;

    case END_KEY:
      editorConfig.cursorX = editorConfig.cols - 1;
      break;

    case PAGE_UP:
    case PAGE_DOWN: {
      int times = editorConfig.rows;
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

