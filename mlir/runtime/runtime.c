// Runtime support linked into every compiled Gazprea program.
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32_t gaz_main(void);

static void rt_error(const char *kind, const char *msg) {
  fflush(stdout);
  fprintf(stderr, "%s: %s\n", kind, msg);
  exit(1);
}

void gaz_print_int(int32_t v) { printf("%d", v); }
void gaz_print_real(float v) { printf("%g", (double)v); }
void gaz_print_bool(int32_t v) { putchar(v ? 'T' : 'F'); }
void gaz_print_char(int8_t v) { putchar((unsigned char)v); }

// ---- std_input ----
// The whole of stdin is read on first use. A read takes the longest prefix that
// matches the type's literal syntax; on a bad match the position is left
// unchanged and stream_state becomes 1, at end of input it becomes 2.
static char *in_buf;
static size_t in_len, in_pos;
static int in_loaded, in_state;

static void in_load(void) {
  if (in_loaded) return;
  in_loaded = 1;
  size_t cap = 4096;
  in_buf = malloc(cap);
  size_t n;
  while ((n = fread(in_buf + in_len, 1, cap - in_len, stdin)) > 0) {
    in_len += n;
    if (in_len == cap) in_buf = realloc(in_buf, cap *= 2);
  }
}
static int in_peek(size_t at) { return at < in_len ? (unsigned char)in_buf[at] : -1; }
static size_t in_skip_ws(size_t at) {
  while (at < in_len && isspace((unsigned char)in_buf[at])) at++;
  return at;
}
static size_t in_digits(size_t at) {
  while (isdigit(in_peek(at))) at++;
  return at;
}

int32_t gaz_stream_state(void) { return in_state; }

int32_t gaz_read_int(void) {
  in_load();
  size_t p = in_skip_ws(in_pos);
  if (p >= in_len) { in_pos = p; in_state = 2; return 0; }
  size_t q = p;
  if (in_peek(q) == '+' || in_peek(q) == '-') q++;
  size_t d = in_digits(q);
  if (d == q) { in_state = 1; return 0; }
  char tmp[64];
  size_t n = d - p;
  if (n >= sizeof tmp) { in_state = 1; return 0; }
  memcpy(tmp, in_buf + p, n);
  tmp[n] = 0;
  long long v = strtoll(tmp, NULL, 10);
  if (v > INT32_MAX || v < INT32_MIN) { in_state = 1; return 0; }
  in_pos = d;
  in_state = 0;
  return (int32_t)v;
}

float gaz_read_real(void) {
  in_load();
  size_t p = in_skip_ws(in_pos);
  if (p >= in_len) { in_pos = p; in_state = 2; return 0; }
  size_t q = p;
  if (in_peek(q) == '+' || in_peek(q) == '-') q++;
  size_t ip = in_digits(q), e = ip;
  int digits = ip > q;
  if (in_peek(e) == '.') {
    size_t fp = in_digits(e + 1);
    digits = digits || fp > e + 1;
    if (digits) e = fp;
  }
  if (!digits) { in_state = 1; return 0; }
  if (in_peek(e) == 'e' || in_peek(e) == 'E') {
    size_t x = e + 1;
    if (in_peek(x) == '+' || in_peek(x) == '-') x++;
    size_t xd = in_digits(x);
    if (xd > x) e = xd;
  }
  char tmp[512];
  size_t n = e - p;
  if (n >= sizeof tmp) { in_state = 1; return 0; }
  memcpy(tmp, in_buf + p, n);
  tmp[n] = 0;
  in_pos = e;
  in_state = 0;
  return strtof(tmp, NULL);
}

int32_t gaz_read_bool(void) {
  in_load();
  size_t p = in_skip_ws(in_pos);
  if (p >= in_len) { in_pos = p; in_state = 2; return 0; }
  int c = in_peek(p);
  if (c != 'T' && c != 'F') { in_state = 1; return 0; }
  in_pos = p + 1;
  in_state = 0;
  return c == 'T';
}

int8_t gaz_read_char(void) {
  in_load();
  if (in_pos >= in_len) { in_state = 2; return (int8_t)0xFF; }
  in_state = 0;
  return (int8_t)in_buf[in_pos++];
}

void gaz_overflow(void) { rt_error("MathError", "integer overflow"); }

int32_t gaz_idiv(int32_t a, int32_t b) {
  if (b == 0) rt_error("MathError", "division by zero");
  if (a == INT32_MIN && b == -1) rt_error("MathError", "integer overflow");
  return a / b;
}
int32_t gaz_imod(int32_t a, int32_t b) {
  if (b == 0) rt_error("MathError", "modulo by zero");
  if (a == INT32_MIN && b == -1) rt_error("MathError", "integer overflow");
  return a % b;
}
// Exact integer power; a negative exponent truncates toward zero.
int32_t gaz_ipow(int32_t base, int32_t exp) {
  if (base == 0 && exp <= 0) rt_error("MathError", "zero raised to a non-positive power");
  if (exp < 0) {
    if (base == 1) return 1;
    if (base == -1) return (exp % 2) ? -1 : 1;
    return 0;
  }
  int64_t r = 1;
  while (exp--) {
    r *= base;
    if (r > INT32_MAX || r < INT32_MIN) rt_error("MathError", "integer overflow");
  }
  return (int32_t)r;
}
float gaz_rpow(float a, float b) { return powf(a, b); }
float gaz_rmod(float a, float b) { return fmodf(a, b); }

int main(void) { return gaz_main(); }
