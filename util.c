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

int neighbourhood4(int x, int y, int w, int h, int *idx_out)
{
    int count = 0;
    if (x > 0) {
        idx_out[count] = y * w + x - 1;
        count++;
    }
    if (y > 0) {
        idx_out[count] = (y - 1) * w + x;
        count++;
    }
    if (x + 1 < w) {
        idx_out[count] = y * w + x + 1;
        count++;
    }
    if (y + 1 < h) {
        idx_out[count] = (y + 1) * w + x;
        count++;
    }
    rand_shuffle(idx_out, count);
    return count;
}

int neighbourhood4wrap(int x, int y, int w, int h, int *idx_out)
{
    idx_out[0] = y * w + (x > 0 ? x : w) - 1;
    idx_out[1] = ((y > 0 ? y : h) - 1) * w + x;
    idx_out[2] = y * w + (x + 1 == w ? 0 : x + 1);
    idx_out[3] = (y + 1 == h ? 0 : y + 1) * w + x;

    rand_shuffle(idx_out, 4);
    return 4;
}

int neighbourhood8(int x, int y, int w, int h, int *idx_out)
{
    int count = 0;
    int x0 = x;
    int y0 = y;
    int xleft = max(x0 - 1, 0);
    int xright = min(x0 + 1, w - 1);
    int ytop = max(y0 - 1, 0);
    int ybottom = min(y0 + 1, h - 1);

    for (x = xleft; x <= xright; x++) {
        for (y = ytop; y <= ybottom; y++) {
            if (x != x0 || y != y0) {
                idx_out[count] = y * w + x;
                count += 1;
            }
        }
    }

    rand_shuffle(idx_out, count);
    return count;
}

int neighbourhood8wrap(int x, int y, int w, int h, int *idx_out)
{
    int x2 = x;
    int y2 = y;
    int x1 = (x2 > 0 ? x2 : w) - 1;
    int y1 = (y2 > 0 ? y2 : h) - 1;
    int x3 = x2 + 1 < w ? x2 + 1 : 0;
    int y3 = y2 + 1 < h ? y2 + 1 : 0;

    idx_out[0] = y1 * w + x1;
    idx_out[1] = y1 * w + x2;
    idx_out[2] = y1 * w + x3;
    idx_out[3] = y2 * w + x1;

    idx_out[4] = y2 * w + x3;
    idx_out[5] = y3 * w + x1;
    idx_out[6] = y3 * w + x2;
    idx_out[7] = y3 * w + x3;

    rand_shuffle(idx_out, 8);
    return 8;
}
