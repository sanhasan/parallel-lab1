#include "quicksort.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <parlay/parallel.h>

using namespace std;

static constexpr size_t SEQ_BLOCK = 1 << 16;
static constexpr size_t PART_BLOCK = 1 << 13;

void quick_sort_impl(
    vector<int>& data,
    vector<int>& tmp,
    size_t left,
    size_t right
) {
    const size_t n = right - left;

    if (n < 2) {
        return;
    }

    if (n <= SEQ_BLOCK) {
        quick_sort_seq(data, left, right);
        return;
    }

    const int pivot = utils::median(
        data[left],
        data[left + n / 2],
        data[right - 1]
    );

    const size_t blocks = (n + PART_BLOCK - 1) / PART_BLOCK;

    vector<size_t> less_count(blocks);
    vector<size_t> equal_count(blocks);

    // 1. Подсчитываем элементы в каждом блоке.
    parlay::parallel_for(0, blocks, [&](size_t b) {
        const size_t begin = left + b * PART_BLOCK;
        const size_t end = min(begin + PART_BLOCK, right);

        size_t less = 0;
        size_t equal = 0;

        for (size_t i = begin; i < end; ++i) {
            less += data[i] < pivot;
            equal += data[i] == pivot;
        }

        less_count[b] = less;
        equal_count[b] = equal;
    }, 1);

    size_t total_less = 0;
    size_t total_equal = 0;

    for (size_t b = 0; b < blocks; ++b) {
        total_less += less_count[b];
        total_equal += equal_count[b];
    }

    if (total_equal == n) {
        return;
    }

    vector<size_t> less_offset(blocks);
    vector<size_t> equal_offset(blocks);
    vector<size_t> greater_offset(blocks);

    // 2. Вычисляем позиции блоков.
    // Здесь scan по числу блоков выполняется последовательно.
    size_t less_pos = 0;
    size_t equal_pos = total_less;
    size_t greater_pos = total_less + total_equal;

    for (size_t b = 0; b < blocks; ++b) {
        less_offset[b] = less_pos;
        equal_offset[b] = equal_pos;
        greater_offset[b] = greater_pos;

        const size_t begin = left + b * PART_BLOCK;
        const size_t end = min(begin + PART_BLOCK, right);

        less_pos += less_count[b];
        equal_pos += equal_count[b];
        greater_pos += end - begin
                     - less_count[b] - equal_count[b];
    }

    // 3. Раскладываем элементы в один общий массив.
    parlay::parallel_for(0, blocks, [&](size_t b) {
        const size_t begin = left + b * PART_BLOCK;
        const size_t end = min(begin + PART_BLOCK, right);

        size_t l = left + less_offset[b];
        size_t e = left + equal_offset[b];
        size_t g = left + greater_offset[b];

        for (size_t i = begin; i < end; ++i) {
            const int value = data[i];

            if (value < pivot) {
                tmp[l++] = value;
            } else if (value == pivot) {
                tmp[e++] = value;
            } else {
                tmp[g++] = value;
            }
        }
    }, 1);

    // 4. Копируем результат обратно.
    parlay::parallel_for(left, right, [&](size_t i) {
        data[i] = tmp[i];
    });

    // 5. Рекурсивная сортировка двух частей.
    parlay::par_do(
        [&] {
            quick_sort_impl(
                data, tmp,
                left,
                left + total_less
            );
        },
        [&] {
            quick_sort_impl(
                data, tmp,
                left + total_less + total_equal,
                right
            );
        }
    );
}

void quick_sort_par(vector<int>& data) {
    if (data.size() < 2) {
        return;
    }

    vector<int> tmp(data.size());
    quick_sort_impl(data, tmp, 0, data.size());
}
