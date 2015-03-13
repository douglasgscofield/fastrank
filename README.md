fastrank
========

An R package providing fast ranking for integer and numeric vectors, as an
alternative to calling `.Internal(rank(...))`, which packages cannot do.

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

1. **Still sorting the passed vector**, need to not do that.
2. Do general interface with `R_orderVector`.
3. What are the errors once sees with incorrect data?
4. Update all these experiences over in the **R-package-utilities** repository.
5. No character values accepted (we can't collate like R does)
6. Is our sort stable?
7. 

### Latest results, with general fastrank()

```R
> rank_new <- function (x) .Internal(rank(x, length(x), "min"))
> microbenchmark(rank_new(x), fastrank(x, "min", 1L), fastrank(x, "min", 2L), times=100000)
Unit: microseconds
                   expr   min    lq     mean median    uq      max neval
            rank_new(x) 1.333 1.494 1.812441  1.555 1.641 6519.842 1e+05
 fastrank(x, "min", 1L) 3.124 3.349 3.751827  3.452 3.585 4668.169 1e+05
 fastrank(x, "min", 2L) 3.158 3.369 3.896483  3.473 3.606 7170.547 1e+05
> rank_new <- function (x) .Internal(rank(x, length(x), "average"))
> microbenchmark(rank_new(x), fastrank(x, "average", 1L), fastrank(x, "average", 2L), times=100000)
Unit: microseconds
                       expr   min    lq     mean median    uq      max neval
                rank_new(x) 1.305 1.482 1.674075  1.550 1.642 5776.435 1e+05
 fastrank(x, "average", 1L) 3.135 3.358 3.908632  3.461 3.595 7331.003 1e+05
 fastrank(x, "average", 2L) 3.142 3.362 4.058442  3.466 3.600 5788.526 1e+05
```

The call overhead is fairly high, going through `.Internal(...)` seems to give
a good gain.  If we simplify our call to `fastrank_(...)` we see a speedup.

```R
> rank_new <- function (x) .Internal(rank(x, length(x), "average"))
> fr1 = function(x) .Call("fastrank_", x, "average", 1L)
> fr2 = function(x) .Call("fastrank_", x, "average", 2L)
> microbenchmark(rank_new(x), fr1(x), fr2(x), times=100000)
Unit: microseconds
        expr   min    lq     mean median    uq      max neval
 rank_new(x) 1.304 1.435 1.666147  1.507 1.597 8247.283 1e+05
      fr1(x) 2.638 2.809 3.489785  2.899 3.012 8499.078 1e+05
      fr2(x) 2.649 2.815 3.411143  2.905 3.017 8643.985 1e+05
> microbenchmark(rank_new(x), fr1(x), fr2(x), times=100000)
Unit: microseconds
        expr   min    lq     mean median    uq      max neval
 rank_new(x) 1.310 1.429 1.880561  1.497 1.584 8671.646 1e+05
      fr1(x) 2.646 2.807 3.375271  2.892 3.001 9493.157 1e+05
      fr2(x) 2.650 2.812 3.205432  2.897 3.007 7841.215 1e+05

```

Still need to find some time... also the 2nd method of finding ties is faster.  There is also an effect of providing additional arguments through the R interface.  Better to use `.Call` directly.

```R
> rank_new
function (x) .Internal(rank(x, length(x), "average"))
> fr = function(x) .Call("fastrank_", x, "average")
> microbenchmark(rank_new(x), fastrank(x,"average"), times=100000)
Unit: microseconds
                   expr   min    lq     mean median    uq      max neval
            rank_new(x) 1.315 1.513 1.883313  1.656 1.755 4286.676 1e+05
 fastrank(x, "average") 5.323 5.717 6.535513  5.876 6.084 5310.369 1e+05
> microbenchmark(rank_new(x), fastrank(x), times=100000)
Unit: microseconds
        expr   min    lq     mean median    uq       max neval
 rank_new(x) 1.307 1.512 2.295412  1.653 1.753 36058.038 1e+05
 fastrank(x) 5.299 5.673 6.462977  5.828 6.035  4674.833 1e+05
> microbenchmark(rank_new(x), fr(x), times=100000)
Unit: microseconds
        expr   min    lq     mean median    uq      max neval
 rank_new(x) 1.303 1.415 1.792045  1.484 1.580 9280.130 1e+05
       fr(x) 2.645 2.810 3.279320  2.890 2.997 7785.971 1e+05
```

And avoiding the R interface entirely gives a touch more, and doing so shows us clearly the difference between giving the bare name of the C function and the character string of the name.

```R
> microbenchmark(.Internal(rank(x, length(x), "average")), .Call("fastrank_", x, "average"), .Call(fastrank_, x, "average"), times=100000)
Unit: microseconds
                                     expr   min    lq     mean median    uq       max neval
 .Internal(rank(x, length(x), "average")) 1.042 1.133 1.327148  1.171 1.243  8696.792 1e+05
         .Call("fastrank_", x, "average") 2.361 2.487 2.994419  2.558 2.646  9965.114 1e+05
           .Call(fastrank_, x, "average") 2.406 2.589 3.200212  2.657 2.743 10298.581 1e+05
```

### Initial results with direct routine

After following the advice of <http://ftp.sunet.se/pub/lang/CRAN/doc/manuals/r-devel/R-exts.html#Registering-native-routines> and registering my C routine with R to reduce symbol search times, we are getting much more comparable benchmark results:

```C
#include <R_ext/Rdynload.h>

...

SEXP fastrank_numeric_average(SEXP s_x);

// Registering the routines with R
static R_NativePrimitiveArgType fastrank_numeric_average_t[] = {
    REALSXP
};
static R_CMethodDef cMethods[] = {
    {"fastrank_numeric_average", (DL_FUNC) &fastrank_numeric_average, 1, 
        fastrank_numeric_average_t},
    {NULL, NULL, 0}
};
static R_CallMethodDef callMethods[] = {
    {"fastrank_numeric_average", (DL_FUNC) &fastrank_numeric_average, 1},
    {NULL, NULL, 0}
};
void R_init_fastrank(DllInfo *info) {
    R_registerRoutines(info, cMethods, callMethods, NULL, NULL);
}
```

However I still cannot call the C routine directly within R.  Will I ever be able to do that, or is the intervening R stub always necessary?

```R
> devtools::load_all()
Loading fastrank
> microbenchmark(rank(xx), rank_new(xx), fastrank(xx), times=10000)
Unit: microseconds
         expr    min      lq      mean  median      uq      max neval
     rank(xx) 26.694 28.4265 32.418030 28.9940 29.7285 2998.105 10000
 rank_new(xx)  2.383  2.7710  3.038986  2.9700  3.1410   84.867 10000
 fastrank(xx)  2.247  2.7210  4.979082  3.1975  3.6435 2958.373 10000
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy), times=10000)
Unit: microseconds
         expr    min     lq      mean median     uq      max neval
     rank(yy) 26.361 28.247 31.896437 28.834 29.539 3049.120 10000
 rank_new(yy)  2.102  2.488  3.871496  2.689  2.861 2881.963 10000
 fastrank(yy)  2.114  2.574  4.350697  3.085  3.494 2905.780 10000
> microbenchmark(rank(xx), rank_new(xx), fastrank(xx), fastrank_numeric_average(xx), times=10000)
Error in microbenchmark(rank(xx), rank_new(xx), fastrank(xx), fastrank_numeric_average(xx),  :
  could not find function "fastrank_numeric_average"
```


### Initial results

Well my initial implementation is complete and the first results are in.
Bummer, it is not yet faster than calling `.Internal(rank(...))` but I feel it
really must be.  Some observations:

* It seems in relative terms that `fastrank` is faster than `.Internal(rank())` (or at least slows down less) when there are ties in the data, see contrast between results with `xx` which can have ties, and `yy` which essentially cannot.
* Note the error by failing to make `xx` be numeric
* Note the error for `fastrank_numeric_average` not being available, can I make the C interface directly available?

```R
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
```


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



LICENSE
-------

GPL 2, just like R itself.
