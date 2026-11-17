#pragma once

#include <vector>

#include <parlay/parallel.h>

void quick_sort_seq(std::vector<int>& data, int begin, int end);
void quick_sort_par(std::vector<int>& data);

void quick_sort(std::vector<int>& data, int threads = 1) {
    if (threads <= 1) {
        quick_sort_seq(data, 0, data.size());
        return;
    }

    parlay::execute_with_scheduler(threads, [&] {
        quick_sort_par(data);
    });
}
