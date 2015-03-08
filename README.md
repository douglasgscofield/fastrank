fastrank
========

An R package providing fast ranking for integer and numeric vectors, as an
alternative to calling .Internal(rank(...)), which packages cannot do.

The motivation for this comes from my development of the `nestedRanksTest`
package.  A standard run with the default 10,000 bootstrap iterations takes
a few seconds to complete on a test data set.

~~~~
> library(nestedRanksTest)
> data(woodpecker_multiyear)
> adat <- subset(woodpecker_multiyear, Species == "agrifolia")
> summary(adat)
      Species         Year         Granary         Distance
 agrifolia:316   Min.   :2006   Min.   : 10.0   Min.   :  10.90
 lobata   :  0   1st Qu.:2006   1st Qu.:151.0   1st Qu.:  52.00
                 Median :2006   Median :152.0   Median :  53.60
                 Mean   :2006   Mean   :431.5   Mean   :  98.67
                 3rd Qu.:2007   3rd Qu.:938.0   3rd Qu.:  84.40
                 Max.   :2007   Max.   :942.0   Max.   :1448.50
> system.time(with(adat, nestedRanksTest(y = Distance, x = Year, groups = Granary)))
   user  system elapsed 
  5.252   0.067   5.318 
~~~~

Profiling with `library(lineprof)` revealed that the bottleneck here is in the
utility function `nestedRanksTest_Z`, specifically in the calculation of ranks
via the base R function `rank`.  A stripped-down `rank_new` is 8-9&times;
faster than the default rank for a vector of 100 values:

~~~~
> library(microbenchmark)
> rank_new <- function (x) .Internal(rank(x, length(x), "average"))
> yy <- rnorm(100)
> microbenchmark(rank(yy), rank_new(yy), times=100000)
Unit: microseconds
         expr    min     lq      mean median      uq       max neval
     rank(yy) 29.148 31.945 36.931556 32.678 33.5165 57192.350 1e+05
 rank_new(yy)  3.755  4.300  4.789952  4.542  4.7290  6784.741 1e+05
~~~~

For 1000-value vectors the speedup is more modest, about 2&times;, and for
10,000-value vectors the speedup is only in the neighbourhood of 20-30%:

~~~~
> yyy <- rnorm(1000)
> microbenchmark(rank(yyy), rank_new(yyy), times=100000)
Unit: microseconds
          expr     min      lq      mean  median      uq      max neval
     rank(yyy) 114.693 118.766 134.38290 120.071 123.598 18734.74 1e+05
 rank_new(yyy)  63.150  64.877  67.71052  65.340  67.062 16023.45 1e+05
> yyy <- rnorm(10000)
> microbenchmark(rank(yyy), rank_new(yyy), times=1000)
Unit: microseconds
          expr      min        lq    mean   median        uq      max neval
     rank(yyy) 1238.485 1263.3515 1361.22 1279.580 1303.5785 6436.039  1000
 rank_new(yyy)  955.013  964.9215 1002.81  967.849  992.6315 6072.219  1000
~~~~

I experimented with "genericifying" `rank` with `xtfrm` (I saw this suggestion
somewhere) but that only slowed things down.

~~~~
> yy <- rnorm(100)
> microbenchmark(rank(yy), rank(xtfrm(yy)), rank_new(yy), times=100000)
Unit: microseconds
            expr    min     lq      mean median     uq       max neval
        rank(yy) 29.683 32.070 37.044963 32.845 33.805 42099.964 1e+05
 rank(xtfrm(yy)) 30.518 33.123 38.756965 33.925 34.909 41573.036 1e+05
    rank_new(yy)  3.878  4.523  5.022672  4.670  4.831  3823.488 1e+05
~~~~


Current results
---------------

Well my initial implementation is complete and the first results are in.
Bummer, it is not yet faster than calling `.Internal(rank(...))` but I feel it
really must be.

~~~~
> library(microbenchmark)
> rank_new <- function (x) .Internal(rank(x, length(x), "average"))
> xx <- sample(100, 100, replace=TRUE)
> yy <- rnorm(100)
> microbenchmark(rank(xx), rank_new(xx), fastrank(xx), times=10000)
Error in fastrank(xx) :
  REAL() can only be applied to a 'numeric', not a 'integer'
> xx <- as.numeric(xx)
> microbenchmark(rank(xx), rank_new(xx), fastrank(xx), times=10000)
Unit: microseconds
         expr    min      lq      mean median      uq      max neval
     rank(xx) 27.563 30.1950 35.407322 31.365 32.6175 2745.064 10000
 rank_new(xx)  2.403  2.9230  3.814224  3.144  3.3095 2596.105 10000
 fastrank(xx)  3.192  3.7285  6.537482  5.174  5.5830 2637.797 10000
> fastrank
function(x) {
    .Call("fastrank_numeric_average", x, PACKAGE = "fastrank")
}
<environment: namespace:fastrank>
> fastrank_new <- function(x) .Call("fastrank_numeric_avg", x, PACKAGE="fastrank")
> microbenchmark(rank(xx), rank_new(xx), fastrank(xx), fastrank_new(xx), times=10000)
Error in .Call("fastrank_numeric_avg", x, PACKAGE = "fastrank") :
  "fastrank_numeric_avg" not available for .Call() for package "fastrank"
> microbenchmark(rank(xx), rank_new(xx), fastrank(xx), times=10000)
Unit: microseconds
         expr    min      lq      mean  median     uq      max neval
     rank(xx) 26.980 29.7335 35.094786 30.8445 32.027 4204.288 10000
 rank_new(xx)  2.417  2.9140  3.538192  3.1310  3.288 3106.531 10000
 fastrank(xx)  3.212  3.7170  6.058158  5.1055  5.531 3164.499 10000
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy), times=10000)
Unit: microseconds
         expr    min     lq      mean  median     uq      max neval
     rank(yy) 26.551 28.874 33.818973 29.9980 31.039 2944.843 10000
 rank_new(yy)  2.114  2.572  3.119958  2.7940  2.949 2488.677 10000
 fastrank(yy)  3.065  3.575  5.801389  4.8815  5.340 2937.812 10000
~~~~


The plan
--------

R already provides an interface to `orderVector` through user C code via
[`R_orderVector`][R_orderVector].  I may be able to build on this to provide
the functions here.  One big advantage is that it ranks an `SEXP`, it is as
general as the base `rank`.

[R_orderVector]: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions

* * *

Still working this part out.  The alternatives I am considering are:

1. Copy the internal R function `do_rank` within `src/main/sort.c` that is what
   we reach when doing the `.Internal(rank(...))` call.  This is the simplest, but
   has the same advantages and disadvantages as the benchmarks above.  It would of
   course be as flexible as the internal function and functionally identical.  I
   would also need to include R-core as package authors following
   http://r-pkgs.had.co.nz/check.html.  I am now not leaning toward this, because I
   would have to copy `orderVector1()` along with sort, order etc.
2. Write my own C function that is callable from R directly.  I can try to make
   it faster than `.Internal(rank(...))`, perhaps by choosing between quicksort
   and mergesort following http://rosettacode.org/wiki/Sorting_algorithms/Quicksort.
   I am leaning toward this option.
3. Write my own C++ function using the `Rcpp` package, which likely makes
   working with the interface much easier.  I see that as a drawback, but I may be
   able to make the package easier to write and maintain with templates, and also
   would be able to apply the same sort-choice speedups as for (2).

Notes:

* http://stat.ethz.ch/R-manual/R-devel/library/base/html/sort.html
* http://stat.ethz.ch/R-manual/R-devel/library/base/html/xtfrm.html.
* https://stat.ethz.ch/R-manual/R-devel/library/base/html/order.html



The API
-------

* Should I provide separate entrypoints for `fastrank_average`, `fastrank_minimum`, etc., to avoid the `strcmp()`, even though that will be very fast?  Benchmark the solutions.


TO DO
-----

* real package environment
* use Authors@R

