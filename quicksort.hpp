#pragma once

#include <cstddef>
#include <vector>

#include <parlay/parallel.h>

using namespace std;

void quick_sort_seq(int* data, size_t begin, size_t end);
void quick_sort_seq(vector<int>& data, size_t begin, size_t end);
void quick_sort_par(vector<int>& data);

inline void quick_sort(vector<int>& data, size_t threads = 1) {
    if (threads <= 1) {
        quick_sort_seq(data, 0, data.size());
        return;
    }

    parlay::execute_with_scheduler(threads, 
        [&] {quick_sort_par(data);});
}
