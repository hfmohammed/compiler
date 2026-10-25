// Runtime support linked into every compiled Gazprea program.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int32_t gaz_main(void);

static void rt_error(const char *msg) {
  fflush(stdout);
  fprintf(stderr, "RuntimeError: %s\n", msg);
  exit(1);
}

void gaz_print_int(int32_t v) { printf("%d", v); }
void gaz_print_real(float v) { printf("%g", (double)v); }
void gaz_print_bool(int32_t v) { putchar(v ? 'T' : 'F'); }
void gaz_print_char(int8_t v) { putchar((unsigned char)v); }

int32_t gaz_read_int(void) {
  int32_t v;
  if (scanf("%d", &v) != 1) rt_error("failed to read integer from std_input");
  return v;
}
float gaz_read_real(void) {
  float v;
  if (scanf("%f", &v) != 1) rt_error("failed to read real from std_input");
  return v;
}
int8_t gaz_read_char(void) {
  int c = getchar();
  if (c == EOF) rt_error("failed to read character from std_input");
  return (int8_t)c;
}

int32_t gaz_idiv(int32_t a, int32_t b) {
  if (b == 0) rt_error("integer division by zero");
  if (a == INT32_MIN && b == -1) return INT32_MIN;
  return a / b;
}
int32_t gaz_imod(int32_t a, int32_t b) {
  if (b == 0) rt_error("integer modulo by zero");
  if (b == -1) return 0;
  return a % b;
}
int32_t gaz_ipow(int32_t base, int32_t exp) {
  if (exp < 0) {
    if (base == 0) rt_error("zero raised to a negative power");
    if (base == 1) return 1;
    if (base == -1) return (exp % 2) ? -1 : 1;
    return 0;
  }
  int32_t r = 1;
  while (exp--) r = (int32_t)((uint32_t)r * (uint32_t)base);
  return r;
}
float gaz_rpow(float a, float b) { return powf(a, b); }

int main(void) { return gaz_main(); }
