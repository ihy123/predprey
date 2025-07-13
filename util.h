#ifndef UTIL_H_SENTRY
#define UTIL_H_SENTRY

static int max(int a, int b) { return a > b ? a : b; }
static int min(int a, int b) { return a < b ? a : b; }

void rand_shuffle(int *arr, int count);

#endif
