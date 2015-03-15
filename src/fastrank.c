#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

/* include inline debug statements? */
#define DEBUG 0

#ifdef LONG_VECTOR_SUPPORT
#  define MY_SIZE_T R_xlen_t
#  define MY_LENGTH xlength
#else
#  define MY_SIZE_T int
#  define MY_LENGTH length
#endif



/* FUNCTION PROTOTYPE DECLARATION *********************************/

static void fr_shellsort_double_i_ (const double * a, MY_SIZE_T indx[], const MY_SIZE_T n,
                                    const MY_SIZE_T * the_gaps, const int N_GAPS);
static void fr_shellsort_c_double_i_(const double * a, MY_SIZE_T indx[], const MY_SIZE_T n);
static void fr_shellsort_s_double_i_(const double * a, MY_SIZE_T indx[], const MY_SIZE_T n);
static void fr_shellsort_t_double_i_(const double * a, MY_SIZE_T indx[], const MY_SIZE_T n);

static void fr_quicksort_double_i_ (const double * a, MY_SIZE_T indx[], const MY_SIZE_T n);

SEXP fastrank_(SEXP s_x, SEXP s_tm, SEXP s_sort);
SEXP fastrank_num_avg_(SEXP s_x);



/* FUNCTION REGISTRATION *********************************/

// Registering the routines with R
static R_CallMethodDef callMethods[] = {
    {"fastrank_",         (DL_FUNC) &fastrank_,         3},
    {"fastrank_num_avg_", (DL_FUNC) &fastrank_num_avg_, 1},
    {NULL, NULL, 0}
};
void R_init_fastrank(DllInfo *info) {
    R_registerRoutines(info, NULL, callMethods, NULL, NULL);
}




/*************************************************************************
 * SORTING
 *
 */

/* Quicksort from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort 
 * modified to return a vector of indices and not modify the array of
 * values a[].  The indx[] vector must be pre-allocated and already be 
 * filled with 0..n-1.
 */

static void fr_quicksort_double_i_ (const double * a,
                                    MY_SIZE_T indx[],
                                    const MY_SIZE_T n) {
    double p;
    MY_SIZE_T i, j, it;
    if (n < 2) return;
    p = a[indx[n / 2]];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[indx[i]] < p) i++;
        while (p < a[indx[j]]) j--;
        if (i >= j) break;
        // swap indices
        it = indx[i]; indx[i] = indx[j]; indx[j] = it;
    }
    fr_quicksort_double_i_(a, indx,     i    );
    fr_quicksort_double_i_(a, indx + i, n - i);
}


 
/* Shellsort, following http://en.wikipedia.org/wiki/Shellsort 
 *
 * The ideal gap is of some debate.  Internally R uses two forms of 
 * gaps, with the faster shellsort using Sedgwick's (1986) gaps.
 * The others below are other apparently better gap schemes 
 * suggested at the above Wikipedia page.

   sedgwick_gap <- function(k) {
       if (k == 1) return(1)
       k <- k - 1
       return(4^k + (3 * 2^(k - 1)) + 1)
   }
   tokuda_gap <- function(k) ceiling((9^k - 4^k)/(5 * 4^(k-1)))
   ciura_gap <- function(k) {
       g <- c(1L, 4L, 10L, 23L, 57L, 132L, 301L, 701L)
       max_g <- length(g)
       if (k > max_g) {
           h <- g[max_g]
           while (k > max_g) {
               h <- floor(h * 2.25)
               k <- k - 1
           }
           return(h)
       }
       return(g[k])
   }
 */

#ifdef LONG_VECTOR_SUPPORT
   /* R's Sedgwick gaps go up to between 2^40 and 2^41, 
    * use this criteria for the others */
#  define N_GAPS_SEDGWICK 21
#  define N_GAPS_TOKUDA   35
#  define N_GAPS_CIURA    35
#else
   /* Here, gaps up to just under 2^31 - 1, MAXINT */
#  define N_GAPS_SEDGWICK 17
#  define N_GAPS_TOKUDA   27
#  define N_GAPS_CIURA    26
#endif
static const MY_SIZE_T ciura_gaps [N_GAPS_CIURA] = {
#ifdef LONG_VECTOR_SUPPORT
    2262162137148L, 1005405394288L, 446846841906L, 198598596403L,
    88266042846L,   39229352376L,   17435267723L,  7749007877L,
    3444003501L,
#endif
    1530668223, 680296988, 302354217, 134379652,  59724290,
    26544129,   11797391,  5243285,   2330349,    1035711,
    460316,     204585,    90927,     40412,      17961,
    7983,       3548,      1577,      701,        301,
    132,        57,        23,        10,         4,
    1
};
static const MY_SIZE_T sedgwick_gaps [N_GAPS_SEDGWICK] = {
#ifdef LONG_VECTOR_SUPPORT
    1099513200641L, 274878693377L, 68719869953L, 17180065793L, 4295065601L,
#endif
    1073790977, 268460033, 67121153, 16783361, 4197377, 
    1050113,    262913,    65921,    16577,    4193,
    1073,       281,       77,       23,       8,
    1
};
static const MY_SIZE_T tokuda_gaps [N_GAPS_TOKUDA] = {
#ifdef LONG_VECTOR_SUPPORT
    1696204147864L, 753868510162L, 335052671183L, 148912298303L, 66183243690L,
    29414774973L, 13073233321L, 5810325920L, 2582367076L,
#endif
    1147718700, 510097200, 226709866, 100759940, 44782196,
    19903198,   8845866,   3931496,   1747331,   776591,
    345152,     153401,    68178,     30301,     13467,
    5985,       2660,      1182,      525,       233,
    103,        46,        20,        9,         4,
    1
};



static void fr_shellsort_double_i_ (const double * a,
                                    MY_SIZE_T indx[],
                                    const MY_SIZE_T n,
                                    const MY_SIZE_T * the_gaps,
                                    const int N_GAPS) {
    int ig;
    MY_SIZE_T i, it, j, gap;
    for (ig = 0; the_gaps[ig] >= n; ++ig);
    for (; ig < N_GAPS; ++ig) {
        gap = the_gaps[ig];
        if (DEBUG) {
            printf("ig = %d    gap = %td   n = %td\n", ig, gap, n);
            for (int q = 0; q < n; ++q) printf("%td   ", indx[q]); printf("\n");
        }
        for (i = gap; i < n; ++i) {
            it = indx[i];
            for (j = i; j >= gap && a[indx[j - gap]] > a[it]; j -= gap) {
                indx[j] = indx[j - gap];
            }
            indx[j] = it;
        }
    }
}



static void fr_shellsort_c_double_i_(const double * a, MY_SIZE_T indx[], const MY_SIZE_T n) {
    fr_shellsort_double_i_(a, indx, n, &ciura_gaps[0], N_GAPS_CIURA);
}



static void fr_shellsort_s_double_i_(const double * a, MY_SIZE_T indx[], const MY_SIZE_T n) {
    fr_shellsort_double_i_(a, indx, n, &sedgwick_gaps[0], N_GAPS_SEDGWICK);
}



static void fr_shellsort_t_double_i_(const double * a, MY_SIZE_T indx[], const MY_SIZE_T n) {
    fr_shellsort_double_i_(a, indx, n, &tokuda_gaps[0], N_GAPS_TOKUDA);
}




/** END OF SORTING *****************************************************/




/* General ranking (no characters), called from fastrank() wrapper */
SEXP fastrank_(SEXP s_x, SEXP s_tm, SEXP s_sort) {

    if (TYPEOF(s_x) != REALSXP)
        error(" for 'sort' benchmarking, x must be numeric");
    if (TYPEOF(s_x) == STRSXP)
        error("'character' values not allowed");

    int sort_method = INTEGER(s_sort)[0];		
    if (sort_method < 1 || sort_method > 5)		
        error("'sort' must be between 1 and 5");
    enum { R_ORDERVECTOR = 1,
           FR_QUICKSORT, 
           FR_SHELLSORT_CIURA,
           FR_SHELLSORT_SEDGWICK,
           FR_SHELLSORT_TOKUDA };

    MY_SIZE_T n = MY_LENGTH(s_x);
    if (DEBUG) Rprintf("length of s_x = %d\n", n);

    /* Now process ties.method */
    if (TYPEOF(s_tm) != STRSXP)
        error("ties.method must be \"average\", \"first\", \"random\", \"max\", or \"min\"");
    const char* tm = CHAR(STRING_ELT(s_tm, 0));

    enum { TIES_ERROR = 0, TIES_AVERAGE, TIES_FIRST, TIES_RANDOM,
           TIES_MAX, TIES_MIN } ties_method;

    /* method 1, this is a bit faster, like 0.2% */
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
        break;
    default:
        ties_method = TIES_ERROR;
        error("unknown ties.method");
        break;
    }
#if 0
    /* method 2 */
    if (! strcmp(tm, "average"))     ties_method = TIES_AVERAGE;
    else if (! strcmp(tm, "first"))  ties_method = TIES_FIRST;
    else if (! strcmp(tm, "random")) ties_method = TIES_RANDOM;
    else if (! strcmp(tm, "max"))    ties_method = TIES_MAX;
    else if (! strcmp(tm, "min"))    ties_method = TIES_MIN;
    else {
        ties_method = TIES_ERROR;
        error("unknown ties.method");
    }
#endif

    MY_SIZE_T *indx = (MY_SIZE_T *) R_alloc(n, sizeof(MY_SIZE_T));

    switch(sort_method) {
    case R_ORDERVECTOR:
        error("sort.method = 1 (R_orderVector) already off the island");
        //R_orderVector(indx, n, Rf_lang1(s_x), TRUE, FALSE);
        break;
    case FR_QUICKSORT:
        for (MY_SIZE_T i = 0; i < n; ++i) indx[i] = i;
        fr_quicksort_double_i_ (REAL(s_x), indx, n);
        break;
    case FR_SHELLSORT_CIURA:
        for (MY_SIZE_T i = 0; i < n; ++i) indx[i] = i;
        fr_shellsort_c_double_i_(REAL(s_x), indx, n);
        break;
    case FR_SHELLSORT_SEDGWICK:
        for (MY_SIZE_T i = 0; i < n; ++i) indx[i] = i;
        fr_shellsort_s_double_i_(REAL(s_x), indx, n);
        break;
    case FR_SHELLSORT_TOKUDA:
        for (MY_SIZE_T i = 0; i < n; ++i) indx[i] = i;
        fr_shellsort_t_double_i_(REAL(s_x), indx, n);
        break;
    default:
        error("unaccounted-for sort method");
        break;
    }

    if (DEBUG) {
        Rprintf("sort return indx:\n");
        for (int i = 0; i < n; ++i) Rprintf("%d ", indx[i]);
        Rprintf("\n");
    }
    // indx[i] holds the index of the value in s_x that belongs in position i,
    // e.g., indx[0] holds the position in s_x of the first value

    SEXP s_ranks = NULL;  /* return value, allocated below */

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
    if (DEBUG) Rprintf("min, ranks[%d .. %d] <- %d   " __loc__ "\n", \
                       indx[ib], indx[i - 1], rnk); \
    for (MY_SIZE_T j = ib; j <= i - 1; ++j) { \
        ranks[indx[j]] = rnk; \
    } \
    }

/* ties' rank is the maximum of their ranks */
#define FR_ties_max(__RTYPE, __loc__) \
    { \
    __RTYPE rnk = (__RTYPE)i; \
    if (DEBUG) Rprintf("max, ranks[%d .. %d] <- %d   " __loc__ "\n", \
                       indx[ib], indx[i - 1], rnk); \
    for (MY_SIZE_T j = ib; j <= i - 1; ++j) { \
        ranks[indx[j]] = rnk; \
    } \
    }

/* ties' rank is the average of their ranks */
#define FR_ties_average(__RTYPE, __loc__) \
    { \
    __RTYPE rnk = (i - 1 + ib + 2) / 2.0; \
    if (DEBUG) Rprintf("average, ranks[%d .. %d] <- %d   " __loc__ "\n", \
                       indx[ib], indx[i - 1], rnk); \
    for (MY_SIZE_T j = ib; j <= i - 1; ++j) { \
        ranks[indx[j]] = rnk; \
    } \
    }

/* ties' rank is consecutive on their order */
#define FR_ties_first(__RTYPE, __loc__) \
    { \
    if (DEBUG) Rprintf("is 'first' only correct when sort is stable?"); \
    for (MY_SIZE_T j = ib; j <= i - 1; ++j) { \
        __RTYPE rnk = (__RTYPE)(j + 1); \
        if (DEBUG) Rprintf("first, ranks[%d] <- %d  " __loc__ "\n", indx[j], rnk); \
        ranks[indx[j]] = rnk; \
    } \
    }

/* ties' rank is a random shuffling of their order */
#define FR_ties_random(__RTYPE, __loc__) \
    { \
    MY_SIZE_T tn = i - ib; \
    MY_SIZE_T* t = (MY_SIZE_T*) R_alloc(tn, sizeof(MY_SIZE_T)); \
    MY_SIZE_T j; \
    for (j = 0; j < tn; ++j) \
        t[j] = j; \
    for (j = ib; j < i - 1; ++j) { \
        MY_SIZE_T k = (MY_SIZE_T)(tn * unif_rand()); \
        __RTYPE rnk = (__RTYPE)(t[k] + ib + 1); \
        if (DEBUG) Rprintf("random, rank[%d] <- %d  " __loc__ "\n", indx[j], rnk); \
        ranks[indx[j]] = rnk; \
        t[k] = t[--tn]; \
    } \
    __RTYPE rnk = (__RTYPE)(t[0] + ib + 1); \
    if (DEBUG) Rprintf("random, rank[%d] <- %d  " __loc__ "\n", indx[j], rnk); \
    ranks[indx[j]] = rnk; \
    }

 

#define FR_rank(__TIES__, __TYPE, __TCONV, __RTYPE, __R_RTYPE, __R_TCONV) \
    { \
    s_ranks = PROTECT(allocVector(__R_RTYPE, n)); \
    __RTYPE* ranks = __R_TCONV(s_ranks); \
    if (DEBUG) Rprintf("address of ranks = 0x%p\n", ranks); \
    __TYPE* x = __TCONV(s_x); \
    MY_SIZE_T ib = 0; \
    __TYPE b = XI(0); \
    MY_SIZE_T i; \
    if (DEBUG) Rprintf("ib = %d\n", ib); \
    for (i = 1; i < n; ++i) { \
        if (! EQUAL(XI(i), b)) { \
            if (DEBUG) Rprintf("XI(%d) %.1f != b %.1f\n", i, (double)XI(i), (double)b); \
            if (ib < i - 1) { \
                __TIES__(__RTYPE, "MID") \
            } else { \
                if (DEBUG) \
                    Rprintf("ranks[%d] <- %.1f  MID\n", indx[ib], (double)(ib + 1)); \
                ranks[indx[ib]] = (__RTYPE)(ib + 1); \
            } \
            b = XI(i); \
            ib = i; \
            if (DEBUG) Rprintf("ib = %d\n", ib); \
        } \
    } \
    if (ib == i - 1) {\
        if (DEBUG) Rprintf("ranks[%d] <- %.1f  FIN\n", ib, (double)(indx[ib])); \
        ranks[indx[ib]] = (__RTYPE)(i); \
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
#if 0
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
#endif
    default:
        error("'x' is not an integer, numeric or complex vector");
        break;
    }

    UNPROTECT(1);
    return s_ranks;
}



/* DIRECT ENTRIES ******************************************/


/* We want to use the above macros to set these up */


/* Rank a numeric vector giving ties their average rank */
SEXP fastrank_num_avg_(SEXP s_x) {
    MY_SIZE_T n = MY_LENGTH(s_x);
    double *x = REAL(s_x);
    if (DEBUG) {
        Rprintf("    x:  ");
        for (int i = 0; i < n; ++i) Rprintf("%.3f ", x[i]); 
        Rprintf("\n");
    }

    /* double because "average" */
    SEXP s_ranks = PROTECT(allocVector(REALSXP, n));
    double *ranks = REAL(s_ranks);
    MY_SIZE_T *indx = (MY_SIZE_T *) R_alloc(n, sizeof(MY_SIZE_T));
    /* pre-fill indx with index from 0..n-1 */
    for (MY_SIZE_T i = 0; i < n; ++i)
        indx[i] = i;
    fr_quicksort_double_i_(x, indx, n);
    if (DEBUG) {
        Rprintf(" indx:   ");
        for (int i = 0; i < n; ++i) Rprintf("%d    ", indx[i]); 
        Rprintf("\n");
    }

    MY_SIZE_T ib = 0;
    double b = x[indx[0]];
    MY_SIZE_T i;
    for (i = 1; i < n; ++i) {
        if (x[indx[i]] != b) { /* consecutive numbers differ */
            if (ib < i - 1) {  /* average of sum of ranks */
                double rnk = (i - 1 + ib + 2) / 2.0;
                for (MY_SIZE_T j = ib; j <= i - 1; ++j)
                    ranks[indx[j]] = rnk;
            } else {
                ranks[indx[ib]] = (double)(ib + 1);
            }
            b = x[indx[i]];
            ib = i;
        }
    }
    /* now check leftovers */
    if (ib == i - 1)  /* last two were unique */
        ranks[indx[ib]] = (double)i;
    else {  /* ended with ties */
        double rnk = (i - 1 + ib + 2) / 2.0;
        for (MY_SIZE_T j = ib; j <= i - 1; ++j)
            ranks[indx[j]] = rnk;
    }
    if (DEBUG) {
        Rprintf("ranks:  ");
        for (int i = 0; i < n; ++i) Rprintf("%.1f   ", ranks[i]); 
        Rprintf("\n");
    }

    UNPROTECT(1);
    return s_ranks;
}



