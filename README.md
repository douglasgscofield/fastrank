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

**Result so far:** Quicksort and shellsort are faster than `R_orderVector`, and get faster with vector length.  Shellsort with Ciura-gaps is a touch faster 10 to 100 vector length, but quicksort is definitely faster at 10000 vector length.  This gaps issue definitely needs some more work.

According to <http://en.wikipedia.org/wiki/Insertion_sort>, insertion sort is faster than quicksort when there are less than about 10 elements.  Should I implement this speedup?

####  R_orderVector vs. Quicksort

Compare `R_orderVector` (1) with `fr_quicksort_double_i_` (2).  I modified the type of my quicksort to be `int` to match `R_orderVector`.

```R
> y
 [1] 108 101 101 105 109 101 105 110 107 105
> microbenchmark(rank(y), rank_new(y), fastrank_num_avg(y), fastrank(y, sort.method=1L), fastrank(y, sort.method=2L), times=100000)
Unit: nanoseconds
                          expr   min    lq      mean median    uq      max neval
                       rank(y) 21935 24419 26731.610  25078 25790 33472509 1e+05
                   rank_new(y)   645  1062  1229.118   1192  1320   915326 1e+05
           fastrank_num_avg(y)  1279  1802  2135.015   2009  2448   866067 1e+05
 fastrank(y, sort.method = 1L)  3501  4231  5041.972   4589  5693   975332 1e+05
 fastrank(y, sort.method = 2L)  3294  4036  4896.239   4389  5471  1791393 1e+05
```
adding explicit `ties.method = "average"` where I can:
```R
> rank_new <- function(x, ties.method="average") .Internal(rank(x, length(x), ties.method))
> microbenchmark(rank(y, ties.method="average"), rank_new(y, ties.method="average"), fastrank_num_avg(y), fastrank(y, ties.method="average", sort.method=1L), fastrank(y, ties.method="average", sort.method=2L), times=100000)
Unit: nanoseconds
                                                   expr   min    lq      mean median    uq      max neval
                       rank(y, ties.method = "average") 24626 27346 29872.924  28052 28833 34338523 1e+05
                   rank_new(y, ties.method = "average")   745  1207  1442.952   1373  1537   949948 1e+05
                                    fastrank_num_avg(y)  1298  1809  2216.323   2031  2480  2571378 1e+05
 fastrank(y, ties.method = "average", sort.method = 1L)  3519  4296  5137.635   4645  5759   911103 1e+05
 fastrank(y, ties.method = "average", sort.method = 2L)  3365  4093  5001.411   4434  5541   947911 1e+05
```

The difference between sort methods increases as vector length grows.

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
> yyy = as.double(sample(10000, 10000, replace=TRUE))
> microbenchmark(rank(yyy), rank_new(yyy), fastrank(yyy, sort=1L), fastrank(yyy, sort=2L), times=1000)
Unit: microseconds
                     expr      min        lq      mean    median        uq       max neval
                rank(yyy) 1305.333 1333.2425 1496.8820 1356.7100 1408.1640 33502.222  1000
            rank_new(yyy) 1022.618 1031.4735 1097.7066 1054.5470 1078.7895  3135.501  1000
 fastrank(yyy, sort = 1L) 2603.435 2633.9570 2770.0089 2675.6040 2733.2100  5759.185  1000
 fastrank(yyy, sort = 2L)  771.286  787.1900  838.3540  802.7980  822.3890  3434.097  1000
```

#### Quicksort vs. three different shellsorts

Now to add shellsort, with three implementations of gap distances, following the Wikipedia page for shellsort: Ciura (`3L`), Sedgwick (`4L`, R uses this), and Tokuda (`5L`).  This is in addition to quicksort (`2L`).

```R
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=2L), fastrank(y, sort=3L), fastrank(y, sort=4L), fastrank(y, sort=5L), times=100000)
Unit: nanoseconds
                   expr   min    lq    mean median    uq      max neval
                rank(y) 22125 24859 27353.3  25544 26422  3011293 1e+05
            rank_new(y)   715  1154  1404.9   1293  1462   980087 1e+05
 fastrank(y, sort = 2L)  3341  3940  4832.4   4289  5027  1054206 1e+05
 fastrank(y, sort = 3L)  3285  3909  4793.3   4254  5005  1336451 1e+05
 fastrank(y, sort = 4L)  3283  3905  5134.4   4260  5048 34984009 1e+05
 fastrank(y, sort = 5L)  3321  3904  4748.3   4255  5005  1184770 1e+05
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy, sort=2L), fastrank(yy, sort=3L), fastrank(yy, sort=4L), fastrank(yy, sort=5L), times=100000)
Unit: microseconds
                    expr    min     lq    mean median      uq     max neval
                rank(yy) 27.129 30.446 36.6337 31.249 32.2760 37390.8 1e+05
            rank_new(yy)  2.738  3.302  3.6803  3.468  3.6480  3886.2 1e+05
 fastrank(yy, sort = 2L)  4.877  5.684  7.1287  6.048  7.1675  4560.8 1e+05
 fastrank(yy, sort = 3L)  4.825  5.608  6.8699  5.963  7.0120  3886.8 1e+05
 fastrank(yy, sort = 4L)  4.761  5.589  7.2594  5.940  6.9760  6390.8 1e+05
 fastrank(yy, sort = 5L)  4.767  5.592  7.3154  5.948  6.9990  6705.2 1e+05
> microbenchmark(rank(yyy), rank_new(yyy), fastrank(yyy, sort=2L), fastrank(yyy, sort=3L), fastrank(yyy, sort=4L), fastrank(yyy, sort=5L), times=10000)
Unit: microseconds
                     expr     min      lq    mean  median      uq     max neval
                rank(yyy) 1303.76 1357.96 1493.97 1381.67 1425.34  7939.7 10000
            rank_new(yyy) 1021.53 1051.86 1132.73 1068.39 1092.25 38452.1 10000
 fastrank(yyy, sort = 2L)  768.36  796.68  862.12  815.01  836.12  6217.7 10000
 fastrank(yyy, sort = 3L) 1053.28 1085.09 1159.29 1100.95 1125.74 39565.2 10000
 fastrank(yyy, sort = 4L)  961.48  990.81 1064.55 1007.32 1030.55  7599.5 10000
 fastrank(yyy, sort = 5L) 1064.45 1096.13 1162.95 1111.71 1137.29  7471.9 10000
```
I'll do a recreation of `y`, `yy` and `yyy` and see if it changes:
```R
> yyy = as.double(sample(10000, 10000, replace=TRUE))
> yy = as.double(sample(100, 100, replace=TRUE))
> y = as.double(sample(10, 10, replace=TRUE))
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=2L), fastrank(y, sort=3L), fastrank(y, sort=4L), fastrank(y, sort=5L), times=100000)
Unit: nanoseconds
                   expr   min    lq    mean median      uq      max neval
                rank(y) 21959 24804 27706.5  25512 26426.0 36181742 1e+05
            rank_new(y)   718  1156  1398.0   1294  1459.0  1089264 1e+05
 fastrank(y, sort = 2L)  3314  3931  4831.2   4281  5062.5  3505259 1e+05
 fastrank(y, sort = 3L)  3281  3909  4723.3   4256  5009.0  1023524 1e+05
 fastrank(y, sort = 4L)  3279  3900  4797.8   4254  5008.0  1874497 1e+05
 fastrank(y, sort = 5L)  3314  3914  5214.9   4266  5064.0 37951614 1e+05
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy, sort=2L), fastrank(yy, sort=3L), fastrank(yy, sort=4L), fastrank(yy, sort=5L), times=100000)
Unit: microseconds
                    expr    min     lq    mean median      uq     max neval
                rank(yy) 27.194 30.419 36.6001 31.238 32.3960  6531.8 1e+05
            rank_new(yy)  2.817  3.381  3.8633  3.548  3.7330  4071.3 1e+05
 fastrank(yy, sort = 2L)  5.053  5.882  7.5902  6.248  7.4710  4168.5 1e+05
 fastrank(yy, sort = 3L)  4.848  5.713  7.2580  6.082  7.2660  4610.4 1e+05
 fastrank(yy, sort = 4L)  4.778  5.664  7.6421  6.021  7.1930 37804.2 1e+05
 fastrank(yy, sort = 5L)  4.887  5.799  7.2520  6.165  7.3485  4159.7 1e+05
> microbenchmark(rank(yyy), rank_new(yyy), fastrank(yyy, sort=2L), fastrank(yyy, sort=3L), fastrank(yyy, sort=4L), fastrank(yyy, sort=5L), times=1000)
Unit: microseconds
                     expr     min      lq    mean  median      uq    max neval
                rank(yyy) 1308.30 1352.39 1487.16 1377.26 1417.61 7832.1  1000
            rank_new(yyy) 1020.99 1048.81 1103.38 1064.71 1087.65 1845.5  1000
 fastrank(yyy, sort = 2L)  753.90  775.76  847.48  795.55  816.05 6842.6  1000
 fastrank(yyy, sort = 3L) 1055.15 1087.10 1163.28 1103.29 1128.67 7432.4  1000
 fastrank(yyy, sort = 4L)  964.95  985.61 1070.32 1006.85 1033.01 6733.0  1000
 fastrank(yyy, sort = 5L) 1071.72 1097.27 1170.60 1117.76 1142.90 6980.9  1000
```
and without repeats:
```R
> y = as.double(sample(10, 10, replace=FALSE))
> yy = as.double(sample(100, 100, replace=FALSE))
> yyy = as.double(sample(10000, 10000, replace=FALSE))
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=2L), fastrank(y, sort=3L), fastrank(y, sort=4L), fastrank(y, sort=5L), times=100000)
Unit: nanoseconds
                   expr   min    lq    mean median    uq      max neval
                rank(y) 21820 24971 27910.8  25721 26751  3601799 1e+05
            rank_new(y)   712  1161  1427.0   1307  1484  2661588 1e+05
 fastrank(y, sort = 2L)  3291  3945  5200.1   4307  5336 34690966 1e+05
 fastrank(y, sort = 3L)  3307  3910  4852.6   4269  5291  2957495 1e+05
 fastrank(y, sort = 4L)  3298  3911  4851.2   4275  5307  1066437 1e+05
 fastrank(y, sort = 5L)  3272  3913  4846.1   4272  5293  1001901 1e+05
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy, sort=2L), fastrank(yy, sort=3L), fastrank(yy, sort=4L), fastrank(yy, sort=5L), times=100000)
Unit: microseconds
                    expr    min     lq    mean median      uq     max neval
                rank(yy) 27.340 30.560 37.4109 31.412 32.6195 40227.8 1e+05
            rank_new(yy)  2.880  3.458  4.1874  3.628  3.8210  4551.8 1e+05
 fastrank(yy, sort = 2L)  4.813  5.551  7.2815  5.934  7.2020  6273.1 1e+05
 fastrank(yy, sort = 3L)  4.719  5.531  6.9593  5.903  7.1350  4583.1 1e+05
 fastrank(yy, sort = 4L)  4.565  5.433  7.0380  5.806  7.0395  4582.9 1e+05
 fastrank(yy, sort = 5L)  4.687  5.511  7.0318  5.886  7.1360  4414.6 1e+05
> microbenchmark(rank(yyy), rank_new(yyy), fastrank(yyy, sort=2L), fastrank(yyy, sort=3L), fastrank(yyy, sort=4L), fastrank(yyy, sort=5L), times=1000)
Unit: microseconds
                     expr     min      lq    mean  median      uq    max neval
                rank(yyy) 1241.12 1286.06 1432.83 1310.03 1350.22 7985.5  1000
            rank_new(yyy)  955.01  975.60 1047.65  993.63 1016.59 6012.3  1000
 fastrank(yyy, sort = 2L)  741.24  763.97  828.17  779.49  798.70 6503.4  1000
 fastrank(yyy, sort = 3L) 1025.60 1055.85 1126.23 1071.90 1098.06 7014.3  1000
 fastrank(yyy, sort = 4L)  930.95  955.53 1015.77  973.68  995.82 6791.8  1000
 fastrank(yyy, sort = 5L) 1042.63 1070.03 1140.68 1087.28 1112.41 6823.2  1000
```



### Which is faster, .Call or .C?

**Result:** `.Call`, definitely.  With length 10 and 100 it is about 25% faster and with length 10000 it is about 5% faster.

I wrote a `.C` version of the direct routine, `fastrank_num_avg_C_`.  Benchmark with a short vector shows it is slower than `.Call`, at a variety of vector sizes.

**Note also** that with vector length 10000 my direct routines are faster than even calling `.Internal(rank(...))`, yippee :-)  At vector lengths 10k or more, the advantage of calling the direct routine diminishes to almost nothing.  The direct routines are good for relatively shorter vectors.

```R
> y
 [1] 108 101 101 105 109 101 105 110 107 105
> fastrank_num_avg_C(y)
 [1]  8  2  2  5  9  2  5 10  7  5
> fastrank_num_avg(y)
 [1]  8  2  2  5  9  2  5 10  7  5
> microbenchmark(rank(y), rank_new(y), fastrank(y), fastrank_num_avg(y), fastrank_num_avg_C(y), times=100000)
Unit: nanoseconds
                  expr   min    lq      mean median    uq      max neval
               rank(y) 21840 24758 27237.822  25509 26304 34307087 1e+05
           rank_new(y)   703  1165  1339.865   1311  1458   877230 1e+05
           fastrank(y)  3459  4251  5041.774   4583  5583   984864 1e+05
   fastrank_num_avg(y)  3056  3780  4523.491   4092  4984   976645 1e+05
 fastrank_num_avg_C(y)  4709  5730  6734.765   6315  7336  1045547 1e+05
> yy = as.double(sample(100, 100, replace=TRUE))
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy), fastrank_num_avg(yy), fastrank_num_avg_C(yy), times=100000)
Unit: microseconds
                   expr    min     lq      mean median      uq       max neval
               rank(yy) 26.981 30.106 35.362497 30.954 31.9030  6030.124 1e+05
           rank_new(yy)  2.759  3.297  3.857391  3.459  3.6130  3683.018 1e+05
           fastrank(yy) 10.599 11.815 13.249962 12.268 13.2970  4209.686 1e+05
   fastrank_num_avg(yy)  4.617  5.438  6.974342  5.771  6.6745 36437.274 1e+05
 fastrank_num_avg_C(yy)  6.698  7.851  9.591220  8.487  9.5000  6090.163 1e+05
> yyy = as.double(sample(10000, 10000, replace=TRUE))
> microbenchmark(rank(yyy), rank_new(yyy), fastrank(yyy, sort=1L), fastrank(yyy, sort=2L), fastrank_num_avg(yyy), fastrank_num_avg_C(yyy), times=1000)
Unit: microseconds
                     expr      min        lq      mean    median        uq       max neval
                rank(yyy) 1305.333 1333.2425 1496.8820 1356.7100 1408.1640 33502.222  1000
            rank_new(yyy) 1022.618 1031.4735 1097.7066 1054.5470 1078.7895  3135.501  1000
 fastrank(yyy, sort = 1L) 2603.435 2633.9570 2770.0089 2675.6040 2733.2100  5759.185  1000
 fastrank(yyy, sort = 2L)  771.286  787.1900  838.3540  802.7980  822.3890  3434.097  1000
    fastrank_num_avg(yyy)  769.002  784.2395  846.8260  801.3095  823.0885  3014.107  1000
  fastrank_num_avg_C(yyy)  806.524  824.8475  891.3752  839.8600  863.3015  3237.999  1000
```

## Remaining performance questions

Of course I want to squeeze as much time as I can, so need to explore an updated `fastrank_num_avg` since the direct entries *should* always be fastest, but there are a few more general points to explore. 

* Is it faster to compute length of `x` internally, as I do now, or accept it as a passed argument like `.Internal(rank(...))`?
* In general, how does `.Internal(rank(...))` receive its arguments, and return its results?  In the benchmarks above there are such performance differences between different interface options.
* Does it make a difference to byte-compile the R wrapper?




LICENSE
-------

GPL 2, just like R itself.
