#include <buffer.h>
#include <highlight.h>
#include <lines.h>
#include <output.h>
#include <tools.h>

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
  editorSetCursorPosition(&buff);

  appendBuffer(&buff, "\x1b[?25h", 6); // Make cursor visible

  write(STDOUT_FILENO, buff.content, buff.length);
  freeBuffer(&buff);
}

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

void editorDrawLines(buffer *buff) {
  for (int i = 0; i < E.screenRows; i++) {
    int filerow = i + E.rowOffset;
    color_t backgroundColor = filerow == E.cursorY ? theme.activeLine : theme.background;

    setDefaultColors(buff);
    
    if (filerow < E.numlines) {
      printLineNumber(filerow, buff);
      editorHighlightOutput(buff, backgroundColor);
      printTextLine(filerow, backgroundColor, buff);
    }

    if (E.splashScreen && i == E.screenRows / 3) {
      printMainScreen(buff);
    }

    appendBuffer(buff, "\x1b[K", 3); // Erase from cursor to end of line
    appendBuffer(buff, "\r\n", 2);   // Carriage Return + Line Feed
  }

  setDefaultColors(buff);
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

void editorSetCursorPosition(buffer *buff) {
  char temp[32];
  
  int cx = (E.rCursorX - E.colOffset + E.sidebarWidth) + 1;
  int cy = (E.cursorY - E.rowOffset) + 1;

  snprintf(temp, sizeof(temp), "\x1b[%d;%dH", cy, cx);
  appendBuffer(buff, temp, strlen(temp));
}

void editorSetStatusMessage(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), format, args);
  va_end(args);
  E.statusmsg_time = time(NULL);
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

void setDefaultColors(buffer *buff) {
  editorHighlightOutput(buff, theme.text.standard);
  editorHighlightOutput(buff, theme.background);
}

void printLineNumber(int row, buffer *buff) {
  char sidebarLine[E.sidebarWidth];
  memset(sidebarLine, ' ', E.sidebarWidth);

  char num[E.sidebarWidth];
  int n;

  if (row == E.cursorY) {
    editorHighlightOutput(buff, theme.sidebar.activeNumber);
    n = snprintf(num, E.sidebarWidth, "%d", row + 1);
    memcpy(sidebarLine + 1, num, n);
  }
  else {
    editorHighlightOutput(buff, theme.sidebar.number);
    n = snprintf(num, E.sidebarWidth, "%d", abs(row - E.cursorY));
    memcpy(sidebarLine + E.sidebarWidth - n - 2, num, n);
  }

  appendBuffer(buff, sidebarLine, E.sidebarWidth);
  editorHighlightOutput(buff, theme.text.standard);
}

void printTextLine(int row, color_t background, buffer *buff) {
  color_t prevColor = theme.text.standard;

  char *content = &E.lines[row].renderContent[E.colOffset];
  int length = clamp(0, E.lines[row].renderLength - E.colOffset, E.screenCols);
  color_t *highlight = &E.lines[row].highlight[E.colOffset];

  for (int j = 0; j < length; j++) {
    color_t curColor = highlight[j];

    if (!colorcmp(curColor, prevColor)) {
      if (curColor.isBackground)
        editorHighlightOutput(buff, curColor.isDark ? theme.text.light : theme.text.dark);
      else if (prevColor.isBackground)
        editorHighlightOutput(buff, background);

      editorHighlightOutput(buff, curColor);
      prevColor = curColor;
    }
    appendBuffer(buff, &content[j], 1);
  }
}

void printMainScreen(buffer *buff) {
  char welcome[80];
  int length = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);

  length = min(length, E.screenCols);

  int padding = (E.screenCols - length) / 2;

  while (padding--) {
    appendBuffer(buff, " ", 1);
  }
  appendBuffer(buff, welcome, length);

  E.splashScreen = false;
}

void adjustSidebarWidth(void) {
  int num = E.numlines;
  int count = 0;
  do {
    num /= 10;
    count++; 
  } while (num != 0);

  E.sidebarWidth = count >= 4 ? count + 3 : 6;
}

