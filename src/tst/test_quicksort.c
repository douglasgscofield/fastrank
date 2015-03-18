#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define MY_SIZE_T size_t

// quicksort
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
#define exch(_a,_b) { int t = _a; _a = _b; _b = t; }
void quicksort(int aa[], int nn, int a[], int l, int r) {
    int ll = 0; int rr = nn - 1;
    int ii = ll-1, jj = rr, pp = ll-1, qq = rr, kk;
    int vv = aa[rr];
    if (rr <= ll) return; 

    int i = l-1, j = r, p = l-1, q = r, k;
    int v = a[r];
    if (r <= l) return; 

    for (;;) {
        while (aa[++ii] < vv) ;
        while (vv < aa[--jj]) if (jj == ll) break; 
        if (ii >= jj) break;
        exch(aa[ii], aa[jj]);
        if (aa[ii] == vv) { pp++; exch(aa[pp], aa[ii]); } 
        if (vv == aa[jj]) { qq--; exch(aa[jj], aa[qq]); }
    }

    for (;;) {
        while (a[++i] < v) ;
        while (v < a[--j]) if (j == l) break; 
        if (i >= j) break;
        exch(a[i], a[j]);
        if (a[i] == v) { p++; exch(a[p], a[i]); } 
        if (v == a[j]) { q--; exch(a[j], a[q]); }
    }

    exch(aa[ii], aa[rr]); 
    jj = ii-1; 
    ii = ii+1;
    for (kk = ll; kk < pp; kk++, jj--) exch(aa[kk], aa[jj]); 
    for (kk = rr-1; kk > qq; kk--, ii++) exch(aa[ii], aa[kk]);

    exch(a[i], a[r]); 
    j = i-1; 
    i = i+1;
    for (k = l; k < p; k++, j--) exch(a[k], a[j]); 
    for (k = r-1; k > q; k--, i++) exch(a[i], a[k]);

    quicksort(     aa,      jj,  a, l, j);
    quicksort(aa + ii, nn - ii,  a, i, r);
}

void quicksort2(int a[], int l, int r) {
    int i = l-1, j = r, p = l-1, q = r, k;
    int v = a[r];
    if (r <= l) return; 
    for (;;) {
        while (a[++i] < v) ;
        while (v < a[--j]) if (j == l) break; 
        if (i >= j) break;
        exch(a[i], a[j]);
        if (a[i] == v) { p++; exch(a[p], a[i]); } 
        if (v == a[j]) { q--; exch(a[j], a[q]); }
    }
    exch(a[i], a[r]); 
    j = i-1; 
    i = i+1;
    for (k = l; k < p; k++, j--) exch(a[k], a[j]); 
    for (k = r-1; k > q; k--, i++) exch(a[i], a[k]);
    quicksort2(a, l, j);
    quicksort2(a, i, r);
}

static void fr_quicksort2_int_ (int * a,
                                   MY_SIZE_T indx[],
                                   const MY_SIZE_T n,
                                   const MY_SIZE_T crit_size) {
    if (n <= 1)
        return;
    MY_SIZE_T i = 0, p = 0, k;
    MY_SIZE_T j = n - 1;
    MY_SIZE_T q = j;
    int val = a[j];
    int tmp;
    for (;;) {
        while (a[i] < val) i++;
        while (val < a[j]) {
            j--;
            if (j == 1) break;  // or is this j == 0
        }
        if (i >= j) break;
        tmp = a[i]; a[i] = a[j]; a[j] = tmp;
        if (a[i] == val) { 
            p++;
            tmp = a[i]; a[i] = a[p]; a[p] = tmp;
        }
        if (val == a[j]) {
            q--;
            tmp = a[j]; a[j] = a[q]; a[q] = tmp;
        }
    }
    tmp = a[i]; a[i] = a[n - 1]; a[n - 1] = tmp;
    for (k = 0;  k < p; k++, j--) {
        tmp = a[j]; a[j] = a[k]; a[k] = tmp;
    }
    for (k = n - 1;  k > q; k--, i++) {
        tmp = a[k]; a[k] = a[i]; a[i] = tmp;
    }
    fr_quicksort2_int_ (a,     indx,     j,     crit_size);
    fr_quicksort2_int_ (a + i, indx + i, n - i, crit_size);
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
    srand(time(NULL));
    //for (int i = 0; i < argc; ++i)
    //    printf("argv[%d]: %s   ", i, argv[i]);
    //putchar('\n');
    //printf("breaking ties with average rank\n");
    int max = atoi(argv[1]);
    int n = atoi(argv[2]);
    printf("random vector:  max: %d   n: %d\n", max, n);
    //float* x = vrandr(10, max, n);
    int* x = vrandi(10, max, n);
    int* y = (int*) malloc(n * sizeof(int));
    for (int i = 0; i < n; ++i) y[i] = x[i];
    //printf("sorted vector with indx:\n");
    MY_SIZE_T* indx = (MY_SIZE_T *) malloc(n * sizeof(MY_SIZE_T));
    for (MY_SIZE_T i = 0; i < n; ++i) indx[i] = i;
    printf("before sort: x[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%d   ", x[i]); putchar('\n');
    //for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
    //fr_quicksort2_int_(x, indx, n, 0);
    //quicksort(x, n);
    quicksort(x, n, y, 0, n - 1);
    //quicksort2(x, 0, n - 1);
    printf("after sort: x[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%d   ", x[i]); putchar('\n');
    printf("after sort: y[] and indx[]\n");
    for (int i = 0; i < n; ++i) printf("%d   ", y[i]); putchar('\n');
    //for (int i = 0; i < n; ++i) printf("%d    ", indx[i]); putchar('\n');
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
    srand(42);
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

