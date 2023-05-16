#include <tools.h>

int min(int a, int b) {
  return (a < b ? a : b);
}

int clamp(int min, int value, int max) {
  return (value < min ? min : value > max ? max : value);
}

int mod(int a, int b) {
  return ((a % b + b) % b);
}

bool isSeparator(int c) {
  return isspace(c) || c == '\0' || strchr("()[]{}<>,.+-/*=~%|&!;", c) != NULL;
}

bool colorcmp(color_t x, color_t y) {
  return x.r == y.r && x.g == y.g && x.b == y.b;
}

