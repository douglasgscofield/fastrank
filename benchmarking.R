library(fastrank)
library(microbenchmark)
library(lattice)

vector.length <- as.vector(outer(c(1, 2, 5, 10), c(10, 100, 1000, 10000, 100000)))
#vector.length <- as.vector(outer(c(1, 2, 5, 10), c(10)))

sample.fraction = c(10, 5, 2, 1, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1)

micro.times <- rep(rev(c(10, 100, 1000, 10000, 100000)), c(4, 4, 4, 4, 4))

iterations <- 10

do.benchmark <- function(sort1 = 4L, sort2 = 7L, VL = vector.length, SF = sample.fraction,
                         MT = micro.times, IT = iterations)
{
    bmark <- data.frame()
    for (il in seq_along(VL)) {
        for (ir in seq_along(SF)) {
            a.diff <- c()
            a.dupl <- c()
            for (i in seq(IT)) {
                cat("sort1 =", sort1, " sort2 =", sort2, " VL =", VL[il], " SF =", SF[ir], " i =", i, "...\n")
                x <- sample(round(VL[il] * SF[ir]), VL[il], ifelse(SF[ir] == 10, FALSE, TRUE))
                a.dupl <- c(a.dupl, sum(duplicated(sort(x))) / VL[il])
                res <- microbenchmark(fastrank(x, sort = sort1),
                                      fastrank(x, sort = sort2),
                                      times = MT[il])
                res <- summary(res)
                a.diff <- c(a.diff, res$median[1] - res$median[2])
            }
            bmark <- rbind(bmark,
                           data.frame(length = VL[il],
                                      repet = SF[ir],
                                      iter = iterations,
                                      dupl = mean(a.dupl),
                                      diff = mean(a.diff)))
        }
    }
    bmark
}


plot.benchmark <- function(b, ...)
{
    levelplot(diff ~ length * dupl, data = b, ...)
}
