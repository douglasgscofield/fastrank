#include <R.h>
#include <Rinternals.h>

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
//' @export

// Note: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions
//
// void R_orderVector (int* indx, int n, SEXP arglist, Rboolean nalast, Rboolean decreasing)
//
// From 3.0.0, vectors may not reliably be int, so use R_xlen_t for the length type and
// xlength() to get a vector length, hence R_xlen_t n = xlength(x)

// http://ftp.sunet.se/pub/lang/CRAN/doc/manuals/r-devel/R-exts.html#Handling-R-objects-in-C
SEXP fastrank_numeric_average(SEXP x) {
    R_xlen_t n = xlength(x);
    // Because ties are average, the ranks returned are reals
    SEXP result = PROTECT(allocVector(REALSXP, n));
    double *rx = REAL(x), *rresult = REAL(result);
    // do I need to PROTECT here?
    R_xlen_t *indx = (R_xlen_t *) R_alloc(n, sizeof(R_xlen_t));
    double *ranks = (R_xlen_t *) R_alloc(n, sizeof(double));
    // Note: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions
    rsort_with_index(rx, indx, n);  // same permutation to indx
    // traverse rx, looking for ties and remembering rank when we have them

    // now to break ties, step through sorted values
    double rb = rx[0];
    R_xlen_t ib = 0;
    for (R_xlen_t i = 1; i < n; ++i) {
        if (rx[i] != rb) { // consecutive numbers differ
            if (ib < i - 1) { // there is at least one previous tie
                // sum of ranks from a to b
                // sum = (b + a) * (b - a + 1) / 2;
                // avg_rank = sum / (b - a + 1);
                // simple_avg_rank = (b + a) / 2.0;
                double rnk = (indx[i - 1] + indx[ib]) / 2.0;
                for (R_xlen_t j = ib; j <= i - 1; ++j)
                    ranks[j] = rnk;
            } else {
                ranks[i] = indx[i];
                rb = rx[i];
                ib = i;
            }
        }
    }
    // leftover ties
    if (ib < i - 1) {
        double rnk = (indx[i - 1] + indx[ib]) / 2.0;
        for (R_xlen_t j = ib; j <= i - 1; ++j)
            ranks[j] = rnk;
    }
    // reorder ranks into the answer
    for (R_xlen_t i = 0; i < n; ++i)
        result[indx[i]] = ranks[i];
    UNPROTECT(1);
    return result;
}

// quick_sort code modified from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort

template<class T>
void quick_sort (std::vector<T>& a, const int n) {
    T p, t;
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
    quick_sort(a, i);
    quick_sort(a + i, n - i);
}
 
