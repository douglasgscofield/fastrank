#include <R.h>
#include <Rinternals.h>

SEXP fastrank_int(SEXP x, SEXP n, SEXP ties_method) {
    // x is an integer vector, convert to...
    // n is length of x, convert to...
    // ties_method, convert to C string for use with strmp()
    enum { TIE_average, TIE_first, TIE_max, TIE_min } TIE_method;
    // compare ties_method as C string
    if (! strcmp(ties_method, "average")) {
        TIE_method = TIE_average;
    } else if (! strcmp(ties_method, "first")) {
        TIE_method = TIE_first;
    } else if (! strcmp(ties_method, "max")) {
        TIE_method = TIE_max;
    } else if (! strcmp(ties_method, "min")) {
        TIE_method = TIE_min;
    }
  SEXP result = PROTECT(allocVector(REALSXP, 1));
  REAL(result)[0] = asReal(a) + asReal(b);
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
 
