#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include <parlay/parallel.h>

using namespace std;

namespace utils {

inline int median(int a, int b, int c) {
    if (min(a, b) <= c && c <= max(a, b)) {
        return c;
    }
    if (min(a, c) <= b && b <= max(a, c)) {
        return b;
    }
    return a;
}

struct BlockInfo {
    size_t less;
    size_t equal;
    size_t greater;
};

struct PartitionResult {
    size_t total_less;
    size_t total_equal;
};

inline PartitionResult partition(
    const vector<int>& input,
    vector<int>& output,
    size_t left,
    size_t right,
    int pivot,
    size_t block_size
) {
    const size_t n = right - left;
    const size_t blocks = (n + block_size - 1) / block_size;
    vector<BlockInfo> block_info(blocks);
    PartitionResult result{0, 0};

    parlay::parallel_for(0, blocks, [&](size_t b) {
        const size_t begin = left + b * block_size;
        const size_t end = min(begin + block_size, right);

        size_t less = 0;
        size_t equal = 0;
        for (size_t i = begin; i < end; ++i) {
            less += input[i] < pivot;
            equal += input[i] == pivot;
        }

        block_info[b] = {
            less,
            equal,
            end - begin - less - equal
        };
    }, 1);

    for (const BlockInfo& block : block_info) {
        result.total_less += block.less;
        result.total_equal += block.equal;
    }

    if (result.total_equal == n) {
        return result;
    }

    size_t less_pos = 0;
    size_t equal_pos = result.total_less;
    size_t greater_pos = result.total_less + result.total_equal;

    for (BlockInfo& block : block_info) {
        const BlockInfo counts = block;
        block = {less_pos, equal_pos, greater_pos};

        less_pos += counts.less;
        equal_pos += counts.equal;
        greater_pos += counts.greater;
    }

    parlay::parallel_for(0, blocks, [&](size_t b) {
        const size_t begin = left + b * block_size;
        const size_t end = min(begin + block_size, right);
        size_t l = left + block_info[b].less;
        size_t e = left + block_info[b].equal;
        size_t g = left + block_info[b].greater;

        for (size_t i = begin; i < end; ++i) {
            if (input[i] < pivot) {
                output[l++] = input[i];
            } else if (input[i] == pivot) {
                output[e++] = input[i];
            } else {
                output[g++] = input[i];
            }
        }
    }, 1);
    return result;
}

}  // namespace utils
