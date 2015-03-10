#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

#undef use_R_sort

#ifdef LONG_VECTOR_SUPPORT
#  define MY_SIZE_T R_xlen_t
#  define MY_LENGTH xlength
#else
#  define MY_SIZE_T int
#  define MY_LENGTH length
#endif

SEXP fastrank_(SEXP s_x);
SEXP fastrank_num_avg_(SEXP s_x);

// Registering the routines with R
static R_NativePrimitiveArgType fastrank_real_t[] = {
    REALSXP
};
static R_CMethodDef cMethods[] = {
    {"fastrank_",                 (DL_FUNC) &fastrank_,                 1,
        fastrank_real_t},
    {"fastrank_num_avg_", (DL_FUNC) &fastrank_num_avg_, 1, 
        fastrank_real_t},
    {NULL, NULL, 0}
};
static R_CallMethodDef callMethods[] = {
    {"fastrank_",                 (DL_FUNC) &fastrank_,                 1},
    {"fastrank_num_avg_", (DL_FUNC) &fastrank_num_avg_, 1},
    {NULL, NULL, 0}
};
void R_init_fastrank(DllInfo *info) {
    R_registerRoutines(info, cMethods, callMethods, NULL, NULL);
}

void fastrank_rquicksort_I_(double a[], MY_SIZE_T indx[], const MY_SIZE_T n);

// Fast calculation of ranks of a double vector, assigning average rank to ties
//
// This function quickly calculates the ranks of a double vector while assigning
// average rank to ties, and does absolutely no error checking on its arguments.
// If you call it incorrectly, it will probably crash R.
// 
// @param x            a vector of integers, there must be no NA values.
// 
// @return a vector the same length as \code{x} with double ranks of the 
//         corresponding elements in \code{x}.
//
// @seealso \code{\link{fastrank}}
// 
// @keywords internal
// 
// @export fastrank_num_avg_
// 
SEXP fastrank_num_avg_(SEXP s_x) {
    MY_SIZE_T n = MY_LENGTH(s_x);

    SEXP s_result = PROTECT(allocVector(REALSXP, n)); // ranks are doubles
    double *x = REAL(s_x), *result = REAL(s_result);
    MY_SIZE_T *indx = (MY_SIZE_T *) R_alloc(n, sizeof(MY_SIZE_T));
#ifdef use_R_sort
    R_qsort_I(x, indx, (MY_SIZE_T)1, (MY_SIZE_T)n);
#else
    for (MY_SIZE_T i = 0; i < n; ++i)  // pre-fill indx with index from 0..n-1
        indx[i] = i;
    fastrank_rquicksort_I_(x, indx, n);
#endif

    // Note: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions

    // rsort_with_index(s_x, indx, n);

    double *ranks = (double *) R_alloc(n, sizeof(double));
    double b = x[0];
    MY_SIZE_T ib = 0;
    MY_SIZE_T i;
    for (i = 1; i < n; ++i) {
        if (x[i] != b) { // consecutive numbers differ
            if (ib < i - 1) {
                // at least one previous tie, b=i-1, a=ib
                // sum of ranks = (b + a) * (b - a + 1) / 2;
                // avg_rank = sum / (b - a + 1);
                // simple_avg_rank = (b + a) / 2.0;
                // add 2 to compensate for index from 0
                double rnk = (i - 1 + ib + 2) / 2.0;
                for (MY_SIZE_T j = ib; j <= i - 1; ++j)
                    ranks[j] = rnk;
            } else {
                ranks[ib] = ib + 1;
            }
            b = x[i];
            ib = i;
        }
    }
    // now check leftovers
    if (ib == i - 1)  // last two were unique
        ranks[ib] = i;
    else {  // ended with ties
        double rnk = (i - 1 + ib + 2) / 2.0;
        for (MY_SIZE_T j = ib; j <= i - 1; ++j)
            ranks[j] = rnk;
    }
    //Rprintf("ranks of sorted vector:\n");
    //for (int i = 0; i < n; ++i) Rprintf("%.1f   ", ranks[i]); Rprintf("\n");

    // reorder ranks into the answer
    for (MY_SIZE_T i = 0; i < n; ++i)
        result[indx[i]] = ranks[i];
    //Rprintf("reorder ranks into original order of vector:\n");
    //for (int i = 0; i < n; ++i) Rprintf("%.1f   ", result[i]); Rprintf("\n");

    UNPROTECT(1);
    return s_result;
}

// quick_sort code modified from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort
// to return a vector of previous indices
void fastrank_rquicksort_I_ (double a[], MY_SIZE_T indx[], const MY_SIZE_T n) {
    double p, t;
    MY_SIZE_T i, j, it;
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
    fastrank_rquicksort_I_(a, indx, i);
    fastrank_rquicksort_I_(a + i, indx + i, n - i);
}
 


// Calculate rank of general vector, an alternative to calling \code{.Internal(rank(...))}
//
// @param x            a vector of integers, there must be no NA values.
// 
// @return a vector the same length as \code{x} with double ranks of the 
//         corresponding elements in \code{x}.
//
// @seealso \code{\link{fastrank}}
// 
// @keywords internal
// 
// @export fastrank_
// 
SEXP fastrank_(SEXP s_x) {
    Rprintf("General fastrank() in early stages\n");

    int n = length(s_x);
    Rprintf("length of s_x = %d\n", n);

    SEXP s_result = PROTECT(allocVector(REALSXP, n)); // ranks are doubles
    double *result = REAL(s_result);
    Rprintf("address of result = 0x%p\n", result);

    int *indx = (int *) R_alloc(n, sizeof(int));
    Rprintf("address of indx = 0x%p\n", indx);

    // The Rf_lang1() wrapper is **required**
    R_orderVector(indx, n, Rf_lang1(s_x), TRUE, FALSE);
    // indx[i] holds the index of the value in s_x that belongs in position i,
    // e.g., indx[0] holds the position in s_x of the first value
    //
    // Now get proper pointer and proper == comparison routine
    int *ix = (int*)NULL;  // INTEGER(s_x), types LGLSXP and INTSXP
    double *rx = (double*)NULL;  // REAL(s_x), type REALSXP
    SEXP *sx = (SEXP*)NULL;  // STRING_PTR(s_x), type STRSXP
    Rcomplex *cx = (Rcomplex*)NULL;  // COMPLEX(s_x), type CPLXSXP
    switch (TYPEOF(s_x)) {
        case LGLSXP:
        case INTSXP:
            ix = INTEGER(s_x); break;
        case REALSXP:
            rx = REAL(s_x); break;
        case STRSXP:
            sx = STRING_PTR(s_x); break;
        case CPLXSSP:
            cx = COMPLEX(s_x); break;
    }
// note stable comparison @ src/main/sort.c:1003, '<' compares index if '=='
// these do not need to be stable as they do not trigger rearrangement
#define XI(_i) x[indx[_i]]
#undef EQUAL
#undef TYPE
#define SEQUAL(_x, _y) (Scollate(sx[indx[_x]], sx[indx[_y]]) == 0)
#define CEQUAL(_x, _y) (cx[indx[_x]].r == cx[indx[_y]].r && cx[indx[_x]].i == cx[indx[_y]].i)
#define rank_avg_XI \
    TYPE b = XI(0); \
    int ib = 0; \
    int i; \
    for (i = 1; i < n; ++i) { \
        if (! EQUAL(XI(i), b)) { \
            if (ib < i - 1) { \
                double rnk = (i - 1 + ib + 2) / 2.0; \
                for (int j = ib; j <= i - 1; ++j) \
                    result[j] = rnk; \
            } else { \
                result[ib] = ib + 1; \
            } \
            b = XI(i); \
            ib = i; \
        } \
    } \
    if (ib == i - 1) \
        result[ib] = i; \
    else { \
        double rnk = (i - 1 + ib + 2) / 2.0; \
        for (int j = ib; j <= i - 1; ++j) \
            result[j] = rnk; \
    }

    switch (TYPEOF(s_x)) {
        case LGLSXP:
        case INTSXP:
            {
#define EQUAL(_x, _y) (_x == _y)
#define TYPE int
                TYPE* x = INTEGER(s_x);
                rank_avg_XI
#undef EQUAL
#undef TYPE
            }
            break;
        case REALSXP:
            {
#define EQUAL(_x, _y) (_x == _y)
#define TYPE double
                TYPE* x = REAL(s_x);
                rank_avg_XI
#undef EQUAL
#undef TYPE
            }
            break;
        case STRSXP:
            {
#define EQUAL(_x, _y) (Scollate(_x, _y) == 0)
#define TYPE SEXP
                TYPE* x = STRING_PTR(s_x);
                rank_avg_XI
#undef EQUAL
#undef TYPE
            }
            break;
        case CPLXSSP:
            {
#define EQUAL(_x, _y) (_x.r == _y.r && _x.i == _y.i)
#define TYPE Rcomplex
                TYPE* x = COMPLEX(s_x); break;
                rank_avg_XI
#undef EQUAL
#undef TYPE
            }
            break;
    }

// step through indx[] in order to get the index in s_x (ix[], rx[], etc)
// in order.  the i we remember is the index into indx[], but the position
// we check for equality and the rank position we modify is indx[i]
// TODO: make this a #define macro


    UNPROTECT(1);
    return s_result;
}

