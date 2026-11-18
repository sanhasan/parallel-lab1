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

inline void fill(int* begin, int* end, int value) {
    const size_t size = end - begin;
    parlay::parallel_for(0, size, [&](size_t i) {
        begin[i] = value;
    }, 1);
}

static constexpr size_t BLOCK_SIZE = 1 << 13;

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
    int* input,
    int* output,
    size_t left,
    size_t right,
    int pivot
) {
    const size_t n = right - left;
    const size_t blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;

    vector<BlockInfo> block_info(blocks);
    PartitionResult result{0, 0};

    parlay::parallel_for(0, blocks, [&](size_t b) {
        const size_t begin = left + b * BLOCK_SIZE;
        const size_t end = min(begin + BLOCK_SIZE, right);

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
        const size_t begin = left + b * BLOCK_SIZE;
        const size_t end = min(begin + BLOCK_SIZE, right);
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
    return result;
}

}  // namespace utils
