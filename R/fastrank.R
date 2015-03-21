#' Ranking vectors with low overhead
#'
#' An R package providing fast ranking for integer and numeric vectors, as an
#' alternative to calling .Internal(rank(...)), which packages cannot do.
#'
#' The motivation for this comes from my development of the
#' \code{nestedRanksTest} package.  A standard run with the default 10,000
#' bootstrap iterations takes a few seconds to complete on a test data set.
#' Profiling with \code{library(lineprof)} revealed that the bottleneck here is
#' in the utility function \code{nestedRanksTest_Z}, specifically in the
#' calculation of ranks via the base R function \code{rank}.  A stripped-down
#' \code{rank_new} is 8-9 times faster than the default rank for a vector of 100
#' values.  For 1000-value vectors the speedup is more modest, about 2&times;,
#' and for 10,000-value vectors the speedup is only in the neighbourhood of
#' 20-30%.
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
#' The vector must also not be of type character.
#'
#' @param x A vector of values to rank.  Note that character vectors are not
#' accepted, as the internal R routines for comparing characters as R does
#' are not part of the R API.  Please use \code{\link{rank}} from base R
#' to rank character vectors.
#'       
#' @param x            Vector to calculate ranks for
#' @param ties.method  Method for resolving rank ties in \code{x}, all in
#' \code{\link{rank}} are available
#' @param find         Method for finding \code{ties.method}, either 1 or 2
#'
#' @return A vector of ranks of values in \code{x}, with length
#' the same as \code{length(x)}.  Ranks of tied values are handled according
#' to \code{ties.method}, see \code{\link{rank}}.  When \code{ties.method} is
#' \code{"average"}, a numeric vector is returned, otherwise an integer
#' vector is returned.
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
fastrank <- function(x, ties.method = "average", sort.method = 1L) {
    .Call("fastrank_", x, ties.method, sort.method, PACKAGE = "fastrank")
}



#' Rank numeric (double) vectors, assigning ties the average rank
#'
#' An R function providing fast ranking for numeric vectors, assigning tied
#' values the average rank.
#'
#' There is still a bit of overhead in calling this method.  If you really want
#' the fastest interface, then call .\code{.Internal(rank(...))} function 
#' directly.
#'
#' @note The vector must not include NAs or NaNs.  This is **not** checked.
#'
#' @param x A vector of numeric (double) values to rank
#'
#' @return A numeric vector of ranks of values in \code{x}, with length
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

