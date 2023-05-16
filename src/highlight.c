#include <highlight.h>
#include <init.h>
#include <tools.h>

void colorLine(editorLine *line, int start, color_t c, int len) {
  for (int i = start, n = start + len; i < n; i++) {
    line->highlight[i] = c;
  }
}

bool highlightSinglelineComments(editorLine *line, highlightController *hc) {
  struct comment singleline = E.syntax->comment.singleline;

  if (!singleline.length || hc->inString || hc->inComment) return false;

  if (!strncmp(&line->renderContent[hc->idx], singleline.value, singleline.length)) {
    colorLine(line, hc->idx, theme.comment, line->renderLength - hc->idx);
    hc->idx = line->renderLength;
    return true;
  }
  return false;
}

bool highlightMultilineComments(editorLine *line, highlightController *hc) {
  struct comment multilineStart = E.syntax->comment.multiline.start;
  struct comment multilineEnd = E.syntax->comment.multiline.end;

  if (!multilineStart.length || !multilineEnd.length || hc->inString) return false;

  if (hc->inComment) {
    if (!strncmp(&line->renderContent[hc->idx], multilineEnd.value, multilineEnd.length)) {
      colorLine(line, hc->idx, theme.comment, multilineEnd.length);
      hc->idx += multilineEnd.length;
      hc->inComment = false;
      hc->isPrevSep = true;
    }
    else {
      line->highlight[hc->idx++] = theme.comment;
    }
    return true;
  }

  if (!strncmp(&line->renderContent[hc->idx], multilineStart.value, multilineStart.length)) {
    colorLine(line, hc->idx, theme.comment, multilineStart.length);
    hc->idx += multilineStart.length;
    hc->inComment = true;
    return true;
  }

  return false;
}

bool highlightStrings(editorLine *line, highlightController *hc) {
  char c = line->renderContent[hc->idx];

  if (!(E.syntax->flags & HIGHLIGHT_STRINGS)) return false;

  if (hc->inString) {
    line->highlight[hc->idx] = theme.string;

    if (c == '\\' && hc->idx + 1 < line->renderLength) {
      line->highlight[hc->idx + 1] = theme.string;
      hc->idx += 2;
      return true;
    }

    if (c == hc->inString) {
      hc->inString = 0;
    }

    hc->isPrevSep = true;
    hc->idx++;
    return true;
  }

  if (c == '"' || c == '\'') {
    hc->inString = c;
    line->highlight[hc->idx] = theme.string;
    hc->idx++;
    return true;
  }

  return false;
}

bool highlightNumbers(editorLine *line, highlightController *hc) {
  char c = line->renderContent[hc->idx];
  char nextChar = hc->idx + 1 < line->renderLength ? line->renderContent[hc->idx + 1] : 0;

  if (!(E.syntax->flags & HIGHLIGHT_NUMBERS)) return false;

  bool isPrevNumber = colorcmp(hc->prevHL, theme.number);
  bool isInt = isdigit(c) && (hc->isPrevSep || isPrevNumber);
  bool isFloat = c == '.' && (isPrevNumber || isdigit(nextChar));

  if (isInt || isFloat) {
    line->highlight[hc->idx] = theme.number;
    hc->isPrevSep = false;
    hc->idx++;
    return true;
  }
  return false;
}

bool highlightKeywords(editorLine *line, highlightController *hc) {
  char **keywords = E.syntax->keywords;

  if (!hc->isPrevSep) return false;

  int j;
  for (j = 0; keywords[j]; j++) {
    int klen = strlen(keywords[j]);
    bool isPreprocessor = *keywords[j] == '#';
    bool isDatatype = keywords[j][klen - 1] == '|';

    if (isDatatype) klen--;
  
    if (hc->idx + klen >= line->renderLength)
      continue;

    if (!strncmp(&line->renderContent[hc->idx], keywords[j], klen) && 
        isSeparator(line->renderContent[hc->idx + klen]))
    {
      color_t color = isDatatype ? theme.datatype : isPreprocessor ? theme.preprocessor : theme.keyword;
      colorLine(line, hc->idx, color, klen);
      hc->idx += klen;

      if (!strcmp(keywords[j], "#include")) {
        colorLine(line, hc->idx, theme.string, line->renderLength - hc->idx);
        hc->idx = line->renderLength;
      }

      hc->isPrevSep = false;
      break;
    }
  }

  return keywords[j] != NULL;
}

bool highlightOperators(editorLine *line, highlightController *hc) {
  return highlightSymbols(line, hc, E.syntax->operators, theme.operators);
}

bool highlightBrackets(editorLine *line, highlightController *hc) {
  return highlightSymbols(line, hc, E.syntax->brackets, theme.brackets);
}

bool highlightEndStatemetns(editorLine *line, highlightController *hc) {
  return highlightSymbols(line, hc, E.syntax->endStatements, theme.endStatement);
}

bool highlightSymbols(editorLine *line, highlightController *hc, char **symbols, color_t color) {
  int j;
  for (j = 0; symbols[j]; j++) {
    if (line->renderContent[hc->idx] == *symbols[j]) {
      colorLine(line, hc->idx, color, 1);
      hc->idx++;
      hc->isPrevSep = true;
      break;
    }
  }
  return symbols[j] != NULL;
}

void editorUpdateHighlight(editorLine *line) {
  line->highlight = realloc(line->highlight, sizeof(color_t) * line->renderLength);
  colorLine(line, 0, theme.lightText, line->renderLength);

  if (E.syntax == NULL) return;

  highlightController hc;
  hc.isPrevSep = true;
  hc.inComment = line->index > 0 && E.lines[line->index - 1].isOpenComment;
  hc.inString = 0;
  hc.idx = 0;

  while (hc.idx < line->renderLength) {
    hc.prevHL = (hc.idx > 0) ? line->highlight[hc.idx - 1] : theme.lightText;

    bool wasHighlighted = (
      highlightSinglelineComments(line, &hc) ||
      highlightMultilineComments(line, &hc) ||
      highlightStrings(line, &hc) ||
      highlightNumbers(line, &hc) ||
      highlightKeywords(line, &hc) ||
      highlightOperators(line, &hc) ||
      highlightBrackets(line, &hc) ||
      highlightEndStatemetns(line, &hc)
    );

    if (wasHighlighted) continue;

    hc.isPrevSep = isSeparator(line->renderContent[hc.idx]);
    hc.idx++;
  }

  bool changed = line->isOpenComment != hc.inComment;
  bool isLastLine = line->index + 1 >= E.numlines;
  line->isOpenComment = hc.inComment;

  if (changed && !isLastLine) {
    editorUpdateHighlight(&E.lines[line->index + 1]);
  }
}

void editorSelectSyntaxHighlight(void) {
  E.syntax = NULL;

  if (E.filename == NULL) return;

  char *extension = strrchr(E.filename, '.');

  for (size_t i = 0; i < HLDB_ENTRIES; i++) {
    editorSyntax *syntax = &HLDB[i];
    for (int j = 0; syntax->filematch[j]; j++) {
      bool isExtension = (syntax->filematch[j][0] == '.');

      if ((isExtension && extension && !strcmp(extension, syntax->filematch[j])) || 
          (!isExtension && strstr(E.filename, syntax->filematch[j]))) {
        E.syntax = syntax;

        for (int k = 0; k < E.numlines; k++) {
          editorUpdateHighlight(&E.lines[k]);
        }

        return;
      }
    }
  }
}

void clearSearchHighlight(void) {  
  int cur = E.rowOffset;
  int last = min(cur + E.screenRows, E.numlines);
  while (cur < last) {
    editorUpdateHighlight(&E.lines[cur]);
    cur++; 
  }
}

