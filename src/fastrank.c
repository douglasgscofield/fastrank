#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

/* include inline debug statements? */
#define DEBUG 1

#undef use_R_sort

#ifdef LONG_VECTOR_SUPPORT
#  define MY_SIZE_T R_xlen_t
#  define MY_LENGTH xlength
/* R_orderVector() types */
#  define ROV_SIZE_T int
#  define ROV_LENGTH length
#else
#  define MY_SIZE_T int
#  define MY_LENGTH length
/* R_orderVector() types */
#  define ROV_SIZE_T int
#  define ROV_LENGTH length
#endif

// need to add ties.method, also the registration needs to change for that
// API routines
SEXP fastrank_(SEXP s_x, SEXP s_tm, SEXP s_find);
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

// internal routines
static void fastrank_rquicksort_I_(double a[], MY_SIZE_T indx[], const MY_SIZE_T n);



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

    SEXP s_result = PROTECT(allocVector(REALSXP, n)); // doubles if "average"
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
                ranks[ib] = (double)(ib + 1);
            }
            b = x[i];
            ib = i;
        }
    }
    // now check leftovers
    if (ib == i - 1)  // last two were unique
        ranks[ib] = (double)i;
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


//
// quick_sort code modified from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort
// to return a vector of previous indices
static void fastrank_rquicksort_I_ (double a[],
                                    MY_SIZE_T indx[],
                                    const MY_SIZE_T n) {
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
SEXP fastrank_(SEXP s_x, SEXP s_tm, SEXP s_find) {
    if (DEBUG) Rprintf("General fastrank(), development in early stages\n");

    int find_method = INTEGER(s_find)[0];
    if (find_method < 1 || find_method > 2)
        error("'find' must be 1 or 2");

    /* Check vector to rank.
     *
     * Is the check here enough?  The check in R code is probably slower,
     * and checking in the switch() below is too late, after the 
     * R_orderVector() call
     */
    if (TYPEOF(s_x) == STRSXP)
        error("'character' values not allowed, consider base R 'rank' or converting to a compatible type");
    ROV_SIZE_T n = ROV_LENGTH(s_x);
    if (DEBUG) Rprintf("length of s_x = %d\n", n);

    /* Now process ties.method */
    if (TYPEOF(s_tm) != STRSXP)
        error("ties.method must be one of \"average\", \"first\", \"random\", \"max\", \"min\"");
    const char* tm = CHAR(STRING_ELT(s_tm, 0));

    enum { TIES_ERROR = 0, TIES_AVERAGE, TIES_FIRST, TIES_RANDOM,
           TIES_MAX, TIES_MIN } ties_method;

    if (find_method == 1) {

        /* method 1, is this faster than method 2? is it preferred to method 2? */
        switch(tm[0]) {
        case 'a':
            ties_method = TIES_AVERAGE; break;
        case 'f':
            ties_method = TIES_FIRST; break;
        case 'r':
            ties_method = TIES_RANDOM; break;
        case 'm':
            switch(tm[1]) {
            case 'a':
                ties_method = TIES_MAX; break;
            case 'i':
                ties_method = TIES_MIN; break;
            default:
                ties_method = TIES_ERROR;
                error("unknown ties.method");
                break;
            }
        default:
            ties_method = TIES_ERROR;
            error("unknown ties.method");
            break;
        }

    } else {

        /* method 2 */
        if (! strcmp(tm, "average"))
            ties_method = TIES_AVERAGE;
        else if (! strcmp(tm, "first"))
            ties_method = TIES_FIRST;
        else if (! strcmp(tm, "random"))
            ties_method = TIES_RANDOM;
        else if (! strcmp(tm, "max"))
            ties_method = TIES_MAX;
        else if (! strcmp(tm, "min"))
            ties_method = TIES_MIN;
        else {
            ties_method = TIES_ERROR;
            error("unknown ties.method");
        }

    }

    ROV_SIZE_T *indx = (ROV_SIZE_T *) R_alloc(n, sizeof(ROV_SIZE_T));
    if (DEBUG) Rprintf("address of indx = 0x%p\n", indx);

    // The Rf_lang1() wrapper is **required**, '1' is the number of args
    R_orderVector(indx, n, Rf_lang1(s_x), TRUE, FALSE);
    // indx[i] holds the index of the value in s_x that belongs in position i,
    // e.g., indx[0] holds the position in s_x of the first value

    /* Points to the return value, which is created below */
    SEXP s_ranks = NULL;

    // Now do rank calculations with type-specific pointers and comparisons
    // note this uses ROV_SIZE_T for vector indexing and ROV_LENGTH for
    // determing vector length.  R_orderVector returns int*, but it might not
    // in future.

/* for accessing the value of the vector through the index entry */
#define XI(_i_) x[indx[_i_]]
/* comparison for equality for each vector type */
#undef EQUAL
/* general type of vector passed in */
#undef TYPE
/* general R API conversion for type of vector passed in */
#undef TCONV
/* ensure no collisions with macro arg names */
#undef __loc__
/* ties method, see #define's below */
#undef __TIES__
/* type of vector passed in */
#undef __TYPE
/* R API conversion for type of vector passed in */
#undef __TCONV
/* type of rank returned */
#undef __RTYPE
/* R API type of rank returned */
#undef __R_RTYPE
/* R API conversion for type of rank returned */
#undef __R_TCONV

/* ties' rank is the minimum of their ranks */
#define FR_ties_min(__RTYPE, __loc__) \
    { \
    __RTYPE rnk = (__RTYPE)(ib + 1); \
    if (DEBUG) Rprintf("min, ranks[%d .. %d] <- %f   " __loc__ "\n", ib, i - 1, rnk); \
    for (ROV_SIZE_T j = ib; j <= i - 1; ++j) { \
        ranks[j] = rnk; \
    } \
    }

/* ties' rank is the maximum of their ranks */
#define FR_ties_max(__RTYPE, __loc__) \
    { \
    __RTYPE rnk = (__RTYPE)i; \
    if (DEBUG) Rprintf("max, ranks[%d .. %d] <- %f   " __loc__ "\n", ib, i - 1, rnk); \
    for (ROV_SIZE_T j = ib; j <= i - 1; ++j) { \
        ranks[j] = rnk; \
    } \
    }

/* ties' rank is the average of their ranks */
#define FR_ties_average(__RTYPE, __loc__) \
    { \
    __RTYPE rnk = (i - 1 + ib + 2) / 2.0; \
    if (DEBUG) Rprintf("average, ranks[%d .. %d] <- %f   " __loc__ "\n", ib, i - 1, rnk); \
    for (ROV_SIZE_T j = ib; j <= i - 1; ++j) { \
        ranks[j] = rnk; \
    } \
    }

/* ties' rank is consecutive on their order */
#define FR_ties_first(__RTYPE, __loc__) \
    { \
    Rprintf("ties.method == 'first' only truly correct when the sort is stable"); \
    for (ROV_SIZE_T j = ib; j <= i - 1; ++j) { \
        __RTYPE rnk = (__RTYPE)(j + 1); \
        if (DEBUG) Rprintf("first, ranks[%d] <- %f  " __loc__ "\n", j, rnk); \
        ranks[j] = rnk; \
    } \
    }

/* ties' rank is a random shuffling of their order */
#define FR_ties_random(__RTYPE, __loc__) \
    { \
    ROV_SIZE_T tn = i - ib; \
    ROV_SIZE_T* t = (ROV_SIZE_T*) R_alloc(tn, sizeof(ROV_SIZE_T)); \
    ROV_SIZE_T j; \
    for (j = 0; j < tn; ++j) \
        t[j] = j; \
    for (j = ib; j < i - 1; ++j) { \
        ROV_SIZE_T k = (ROV_SIZE_T)(tn * unif_rand()); \
        __RTYPE rnk = (__RTYPE)(t[k] + ib + 1); \
        if (DEBUG) Rprintf("random, rank[%d] <- %f  " __loc__ "\n", j, rnk); \
        ranks[j] = rnk; \
        t[k] = t[--tn]; \
    } \
    __RTYPE rnk = (__RTYPE)(t[0] + ib + 1); \
    if (DEBUG) Rprintf("random, rank[%d] <- %f  " __loc__ "\n", j, rnk); \
    ranks[j] = rnk; \
    }

 

#define FR_rank(__TIES__, __TYPE, __TCONV, __RTYPE, __R_RTYPE, __R_TCONV) \
    { \
    s_ranks = PROTECT(allocVector(__R_RTYPE, n)); \
    __RTYPE* ranks = __R_TCONV(s_ranks); \
    if (DEBUG) Rprintf("address of ranks = 0x%p\n", ranks); \
    __TYPE* x = __TCONV(s_x); \
    __TYPE b = XI(0); \
    ROV_SIZE_T ib = 0; \
    ROV_SIZE_T i; \
    if (DEBUG) Rprintf("ib = %d\n", ib); \
    for (i = 1; i < n; ++i) { \
        if (! EQUAL(XI(i), b)) { \
            if (DEBUG) Rprintf("XI(%d) != b\n", ib, b); \
            if (ib < i - 1) { \
                __TIES__(__RTYPE, "MID") \
            } else { \
                if (DEBUG) \
                    Rprintf("ranks[%d] <- %f\n MID", ib, (__RTYPE)(ib + 1)); \
                ranks[ib] = (__RTYPE)(ib + 1); \
            } \
            b = XI(i); \
            ib = i; \
            if (DEBUG) Rprintf("ib = %d\n", ib); \
        } \
    } \
    if (ib == i - 1) {\
        if (DEBUG) Rprintf("ranks[%d] <- %f FIN\n", ib, (__RTYPE)(ib + 1)); \
        ranks[ib] = (__RTYPE)i; \
    } else { \
        __TIES__(__RTYPE, "FIN") \
    } \
    }

    /* now decide which way to go and do it! */

    switch (TYPEOF(s_x)) {
    case LGLSXP:
    case INTSXP:
        {
#define EQUAL(_x, _y) (_x == _y)
#define TYPE int
#define TCONV INTEGER
            switch(ties_method) {
            case TIES_AVERAGE:
                FR_rank(FR_ties_average, TYPE, TCONV, double, REALSXP, REAL)
                break;
            case TIES_FIRST:
                FR_rank(FR_ties_first, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
                break;
            case TIES_RANDOM:
                GetRNGstate();
                FR_rank(FR_ties_random, TYPE, TCONV, int, INTSXP, INTEGER)
                PutRNGstate();
                break;
            case TIES_MAX:
                FR_rank(FR_ties_max, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
            case TIES_MIN:
                FR_rank(FR_ties_min, TYPE, TCONV, int, INTSXP, INTEGER)
            default:
                error("unknown 'ties.method', should never be reached");
                break;
            }
#undef EQUAL
#undef TYPE
#undef TCONV
        }
        break;
    case REALSXP:
        {
#define EQUAL(_x, _y) (_x == _y)
#define TYPE double
#define TCONV REAL
            switch(ties_method) {
            case TIES_AVERAGE:
                FR_rank(FR_ties_average, TYPE, TCONV, double, REALSXP, REAL)
                break;
            case TIES_FIRST:
                FR_rank(FR_ties_first, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
            case TIES_RANDOM:
                GetRNGstate();
                FR_rank(FR_ties_random, TYPE, TCONV, int, INTSXP, INTEGER)
                PutRNGstate();
                break;
            case TIES_MAX:
                FR_rank(FR_ties_max, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
            case TIES_MIN:
                FR_rank(FR_ties_min, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
            default:
                error("unknown 'ties.method', should never be reached");
                break;
            }
#undef EQUAL
#undef TYPE
#undef TCONV
        }
        break;
    case CPLXSXP:
        {
#define EQUAL(_x, _y) (_x.r == _y.r && _x.i == _y.i)
#define TYPE Rcomplex
#define TCONV COMPLEX
            switch(ties_method) {
            case TIES_AVERAGE:
                FR_rank(FR_ties_average, TYPE, TCONV, double, REALSXP, REAL)
                break;
            case TIES_FIRST:
                FR_rank(FR_ties_first, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
            case TIES_RANDOM:
                GetRNGstate();
                FR_rank(FR_ties_random, TYPE, TCONV, int, INTSXP, INTEGER)
                PutRNGstate();
                break;
            case TIES_MAX:
                FR_rank(FR_ties_max, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
            case TIES_MIN:
                FR_rank(FR_ties_min, TYPE, TCONV, int, INTSXP, INTEGER)
                break;
            default:
                error("unknown 'ties.method', should never be reached");
                break;
            }
#undef EQUAL
#undef TYPE
#undef TCONV
        }
        break;
    default:
        error("'x' is not an integer, numeric or complex vector");
        break;
    }

    UNPROTECT(1);
    return s_ranks;
}

