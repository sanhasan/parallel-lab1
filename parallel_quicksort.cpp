#include "quicksort.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <parlay/parallel.h>

using namespace std;

static constexpr size_t SEQ_BLOCK = 1 << 17;

struct SortedRange {
    size_t left;
    size_t right;
    vector<int>* storage;
};

vector<SortedRange> quick_sort_impl(
    vector<int>& input,
    vector<int>& output,
    size_t left,
    size_t right
) {
    const size_t n = right - left;

    if (n == 0) {
        return {};
    } else if (n <= SEQ_BLOCK) {
        quick_sort_seq(input, left, right);
        return {{left, right, &input}};
    }

    const int pivot = utils::median(input[left], input[(left + right) / 2], input[right - 1]);

    utils::PartitionResult partition =
        utils::partition(input, output, left, right, pivot);

    if (partition.total_equal == n) {
        return {{left, right, &input}};
    }

    vector<SortedRange> left_ranges, right_ranges;
    const size_t middle_begin = left + partition.total_less;
    const size_t middle_end = middle_begin + partition.total_equal;

    parlay::par_do(
        [&] {
            left_ranges = quick_sort_impl(
                output, input,
                left,
                middle_begin
            );
        },
        [&] {
            right_ranges = quick_sort_impl(
                output, input,
                middle_end,
                right
            );
        }
    );

    vector<SortedRange> ranges;
    ranges.reserve(left_ranges.size() + right_ranges.size() + 1);

    const auto append = [&](const SortedRange& range) {
        if (!ranges.empty()
            && ranges.back().right == range.left
            && ranges.back().storage == range.storage) {
            ranges.back().right = range.right;
        } else {
            ranges.push_back(range);
        }
    };

    for (const SortedRange& range : left_ranges) {
        append(range);
    }
    append({middle_begin, middle_end, &output});
    for (const SortedRange& range : right_ranges) {
        append(range);
    }

    return ranges;
}

void quick_sort_par(vector<int>& data) {
    vector<int> out(data.size());
    vector<SortedRange> ranges =
        quick_sort_impl(data, out, 0, data.size());

    if (ranges.size() == 1 && ranges[0].storage == &out) {
        data.swap(out);
        return;
    }

    parlay::parallel_for(0, ranges.size(), [&](size_t r) {
        const SortedRange& range = ranges[r];
        if (range.storage == &data) {
            return;
        }

        for (size_t i = range.left; i < range.right; ++i) {
            data[i] = (*range.storage)[i];
        }
    }, 1);
}
