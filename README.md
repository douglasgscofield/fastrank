fastrank: rank faster than `rank`
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

## `fastrank` API

I can use the same API as `rank`:

```R
rank(x, na.last = TRUE, ties.method = c("average", "first",
                                        "random", "max", "min"))

```

or can duplicate the internal API:

```R
.Internal(rank(x, length(x), ties.method = c("average", "max", "min"))
```

(note that base `rank` handles `"first"` and `"random"` in R code), or I can pursue a third path.  My first C implementation gets the length internally from `x`, and handles all `ties.method` values internally, with an API:

```R
fastrank(x, ties.method = "average")
```
that sets `ties.method` if not provided and checks against `c("average", "first", "random", "max", "min")` internally.

Benchmarking has shown that the interface through R can cost quite a bit of time, so it may be benchmarking that shows me the best way.

The plan
--------

R already provides an interface to `orderVector` through user C code via
[`R_orderVector`][R_orderVector].  I may be able to build on this to provide
the functions here.  One big advantage is that it ranks an `SEXP`, it is as
general as the base `rank`.

[R_orderVector]: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions

I would like to be as general as base R `rank`, but as progress has been made here I've learned that `Scollate`, an internal R routines for comparing character strings using locale information, is not part of the R API.  Either I duplicate that functionality, which would probably be a bigger job than the rest of `fastrank`, or I leave character strings out.  So I'm leaving characters out.

I *think* it is necessary to use a stable sorting algorithm because of the `"first"` ties method.

The implementation alternatives I am considering are:

1. Use `R_orderVector` for sorting and then rank the `order` that comes back.  This is what I have used for the first implementation of a generic `fastrank`, see benchmarking results below.

2. Copy in its entirety the internal R function `do_rank` within `src/main/sort.c` that is what we reach when doing the `.Internal(rank(...))` call.  This is the simplest, but isn't really possible to do completely because of the character issue above and I would extend it to handle the missing ties methods.  If I do this, I need to include R-core as package authors following <http://r-pkgs.had.co.nz/check.html>.

3. Write my own C function with my own sort that I try to make faster than `.Internal(rank(...))`, perhaps by choosing sorting algorithms following <http://rosettacode.org/wiki/Sorting_algorithms/Quicksort> or some other advice.  I followed this option for `fastrank_numeric_average`, but its use of quicksort means it is not currently stable.  This is still not as fast as `.Internal(rank(...))`.

4. Write my own C++ function using the `Rcpp` package instead of C.  I could use STL algorithms easily, but drawing in `Rcpp` brings in some heavyweight stuff that kind of defeats the purpose.

* <http://stat.ethz.ch/R-manual/R-devel/library/base/html/sort.html>
* <http://stat.ethz.ch/R-manual/R-devel/library/base/html/xtfrm.html>
* <https://stat.ethz.ch/R-manual/R-devel/library/base/html/order.html>



## Progress

### Initial results with `fastrank_numeric_average`

Well my initial implementation using my own quicksort and restricting the interface to a specific input type and specific ties method is complete and the first benchmark results are in. Bummer, it is not yet faster than calling `.Internal(rank(...))`.

* It seems in relative terms that `fastrank` is faster than `.Internal(rank())` (or at least slows down less) when there are ties in the data, see contrast between results with `xx` which can have ties, and `yy` which essentially cannot.
* Note the error by failing to make `xx` be numeric

```R
> library(microbenchmark)
> rank_new <- function (x) .Internal(rank(x, length(x), "average"))
> fastrank
function(x) {
    .Call("fastrank_numeric_average", x, PACKAGE = "fastrank")
}
<environment: namespace:fastrank>
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
```


### Faster after registering `fastrank_numeric_average`

After following the advice of <http://ftp.sunet.se/pub/lang/CRAN/doc/manuals/r-devel/R-exts.html#Registering-native-routines> and registering my C routine with R to reduce symbol search times, we are getting more comparable benchmark results:

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

```R
> devtools::load_all()
Loading fastrank
> microbenchmark(rank(xx), rank_new(xx), fastrank(xx), times=10000)
Unit: microseconds
         expr    min      lq      mean  median      uq      max neval
     rank(xx) 26.694 28.4265 32.418030 28.9940 29.7285 2998.105 10000
 rank_new(xx)  2.383  2.7710  3.038986  2.9700  3.1410   84.867 10000
 fastrank(xx)  2.247  2.7210  4.979082  3.1975  3.6435 2958.373 10000
```

However I still cannot call the C routine directly within R, `.Call` is always required.  My docs for the package should definitely include the `.Call` interface to save a bit more time.


### General `fastrank`

Now I've finished a preliminary version of a more general `fastrank(x, ties.method)` that can rank logical, integer, numeric, and (soon) complex vectors and can handle any of the ties methods.

My first question was: is it faster to do a single-character shortcut evaluation of the `ties.method` value, or use `strcmp` to find it?  I added a third argument to choose between methods, `1L` for the shortcut and `2L` for `strcmp`.  The shortcut method is 0.5-1% faster.

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


### R wrapper call overhead

The call overhead of the R wrapper is fairly high, especially with assigning argument values, so that going through `.Internal(...)` seems to give
a good gain.  If we simplify our call to `.Call("fastrank_", ...)` we see a speedup.

```R
> rank_new
function (x) .Internal(rank(x, length(x), "average"))
> fastrank
function(x, ties.method = "average") {
    #cat(x, "\n");
    .Call("fastrank_", x, ties.method)
}
<environment: namespace:fastrank>
> fr
function(x) .Call("fastrank_", x, "average")
> microbenchmark(rank(x), rank_new(x), fastrank(x), fr(x), times=10000)
Unit: microseconds
        expr    min      lq      mean  median     uq      max neval
     rank(x) 24.364 26.7235 31.100900 27.4845 28.499 2983.331 10000
 rank_new(x)  1.336  1.7210  2.024409  1.8830  2.016  159.468 10000
 fastrank(x)  5.429  6.2780  7.834245  7.1960  7.824 2621.588 10000
       fr(x)  2.716  3.1580  3.636770  3.3810  3.854   72.124 10000
```

### Which type of `.Call`?

Note also a speedup if we use a character value for the function name in `.Call`, rather than the function name directly which passes the registration.  There is some variation (I ran `microbenchmark` three times below) but the advantage is on the order of about 1%.

```R
> fastrank_
$name
[1] "fastrank_"

$address
<pointer: 0x7fb9dd708b60>
attr(,"class")
[1] "RegisteredNativeSymbol"

$package
DLL name: fastrank
Filename: /Users/douglasgscofield/Dropbox/_my-R-packages/fastrank/src/fastrank.so
Dynamic lookup: TRUE

$numParameters
[1] 2

attr(,"class")
[1] "CallRoutine"      "NativeSymbolInfo"
> fr_char <- function(x) .Call("fastrank_", x, "average")
> fr_name <- function(x) .Call(fastrank_, x, "average")
> microbenchmark(fastrank(x, "average"), fr_char(x), fr_name(x), times=100000)
Unit: microseconds
                   expr   min    lq     mean median    uq       max neval
 fastrank(x, "average") 5.278 5.717 6.982661  5.890 6.113 36695.486 1e+05
             fr_char(x) 2.661 2.907 3.299201  3.031 3.158  4985.642 1e+05
             fr_name(x) 2.704 2.954 3.425349  3.075 3.203  4881.712 1e+05
> microbenchmark(fastrank(x, "average"), fr_char(x), fr_name(x), times=100000)
Unit: microseconds
                   expr   min    lq     mean median    uq       max neval
 fastrank(x, "average") 5.264 5.720 6.860308  5.893 6.107  5091.558 1e+05
             fr_char(x) 2.655 2.907 3.471290  3.027 3.152 35767.413 1e+05
             fr_name(x) 2.695 2.958 3.333991  3.076 3.202  4856.643 1e+05
> microbenchmark(fastrank(x, "average"), fr_char(x), fr_name(x), times=100000)
Unit: microseconds
                   expr   min    lq     mean median    uq       max neval
 fastrank(x, "average") 5.301 5.709 6.999949  5.875 6.100 37587.497 1e+05
             fr_char(x) 2.653 2.902 3.245166  3.022 3.147  5041.442 1e+05
             fr_name(x) 2.697 2.952 3.427632  3.076 3.204  5123.834 1e+05
```

Avoiding the R interface entirely gives us even more performance.  This also makes more clear the difference between the ways of specifiying the function to `.Call`.

```R
> microbenchmark(.Internal(rank(x, length(x), "average")), .Call("fastrank_", x, "average"), .Call(fastrank_, x, "average"), times=100000)
Unit: microseconds
                                     expr   min    lq     mean median    uq       max neval
 .Internal(rank(x, length(x), "average")) 1.042 1.133 1.327148  1.171 1.243  8696.792 1e+05
         .Call("fastrank_", x, "average") 2.361 2.487 2.994419  2.558 2.646  9965.114 1e+05
           .Call(fastrank_, x, "average") 2.406 2.589 3.200212  2.657 2.743 10298.581 1e+05
```

Note for `fastrank` we get a large performance boost by avoiding the R wrapper.


### Which type of sort?

Compare `R_orderVector` (1) with `fr_quicksort_double_i_` (2): 

```R
> y
 [1] 108 101 101 105 109 101 105 110 107 105
> microbenchmark(fastrank_num_avg(y), fastrank(y, sort.method=1L), fastrank(y, sort.method=2L), times=100000)
Unit: microseconds
                          expr   min    lq     mean median    uq      max neval
           fastrank_num_avg(y) 1.263 1.632 1.779896 1.7430 1.845  735.960 1e+05
 fastrank(y, sort.method = 1L) 3.406 3.814 4.226931 3.9770 4.197 1597.970 1e+05
 fastrank(y, sort.method = 2L) 3.253 3.629 4.038412 3.7925 4.006 1646.718 1e+05
```

The difference is clearer with a longer vector.

```R
> y = as.numeric(sample(100, 50, replace=TRUE)) + 1000
> microbenchmark(rank(y), rank_new(y), fastrank_num_avg(y), fastrank(y, sort.method=1L), fastrank(y, sort.method=2L), times=100000)
Unit: microseconds
                          expr    min     lq      mean median     uq       max neval
                       rank(y) 24.436 27.201 31.147960 27.953 28.857  5090.032 1e+05
                   rank_new(y)  1.572  2.018  2.373838  2.159  2.291  4246.831 1e+05
           fastrank_num_avg(y)  1.966  2.526  3.190666  2.760  3.207  3313.549 1e+05
 fastrank(y, sort.method = 1L)  6.349  7.255  8.621403  7.652  8.782  3098.557 1e+05
 fastrank(y, sort.method = 2L)  4.091  4.913  6.576812  5.270  6.390 35683.247 1e+05
```

## Remaining performance questions

Of course I want to squeeze as much time as I can, so need to explore an updated `fastrank_num_avg` since the direct entries *should* always be fastest, but there are a few more general points to explore. 

* Is there  a difference in time between using the older `.C` interface and `.Call`?
* Is it faster to compute length of `x` internally, as I do now, or accept it as a passed argument like `.Internal(rank(...))`?
* In general, how does `.Internal(rank(...))` receive its arguments, and return its results?  In the benchmarks above there are such performance differences between different interface options.
* Does it make a difference to byte-compile the R wrapper?




LICENSE
-------

GPL 2, just like R itself.
