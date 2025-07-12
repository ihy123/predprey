#ifndef UTIL_H_SENTRY
#define UTIL_H_SENTRY

static int max(int a, int b) { return a > b ? a : b; }
static int min(int a, int b) { return a < b ? a : b; }

void rand_shuffle(int *arr, int count);

int neighbourhood4(int x, int y, int w, int h, int *idx_out);
int neighbourhood4wrap(int x, int y, int w, int h, int *idx_out);
int neighbourhood8(int x, int y, int w, int h, int *idx_out);
int neighbourhood8wrap(int x, int y, int w, int h, int *idx_out);

#endif
