TODO
----

* Check for bug, if vector ends with ties, is rank calculated as +1 its true value?
* **Still sorting the passed vector**, need to not do that
* Proper makefile for compiling C routines
* Other C interfaces
  * fastrank_numeric_first
  * fastrank_numeric_max
  * fastrank_numeric_min
  * fastrank_numeric_random
  * fastrank_integer_average
  * fastrank_integer_first
  * fastrank_integer_max
  * fastrank_integer_min
  * fastrank_integer_random
* A general fastrank interface that uses `R_orderVector()`
* BE FASTER, see README
* What are the errors once sees with incorrect data?
* Update all these experiences over in the **R-package-utilities** repository.

Completed
---------

* Registered the single function so far for efficiency while loading, http://cran.rstudio.com/doc/manuals/r-devel/R-exts.html#Registering-native-routines, and it makes a sizable difference, see the README.
* Completed C interfaces
  * fastrank_numeric_average
