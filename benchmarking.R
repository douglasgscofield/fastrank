#library(fastrank)
devtools::load_all('.')
library(microbenchmark)
library(lattice)
library(RColorBrewer)

ordermag <- c(10, 100, 1000, 10000, 100000)
vector.length <- as.vector(outer(c(1, 2, 5, 10), ordermag))
#vector.length <- as.vector(outer(c(1, 2, 5, 10), c(10)))

micro.times <- rep(rev(ordermag), c(4, 4, 4, 4, 4))

#sample.fraction = c(10, 5, 2, 1, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1)
sample.fraction = c(10, 7, 5, 4, 3, 2, 1.7, 1.5, 1.3, 1.2, 
                    1, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.05)

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
    attr(bmark, "sort1") <- sort1
    attr(bmark, "sort2") <- sort2
    attr(bmark, "VL") <- VL
    attr(bmark, "SF") <- SF
    attr(bmark, "MT") <- MT
    attr(bmark, "IT") <- IT
    bmark
}


plot.benchmark <- function(b, compress = 0.5, 
                           min.label = attr(b, "sort1"),
                           mid.label = "none",
                           max.label = attr(b, "sort2"),
                           ...)
{
    png(file = paste0("diff_", min.label, "_", max.label, ".png"), width = 800, height = 800)
    opa <- par(mfrow = c(1,1), las = 2, mar = c(4, 4, 1, 1), mgp = c(3, 0.4, 0), tcl = -0.3)
    b$length = ordered(b$length)
    main <- paste("orig range", paste(collapse = " ", (signif(range(b$diff), 3))))
    if (! is.null(compress)) {
        b$diff <- ifelse(b$diff > compress, compress, b$diff)
        b$diff <- ifelse(b$diff < -compress, -compress, b$diff)
        main <- paste(main, "compress", compress)
    }
    lvls <- 11
    zrng <- c(-1, +1) * max(abs(range(b$diff)))
    zdiff <- as.numeric(cut(c(zrng, b$diff), lvls))
    zpalette <- brewer.pal(lvls, "Spectral")
    #zpalette <- colorRampPalette(c("red","blue"))(lvls)
    zcol <- zpalette[zdiff]
    plot.default(b$length, b$dupl, ylim = c(0, 1), axes = FALSE,
                 pch = 19, cex = 1.5, cex.lab = 1, col = zcol,
                 xlab = "Vector length", ylab = "Value duplicate fraction",
                 main = main) 
    axis(1, at = unique(sort(as.numeric(b$length))), labels = levels(b$length))
    axis(2)
    leg <- c(zrng[1], 0, zrng[2])
    leg <- c(min.label, mid.label, max.label)
    legend("top", legend = leg, pch = 19, pt.cex = 1.5,
           col = c(zpalette[1], zpalette[(lvls + 1)/2], zpalette[lvls]),
           horiz = TRUE, bty = "n")
    #plot(b$length, b$repet, pch = 19, cex = 1.5, col = zcol) 
    par(opa)
    dev.off()
}
