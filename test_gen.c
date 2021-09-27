#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "fail.h"

size_t i, j, n, m;

double r_max = (double) RAND_MAX;

double randf64() {
    double p = (double)(rand()) / r_max;
    double q = ((double)(rand()) / r_max) * 10.0;
    return p * q;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    if (argc != 3) {
        printf("Usage: %s <amount of rows> <amount of columns>\n", argv[0]);
        return 0;
    }
    int res;
    res = sscanf(argv[1], "%zu", &n);
    if (!res) { failwithf("Could not parse rows amount from '%s'\n", argv[1]); }
    res = sscanf(argv[2], "%zu", &m);
    if (!res) { failwithf("Could not parse columns amount from '%s'\n", argv[2]); }

    for (i = 0; i < n; i++) {
        for (j = 0; j < m - 1; j++) {
            printf("%lf,", randf64());
        }
        printf("%lf\n", randf64());
    }

    return 0;
}

