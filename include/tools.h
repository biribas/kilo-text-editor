#include <main.h>

#ifndef TOOLS_H_INCLUDED
#define TOOLS_H_INCLUDED
  int min(int, int);
  int clamp(int min, int value, int max);
  int mod(int, int);
  bool isSeparator(int);
  bool colorcmp(color_t, color_t);
#endif
