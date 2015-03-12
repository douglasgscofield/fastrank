#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void iquicksort_I (int a[], int indx[], const int n);
void rquicksort_I (double a[], int indx[], const int n);
void iquicksort   (int a[], const int n);
void rquicksort   (double a[], const int n);
int* vrandi (const int low, const int high, const int n);

int main(int argc, char* argv[]) {
    srand(time(NULL));
    for (int i = 0; i < argc; ++i)
        printf("argv[%d]: %s   ", i, argv[i]);
    putchar('\n');
    printf("breaking ties with random ranks\n");
    int max = atoi(argv[1]);
    int n = atoi(argv[2]);
    printf("random vector:  max: %d   n: %d\n", max, n);
    int* x = vrandi(10, max, n);
    for (int i = 0; i < n; ++i) printf("%d    ", x[i]); putchar('\n');
    //for (int i = 0; i < n; ++i) printf("%d   ", x[i]); putchar('\n');
    //printf("sorted vector with indx:\n");
    int* indx = (int *) malloc(n * sizeof(int));
    for (int i = 0; i < n; ++i) indx[i] = i;
    iquicksort_I(x, indx, n);
    //for (int i = 0; i < n; ++i) printf("%d   ", x[i]); putchar('\n');
    //for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
    double* result = (double*) malloc(n * sizeof(double));
    double* ranks = (double*) malloc(n * sizeof(double));
    // now to break ties, step through sorted values
    int b = x[0];
    int ib = 0;
    int i;
    for (i = 1; i < n; ++i) {
        if (x[i] != b) { // consecutive numbers differ
            if (ib < i - 1) {
                // at least one previous tie, b=i-1, a=ib
                // number of ties is i - ib
                // STILL MAJOR BUGS HERE
                int tn = i - ib;
                int* t = (int*) malloc(tn * sizeof(int));
                for (int j = 0; j < tn; ++j)
                    t[j] = j;
                for (int j = ib; j <= i - 1; ++j) {
                    int k = (int)(tn * (rand()/(double)RAND_MAX));
                    printf("j: %d, k rnd index: %d  MIDDLE\n", j, k);
                    ranks[j] = (double)(t[k] + j + 1);
                    t[k] = t[--tn];  // shrink the pool
                }
            } else {
                ranks[ib] = ib + 1;
            }
            b = x[i];
            ib = i;
        }
    }
    // now i == n, check leftovers
    if (ib == i - 1)  // last two were unique
        ranks[ib] = i;
    else {  // ended with ties
        // STILL MAJOR BUGS HERE
        int tn = i - ib;
        int* t = (int*) malloc(tn * sizeof(int));
        for (int j = 0; j < tn; ++j)
            t[j] = j;
        for (int j = ib; j <= i - 1; ++j) {
            int k = (int)(tn * (rand()/(double)RAND_MAX));
            printf("j: %d, k rnd index: %d  FINAL\n", j, k);
            ranks[j] = (double)(t[k] + j + 1);
            t[k] = t[--tn];  // shrink the pool
        }
    }
    //printf("ranks of sorted vector:\n");
    //for (int i = 0; i < n; ++i) printf("%.1f   ", ranks[i]); putchar('\n');

    // reorder ranks into the answer
    for (int i = 0; i < n; ++i)
        result[indx[i]] = ranks[i];
    printf("reorder ranks into original order of vector:\n");
    for (int i = 0; i < n; ++i) printf("%.1f   ", result[i]); putchar('\n');
}

int* vrandi(const int low, const int high, const int n) { 
    if (n < 1) 
        return((int*) 0);
    int* result = (int *) malloc (n * sizeof(int));
    // modified from https://groups.google.com/forum/#!topic/comp.lang.c/WUbyVHk7D0A/discussion%5B1-25%5D
    (void)rand();
    for (int i = 0; i < n; ++i) 
        result[i] = (high - low + 1) * (rand() / (RAND_MAX + 1.0)) + low; 
    return(result);
} 


// quick_sort code modified from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort
void rquicksort_I (double a[], int indx[], const int n) {
    double p, t;
    int i, j, it;
    if (n < 2)
        return;
    p = a[n / 2];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[i] < p)
            i++;
        while (p < a[j])
            j--;
        if (i >= j)
            break;
        // swap values, and swap indices
        t = a[i];     it = indx[i];
        a[i] = a[j];  indx[i] = indx[j];
        a[j] = t;     indx[j] = it;
    }
    rquicksort_I(a, indx, i);
    rquicksort_I(a + i, indx + i, n - i);
}
 

void iquicksort_I (int a[], int indx[], const int n) {
    int p, t;
    int i, j, it;
    if (n < 2)
        return;
    p = a[n / 2];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[i] < p)
            i++;
        while (p < a[j])
            j--;
        if (i >= j)
            break;
        // swap values, and swap indices
        t = a[i];     it = indx[i];
        a[i] = a[j];  indx[i] = indx[j];
        a[j] = t;     indx[j] = it;
    }
    iquicksort_I(a, indx, i);
    iquicksort_I(a + i, indx + i, n - i);
}
 

void iquicksort(int a[], const int n) {
    int p, t;
    int i, j;
    if (n < 2)
        return;
    p = a[n / 2];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[i] < p)
            i++;
        while (p < a[j])
            j--;
        if (i >= j)
            break;
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    iquicksort(a, i);
    iquicksort(a + i, n - i);
}
 
void rquicksort(double a[], const int n) {
    double p, t;
    int i, j;
    if (n < 2)
        return;
    p = a[n / 2];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[i] < p)
            i++;
        while (p < a[j])
            j--;
        if (i >= j)
            break;
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    rquicksort(a, i);
    rquicksort(a + i, n - i);
}
 
