fastrank: faster than `rank`
=================================

We are doing a re-write of the API.

`fastrank` an R package providing fast ranking for integer, numeric, logical
and complex vectors, as an alternative to calling `.Internal(rank(...))`, which
packages cannot do.  Its API is a bit more restrictive, in the interests of
speed.  You cannot sort `character` vectors or vectors containing `NA` with
`fastrank`. if you need these capabilities, use base `rank` or convert your
data to a form accepted by `fastrank`.

The package provides a general interface via the `fastrank` function, a
replacement for the base R `rank`.  It accepts any of the above accepted
datatypes and any `ties.method`:

```R
fastrank(x, ties.method = c("average", "first", "random", "max", "min"))
```

There are also direct interfaces for specific data types with specific
tie-breaking methods, if you can guarantee the data type of your vectors.
These are slightly faster for shorter vectors because setup time is reduced:

```R
fastrank_num_avg(x)
```

For all functions, the ranks of `x` are returned in a vector of the same length
as `x`.  As with `rank`, for `ties.method = "average"` the vector returned is
`numeric` because ranks can be fractional; for all other methods the vector is
`integer`.

`fastrank` is designed to handle all `ties.method` arguments identically to
base `rank`.  `fastrank` handles `"first"` and `"random"` in C code, so should
be much faster for these.  **Unfortunately, the `"first"` method is not
currently comparable** because the sort routine used does not preserve the
order of equivalent items.  In future `fastrank` may switch to using a stable
sort if `"first"` is requested.

No `fastrank` entry handles `NA` in data, nor do they accept `character`
vectors for ranking.  The `Scollate` internal R routines for comparing
character strings using locales is not part of the R API, and it would probably
be a bigger job to provide this than the rest of `fastrank`.



Performance
===========

For complete benchmarking and performance details, see [BENCHMARKING.md][BENCHMARKING.md].

[BENCHMARKING.md]: https://github.com/douglasgscofield/fastrank/blob/master/BENCHMARKING.md

We are *almost* as fast as `.Internal(rank(...))` for vectors length 10, and the direct routines are about 10% faster than the general routine for short vectors, about 5% faster for 100 vectors, and essentially no difference for 10000 vectors.

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



Motivation
==========

The motivation for this comes from my development of the `nestedRanksTest`
package.  A standard run with the default 10,000 bootstrap iterations takes
a few seconds to complete on a test data set.

```R
> library(nestedRanksTest)
> data(woodpecker_multiyear)
> adat <- subset(woodpecker_multiyear, Species == "agrifolia")
> system.time(with(adat, nestedRanksTest(y = Distance, x = Year, groups = Granary)))
   user  system elapsed 
  5.252   0.067   5.318 
```

Profiling with `library(lineprof)` revealed that the bottleneck was in the base
R function `rank`.  A stripped-down `rank_new` that simply calls the
`.Internal(rank(...))` routine is 8-9&times; faster than the default rank for a
vector of 100 values, about 2&times; faster for 1000-value vectors, and only about
20-30% for 10,000-value vectors.

```R
> library(microbenchmark)
> rank_new <- function (x) .Internal(rank(x, length(x), "average"))
> yy <- rnorm(100)
> microbenchmark(rank(yy), rank_new(yy), times=100000)
Unit: microseconds
         expr    min     lq      mean median      uq       max neval
     rank(yy) 29.148 31.945 36.931556 32.678 33.5165 57192.350 1e+05
 rank_new(yy)  3.755  4.300  4.789952  4.542  4.7290  6784.741 1e+05
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



Implementation Details
======================

`fastrank` uses my own code, with some guidance for the basics of e.g., sorting
routines from Wikipedia.  In the course of writing the package I frequently
consulted R source for guidance in writing the C language interface, and when
benchmarking I implemented R's own shellsort for comparison purposes.  The
repository history can be consulted for details, and the `src/tst` directory
contains some test files.

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
slower than Quicksort and other methods.  **Perhaps I could handle character
data this way?  I would like to open up the API further...**

I also considered copy in its entirety the internal R function `do_rank` within
`src/main/sort.c` that is what we reach when doing the `.Internal(rank(...))`
call.  This ultimately proved to be impossible because of the numerous internal
features used that are not part of the R API.

Finally, I considered using C++ and the [**Rcpp**][Rcpp] package for this,
using an STL sorting routine which is probably quite comparable in performance
to what I have implemented.  However, my reading indicated that using `Rcpp`
pulls in some heavyweight object code, and I prefer to avoid that.

I still have some performance tweaking to do, but major decisions based on
benchmarking are now completed and the main structure is in place.  **Actually
this is no longer true**, I am looking at a couple of other sorting options for
general sorting, and am looking for a fast stable option for `ties.method = "first"`.

[R_orderVector]: http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Utility-functions
[Rcpp]: http://cran.r-project.org/web/packages/Rcpp/index.html



Remaining performance questions
===============================

Of course I want to squeeze as much time as I can, so need to explore an
updated `fastrank_num_avg` since the direct entries *should* always be fastest,
but there are a few more general points to explore. 

* What does GC Torture mean when it comes to benchmarking?
* Can I modify my Quicksort to be stable, for `"first"`?
* What about Dual-Pivot Quicksort or Quicksort?



Identity of results with `fastrank` vs. `rank`
==============================================

I have created a large set of tests for all `ties.method` values that check whether `rank` and `fastrank` are absolutely identical in their results.  So far this is true for `"average"`, `"max"`, and `"min"`, but not for `"first"`, because the sort used is not yet stable.  The `"random"` method needs to be checked, it is hoped we can duplicate `rank`'s behaviour if the seed is identical beforehand.  These have only been checked for `numeric` vectors.

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

