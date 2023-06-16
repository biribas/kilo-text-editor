#include <tools.h>

int min(int a, int b) {
  return (a < b ? a : b);
}

int max(int a, int b) {
  return (a > b ? a : b);
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

bool isSpecial(int c) {
  return !(isdigit(c) || isalpha(c));
}

bool colorcmp(color_t x, color_t y) {
  return x.r == y.r && x.g == y.g && x.b == y.b;
}

bool isDark(int r, int g, int b) {
  float lightness = (0.299 * r + 0.587 * g + 0.114 * b) / 255;
  return lightness < 0.55;
}

