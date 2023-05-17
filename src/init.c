#include <init.h>
#include <lines.h>
#include <terminal.h>
#include <tools.h>

#define COLOR_RGB(r, g, b, isBg) (color_t){r, g, b, isBg, isDark(r, g, b)}

editorConfig E;
colors theme;

char *C_EXTENSIONS[] = {".c", ".cpp", ".h", NULL};
char *C_KEYWORDS[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case", "int|",
  "long|", "double|", "float|", "char|", "unsigned|", "signed|", "void|",
  "#include", "#define", "#undef", "#ifdef", "#ifndef", "#if", "#elif",
  "#else", "#endif", "#line", "#error", "#warning", "#region", "#endregion", NULL
};
char *C_OPERATORS[] = { "+", "-", "*", "/", "%", "=", "!", "<", ">", "&", "|", "^", ".", NULL};
char *C_BRACKETS[] = {"(", ")", "{", "}", "[", "]", NULL};
char *C_END_STATEMENTS[] = {",", ";", NULL};

// Highlight Database
editorSyntax HLDB[] = {
  {
    C_EXTENSIONS,
    C_KEYWORDS,
    C_OPERATORS,
    C_BRACKETS,
    C_END_STATEMENTS,
    HIGHLIGHT_NUMBERS | HIGHLIGHT_STRINGS,
    { {"//", 2}, { {"/*", 2}, {"*/", 2} } }
  }
};

size_t HLDB_ENTRIES = (int)(sizeof(HLDB) / sizeof(HLDB[0]));

void initEditor(void) {
  atexit(freeMemory);
  
  E.cursorX = 0;
  E.cursorY = 0;
  E.highestLastX = 0;
  E.rCursorX = 0;
  E.rowOffset = 0;
  E.colOffset = 0;
  E.numlines = 0;
  E.sidebarWidth = 6;
  E.lines = NULL;
  E.dirty = false;
  E.splashScreen = false;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.syntax = NULL;

  E.statusmsg_time = 0;
  if (!getWindowSize(&E.screenRows, &E.screenCols))
    die("getWindowSize");

  E.screenRows -= 2;
}

void initColors(void) {
  theme.background = COLOR_RGB(30, 30, 46, true);
  theme.keyword = COLOR_RGB(203, 166, 247, false);
  theme.activeLine = COLOR_RGB(42, 43, 60, true);
  theme.datatype = COLOR_RGB(249, 226, 175, false);
  theme.preprocessor = COLOR_RGB(245, 194, 231, false);
  theme.number = COLOR_RGB(250, 179, 135, false);
  theme.string = COLOR_RGB(166, 227, 161, false);
  theme.comment = COLOR_RGB(108, 112, 134, false);
  theme.operators = COLOR_RGB(116, 199, 236, false);
  theme.brackets = COLOR_RGB(147, 153, 178, false);
  theme.endStatement = COLOR_RGB(147, 153, 178, false);

  theme.match.unselected = COLOR_RGB(62, 87, 103, true);
  theme.match.selected = COLOR_RGB(137, 220, 235, true);
  theme.text.light = COLOR_RGB(205, 214, 244, false);
  theme.text.dark = COLOR_RGB(17, 17, 17, false);
  theme.text.standard = theme.background.isDark ? theme.text.light : theme.text.dark;
  theme.sidebar.number = COLOR_RGB(69, 71, 90, false);
  theme.sidebar.activeNumber = COLOR_RGB(180, 190, 254, false);
}

