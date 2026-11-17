#include "utils.hpp"

#include <algorithm>
#include <vector>

using namespace std;

static constexpr size_t BLOCK = 20;

static void insertion_sort(int* data, size_t begin, size_t end) {
    for (size_t i = begin + 1; i < end; ++i) {
        int value = data[i];
        size_t j = i;

        while (j > begin && value < data[j - 1]) {
            data[j] = data[j - 1];
            --j;
        }
        data[j] = value;
    }
}

static pair<size_t, size_t> partition(int* data, size_t begin, size_t end) {
    size_t mid = begin + (end - begin) / 2;
    int pivot = utils::median(data[begin], data[mid], data[end - 1]);
    size_t curr = begin;

    while (curr < end) {
        if (data[curr] < pivot) {
            swap(data[curr++], data[begin++]);
        } else if (data[curr] > pivot) {
            swap(data[curr], data[--end]);
        } else {
            curr++;
        }
    }
    return {begin, end};
}

void quick_sort_seq(int* data, size_t begin, size_t end) {
    if (end - begin <= BLOCK) {
        insertion_sort(data, begin, end);
        return;
    }
    pair<size_t, size_t> split = partition(data, begin, end);
    quick_sort_seq(data, begin, split.first);
    quick_sort_seq(data, split.second, end);
}

void quick_sort_seq(vector<int>& data, size_t begin, size_t end) {
    quick_sort_seq(data.data(), begin, end);
}
