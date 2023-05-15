#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

  #define CTRL_KEY(k) ((k) & 0x1f)
  #define BUFFER_INIT {NULL, 0}
  #define KILO_VERSION "0.0.1"
  #define TAB_SIZE 2
  #define QUIT_TIMES 2

  // Highlight flags
  #define HIGHLIGHT_NUMBERS (1 << 0)
  #define HIGHLIGHT_STRINGS (1 << 1)

  enum editorKeys {
    BACKSPACE = 127,
    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
  };

  typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    bool isBackground;
  } color_t;

  typedef struct {
    color_t background;
    color_t text;
    color_t darkText;
    color_t keyword;
    color_t datatype;
    color_t preprocessor;
    color_t number;
    color_t string;
    color_t comment;
    color_t match;
    color_t currentMatch;
    color_t operators;
    color_t brackets;
    color_t endStatement;
  } colors;

  struct comment {
    char *value;
    int length;
  };

  typedef struct {
    char **filematch;
    char **keywords;
    char **operators;
    char **brackets;
    char **endStatements;
    int flags;

    struct {
      struct comment singleline;
      struct {
        struct comment start;
        struct comment end;
      } multiline;
    } comment;
  } editorSyntax;

  typedef struct {
    bool isPrevSep;
    bool inComment;
    int inString;
    int idx;
    color_t prevHL;
  } highlightController;

  typedef struct {
    int index;
    char *content, *renderContent;
    int length, renderLength;
    bool isOpenComment;
    color_t *highlight;
  } editorLine;

  typedef struct {
    char *content;
    int length;
  } buffer;

  typedef struct {
    int cursorX, cursorY;
    int savedLastY;
    int savedLastX;
    int highestLastX;
    int rCursorX;
    int screenRows, screenCols;
    int rowOffset, colOffset;
    int numlines;
    bool dirty;
    editorLine *lines;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    editorSyntax *syntax;
    struct termios original_state;
  } editorConfig;

  extern editorConfig E; 
  extern colors theme; 

  /*** Filetypes ***/
  extern char *C_EXTENSIONS[];
  extern char *C_KEYWORDS[];
  extern char *C_OPERATORS[];
  extern char *C_BRACKETS[];
  extern char *C_END_STATEMENTS[];

  // Highlight Database
  extern editorSyntax HLDB[];
  extern size_t HLDB_ENTRIES;

#endif
