// what is the right type of indices into R vectors?
// create a quick sort template

// quick_sort code modified from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort

template<class T>
void quick_sort (std::vector<T>& a, const int n) {
    T p, t;
    int i, j;
    if (n < 2)
        return;
    p = a[n / 2];
    for (i = 0, j = n - 1; ; i++, j--) {
        while (a[i] < p)
            i++;
        while (p < a[j])
            j--;
        if (i >= j)
            break;
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    quick_sort(a, i);
    quick_sort(a + i, n - i);
}
 
template<class T>
std::vector<int> fastrank (std::vector<T>& a, const int n, const std::string& method) {
    std::vector<int> r(a.size());

}


