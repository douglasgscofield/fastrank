TODO
----

* From R source sort.c/do_rank(): rank vector is double if `ties.method=="average"` or long vector, integer otherwise
* Warn the user of no long vector support, use base R `rank()`
* Warn the user that character values are not accepted
* Test the multiple ties.methods in one function
* For `ties.method` min, max, first, random, there is no need for a `double` rank return value.  What does R do in these cases?
* For `fastrank`, checking for character in R wrapper *and* in C code, how much faster is it to avoid one or both of these?  The check in R wrapper happens for all invocations, but the check in C code happens after a `switch` and only if the TYPEOF is actually character, so should be much faster for all cases.  Or is that not true?  Did I move the check earlier so as to avoid the `R_orderVector` call?
* Proper makefile for compiling C routines, look into `Makevars` and `Makevars.win` (mentioned in <http://cran.r-project.org/doc/manuals/r-release/R-exts.html#Using-C_002b_002b11-code>)
* Check for bug, if vector ends with ties, is rank calculated as +1 its true value?
* **Still sorting the passed vector**, need to not do that
* Other C interfaces: how to name these?
  * fastrank, general (but no characters), uses `R_orderVector()`
  * fastrank_numeric_first
  * fastrank_numeric_max
  * fastrank_numeric_min
  * fastrank_numeric_random
  * fastrank_integer_average
  * fastrank_integer_first
  * fastrank_integer_max
  * fastrank_integer_min
  * fastrank_integer_random
* BE FASTER, see README
* Really must try to use a stable sort, is `R_orderVector` stable?
* What are the errors once sees with incorrect data?
* Update all these experiences over in the **R-package-utilities** repository.

Completed
---------

* Fixed major bugs in src/tst/test_random.c, seems to work great now!
* Registered the single function so far for efficiency while loading, http://cran.rstudio.com/doc/manuals/r-devel/R-exts.html#Registering-native-routines, and it makes a sizable difference, see the README.
* Completed C interfaces
  * fastrank_num_avg
