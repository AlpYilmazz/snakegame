#include <stdlib.h>

#include "util.h"

int rand_in_range(int l, int r) {
    return l + (rand() % (r - l + 1));
}

float rand_float_discrete(int l, int r, int point_step) {
    float base = (float)rand_in_range(l, r-1);
    float fac = (float)rand_in_range(0, point_step);
    float fl = fac / (float)point_step;
    return base + fl;
}