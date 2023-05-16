#include <fileio.h>
#include <init.h>
#include <input.h>
#include <output.h>
#include <terminal.h>

int main(int argc, char **argv) {
  enableRawMode();
  initEditor();
  initColors();

  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage(HELP_MESSAGE);

  while (true) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

