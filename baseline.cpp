#include "utils.hpp"

#include <algorithm>
#include <vector>

using namespace std;

static const int BLOCK = 20;

static void insertion_sort(vector<int>& data, int begin, int end) {
    for (int i = begin + 1; i < end; ++i) {
        int value = data[i];
        int j = i;

        while (j > begin && value < data[j - 1]) {
            data[j] = data[j - 1];
            --j;
        }
        data[j] = value;
    }
}

static pair<int, int> partition(vector<int>& data, int begin, int end) {
    int mid = (begin + end) / 2;
    int pivot = utils::median(data[begin], data[mid], data[end - 1]);
    int curr = begin;

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

void quick_sort_seq(vector<int>& data, int begin, int end) {
    if (end - begin <= BLOCK) {
        insertion_sort(data, begin, end);
        return;
    }
    pair<int, int> split = partition(data, begin, end);
    quick_sort_seq(data, begin, split.first);
    quick_sort_seq(data, split.second, end);
}
