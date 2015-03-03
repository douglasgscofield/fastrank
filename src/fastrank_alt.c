#include <R.h>
#include <Rinternals.h>

//' Quickly calculate ranks of an integer vector.
//'
//' This function quickly calculates the ranks of an integer vector, and does 
//' absolutely no error checking on its arguments.  If you call it incorrectly,
//' it will probably crash R.
//' 
//' @param x            a vector of integers, there must be no NA values.
//' @param n            the length of \code{x}, this must be correct.
//' @param ties_method  character string specifying the method for breaking
//'                     ties in ranks.  This must match one of the following
//'                     exactly: average, first, max, min.  For more 
//'                     information on the differences between these methods
//'                     see \code{\link{rank}}.  Note that the random method
//'                     is not provided by this function.
//' 
//' @return a vector the same length as \code{x} with integer ranks of the 
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

SEXP rank_(SEXP arg_x, SEXP arg_n) {
    int n = asInteger(arg_n);
    // Because ties are average, the ranks returned are reals
    SEXP result = PROTECT(allocVector(REALSXP, n));
    // BUT R_orderVector wants a vector of ints, so create that and pass it
    int* indx = (int *) R_alloc(n, sizeof(int));
    R_orderVector(indx, n, arg_x, TRUE, FALSE);
    // Now indx holds ranks, from 0:(n-1)

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
 
