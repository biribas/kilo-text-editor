#include <buffer.h>
#include <highlight.h>
#include <lines.h>
#include <output.h>
#include <stdio.h>
#include <string.h>
#include <tools.h>

void editorRefreshScreen(void) {
  E.rCursorX = E.cursorY < E.numlines ? editorLineCxToRx(&E.lines[E.cursorY], E.cursorX) : 0;

  editorScrollX();
  editorScrollY();

  buffer buff = BUFFER_INIT;

  appendBuffer(&buff, "\x1b[?25l", 6); // Make cursor invisible
  appendBuffer(&buff, "\x1b[H", 3); // Moves cursor to home position (0, 0)

  editorDrawLines(&buff);
  if (E.isPromptOpen)
    editorDrawPromptBar(&buff);
  else
    editorDrawStatusBar(&buff);

  editorSetCursorPosition(&buff);

  appendBuffer(&buff, "\x1b[?25h", 6); // Make cursor visible

  write(STDOUT_FILENO, buff.content, buff.length);
  freeBuffer(&buff);
}

void editorScrollX(void) {
  const int colOffsetGap = E.screenCols * 0.15;

  int minOffset = max(0, E.rCursorX - E.screenCols + colOffsetGap + 1);
  int maxOffset = max(0, E.rCursorX - colOffsetGap);

  E.colOffset = clamp(minOffset, E.colOffset, maxOffset);
}

void editorScrollY(void) {
  const int rowOffsetGap = E.screenRows * 0.20;

  int gap = min(rowOffsetGap, E.numlines - E.cursorY);

  int minOffset = max(0, E.cursorY - E.screenRows + 1 + gap);
  int maxOffset = max(0, E.cursorY - rowOffsetGap);

  E.rowOffset = clamp(minOffset, E.rowOffset, maxOffset);
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
  color_t modeColor = theme.mode.normal;
  char mode[80];
  int modeLen = sprintf(mode, " NORMAL ");

  if (E.mode == INSERT) {
    modeColor = theme.mode.insert;
    modeLen = sprintf(mode, " INSERT ");
  }

  appendBuffer(buff, "\x1b[1m", 4);
  editorHighlightOutput(buff, modeColor);
  editorHighlightOutput(buff, theme.mode.text);
  appendBuffer(buff, mode, modeLen);
  appendBuffer(buff, "\x1b[22m", 5);

  const char *filename = E.filename ? E.filename : "[No name]";
  const char *modified = E.dirty ? " *" : "";

  char bufferInfo[80];
  int bufferLen = snprintf(bufferInfo, sizeof(bufferInfo), " %.20s%s ", filename, modified);
  bufferLen = min(bufferLen, E.screenCols);
  editorHighlightOutput(buff, theme.buffer.active.background);
  editorHighlightOutput(buff, theme.buffer.active.text);
  appendBuffer(buff, bufferInfo, bufferLen);

  char percentage[4];
  if (E.numlines == 0 || E.cursorY == 0) {
    sprintf(percentage, "Top");
  }
  else if (E.cursorY >= E.numlines - 1) {
    sprintf(percentage, "Bot");
  }
  else {
    int num = 100 * (E.cursorY + 1) / E.numlines;
    if (num < 10)
      sprintf(percentage, " %d%%", num);
    else
      sprintf(percentage, "%d%%", num);
  }

  char cx[80], cy[80];
  int cxLen = sprintf(cx, "%d", E.cursorX + 1);
  int cyLen = sprintf(cy, "%d", E.cursorY + 1);

  int cursorlen = max(6, cxLen + cyLen + 1);
  char cursorPosition[cursorlen];
  memset(cursorPosition, ' ', cursorlen);

  int yOffset = max(0, 3 - cyLen);
  int xOffset = yOffset + cyLen + 1;
  memcpy(cursorPosition + yOffset, cy, cyLen);
  cursorPosition[xOffset - 1] = ':';
  memcpy(cursorPosition + xOffset, cx, cxLen);

  char position[80];
  int posLen = sprintf(position, " %s  %s ", cursorPosition, percentage);

  editorHighlightOutput(buff, theme.statusBar);
  for (int n = bufferLen + modeLen; n < E.screenCols; n++) {
    if (E.screenCols - n == posLen) {
      appendBuffer(buff, "\x1b[1m", 4);
      editorHighlightOutput(buff, theme.mode.text);
      editorHighlightOutput(buff, modeColor);
      appendBuffer(buff, position, posLen);
      appendBuffer(buff, "\x1b[22m", 5);
      break;
    }
    appendBuffer(buff, " ", 1);
  }

  appendBuffer(buff, "\x1b[0m", 4); // Reset style and colors
}

void editorDrawPromptBar(buffer *buff) {
  editorHighlightOutput(buff, theme.background);
  editorHighlightOutput(buff, theme.text.standard);
  appendBuffer(buff, "\x1b[K", 3); // Erase from cursor to end of line

  int len = strlen(E.statusmsg);
  len = min(len, E.screenCols);

  if (E.isPromptOpen) {
    appendBuffer(buff, E.statusmsg, len);
    E.isPromptOpen = false;
  }

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
  E.isPromptOpen = true;
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
  color_t *highlight = &E.lines[row].highlight[E.colOffset];

  int length = clamp(0, E.lines[row].renderLength - E.colOffset, E.screenCols - E.sidebarWidth);

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

