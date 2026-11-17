#include "quicksort.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <parlay/parallel.h>

using namespace std;

static constexpr size_t SEQ_BLOCK = 1 << 17;
static constexpr size_t PART_BLOCK = 1 << 13;

struct BlockInfo {
    size_t less;
    size_t equal;
    size_t greater;
};

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

    const size_t blocks = (n + PART_BLOCK - 1) / PART_BLOCK;

    vector<BlockInfo> block_info(blocks);

    parlay::parallel_for(0, blocks, [&](size_t b) {
        const size_t begin = left + b * PART_BLOCK;
        const size_t end = min(begin + PART_BLOCK, right);

        size_t less = 0;
        size_t equal = 0;

        for (size_t i = begin; i < end; ++i) {
            less += input[i] < pivot;
            equal += input[i] == pivot;
        }

        block_info[b].less = less;
        block_info[b].equal = equal;
    }, 1);

    size_t total_less = 0;
    size_t total_equal = 0;

    for (size_t b = 0; b < blocks; ++b) {
        total_less += block_info[b].less;
        total_equal += block_info[b].equal;
    }

    if (total_equal == n) {
        return {{left, right, &input}};
    }

    size_t less_pos = 0;
    size_t equal_pos = total_less;
    size_t greater_pos = total_less + total_equal;

    for (size_t b = 0; b < blocks; ++b) {
        const size_t less_count = block_info[b].less;
        const size_t equal_count = block_info[b].equal;

        block_info[b].less = less_pos;
        block_info[b].equal = equal_pos;
        block_info[b].greater = greater_pos;

        const size_t begin = left + b * PART_BLOCK;
        const size_t end = min(begin + PART_BLOCK, right);

        less_pos += less_count;
        equal_pos += equal_count;
        greater_pos += end - begin - less_count - equal_count;
    }

    parlay::parallel_for(0, blocks, [&](size_t b) {
        const size_t begin = left + b * PART_BLOCK;
        const size_t end = min(begin + PART_BLOCK, right);

        size_t l = left + block_info[b].less;
        size_t e = left + block_info[b].equal;
        size_t g = left + block_info[b].greater;

        for (size_t i = begin; i < end; ++i) {
            const int value = input[i];

            if (value < pivot) {
                output[l++] = value;
            } else if (value == pivot) {
                output[e++] = value;
            } else {
                output[g++] = value;
            }
        }
    }, 1);

    vector<SortedRange> left_ranges;
    vector<SortedRange> right_ranges;
    const size_t middle_begin = left + total_less;
    const size_t middle_end = middle_begin + total_equal;

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
