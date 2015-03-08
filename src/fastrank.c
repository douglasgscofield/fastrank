#include <R.h>
#include <Rinternals.h>

#undef use_R_sort

#ifdef LONG_VECTOR_SUPPORT
#  define MY_SIZE_T R_xlen_t
#  define MY_LENGTH xlength
#else
#  define MY_SIZE_T int
#  define MY_LENGTH length
#endif

void rquicksort_I (double a[], MY_SIZE_T indx[], const MY_SIZE_T n);

//' Fast calculation of ranks of a double vector, assigning average rank to ties
//'
//' This function quickly calculates the ranks of a double vector while assigning
//' average rank to ties, and does absolutely no error checking on its arguments.
//' If you call it incorrectly, it will probably crash R.
//' 
//' @param x            a vector of integers, there must be no NA values.
//' 
//' @return a vector the same length as \code{x} with double ranks of the 
//'         corresponding elements in \code{x}.
//'
//' @seealso \code{\link{fastrank}}
//' 
//' @keywords internal
//' 
//' @export fastrank_numeric_average
//' 
SEXP fastrank_numeric_average(SEXP s_x) {
    MY_SIZE_T n = MY_LENGTH(s_x);

    SEXP s_result = PROTECT(allocVector(REALSXP, n)); // ranks are doubles
    double *x = REAL(s_x), *result = REAL(s_result);
    MY_SIZE_T *indx = (MY_SIZE_T *) R_alloc(n, sizeof(MY_SIZE_T));
#ifdef use_R_sort
    R_qsort_I(x, indx, (MY_SIZE_T)1, (MY_SIZE_T)n);
#else
    for (MY_SIZE_T i = 0; i < n; ++i)  // pre-fill indx with index from 0..n-1
        indx[i] = i;
    rquicksort_I(x, indx, n);
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
    Rprintf("ranks of sorted vector:\n");
    for (int i = 0; i < n; ++i) Rprintf("%.1f   ", ranks[i]); Rprintf("\n");

    // reorder ranks into the answer
    for (MY_SIZE_T i = 0; i < n; ++i)
        result[indx[i]] = ranks[i];
    Rprintf("reorder ranks into original order of vector:\n");
    for (int i = 0; i < n; ++i) Rprintf("%.1f   ", result[i]); Rprintf("\n");

    UNPROTECT(1);
    return s_result;
}

// quick_sort code modified from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort
// to return a vector of previous indices
void rquicksort_I (double a[], MY_SIZE_T indx[], const MY_SIZE_T n) {
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
    rquicksort_I(a, indx, i);
    rquicksort_I(a + i, indx + i, n - i);
}
 
// Note: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions
//
// void R_orderVector (int* indx, int n, SEXP arglist, Rboolean nalast, Rboolean decreasing)
//
// From 3.0.0, vectors may not reliably be int, so use R_xlen_t for the length type and
// xlength() to get a vector length, hence R_xlen_t n = xlength(x)

// http://ftp.sunet.se/pub/lang/CRAN/doc/manuals/r-devel/R-exts.html#Handling-R-objects-in-C

