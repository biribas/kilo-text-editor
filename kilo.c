#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)

// Terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char *str);
char editorReadKey(void);
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);
// Output
void editorDrawRows(void);
void editorRefreshScreen(void);
// Input
void editorProcessKeypress(void);
//Init
void initEditor(void);

struct editorConfig {
  int rows;
  int cols;
  struct termios original_termios;
};

struct editorConfig editor;

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
  if (getWindowSize(&editor.rows, &editor.cols) == -1)
    die("getWindowSize");
}

/*** Terminal ***/

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &editor.original_termios) == -1)
    die("tcgetattr");
  
  atexit(disableRawMode);

  struct termios raw = editor.original_termios;
  
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
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.original_termios) == -1) 
    die("tcsetattr");
}

void die(const char *str) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(str);
  exit(1);
}

char editorReadKey(void) {
  int n;
  char c;
  while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
    if (n == -1 && errno != EAGAIN)
      die("read");
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

void editorDrawRows(void) {
  for (int i = 0; i < editor.rows - 1; i++)
    write(STDOUT_FILENO, "~\r\n", 3);

  write(STDOUT_FILENO, "~", 1);
}

void editorRefreshScreen(void) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** Input ***/

void editorProcessKeypress(void) {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }
}

