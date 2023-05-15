#include <fileio.h>
#include <init.h>
#include <input.h>
#include <output.h>
#include <terminal.h>

int main(int argc, char **argv) {
  enableRawMode();
  initEditor();
  initColors();
  initHighlightDataBase();

  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

  while (true) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

