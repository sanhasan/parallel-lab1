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

void quick_sort_impl(
    vector<int>& input,
    vector<int>& output,
    size_t left,
    size_t right,
    bool write_to_output
) {
    const size_t n = right - left;

    if (n < 2) {
        if (n == 1 && write_to_output) {
            output[left] = input[left];
        }
        return;
    }

    if (n <= SEQ_BLOCK) {
        if (write_to_output) {
            copy(input.begin() + left, input.begin() + right,
                 output.begin() + left);
            quick_sort_seq(output, left, right);
        } else {
            quick_sort_seq(input, left, right);
        }
        return;
    }

    const int pivot = utils::median(
        input[left],
        input[left + n / 2],
        input[right - 1]
    );

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
        if (write_to_output) {
            fill(output.begin() + left, output.begin() + right, pivot);
        }
        return;
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
    
    if (!write_to_output) {
        fill(input.begin() + left + total_less,
             input.begin() + left + total_less + total_equal,
             pivot);
    }

    parlay::par_do(
        [&] {
            quick_sort_impl(
                output, input,
                left,
                left + total_less,
                !write_to_output
            );
        },
        [&] {
            quick_sort_impl(
                output, input,
                left + total_less + total_equal,
                right,
                !write_to_output
            );
        }
    );
}

void quick_sort_par(vector<int>& data) {
    if (data.size() < 2) {
        return;
    }

    vector<int> tmp(data.size());
    quick_sort_impl(data, tmp, 0, data.size(), false);
}
