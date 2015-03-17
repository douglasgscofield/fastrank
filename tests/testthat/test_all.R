library(fastrank)

#########################################
context("Setting up some test data")

ties.methods <- c("average", "first", "random", "max", "min")
ties.classes <- c("numeric", "integer", "integer", "integer", "integer")
names(ties.classes) <- ties.methods
# use unname() when comparing

# start as integer, convert to numeric as needed
# also need logical and complex

# 'worst-case' vectors
y.rev <- 9:1
yy.rev <- 103:1
yyy.rev <- 10022:1
yyyy.rev <- 102399:1
y.ident <- rep(9, 9)
yy.ident <- rep(103, 103)
yyy.ident <- rep(10022, 10022)
yyyy.ident <- rep(102399, 102399)
y.fwd <- 1:9
yy.fwd <- 1:103
yyy.fwd <- 1:10022
yyyy.fwd <- 1:102399


set.seed(20030131)

# arguments to sample() for creating random data
#                          range   number   repl  iter
sample.args <- list(list(     10,       5,  TRUE,  10),
                    list(     10,       5, FALSE,  10),
                    list(     10,      10,  TRUE,  10),
                    list(     10,      10, FALSE,  10),
                    list(     10,      20,  TRUE,  10),
                    list(    100,      50,  TRUE,   6),
                    list(    100,      50, FALSE,   6),
                    list(    100,     100,  TRUE,   6),
                    list(    100,     100, FALSE,   6),
                    list(    100,     200,  TRUE,   6),
                    list(   1000,     500,  TRUE,   5),
                    list(   1000,     500, FALSE,   5),
                    list(   1000,    1000,  TRUE,   5),
                    list(   1000,    1000, FALSE,   5),
                    list(   1000,    2000,  TRUE,   5),
                    list(  10000,    5000,  TRUE,   3),
                    list(  10000,    5000, FALSE,   3),
                    list(  10000,   10000,  TRUE,   3),
                    list(  10000,   10000, FALSE,   3),
                    list(  10000,   20000,  TRUE,   3),
                    list( 100000,   50000,  TRUE,   2),
                    list( 100000,   50000, FALSE,   2),
                    list( 100000,  100000,  TRUE,   2),
                    list( 100000,  100000, FALSE,   2),
                    list( 100000,  200000,  TRUE,   2)
                    )

# long vectors

#########################################
for (ti in ties.methods) {

    ctxt <- paste(sep="", "Worst-case \"", ti, "\", vs. rank()")
    context(ctxt)

    test_that("Worst-case reverse vectors and classes == rank()", {
        expect_equal(fastrank(y.rev, ties.method = ti),
                     rank(y.rev, ties.method = ti))
        expect_equal(fastrank(yy.rev, ties.method = ti),
                     rank(yy.rev, ties.method = ti))
        expect_equal(fastrank(yyy.rev, ties.method = ti),
                     rank(yyy.rev, ties.method = ti))

        expect_equal(class(fastrank(y.rev, ties.method = ti)),
                     unname(ties.classes[ti]))
    })
    test_that("Worst-case reverse vectors and classes == rank()", {
        skip_on_cran()
        expect_equal(fastrank(yyyy.rev, ties.method = ti),
                     rank(yyyy.rev, ties.method = ti))
    })


    test_that("Worst-case identical vectors and classes == rank()", {
        expect_equal(fastrank(y.ident, ties.method = ti),
                     rank(y.ident, ties.method = ti))
        expect_equal(fastrank(yy.ident, ties.method = ti),
                     rank(yy.ident, ties.method = ti))
        expect_equal(fastrank(yyy.ident, ties.method = ti),
                     rank(yyy.ident, ties.method = ti))

        expect_equal(class(fastrank(y.rev, ties.method = ti)),
                     unname(ties.classes[ti]))
    })
    test_that("Worst-case reverse vectors and classes == rank()", {
        skip_on_cran()
        expect_equal(fastrank(yyyy.ident, ties.method = ti),
                     rank(yyyy.ident, ties.method = ti))
    })


    test_that("Worst-case forward vectors and classes == rank()", {
        expect_equal(fastrank(y.fwd, ties.method = ti),
                     rank(y.fwd, ties.method = ti))
        expect_equal(fastrank(yy.fwd, ties.method = ti),
                     rank(yy.fwd, ties.method = ti))
        expect_equal(fastrank(yyy.fwd, ties.method = ti),
                     rank(yyy.fwd, ties.method = ti))

        expect_equal(class(fastrank(y.rev, ties.method = ti)),
                     unname(ties.classes[ti]))
    })
    test_that("Worst-case reverse vectors and classes == rank()", {
        skip_on_cran()
        expect_equal(fastrank(yyyy.fwd, ties.method = ti),
                     rank(yyyy.fwd, ties.method = ti))
    })

}


#########################################
for (ti in ties.methods) {

    ctxt <- paste(sep="", "Random, \"", ti, "\", vs. rank(), ")

    for (a in sample.args) {
        ctxt.s <- paste(sep="", "sample(", 
                        paste(sep=",", a[[1]], a[[2]], a[[3]]), 
                        ") * ", a[[4]])

        context(paste(sep="", ctxt, ctxt.s))

        test_that("Random vectors", {
            if (a[[2]] > 10000) skip_on_cran()
            for (i in 1:a[[4]]) {
                    v <- sample(a[[1]], a[[2]], a[[3]])
                    r1 <- rank(v, ties.method = ti)
                    r2 <- fastrank(v, ties.method = ti)
                    expect_equal(r1, r2)
                    expect_equal(class(r1), class(r2))
                    expect_equal(class(r2), unname(ties.classes[ti]))
            }
        })
    }
}


