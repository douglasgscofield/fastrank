#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define MY_SIZE_T size_t

static void fr_shellsort_double_i_ (double * const a, MY_SIZE_T indx[], const MY_SIZE_T n);

#define N_GAPS 20
//#else
//#define N_GAPS 16
//#endif
/* Using Tokuda gaps here */
static const MY_SIZE_T shell_gaps [N_GAPS + 1] = {
#ifdef LONG_VECTOR_SUPPORT
    19903198L,  8845866L,  3931496L,  1747331L,
#endif
    776591L, 345152L, 153401L, 68178L, 30301L,
    13467L, 5985L, 2660L, 1182L, 525L,
    233L, 103L, 46L, 20L, 9L,
    4L, 1L
};

static void fr_shellsort_double_i_ (double * const a,
                                    MY_SIZE_T indx[],
                                    const MY_SIZE_T n) {
    MY_SIZE_T i, it, j, ig, gap;
    double t;
    for (ig = 0; shell_gaps[ig] >= n; ++ig);
    for (; ig < N_GAPS; ++ig) {
        gap = shell_gaps[ig];
        printf("ig = %d    gap = %d   n = %d\n", ig, gap, n);
        for (int q = 0; q < n; ++q) printf("%d   ", indx[q]); printf("\n");
        for (i = gap; i < n; ++i) {
            t = a[i];
            for (j = i; j >= gap && a[j - gap] > t; j -= gap) {
                a[j] = a[j - gap];
            }
            a[j] = t;
        }
    }
}


double* vrandr(const int low, const int high, const int n);

int main(int argc, char* argv[]) {
    srand(time(NULL));
    for (int i = 0; i < argc; ++i)
        printf("argv[%d]: %s   ", i, argv[i]);
    putchar('\n');
    printf("breaking ties with average rank\n");
    int max = atoi(argv[1]);
    int n = atoi(argv[2]);
    printf("random vector:  max: %d   n: %d\n", max, n);
    double* x = vrandr(10, max, n);
    //for (int i = 0; i < n; ++i) printf("%d   ", x[i]); putchar('\n');
    //printf("sorted vector with indx:\n");
    MY_SIZE_T* indx = (MY_SIZE_T *) malloc(n * sizeof(MY_SIZE_T));
    for (MY_SIZE_T i = 0; i < n; ++i) indx[i] = i;
    printf("before fr_shellsort_double_i_: x[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%.0f   ", x[i]); putchar('\n');
    for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
    fr_shellsort_double_i_(x, indx, n);
    printf("after fr_shellsort_double_i_: x[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%.0f   ", x[i]); putchar('\n');
    for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
    double* ranks = (double*) malloc(n * sizeof(double));
    // now to break ties, step through sorted values
    double b = x[indx[0]];
    int ib = 0;
    int i;
    for (i = 1; i < n; ++i) {
        if (x[indx[i]] != b) { // consecutive numbers differ
            if (ib < i - 1) {
                // at least one previous tie, b=i-1, a=ib
                // sum of ranks = (b + a) * (b - a + 1) / 2;
                // avg_rank = sum / (b - a + 1);
                // simple_avg_rank = (b + a) / 2.0;
                // add 2 to compensate for index from 0
                double rnk = (i - 1 + ib + 2) / 2.0;
                for (int j = ib; j <= i - 1; ++j)
                    ranks[indx[j]] = rnk;
            } else {
                ranks[indx[ib]] = ib + 1;
            }
            b = x[indx[i]];
            ib = i;
        }
    }
    // now i == n, check leftovers
    if (ib == i - 1)  // last two were unique
        ranks[indx[ib]] = i;
    else {  // ended with ties
        double rnk = (i - 1 + ib + 2) / 2.0;
        for (int j = ib; j <= i - 1; ++j)
            ranks[indx[j]] = rnk;
    }
    printf("after average ranking: x[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%.0f   ", x[i]); putchar('\n');
    for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
}


double* vrandr(const int low, const int high, const int n) { 
    if (n < 1) 
        return((double*) 0);
    int* result = (int *) malloc (n * sizeof(int));
    (void)rand();
    for (int i = 0; i < n; ++i) 
        result[i] = (high - low + 1) * (rand() / (RAND_MAX + 1.0)) + low; 
    double* rresult = (double*) malloc(n * sizeof(double));
    for (int i = 0; i < n; ++i) 
        rresult[i] = (double)result[i];
    free(result);
    return(rresult);
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
 


