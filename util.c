#include <raylib.h>

#include "util.h"

/* Fisher-Yates shuffle */
void rand_shuffle(int *arr, int count)
{
    int i;
    for (i = count - 1; i >= 1; i--) {
        int j = GetRandomValue(0, i);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}
