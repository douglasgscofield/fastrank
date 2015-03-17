TODO
----

* Use a stable sort for `"first"`... actually I might be able to always use a stable sort, since I have the index vector already I only exchange equivalent values if their index values are inverted.
* Move QUICKSORT_INSERTION_CUTOFF to Makevars
* Determine QUICKSORT_INSERTION_CUTOFF empirically, like with `configure`?
* Continue genericifying Quicksort, `fastrank` and the other interfaces
* Create a huge number of tests that check that `rank` and `fastrank` and direct entries are absolutely identical in all of them
* Restore and debug complex vector support in `fastrank`
* Is it OK to do the shortcut evaluation of `ties.method`?
* Proper makefile for compiling C routines, look into `Makevars` and `Makevars.win` (mentioned in <http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Using-C_002b_002b11-code>)
* Do we need -ffast-math or some other optimisation flags?  Which is better -O2, -O3, etc?
* Other C interfaces: how to name these?
  * fastrank, general
  * fastrank_numeric_first
  * fastrank_numeric_max
  * fastrank_numeric_min
  * fastrank_numeric_random
  * fastrank_integer_average
  * fastrank_integer_first
  * fastrank_integer_max
  * fastrank_integer_min
  * fastrank_integer_random
* What are the errors once sees with incorrect data?
* Update all these experiences over in the **R-package-utilities** repository.


### Potential CRAN issues

I don't know what sorts of things I might run into with submitting this to CRAN, so I would like to anticipate them as much as possible.  Some of the TODO and Completed address this concern but here I will collect more specific links and thoughts.

* Here mentions a mod to a Makevars requested by CRAN: https://github.com/eddelbuettel/rcppgsl/commit/d6cb0261e2736e7ddad1323c13b9a627d2507c89#diff-c5776ec1a71fecf2707202dd685bec29
* How can I check on Solaris?
* How can I check on Windows?



Completed
---------

* Learned that `.Internal` is implemented via a dispatch table compiled into the R executable, so there is no way we can beat the speed of its call interface
* It is much faster to include `PACKAGE = "fastrank"` in the R wrapper
* It is faster to byte-compile the R wrapper
* It is slower to pass length separately, `.Call("fastrank_", x, ties)` is faster than `.Call("fastrank_", x, length(x), ties)`
* We have settled on Quicksort with insertion sort when vector length &le; 20
* We have long vector support
* Indexed shell sort now works, and offers three gap methods: Ciura, Sedgwick, Tokuda, and also some hybrids that switched to insertion sort below gap length about 20.  None of these was faster than Quicksort plus insertion sort, so they were dropped.  See `src/fastrank.c.bak2`.
* `fr_quicksort_double_i_` does indices only
* In `fastrank_num_avg`, no longer sorting the passed vector
* Benchmarked `.C` vs. `.Call`, and found `.Call` is 5-25% faster (faster with shorter vectors)
* No bugs that I have yet found in the rankings (check for bug, if vector ends with ties, is rank calculated as +1 its true value?)
* Warn the user that character values are not accepted
* Test the multiple ties.methods in one function
* For `fastrank`, checking for character in R wrapper *and* in C code, how much faster is it to avoid one or both of these?  The check in R wrapper happens for all invocations, but the check in C code happens after a `switch` and only if the TYPEOF is actually character, so should be much faster for all cases.  Or is that not true?  Did I move the check earlier so as to avoid the `R_orderVector` call?
* As `rank` does, `fastrank` returns a double vector if `ties.method=="average"`, integer otherwise
* Fixed major bugs in src/tst/test_random.c, seems to work great now!
* Registered the single function so far for efficiency while loading, http://cran.rstudio.com/doc/manuals/r-devel/R-exts.html#Registering-native-routines, and it makes a sizable difference, see the README.
* Completed C interfaces
  * fastrank_num_avg
