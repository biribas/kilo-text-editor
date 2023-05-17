#include <buffer.h>
#include <highlight.h>
#include <lines.h>
#include <output.h>
#include <tools.h>

void editorScrollX(void) {
  const int colOffsetGap = E.screenCols * 0.20;

  int gap = min(colOffsetGap, E.rCursorX);

  int min = E.rCursorX - E.screenCols + gap + 1;
  int max = E.rCursorX - gap;

  E.colOffset = clamp(min, E.colOffset, max);
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

  E.rowOffset = clamp(min, E.rowOffset, max);
}

void editorHighlightOutput(buffer *buff, color_t color) {
  char ansi[24];
  int len = snprintf(
              ansi,
              sizeof(ansi),
              color.isBackground ? "\x1b[48;2;%d;%d;%dm" : "\x1b[38;2;%d;%d;%dm",
              color.r, color.g, color.b
            );
  appendBuffer(buff, ansi, len);
}

void editorDrawLines(buffer *buff) {
  for (int i = 0; i < E.screenRows; i++) {
    int filerow = i + E.rowOffset;
    color_t backgroundColor = filerow == E.cursorY ? theme.activeLine : theme.background;

    editorHighlightOutput(buff, theme.text);
    editorHighlightOutput(buff, backgroundColor);

    if (filerow >= E.numlines) {
      if (E.numlines == 0 && i == E.screenRows / 3) {
        char welcome[80];
        int length = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);

        length = min(length, E.screenCols);

        int padding = (E.screenCols - length) / 2;

        if (padding != 0) {
          appendBuffer(buff, "~", 1);
          padding--;
        }

        while (padding--) {
          appendBuffer(buff, " ", 1);
        }

        appendBuffer(buff, welcome, length);
      }
      else if (filerow != 0) { 
        appendBuffer(buff, "~", 1);
      }
    }
    else {
      color_t prevColor = theme.text;

      char *content = &E.lines[filerow].renderContent[E.colOffset];
      int length = clamp(0, E.lines[filerow].renderLength - E.colOffset, E.screenCols);
      color_t *highlight = &E.lines[filerow].highlight[E.colOffset];

      for (int j = 0; j < length; j++) {
        color_t curColor = highlight[j];

        if (!colorcmp(curColor, prevColor)) {
          if (curColor.isBackground)
            editorHighlightOutput(buff, curColor.isDark ? theme.lightText : theme.darkText);
          else if (prevColor.isBackground)
            editorHighlightOutput(buff, backgroundColor);

          editorHighlightOutput(buff, curColor);
          prevColor = curColor;
        }
        appendBuffer(buff, &content[j], 1);
      }
    }
    appendBuffer(buff, "\x1b[K", 3); // Erase from cursor to end of line
    appendBuffer(buff, "\r\n", 2);
  }

  // Reset default colors
  editorHighlightOutput(buff, theme.text);
  editorHighlightOutput(buff, theme.background);
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

  infoLen = min(infoLen, E.screenCols);

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
  editorHighlightOutput(buff, theme.background);
  appendBuffer(buff, "\x1b[K", 3); // Erase from cursor to end of line

  int len = strlen(E.statusmsg);
  len = min(len, E.screenCols);

  if (len && time(NULL) - E.statusmsg_time < 5)
    appendBuffer(buff, E.statusmsg, len);

  appendBuffer(buff, "\x1b[0m", 4); // Reset style and colors
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

