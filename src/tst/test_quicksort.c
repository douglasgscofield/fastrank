#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define MY_SIZE_T size_t

#define exch(_type, _a, _b) { _type t = _a; _a = _b; _b = t; }

// quicksort
static void fr_quicksort_int_i_ (const int * a, MY_SIZE_T indx[], const MY_SIZE_T n, const MY_SIZE_T crit_size);
static void fr_quicksort2_int_i_ (const int * a, MY_SIZE_T indx[], const MY_SIZE_T n, const MY_SIZE_T crit_size);
static void fr_quicksort_double_i_ (const double * a, MY_SIZE_T indx[], const MY_SIZE_T n, const MY_SIZE_T crit_size);
// shellsort
static void fr_shellsort_double_i_ (double * const a, MY_SIZE_T indx[], const MY_SIZE_T n);
// vector of random doubles
float* vrandr(const int low, const int high, const int n);
int* vrandi(const int low, const int high, const int n);

// todo: modify quicksort to use the 3-way partition from Sedgwick's
// Quicksort is optimal presentation

// Sedgwick's Quicksort with 3-pay partition, continue to modify for
// my purposes.
#undef __EQUAL
#undef __LESSER
#define __EQUAL(__A, __B) (__A == __B)
#define __LESSER(__A, __B) (__A < __B)
#define SWAP(__T, __A, __B) { __T t = __A; __A = __B; __B = t; }
static void quicksort_i_(const int *     a, 
                         MY_SIZE_T       indx[],
                         const MY_SIZE_T n,
                         const MY_SIZE_T crit_size) {
    MY_SIZE_T i, j, p, q, k;

    if (n <= crit_size) {
        for (i = 1; i < n; ++i) {
            MY_SIZE_T it = indx[i];
            for (j = i; j > 0 && __LESSER(a[it], a[indx[j - 1]]); --j) {
                indx[j] = indx[j - 1];
            }
            indx[j] = it;
        }
        return;
    }

    int pvt = a[indx[n - 1]];
    for (i = 0; __LESSER(a[indx[i]], pvt); ++i);
    for (j = n - 2; __LESSER(pvt, a[indx[j]]) && j > 0; --j);
    p = 0;
    q = n - 1;
    if (i < j) {
        SWAP(MY_SIZE_T, indx[i], indx[j]);
        if (__EQUAL(a[indx[i]], pvt)) { 
            SWAP(MY_SIZE_T, indx[p], indx[i]);
        } 
        if (__EQUAL(a[indx[j]], pvt)) {
            q--;
            SWAP(MY_SIZE_T, indx[q], indx[j]);
        }
        for (;;) {
            while (__LESSER(a[indx[++i]], pvt)) ;  
            while (__LESSER(pvt, a[indx[--j]])) {  
                if (j == 0) break; 
            }
            if (i >= j) break;
            SWAP(MY_SIZE_T, indx[i], indx[j]);
            if (__EQUAL(a[indx[i]], pvt)) { 
                if (p) p++; 
                SWAP(MY_SIZE_T, indx[p], indx[i]);
            } 
            if (__EQUAL(a[indx[j]], pvt)) {
                q--;
                SWAP(MY_SIZE_T, indx[q], indx[j]);
            }
        }
    }
    SWAP(MY_SIZE_T, indx[i], indx[n - 1]); 
    j = i - 1;
    i++;
    for (k = 0; k < p; k++, j--) {
        SWAP(MY_SIZE_T, indx[k], indx[j]); 
    }
    for (k = n - 2; k > q; k--, i++) {
        SWAP(MY_SIZE_T, indx[i], indx[k]);
    }
    /*
    printf("n=%2d  a[0..%2d]: ", n, n - 1);
    for (int t = 0; t < n; ++t) printf("[%2d] %2d  ", t, a[t]);
    printf("\n");
    /**/

    quicksort_i_(a, indx,     j + 1, crit_size);
    quicksort_i_(a, indx + i, n - i, crit_size);
}


void quicksort(int a[], int n) {

    unsigned int i, j, p, q, k;

    if (n <= 1) return; 

    int pvt = a[n - 1];
    for (i = 0; a[i] < pvt; ++i);
    for (j = n - 2; pvt < a[j] && j > 0; --j);
    p = 0;
    q = n - 1;
    if (i < j) {
        exch(int, a[i], a[j]);
        if (a[i] == pvt) { 
            exch(int, a[p], a[i]);
        } 
        if (a[j] == pvt) {
            q--;
            exch(int, a[q], a[j]);
        }
        for (;;) {
            while (a[++i] < pvt) ;  
            while (pvt < a[--j]) {  
                if (j == 0) break; 
            }
            if (i >= j) break;
            exch(int, a[i], a[j]);
            if (a[i] == pvt) { 
                if (p) p++; 
                exch(int, a[p], a[i]);
            } 
            if (a[j] == pvt) {
                q--;
                exch(int, a[q], a[j]);
            }
        }
    }

    exch(int, a[i], a[n - 1]); 

    j = i - 1;
    i++;

    for (k = 0; k < p; k++, j--) {
        exch(int, a[k], a[j]); 
    }
    for (k = n - 2; k > q; k--, i++) {
        exch(int, a[i], a[k]);
    }
    /*
    printf("n=%2d  a[0..%2d]: ", n, n - 1);
    for (int t = 0; t < n; ++t) printf("[%2d] %2d  ", t, a[t]);
    printf("\n");
    /**/

    quicksort(    a, j + 1);
    quicksort(a + i, n - i);
}

void quicksort2(int a[], int l, int r) {
    int i = l-1, j = r, p = l-1, q = r, k;
    int v = a[r];
    if (r <= l) return; 
    for (;;) {
        while (a[++i] < v) ;
        while (v < a[--j]) if (j == l) break; 
        if (i >= j) break;
        exch(int, a[i], a[j]);
        if (a[i] == v) { p++; exch(int, a[p], a[i]); } 
        if (v == a[j]) { q--; exch(int, a[j], a[q]); }
    }
    exch(int, a[i], a[r]); 
    j = i-1; 
    i = i+1;
    for (k = l; k < p; k++, j--) exch(int, a[k], a[j]); 
    for (k = r-1; k > q; k--, i++) exch(int, a[i], a[k]);
    quicksort2(a, l, j);
    quicksort2(a, i, r);
}


static void fr_quicksort_int_i_ (const int * a,
                                    MY_SIZE_T indx[],
                                    const MY_SIZE_T n,
                                    const MY_SIZE_T crit_size) {
    int p;
    MY_SIZE_T i, j, it;
    if (n <= crit_size) {
        // insertion sort shortcut
        for (i = 1; i < n; ++i) {
            it = indx[i];
            for (j = i; j > 0 && a[indx[j - 1]] > a[it]; --j) {
                indx[j] = indx[j - 1];
            }
        }
        return;
    }
    p = a[indx[n / 2]];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[indx[i]] < p) i++;
        while (p < a[indx[j]]) j--;
        if (i >= j) break;
        it = indx[i]; indx[i] = indx[j]; indx[j] = it;
    }
    fr_quicksort_int_i_(a, indx,     i    , crit_size);
    fr_quicksort_int_i_(a, indx + i, n - i, crit_size);
}



static void fr_quicksort_double_i_ (const double * a,
                                    MY_SIZE_T indx[],
                                    const MY_SIZE_T n,
                                    const MY_SIZE_T crit_size) {
    double p;
    MY_SIZE_T i, j, it;
    if (n < crit_size) {
        // insertion sort shortcut
        for (i = 1; i < n; ++i) {
            it = indx[i];
            for (j = i; j > 0 && a[indx[j - 1]] > a[it]; --j) {
                indx[j] = indx[j - 1];
            }
        }
        return;
    }
    p = a[indx[n / 2]];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[indx[i]] < p) i++;
        while (p < a[indx[j]]) j--;
        if (i >= j) break;
        it = indx[i]; indx[i] = indx[j]; indx[j] = it;
    }
    fr_quicksort_double_i_(a, indx,     i    , crit_size);
    fr_quicksort_double_i_(a, indx + i, n - i, crit_size);
}



int main(int argc, char* argv[]) {
    //srand(42);
    srand(time(NULL));
    //for (int i = 0; i < argc; ++i)
    //    printf("argv[%d]: %s   ", i, argv[i]);
    //putchar('\n');
    //printf("breaking ties with average rank\n");
    int max = atoi(argv[1]);
    int n = atoi(argv[2]);
    printf("random vector:  max: %d   n: %d  sum:", max, n);
    //float* x = vrandr(10, max, n);
    int* v = vrandi(10, max, n);
    double sum = 0; for (int i = 0; i < n; ++i) sum += v[i]; printf("%g\n", sum);
    int* w = (int*) malloc(n * sizeof(int));
    int* x = (int*) malloc(n * sizeof(int));
    int* y = (int*) malloc(n * sizeof(int));
    int* z = (int*) malloc(n * sizeof(int));
    for (int i = 0; i < n; ++i) z[i] = y[i] = x[i] = w[i] = v[i];
    //printf("sorted vector with indx:\n");
    MY_SIZE_T* indx = (MY_SIZE_T *) malloc(n * sizeof(MY_SIZE_T));
    MY_SIZE_T* indx2 = (MY_SIZE_T *) malloc(n * sizeof(MY_SIZE_T));
    for (MY_SIZE_T i = 0; i < n; ++i) indx2[i] = indx[i] = i;
    /*
    printf("before sort: x[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%d   ", x[i]); putchar('\n');
    */
    //for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
    //fr_quicksort_double_i_(x, indx, n, 0);
    //printf("before fr_quicksort_int_i: w[] and indx[]\n");
    //for (int i = 0; i < n; ++i) printf("%d   indx[%d] = %d   w[indx[%d]] = %d\n", i, i, indx[i], i, w[indx[i]]);
    fr_quicksort_int_i_(w, indx, n, 1);
    /*
    fr_quicksort2_int_i_(z, indx2, n, 1);
    printf("comparing fr_quicksort_int_i_ w[] with fr_quicksort2_int_i_ z[]...\n");
    for (int i = 0; i < n; ++i)
        if (w[indx[i]] != z[indx2[i]])
            printf("mismatch: indx[%d] = %d, w[indx[%d]] = %d   indx2[%s] = %d, z[indx[%d]] = %d\n",
                    i, indx[i], i, w[indx[i]], i, indx2[i], i, z[indx[i]]);
    */
    //printf("after fr_quicksort_int_i: w[] and indx[]\n");
    //for (int i = 0; i < n; ++i) printf("%d   indx[%d] = %d   w[indx[%d]] = %d\n", i, i, indx[i], i, w[indx[i]]);
    //quicksort(x, n);
    quicksort2(x, 0, n - 1);
    printf("comparing fr_quicksort_int_i_ w[] with quicksort2 x[]...\n");
    for (int i = 0; i < n; ++i)
        if (w[indx[i]] != x[i])
            printf("mismatch: indx[%d] = %d, w[indx[%d]] = %d    x[%d] = %d\n",
                    i, indx[i], i, w[indx[i]], i, x[i]);
    quicksort(y, n);
    printf("comparing quicksort2 x[] with quicksort y[]...\n");
    for (int i = 0; i < n; ++i)
        if (x[i] != y[i])
            printf("mismatch: x[%d] = %d    y[%d] = %d\n",
                    i, x[i], i, y[i]);
    quicksort_i_(z, indx2, n, 1);
    printf("comparing quicksort_i_ z[] with quicksort2 x[]...\n");
    for (int i = 0; i < n; ++i)
        if (z[indx2[i]] != x[i])
            printf("mismatch: indx2[%d] = %d, z[indx2[%d]] = %d    x[%d] = %d\n",
                    i, indx2[i], i, z[indx2[i]], i, x[i]);
    return 0;
    /*
    printf("after quicksort: x[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%d   ", x[i]); putchar('\n');
    printf("after quicksort: y[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%d   ", y[i]); putchar('\n');
    //for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
    */
    /*
    for (int i = 0; i < n; ++i) y[i] = v[i];
    quicksort2(y, 0, n - 1);
    printf("after quicksort2: y[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%d   ", y[i]); putchar('\n');
    */
    return 0;
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
    for (int i = 0; i < n; ++i) printf(" %.0f  ", x[i]); putchar('\n');
    for (int i = 0; i < n; ++i) printf("%.1f  ", ranks[i]); putchar('\n');
}


int* vrandi(const int low, const int high, const int n) { 
    if (n < 1) 
        return(0);
    int* result = (int *) malloc (n * sizeof(int));
    //srand(42);
    for (int i = 0; i < n; ++i) 
        result[i] = (high - low + 1) * (rand() / (RAND_MAX + 1.0)) + low; 
    return(result);
} 



float* vrandr(const int low, const int high, const int n) { 
    if (n < 1) 
        return((float*) 0);
    int* result = (int *) malloc (n * sizeof(int));
    (void)rand();
    for (int i = 0; i < n; ++i) 
        result[i] = (high - low + 1) * (rand() / (RAND_MAX + 1.0)) + low; 
    float* rresult = (float*) malloc(n * sizeof(float));
    for (int i = 0; i < n; ++i) 
        rresult[i] = (float)result[i];
    free(result);
    return(rresult);
} 



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
            it = indx[i];
            //t = a[i];
            for (j = i; j >= gap && a[indx[j - gap]] > a[it]; j -= gap) {
                //a[j] = a[j - gap];
                indx[j] = indx[j - gap];
            }
            indx[j] = it;
        }
    }
}

