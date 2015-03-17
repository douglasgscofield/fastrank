fastrank: rank faster than `rank`
========

An R package providing fast ranking for integer and numeric vectors, as an
alternative to calling `.Internal(rank(...))`, which packages cannot do.

The motivation for this comes from my development of the `nestedRanksTest`
package.  A standard run with the default 10,000 bootstrap iterations takes
a few seconds to complete on a test data set.

```R
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
```

Profiling with `library(lineprof)` revealed that the bottleneck here is in the
utility function `nestedRanksTest_Z`, specifically in the calculation of ranks
via the base R function `rank`.  A stripped-down `rank_new` is 8-9&times;
faster than the default rank for a vector of 100 values: For 1000-value vectors
the speedup is more modest, about 2&times;, and for 10,000-value vectors the
speedup is only in the neighbourhood of 20-30%:

```R
> library(microbenchmark)
> rank_new <- function (x) .Internal(rank(x, length(x), "average"))
> yy <- rnorm(100)
> microbenchmark(rank(yy), rank_new(yy), times=100000)
Unit: microseconds
         expr    min     lq      mean median      uq       max neval
     rank(yy) 29.148 31.945 36.931556 32.678 33.5165 57192.350 1e+05
 rank_new(yy)  3.755  4.300  4.789952  4.542  4.7290  6784.741 1e+05
```

```R
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
```



`fastrank` API
==============

The general interface:

```R
fastrank(x, ties.method = c("average", "first", "random", "max", "min"))
```

The direct interfaces bypass data type and `ties.method` determination.

```R
fastrank_num_avg(x)
...
```

`fastrank` handles all `ties.method` arguments identically to base `rank`.
`fastrank` handles `"first"` and `"random"` in C code, so should be much faster
for these.

No `fastrank` entry handles `NA` in data, nor do they accept `character`
vectors for ranking.  The `Scollate` internal R
routines for comparing character strings using locale information is not part
of the R API, and it would probably be a
bigger job to provide this than the rest of `fastrank`.

If you need to sort `NA` or `character`, use base `rank`.



Implementation Details
======================

`fastrank` uses my own code, with some guidance for the basics of e.g., sorting
routines from Wikipedia.  In the course of writing the package I frequently
consulted R source for guidance in writing the C language interface, and when
benchmarking I implemented R's own shellsort for comparison purposes.  The
repository history can be consulted for details.

The sort routine at the heart is Quicksort, modified to operate on a vector of
indices rather than the array of values, and also modified to shortcut to an
insertion sort of vector length equal to or shorter than
`QUICKSORT_INSERTION_CUTOFF`, currently set to 20.  See benchmarking results
for much more on sort routine selection.

The main sorting and assigning of ranks is coded in C macros, with concrete
types and comparison functions supplied via macro arguments.  This means that
while there is less duplication of code features in the source, there is some
duplication in the final object code.  This is however likely quite fast, and
datatype-specific optimisations can be applied wherever possible.  I have not
benchmarked my concrete expansions against a more generic approach, but common
sense suggests concrete is faster.

I considered using R's own sorting routines, e.g.,
[`R_orderVector`][R_orderVector], especially considering it can handle any type
of atomic `SEXP`, but benchmarking presented below demonstrated that it is
slower than Quicksort and other methods.

I also considered copy in its entirety the internal R function `do_rank` within
`src/main/sort.c` that is what we reach when doing the `.Internal(rank(...))`
call.  This ultimately proved to be impossible because of the numerous internal
features used that are not part of the R API.

Finally, I considered using C++ and the [**Rcpp**][Rcpp] package for this,
using an STL sorting routine which is probably quite comparable in performance
to what I have implemented.  However, my reading indicated that using `Rcpp`
pulls in some heavyweight object code, and I prefer to avoid that.

I still have some performance tweaking to do, but major decisions based on
benchmarking are now completed and the main structure is in place.

[R_orderVector]: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions
[Rcpp]: http://cran.r-project.org/web/packages/Rcpp/index.html


How does R handle `.Internal`?
==============================

We are interested in this because we are trying to beat `.Internal(rank(...))`
under all conditions.  I think because of the way this works, we will *not*
beat it for short vectors because the call overhead for `.Call` is simply
greater than for `.Internal` and there is no way around this.

`.Internal` is handled via a special dispatch table that is compiled into base
R.  It is described in the R Internals manual, and at a blog post.

* http://cran.r-project.org/doc/manuals/r-release/R-ints.html#g_t_002eInternal-vs-_002ePrimitive
* http://userprimary.net/posts/2010/03/14/r-internals-quick-tour/



Performance Progress
====================

**Note:** This narrative progresses in time and shows the various improvements
as they happened.  See the end for the latest results.

## Initial results with `fastrank_numeric_average`

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


## Faster after registering `fastrank_numeric_average`

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


## General `fastrank`

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


## R wrapper call overhead

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



## Which type of `.Call`?

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


## Which type of sort?

This is now completed, so...

### Summary of sort routine benchmarking

* For `n=10` vectors, all methods perform similarly, and the advantage of `.Internal(rank(...))` seems to be in the call interface.
* For `n=100` vectors, the methods still perform similarly, but the Quicksort with insertion sort is just a bit better than the rest.  Sedgwick shellsort is the best of those, with Ciura2 and Tokuda2 better than their default versions.  The advantage of `.Internal(rank(...))` still exists and is still sizable.
* For `n=10000` vectors, the methods start to separate.  Quicksort with insertion sort is clearly the best and is anywhere from 50% faster (random data) to 100% faster (worst-case data) than `.Internal(rank(...))`.  Sedgwick shellsort is still the best of those.  All methods are at least twice as fast as `rank(...)`.
* All Quicksort and shellsort methods are faster than `R_orderVector`, and get faster with vector length.

R's default Sedgwick shellsort is very good, but quicksort is better especially with the insertion sort speedup.  That is what I will go with.

###  R_orderVector vs. Quicksort

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

### Quicksort vs. three different shellsorts

Now to add shellsort, with three implementations of gap distances, following the Wikipedia page for shellsort: Ciura (`3L`), Sedgwick (`4L`, R uses this), and Tokuda (`5L`).  This is in addition to quicksort (`2L`).

**Result:** Sedgwick gaps work best for 10000 (and presumably longer) vectors, while Ciura and Tokuda are better for shorter vectors with Ciura very slightly better.

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

### Shellsort vs. Quicksort with insertion-sort shortcuts

After some research I've adjusted Quicksort so below a certain vector length
cutoff (the default 2 for `2L`, 11 for `6L`, 21 for `7L`), it switches to
insertion sort.  Note this will apply to all the subiterations of the Quicksort
call tree as well.

**Result:** It looks like the cutoff version is definitely best, with the 21 version best of the two non-trivial versions.  Now it is a competition between Shellsort with Ciura gaps and Quicksort with insertion-sort cutoff of 21.

```R
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=2L), fastrank(y, sort=3L), fastrank(y, sort=4L), fastrank(y, sort=5L), fastrank(y, sort=6L), fastrank(y, sort=7L), times=100000)
Unit: nanoseconds
                   expr   min    lq    mean median    uq      max neval
                rank(y) 22325 24726 26989.0  25282 25938 34553695 1e+05
            rank_new(y)   732  1131  1302.0   1246  1397  1090550 1e+05
 fastrank(y, sort = 2L)  3318  3787  4900.7   4081  4470 34794533 1e+05
 fastrank(y, sort = 3L)  3262  3746  4402.3   4038  4425  1188386 1e+05
 fastrank(y, sort = 4L)  3247  3746  4490.5   4044  4441  2693303 1e+05
 fastrank(y, sort = 5L)  3275  3747  4462.1   4041  4431  1210320 1e+05
 fastrank(y, sort = 6L)  3230  3721  4483.9   4017  4409  1153417 1e+05
 fastrank(y, sort = 7L)  3263  3719  4448.6   4011  4400  1812214 1e+05
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy, sort=2L), fastrank(yy, sort=3L), fastrank(yy, sort=4L), fastrank(yy, sort=5L), fastrank(yy, sort=6L), fastrank(yy, sort=7L), times=100000)
Unit: microseconds
                    expr    min     lq    mean median     uq     max neval
                rank(yy) 27.719 30.641 37.1657 31.354 32.247 37732.8 1e+05
            rank_new(yy)  2.897  3.447  3.9594  3.592  3.746  3871.8 1e+05
 fastrank(yy, sort = 2L)  5.129  6.115  7.5281  6.476  7.005  4148.1 1e+05
 fastrank(yy, sort = 3L)  4.792  5.925  7.0567  6.347  6.973  4228.5 1e+05
 fastrank(yy, sort = 4L)  4.742  5.853  7.0445  6.233  6.783  4019.8 1e+05
 fastrank(yy, sort = 5L)  4.759  5.893  7.2277  6.295  6.891  3975.7 1e+05
 fastrank(yy, sort = 6L)  4.481  5.217  6.2938  5.515  5.929  4015.2 1e+05
 fastrank(yy, sort = 7L)  4.471  5.165  6.3492  5.458  5.845  4381.4 1e+05
> microbenchmark(rank(yyy), rank_new(yyy), fastrank(yyy, sort=2L), fastrank(yyy, sort=3L), fastrank(yyy, sort=4L), fastrank(yyy, sort=5L), fastrank(yyy, sort=6L), fastrank(yyy, sort=7L), times=10000)
Unit: microseconds
                     expr     min      lq    mean  median      uq     max neval
                rank(yyy) 1298.77 1327.51 1430.19 1342.53 1383.03 39838.8 10000
            rank_new(yyy) 1014.79 1025.12 1072.15 1032.54 1060.19  7252.4 10000
 fastrank(yyy, sort = 2L)  764.42  783.65  827.70  788.46  809.03  7073.0 10000
 fastrank(yyy, sort = 3L) 1021.94 1035.91 1082.38 1038.91 1068.78  7807.9 10000
 fastrank(yyy, sort = 4L)  851.50  865.56  907.02  868.46  892.03  7179.7 10000
 fastrank(yyy, sort = 5L) 1034.92 1049.03 1095.74 1052.08 1082.37  7108.3 10000
 fastrank(yyy, sort = 6L)  621.66  639.22  677.92  643.24  661.06 37054.7 10000
 fastrank(yyy, sort = 7L)  570.68  586.12  626.09  589.70  606.95  6908.6 10000
```

For vectors with no duplicates:

```R
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=2L), fastrank(y, sort=3L), fastrank(y, sort=4L), fastrank(y, sort=5L), fastrank(y, sort=6L), fastrank(y, sort=7L), times=100000)
Unit: nanoseconds
                   expr   min    lq    mean median    uq      max neval
                rank(y) 22448 24912 28109.5  25502 26226 35386821 1e+05
            rank_new(y)   710  1132  1334.1   1249  1404  1144394 1e+05
 fastrank(y, sort = 2L)  3335  3821  4553.3   4120  4525  1183310 1e+05
 fastrank(y, sort = 3L)  3276  3778  4496.1   4076  4476  1167939 1e+05
 fastrank(y, sort = 4L)  3256  3775  4566.0   4079  4487  1182768 1e+05
 fastrank(y, sort = 5L)  3290  3782  4546.1   4079  4474  1157244 1e+05
 fastrank(y, sort = 6L)  3243  3752  4515.3   4050  4448  1930598 1e+05
 fastrank(y, sort = 7L)  3241  3749  4513.5   4048  4447  1807156 1e+05
> microbenchmark(rank(yy), rank_new(yy), fastrank(yy, sort=2L), fastrank(yy, sort=3L), fastrank(yy, sort=4L), fastrank(yy, sort=5L), fastrank(yy, sort=6L), fastrank(yy, sort=7L), times=100000)
Unit: microseconds
                    expr    min     lq    mean  median     uq     max neval
                rank(yy) 27.727 30.597 36.1604 31.3020 32.181 38366.0 1e+05
            rank_new(yy)  2.910  3.437  3.8273  3.5780  3.725  5901.0 1e+05
 fastrank(yy, sort = 2L)  5.019  6.043  7.3058  6.4070  6.923  4566.8 1e+05
 fastrank(yy, sort = 3L)  4.722  5.813  7.0080  6.2195  6.823  4035.7 1e+05
 fastrank(yy, sort = 4L)  4.634  5.675  7.0901  6.0500  6.585  4032.0 1e+05
 fastrank(yy, sort = 5L)  4.756  5.902  7.3255  6.3250  6.946  4254.4 1e+05
 fastrank(yy, sort = 6L)  4.511  5.285  6.7856  5.5910  5.994 37152.1 1e+05
 fastrank(yy, sort = 7L)  4.473  5.172  6.3126  5.4630  5.836  4198.7 1e+05
> microbenchmark(rank(yyy), rank_new(yyy), fastrank(yyy, sort=2L), fastrank(yyy, sort=3L), fastrank(yyy, sort=4L), fastrank(yyy, sort=5L), fastrank(yyy, sort=6L), fastrank(yyy, sort=7L), times=10000)
Unit: microseconds
                     expr     min      lq    mean  median      uq     max neval
                rank(yyy) 1233.49 1263.47 1345.28 1268.61 1299.94  7671.6 10000
            rank_new(yyy)  951.45  962.99  994.51  965.10  984.29  7069.6 10000
 fastrank(yyy, sort = 2L)  746.75  768.49  808.69  773.22  788.25 36223.9 10000
 fastrank(yyy, sort = 3L) 1000.74 1015.58 1056.07 1017.83 1038.22  7216.9 10000
 fastrank(yyy, sort = 4L)  827.13  841.99  883.14  844.40  862.52  7768.2 10000
 fastrank(yyy, sort = 5L) 1019.41 1032.75 1079.63 1035.01 1054.63 40789.9 10000
 fastrank(yyy, sort = 6L)  639.68  657.38  698.54  661.24  673.58  7215.6 10000
 fastrank(yyy, sort = 7L)  582.43  598.85  637.22  602.31  614.47  8150.8 10000
```

### Shellsort and Quicksort vs. Shellsort with the small gaps dropped

I wondered whether Shellsort with the last small gaps dropped (this shifting to insertion sort a bit earlier) would work better, so I created sort methods `8L` vs. `3L`, `9L` vs. `4L`, and `10L` vs. `5L` which do this.

**Result:** It didn't really affect Ciura or Tokuda for 10 and 100 vectors, it slowed Sedgwick down most notably for 100 vectors.  Sedgwick is still best for 10000 vectors no matter which gap scheme, and it sped up Ciura and Tokuda for 10000 vectors.  Quicksort still wins for 10000 vectors hands-down, and pretty much wins for all vector lengths.

```R
> y.orig <- y; yy.orig <- yy; yyy.orig <- yyy; rm(y,yy,yyy)
> y <- y.orig
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=3L), fastrank(y, sort=8L), fastrank(y, sort=4L), fastrank(y, sort=9L), fastrank(y, sort=5L), fastrank(y, sort=10L), fastrank(y, sort=7L), times=100000)
Unit: nanoseconds
                    expr   min    lq    mean median      uq      max neval
                 rank(y) 22715 25460 27755.2  26056 26747.0 42516458 1e+05
             rank_new(y)   724  1149  1389.1   1261  1416.0  1231880 1e+05
  fastrank(y, sort = 3L)  3316  3785  4491.5   4072  4460.0  3759215 1e+05
  fastrank(y, sort = 8L)  3308  3779  4554.4   4069  4460.0  3606849 1e+05
  fastrank(y, sort = 4L)  3320  3772  4588.0   4063  4461.0  4635569 1e+05
  fastrank(y, sort = 9L)  3302  3770  4452.9   4063  4455.5  1266400 1e+05
  fastrank(y, sort = 5L)  3321  3781  4422.2   4066  4455.0  1230516 1e+05
 fastrank(y, sort = 10L)  3334  3773  4421.2   4060  4442.0  1259044 1e+05
  fastrank(y, sort = 7L)  3308  3761  4424.0   4049  4439.0  1248294 1e+05
> y <- yy.orig
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=3L), fastrank(y, sort=8L), fastrank(y, sort=4L), fastrank(y, sort=9L), fastrank(y, sort=5L), fastrank(y, sort=10L), fastrank(y, sort=7L), times=100000)
Unit: microseconds
                    expr    min     lq    mean median     uq     max neval
                 rank(y) 28.074 31.430 36.6222 32.242 33.227  8813.2 1e+05
             rank_new(y)  2.914  3.495  4.4505  3.647  3.808 44816.2 1e+05
  fastrank(y, sort = 3L)  4.889  5.947  7.4753  6.372  6.978  5400.0 1e+05
  fastrank(y, sort = 8L)  5.127  6.070  7.2342  6.407  6.873  6047.7 1e+05
  fastrank(y, sort = 4L)  4.870  5.912  7.2208  6.299  6.836  6137.8 1e+05
  fastrank(y, sort = 9L)  5.319  6.254  7.5620  6.577  7.023  5553.2 1e+05
  fastrank(y, sort = 5L)  4.912  5.988  7.7511  6.405  6.982  7715.4 1e+05
 fastrank(y, sort = 10L)  5.107  6.068  7.5910  6.412  6.879  6172.0 1e+05
  fastrank(y, sort = 7L)  4.813  5.752  7.2376  6.066  6.504  5860.8 1e+05
> y <- yyy.orig
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=3L), fastrank(y, sort=8L), fastrank(y, sort=4L), fastrank(y, sort=9L), fastrank(y, sort=5L), fastrank(y, sort=10L), fastrank(y, sort=7L), times=10000)
Unit: microseconds
                    expr     min      lq    mean  median      uq     max neval
                 rank(y) 1298.67 1325.37 1408.24 1334.21 1362.49 42478.6 10000
             rank_new(y) 1012.83 1023.60 1056.65 1027.56 1046.35  9209.7 10000
  fastrank(y, sort = 3L) 1024.31 1036.94 1081.75 1039.58 1059.96  9365.8 10000
  fastrank(y, sort = 8L)  919.52  934.10  969.10  936.01  954.42  9413.3 10000
  fastrank(y, sort = 4L)  855.74  867.79  909.19  870.33  887.65 45789.0 10000
  fastrank(y, sort = 9L)  857.37  869.56  911.68  872.05  890.17  9104.0 10000
  fastrank(y, sort = 5L) 1037.06 1050.97 1089.12 1053.59 1073.25  8925.2 10000
 fastrank(y, sort = 10L)  927.02  940.44  980.17  942.36  960.35  8980.9 10000
  fastrank(y, sort = 7L)  582.45  596.44  630.69  598.84  610.11  9061.4 10000
```

### Shellsorts and Quicksorts with worst-case vectors

So I constructed some worst-case vectors.  I see they are all very similar with 10 vectors, Quicksort is starting to show its quality with 100 vectors, and clearly is best with 10000 vectors.  Sedgwick shellsort (`4L`) is also quite good.

```R
> y.rev <- as.numeric(10:1)
> yy.rev <- as.numeric(100:1)
> yyy.rev <- as.numeric(10000:1)
> y <- y.rev
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=3L), fastrank(y, sort=8L), fastrank(y, sort=4L), fastrank(y, sort=9L), fastrank(y, sort=5L), fastrank(y, sort=10L), fastrank(y, sort=7L), times=100000)
Unit: nanoseconds
                    expr   min    lq    mean median    uq      max neval
                 rank(y) 22643 25508 27689.9  26116 26815 42959798 1e+05
             rank_new(y)   722  1164  1343.2   1274  1424  1207910 1e+05
  fastrank(y, sort = 3L)  3300  3823  4611.2   4109  4478  6201784 1e+05
  fastrank(y, sort = 8L)  3323  3824  4454.7   4110  4478  1237219 1e+05
  fastrank(y, sort = 4L)  3348  3809  4472.4   4098  4470  1248926 1e+05
  fastrank(y, sort = 9L)  3329  3820  4517.6   4108  4480  1261753 1e+05
  fastrank(y, sort = 5L)  3336  3825  4468.8   4110  4473  1238837 1e+05
 fastrank(y, sort = 10L)  3329  3816  4523.5   4101  4468  3507769 1e+05
  fastrank(y, sort = 7L)  3289  3800  4456.5   4081  4451  1281328 1e+05
> y <- yy.rev
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=3L), fastrank(y, sort=8L), fastrank(y, sort=4L), fastrank(y, sort=9L), fastrank(y, sort=5L), fastrank(y, sort=10L), fastrank(y, sort=7L), times=100000)
Unit: microseconds
                    expr    min     lq    mean median     uq     max neval
                 rank(y) 27.784 30.550 35.9258 31.223 32.010  6435.7 1e+05
             rank_new(y)  2.675  3.167  3.6250  3.298  3.445  5508.2 1e+05
  fastrank(y, sort = 3L)  4.644  5.260  6.9515  5.539  5.895  9140.1 1e+05
  fastrank(y, sort = 8L)  4.809  5.391  6.4024  5.672  6.028  5857.7 1e+05
  fastrank(y, sort = 4L)  4.560  5.158  6.4265  5.436  5.795  9400.7 1e+05
  fastrank(y, sort = 9L)  4.802  5.403  6.8206  5.684  6.043  5886.2 1e+05
  fastrank(y, sort = 5L)  4.705  5.270  6.6419  5.551  5.907  5483.9 1e+05
 fastrank(y, sort = 10L)  4.733  5.352  6.4076  5.634  5.992  5410.9 1e+05
  fastrank(y, sort = 7L)  4.265  4.843  6.3471  5.120  5.474 44320.7 1e+05
> y <- yyy.rev
> microbenchmark(rank(y), rank_new(y), fastrank(y, sort=3L), fastrank(y, sort=8L), fastrank(y, sort=4L), fastrank(y, sort=9L), fastrank(y, sort=5L), fastrank(y, sort=10L), fastrank(y, sort=7L), times=1000)
Unit: microseconds
                    expr    min     lq   mean median     uq    max neval
                 rank(y) 561.89 581.20 658.14 583.80 600.22 8472.5  1000
             rank_new(y) 278.82 284.37 316.05 285.05 287.29 8074.6  1000
  fastrank(y, sort = 3L) 222.70 231.35 255.60 232.28 234.04 7869.5  1000
  fastrank(y, sort = 8L) 216.56 225.77 243.15 227.33 228.93 8012.5  1000
  fastrank(y, sort = 4L) 170.21 178.77 195.90 179.72 181.16 7609.5  1000
  fastrank(y, sort = 9L) 191.88 202.45 229.56 204.73 207.46 8685.1  1000
  fastrank(y, sort = 5L) 226.71 234.98 277.30 235.82 238.12 8731.2  1000
 fastrank(y, sort = 10L) 212.28 220.69 236.66 221.96 223.86 8205.7  1000
  fastrank(y, sort = 7L) 114.00 122.39 136.66 123.23 124.45 8202.6  1000
```



## Which is faster, .Call or .C?

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



## Which is faster, getting length internally or externally?

**Result:**  Internally.

The `.Internal(rank(...))` interface requires the length of `x` to be passed as
a separate argument.  I now suspect this is because the internal interface is a
high-performance direct-to-C API that quickly bypasses R objects.  In any
event, I was wondering whether simply passing `length(x)` might be faster than
getting length from `SEXP s_x`.  I wanted to use the direct interface to have
as few things as possible interfering.  Below, `fastrank_num_avg` is the usual
interface while `fastrank_num_avg2` passes the length as an extra argument.
The answer is clear, passing the extra argument is slower.  The `y` vectors 
used for benchmarking are worst-case.

```R
> microbenchmark(rank(y), rank_new(y), fastrank_num_avg(y), fastrank_num_avg2(y), times=100000)
Unit: nanoseconds
                 expr   min    lq    mean median      uq      max neval
              rank(y) 22128 24510 26630.9  25127 25816.5 40674852 1e+05
          rank_new(y)   721  1123  1328.2   1285  1453.0  1003306 1e+05
  fastrank_num_avg(y)  2988  3697  4472.8   4043  5189.0  2450161 1e+05
 fastrank_num_avg2(y)  3179  3886  4737.2   4233  5379.5  3907410 1e+05
> y <- yy.rev
> microbenchmark(rank(y), rank_new(y), fastrank_num_avg(y), fastrank_num_avg2(y), times=100000)
Unit: microseconds
                 expr    min     lq    mean median     uq     max neval
              rank(y) 27.250 29.589 34.2975 30.234 30.964  6050.2 1e+05
          rank_new(y)  2.650  3.128  3.7628  3.297  3.447  5429.5 1e+05
  fastrank_num_avg(y)  3.956  4.658  5.8471  4.984  6.161  4740.8 1e+05
 fastrank_num_avg2(y)  4.128  4.840  6.6161  5.168  6.360 42977.4 1e+05
> y <- yyy.rev
> microbenchmark(rank(y), rank_new(y), fastrank_num_avg(y), fastrank_num_avg2(y), times=1000)
Unit: microseconds
                 expr    min     lq   mean median     uq     max neval
              rank(y) 559.36 577.59 696.53 579.21 591.89 40264.5  1000
          rank_new(y) 278.34 283.83 303.69 284.41 287.30  5127.7  1000
  fastrank_num_avg(y) 113.86 121.30 139.58 122.20 123.57  4386.0  1000
 fastrank_num_avg2(y) 113.39 121.44 152.71 122.47 123.80  5025.4  1000
```



## `fastrank` vs. `fastrank_num_avg`

**Result:** The benefit of the direct interface more clear with shorter vectors, but the
difference really isn't that great.


```R
> microbenchmark(rank(y), rank_new(y), fastrank(y), fastrank_num_avg(y), times=100000)
Unit: nanoseconds
                expr   min    lq    mean median    uq      max neval
             rank(y) 21710 24807 26996.5  25440 26109 39123361 1e+05
         rank_new(y)   715  1153  1375.0   1325  1486  1113322 1e+05
         fastrank(y)  3169  3884  4671.5   4248  5449  1014344 1e+05
 fastrank_num_avg(y)  3057  3750  4514.5   4099  5239  1003286 1e+05
> y <- yy.rev
> microbenchmark(rank(y), rank_new(y), fastrank(y), fastrank_num_avg(y), times=100000)
Unit: microseconds
                expr    min     lq    mean median     uq     max neval
             rank(y) 27.090 29.597 34.5559 30.266 31.002 42357.5 1e+05
         rank_new(y)  2.661  3.135  3.4954  3.300  3.446  4464.5 1e+05
         fastrank(y)  4.071  4.774  6.2388  5.121  6.374  4602.3 1e+05
 fastrank_num_avg(y)  3.955  4.648  5.9011  4.984  6.142  4852.8 1e+05
> y <- yyy.rev
> microbenchmark(rank(y), rank_new(y), fastrank(y), fastrank_num_avg(y), times=1000)
Unit: microseconds
                expr    min     lq   mean median     uq    max neval
             rank(y) 558.40 577.92 653.69 579.69 596.00 6964.1  1000
         rank_new(y) 278.58 283.77 310.77 284.40 286.38 5609.6  1000
         fastrank(y) 112.60 120.73 137.01 122.12 123.72 5487.1  1000
 fastrank_num_avg(y) 112.84 120.83 148.52 122.00 123.65 6271.6  1000
```



## Does byte-compiling the R wrapper make a difference?

**Result:** Yes, especially with short vectors, and the difference matters for
both the general and the direct entry point.  I'm definitely setting byte-compile
in the DESCRIPTION.

```R
> library(compiler)
> frc = cmpfun(fastrank, options=list(optimize=3))
> frnac = cmpfun(fastrank_num_avg, options=list(optimize=3))
> y <- y.rev
> microbenchmark(rank_new(y),fastrank(y),frc(y),fastrank_num_avg(y),frnac(y),times=1000000)
Unit: nanoseconds
                expr  min   lq     mean median   uq      max neval
         rank_new(y)  695 1027 1143.954   1096 1192   806773 1e+06
         fastrank(y) 3030 3370 3762.967   3540 3759 56940488 1e+06
              frc(y) 2897 3252 3740.441   3422 3638 49041917 1e+06
 fastrank_num_avg(y) 2931 3296 3773.189   3459 3671 49656864 1e+06
            frnac(y) 2788 3166 3543.806   3329 3541 49618320 1e+06
> y <- yy.rev
> microbenchmark(rank_new(y),fastrank(y),frc(y),fastrank_num_avg(y),frnac(y),times=100000)
Unit: microseconds
                expr   min    lq     mean median    uq       max neval
         rank_new(y) 2.622 3.000 3.438997  3.116 3.236  5766.928 1e+05
         fastrank(y) 3.884 4.329 5.806631  4.526 4.762 39776.871 1e+05
              frc(y) 3.806 4.236 5.284891  4.431 4.661  8314.444 1e+05
 fastrank_num_avg(y) 3.802 4.255 5.377794  4.443 4.666  5895.072 1e+05
            frnac(y) 3.675 4.124 5.072473  4.314 4.538  5809.602 1e+05
> y <- yyy.rev
> microbenchmark(rank_new(y),fastrank(y),frc(y),fastrank_num_avg(y),frnac(y),times=10000)
Unit: microseconds
                expr     min       lq     mean   median       uq      max neval
         rank_new(y) 278.556 284.2855 319.3234 288.2875 316.2295 23629.18 10000
         fastrank(y) 112.096 119.7870 158.7820 122.3390 159.6330 23861.57 10000
              frc(y) 111.941 119.6615 159.0503 122.1085 159.3405 24332.65 10000
 fastrank_num_avg(y) 111.513 119.8810 161.6919 122.2870 159.6305 24007.11 10000
            frnac(y) 111.481 119.6720 163.0280 121.7155 158.8405 25117.82 10000
```



## `PACKAGE = "fastrank"` in R wrapper, and retrying `.C`

**Result:**  Whoa... **major** jump in performance, and forgetting `.C` for good.

I started doing reading about `.C`, `.Call`, `.Internal`, and `.External`, and
again ran across the advice to specify `.Call(..., PACKAGE = "fastrank")`.  Why
did I stop doing this?  I thought registration took care of this for me, but
apparently it does not give me all the benefit.

I also learned to specify `.C(..., DUP = FALSE, NAOK = TRUE, PACKAGE =
"fastrank")`, so thought to retry `.C`, though R's own docs for it say its
performance isn't as good as `.Call`, and along the way I thought I would retry
the options I just mentioned on `.Call`, too.

I went back to <https://github.com/douglasgscofield/fastrank/commit/23595574ab10da872ea53fa0ad53070969413274> to get code for `fastrank_num_avg_C`.

```R
> fastrank_num_avg
function(x) {
    .Call("fastrank_num_avg_", x, PACKAGE = "fastrank")
}
<environment: namespace:fastrank>
> fastrank_num_avg_C
function(x) {
    .C("fastrank_num_avg_C_", as.numeric(x), as.integer(length(x)),
       double(length(x)), NAOK = TRUE, DUP = FALSE, PACKAGE = "fastrank")[[3]]
<environment: namespace:fastrank>
> frcna = cmpfun(fastrank_num_avg, options=list(optimize=3))
> y <- y.rev
> frcnac = cmpfun(fastrank_num_avg_C, options=list(optimize=3))
> microbenchmark(rank_new(y), fastrank_num_avg(y), frcna(y), fastrank_num_avg_C(y), frcnac(y), times=100000)
Unit: nanoseconds
                  expr  min   lq     mean median   uq      max neval
           rank_new(y)  692 1054 1198.351   1146 1258   793828 1e+05
   fastrank_num_avg(y)  923 1251 1400.316   1355 1463   761700 1e+05
              frcna(y)  866 1243 1387.557   1348 1451   742734 1e+05
 fastrank_num_avg_C(y) 2389 2946 3221.436   3116 3303  1670381 1e+05
             frcnac(y) 2253 2835 3069.196   2991 3158   754608 1e+05
> y <- yy.rev
> microbenchmark(rank_new(y), fastrank_num_avg(y), frcna(y), fastrank_num_avg_C(y), frcnac(y), times=100000)
Unit: microseconds
                  expr   min    lq     mean median    uq       max neval
           rank_new(y) 2.631 3.076 4.222389  3.207 3.363 42883.400 1e+05
   fastrank_num_avg(y) 1.610 2.025 2.540816  2.156 2.306  9490.605 1e+05
              frcna(y) 1.536 2.026 2.445458  2.156 2.298  7042.351 1e+05
 fastrank_num_avg_C(y) 3.204 3.857 5.306157  4.060 4.302 10366.510 1e+05
             frcnac(y) 3.021 3.750 4.870311  3.945 4.178  7749.982 1e+05
> y <- yyy.rev
> microbenchmark(rank_new(y), fastrank_num_avg(y), frcna(y), fastrank_num_avg_C(y), frcnac(y), times=1000)
Unit: microseconds
                  expr     min       lq     mean   median       uq      max neval
           rank_new(y) 278.223 284.1030 323.5126 284.7050 286.2925 8409.026  1000
   fastrank_num_avg(y)  98.264 106.1365 128.9113 106.5320 107.0055 8448.284  1000
              frcna(y)  98.141 106.2425 112.1877 106.6025 107.0620  466.931  1000
 fastrank_num_avg_C(y) 107.054 115.8440 139.7009 116.3220 117.0055 8113.758  1000
             frcnac(y) 106.848 115.7135 145.7971 116.1490 116.6685 8097.104  1000
```


Best benchmarking results so far
================================

**Result:**  We are *almost* as fast as `.Internal(rank(...))` for vectors length 10, and the direct routines are about 10% faster than the general routine for short vectors, about 5% faster for 100 vectors, and essentially no difference for 10000 vectors.

```R
> frc = cmpfun(fastrank, options=list(optimize=3))
> y <- y.rev
> microbenchmark(rank_new(y), fastrank(y), frc(y), fastrank_num_avg(y), frcna(y), times=100000)
Unit: nanoseconds
                expr  min   lq     mean median   uq     max neval
         rank_new(y)  697  878 1039.175    937 1026 2436410 1e+05
         fastrank(y) 1000 1157 1288.889   1229 1331  785916 1e+05
              frc(y)  963 1119 1267.990   1192 1291  755307 1e+05
 fastrank_num_avg(y)  886 1044 1181.111   1113 1197 2219413 1e+05
            frcna(y)  846 1015 1129.037   1084 1172  764383 1e+05
> y <- yy.rev
> microbenchmark(rank_new(y), fastrank(y), frc(y), fastrank_num_avg(y), frcna(y), times=100000)
Unit: microseconds
                expr   min    lq     mean median    uq      max neval
         rank_new(y) 2.597 2.906 3.754443  3.029 3.218 10811.45 1e+05
         fastrank(y) 1.754 2.017 2.950814  2.145 2.401 11680.76 1e+05
              frc(y) 1.714 1.996 3.359293  2.135 2.394 12105.77 1e+05
 fastrank_num_avg(y) 1.634 1.906 2.901634  2.022 2.225 10798.09 1e+05
            frcna(y) 1.611 1.888 2.698567  2.015 2.224 11076.25 1e+05
> y <- yyy.rev
> microbenchmark(rank_new(y), fastrank(y), frc(y), fastrank_num_avg(y), frcna(y), times=5000)
Unit: microseconds
                expr     min       lq     mean   median       uq       max neval
         rank_new(y) 277.910 283.8145 311.0948 284.5125 286.2915  5120.002  5000
         fastrank(y) 107.366 116.2625 154.6016 117.4350 119.2650 37400.171  5000
              frc(y) 107.994 116.2720 147.2273 117.3980 119.1905  4979.048  5000
 fastrank_num_avg(y) 107.891 116.2345 142.0935 117.3810 118.9535  5063.963  5000
            frcna(y) 107.962 116.2380 136.6245 117.5415 119.0410  4836.530  5000
```



Remaining performance questions
===============================

Of course I want to squeeze as much time as I can, so need to explore an
updated `fastrank_num_avg` since the direct entries *should* always be fastest,
but there are a few more general points to explore. 

* What does GC Torture mean when it comes to benchmarking?



Remaining implementation questions
==================================

I must create a large set of tests for all `ties.method` values that verify that `rank` and `fastrank` and the direct entry points are absolutely identical in their results, including class of the returned vector.  One method to pay attention to is `ties.method = "first"`.  Is this stable for us?

```R
> y <- sample(10,10,repl=T)
> y
 [1] 8 6 8 3 3 6 3 9 6 6
> rank(y,ties="first")
 [1]  8  4  9  1  2  5  3 10  6  7
> y <- as.numeric(y)
> fastrank(y, "first")
 [1]  8  4  9  1  2  5  3 10  6  7
>
```



LICENSE
=======

GPL 2, just like R itself.
