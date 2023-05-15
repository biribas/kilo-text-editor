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

