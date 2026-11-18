#include "quicksort.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include <parlay/parallel.h>

using namespace std;

static constexpr size_t SEQ_BLOCK = 1 << 17;

void quick_sort_impl(
    int* input,
    int* output,
    size_t left,
    size_t right,
    bool write_to_out
) {
    const size_t n = right - left;

    if (n == 0) {
        return;
    } else if (n <= SEQ_BLOCK) {
        if (write_to_out) {
            copy(input + left, input + right, output + left);

            quick_sort_seq(output, left, right);
        } else {
            quick_sort_seq(input, left, right);
        }
        return;
    }

    const int pivot = utils::median(
        input[left], input[(left + right) / 2], input[right - 1]);

    utils::PartitionResult partition =
        utils::partition(input, output, left, right, pivot);

    if (partition.total_equal == n) {
        if (write_to_out) {
            utils::fill(output + left, output + right, pivot);
        }
        return;
    }

    const size_t middle_begin = left + partition.total_less;
    const size_t middle_end = middle_begin + partition.total_equal;

    if (!write_to_out) {
        utils::fill(input + middle_begin, input + middle_end, pivot);
    }

    parlay::par_do(
        [&] {
            quick_sort_impl(
                output, input,
                left,
                middle_begin,
                !write_to_out
            );
        },
        [&] {
            quick_sort_impl(
                output, input,
                middle_end,
                right,
                !write_to_out
            );
        }
    );
}

void quick_sort_par(vector<int>& data) {
    unique_ptr<int[]> out(new int[data.size()]);
    quick_sort_impl(data.data(), out.get(), 0, data.size(), false);
}
