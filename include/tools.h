#include <main.h>

#ifndef TOOLS_H_INCLUDED
#define TOOLS_H_INCLUDED
  int min(int, int);
  int max(int, int);
  int clamp(int min, int value, int max);
  int mod(int, int);
  bool isSeparator(int);
  bool isSpecial(int c);
  bool colorcmp(color_t, color_t);
  bool isDark(int r, int g, int b);
#endif
