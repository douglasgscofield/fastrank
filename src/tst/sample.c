#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]) {
    int n;
    if (argc == 2)
        n = atoi(argv[1]);
    else {
        fprintf(stderr, "must supply n\n");
        exit(1);
    }
    int* y = (int*)malloc(n * sizeof(int));
    int* x = (int*)malloc(n * sizeof(int));
    // to understand R's do_sample()
    for (int i = 0; i < n; ++i)
        x[i] = i;
    int xn = n;
    srand(time(NULL));
    for (int i = 0; i < n; ++i) {
        int j = (int)(xn * (rand() / (double)RAND_MAX));
        fprintf(stderr, "i: %d  j rnd index: %d\n", i, j);
        y[i] = x[j] + 1;  // + 1 to go from 0 to 1 based
        x[j] = x[--xn];  // shrink the pool
    }
    for (int i = 0; i < n; ++i)
        printf(" %d ", y[i]);
    putchar('\n');
    return 0;
}
