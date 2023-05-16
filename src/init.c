#include <init.h>
#include <lines.h>
#include <terminal.h>

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
  E.lines = NULL;
  E.dirty = false;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.syntax = NULL;

  E.statusmsg_time = 0;
  if (!getWindowSize(&E.screenRows, &E.screenCols))
    die("getWindowSize");

  E.screenRows -= 2;
}

void initColors(void) {
  theme.background = (color_t){30, 30, 46, true};
  theme.lightText = (color_t){205, 214, 244, false};
  theme.darkText = (color_t){17, 17, 17, false};
  theme.keyword = (color_t){203, 166, 247, false};
  theme.datatype = (color_t){249, 226, 175, false};
  theme.preprocessor = (color_t){245, 194, 231, false};
  theme.number = (color_t){250, 179, 135, false};
  theme.string = (color_t){166, 227, 161, false};
  theme.comment = (color_t){108, 112, 134, false};
  theme.match = (color_t){62, 87, 103, true};
  theme.currentMatch = (color_t){137, 220, 235, true};
  theme.operators = (color_t){116, 199, 236, false};
  theme.brackets = (color_t){147, 153, 178, false};
  theme.endStatement = (color_t){147, 153, 178, false};
}

