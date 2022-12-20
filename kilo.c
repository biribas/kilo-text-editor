#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

// Terminal
void enableRawMode(void);
void disableRawMode(void);
void die(const char*);

struct termios orig_termios;

int main(void) {
  enableRawMode();

  while (1) {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) 
      die("read");

    if (iscntrl(c))
      printf("%d\r\n", c);
    else
      printf("%d ('%c')\r\n", c, c);

    if (c == 'q') break;
  }
  return 0;
}

void die(const char *str) {
  perror(str);
  exit(1);
}

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    die("tcgetattr");
  
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  
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
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) 
    die("tcsetattr");
}
