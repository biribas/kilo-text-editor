#include <fileio.h>
#include <init.h>
#include <input.h>
#include <output.h>
#include <terminal.h>

int main(int argc, char **argv) {
  enableRawMode();
  initEditor();
  initColors();

  editorSetStatusMessage(HELP_MESSAGE);
  editorOpen(argc >= 2 ? argv[1] : NULL);

  while (true) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

