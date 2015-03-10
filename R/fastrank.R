#' Ranking vectors with low overhead
#'
#' An R package providing fast ranking for integer and numeric vectors, as an
#' alternative to calling .Internal(rank(...)), which packages cannot do.
#'
#' The motivation for this comes from my development of the `nestedRanksTest`
#' package.  A standard run with the default 10,000 bootstrap iterations takes
#' a few seconds to complete on a test data set.
#'
#' ~~~~
#' > library(nestedRanksTest)
#' > data(woodpecker_multiyear)
#' > adat <- subset(woodpecker_multiyear, Species == "agrifolia")
#' > system.time(with(adat, nestedRanksTest(y = Distance, x = Year, groups = Granary)))
#'    user  system elapsed 
#'   5.252   0.067   5.318 
#' ~~~~
#'
# Profiling with `library(lineprof)` revealed that the bottleneck here is in the
# utility function `nestedRanksTest_Z`, specifically in the calculation of ranks
# via the base R function `rank`.  A stripped-down `rank_new` is 8-9&times;
# faster than the default rank for a vector of 100 values:
#'
#' ~~~~
#' > library(microbenchmark)
#' > rank_new <- function (x) .Internal(rank(x, length(x), "average"))
#' > yy <- rnorm(100)
#' > microbenchmark(rank(yy), rank_new(yy), times=100000)
#' Unit: microseconds
#'          expr    min     lq      mean median      uq       max neval
#'      rank(yy) 29.148 31.945 36.931556 32.678 33.5165 57192.350 1e+05
#'  rank_new(yy)  3.755  4.300  4.789952  4.542  4.7290  6784.741 1e+05
#' ~~~~
#'
#' For 1000-value vectors the speedup is more modest, about 2&times;, and for
#' 10,000-value vectors the speedup is only in the neighbourhood of 20-30%:
#'
#' ~~~~
#' > yyy <- rnorm(1000)
#' > microbenchmark(rank(yyy), rank_new(yyy), times=100000)
#' Unit: microseconds
#'           expr     min      lq      mean  median      uq      max neval
#'      rank(yyy) 114.693 118.766 134.38290 120.071 123.598 18734.74 1e+05
#'  rank_new(yyy)  63.150  64.877  67.71052  65.340  67.062 16023.45 1e+05
#' > yyy <- rnorm(10000)
#' > microbenchmark(rank(yyy), rank_new(yyy), times=1000)
#' Unit: microseconds
#'           expr      min        lq    mean   median        uq      max neval
#'      rank(yyy) 1238.485 1263.3515 1361.22 1279.580 1303.5785 6436.039  1000
#'  rank_new(yyy)  955.013  964.9215 1002.81  967.849  992.6315 6072.219  1000
# ~~~~
#'
#' @note
#'
#' * http://stat.ethz.ch/R-manual/R-devel/library/base/html/sort.html
#' * http://stat.ethz.ch/R-manual/R-devel/library/base/html/xtfrm.html.
#' * https://stat.ethz.ch/R-manual/R-devel/library/base/html/order.html
#' * Dependent on R >= 3.0.0, so I only worry about long vectors?
#'
#' @references
#' \url{https://github.com/douglasgscofield/fastrank}
#'
#' @docType package
#'
#' @name fastrank-package
#'
NULL



#' Rank vectors with low overhead
#'
#' An R function providing fast ranking vectors, as an alternative to calling 
#' \code{.Internal(rank(...))}, which packages cannot do.  This entry is as
#' general as it can be, but so far there is still a bit of overhead in 
#' calling this method.  If you really want the fastest interface, try using
#' one of the other entry points (e.g., \code{fastrank_numeric_average}) or 
#' (if you are not writing package code) call the C function directly.
#'
#' @note The vector must not include NAs or NaNs.  This is **not** checked.
#'
#' @param x            A vector of values to rank.
# @param ties.method  Method for resolving rank ties in \code{x}.  Provides
#                     all available in \code{rank}.  This does **not** provide
#                     method \code{"random"}.
#'
#' @return An integer vector of ranks of values in \code{x}, with length
#' the same as \code{length(x)}.  Ranks of tied values are handled according
#' to \code{ties.method}, see \code{\link{rank}}.
#'
#' @seealso \code{\link{rank}}
#'
#' @references
#' \url{https://github.com/douglasgscofield/fastrank}
#'
#' @keywords internal
#'
#' @useDynLib fastrank fastrank_
#'
#' @export fastrank
#'
#fastrank <- function(x, ties.method = c("average", "first", "random", "max",
#                                        "min")) {
# TODO: manage ties.method, how does the internal rank do it?
fastrank <- function(x) {
    .Call("fastrank_", x, PACKAGE = "fastrank")
}



#' Rank integer and numeric vectors with low overhead
#'
#' An R function providing fast ranking for integer and numeric vectors, as an
#' alternative to calling .Internal(rank(...)), which packages cannot do.
#'
#' There is still a bit of overhead in calling this method.  If you really want
#' the fastest interface, then call the C function directly.
#'
#' @note The vector must not include NAs or NaNs.  This is **not** checked.
#'
#' @param x            A vector of values to rank.
#'
#' @return An integer vector of ranks of values in \code{x}, with length
#' the same as \code{length(x)}.  Ties are broken by giving entries the
#' average rank of the tied entries.
#'
#' @seealso \code{\link{rank}}
#'
#' @references
#' \url{https://github.com/douglasgscofield/fastrank}
#'
#' @keywords internal
#'
#' @useDynLib fastrank fastrank_num_avg_
#'
#' @export fastrank_num_avg
#'
fastrank_num_avg <- function(x) {
    .Call("fastrank_num_avg_", x, PACKAGE = "fastrank")
}

