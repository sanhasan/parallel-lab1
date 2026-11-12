#include "quicksort.hpp"

#include <cstddef>

namespace quicksort {
namespace detail {
namespace {

constexpr std::size_t kInsertionSortThreshold = 24;

void insertion_sort(value_type* data, std::size_t size) {
  for (std::size_t i = 1; i < size; ++i) {
    const value_type value = data[i];
    std::size_t j = i;
    while (j != 0 && value < data[j - 1]) {
      data[j] = data[j - 1];
      --j;
    }
    data[j] = value;
  }
}

value_type median_of_three(value_type a, value_type b, value_type c) {
  if (a < b) {
    if (b < c) return b;
    return a < c ? c : a;
  }
  if (a < c) return a;
  return b < c ? c : b;
}

value_type choose_pivot(const value_type* data, std::size_t size) {
  if (size < 64) return median_of_three(data[0], data[size / 2], data[size - 1]);

  // Tukey's ninther costs only twelve comparisons and produces considerably
  // more even partitions than a single median-of-three on large ranges.
  const std::size_t step = size / 8;
  const value_type a = median_of_three(data[0], data[step], data[2 * step]);
  const value_type b = median_of_three(
      data[3 * step], data[4 * step], data[5 * step]);
  const value_type c = median_of_three(
      data[6 * step], data[7 * step], data[size - 1]);
  return median_of_three(a, b, c);
}

void swap_values(value_type& a, value_type& b) {
  const value_type tmp = a;
  a = b;
  b = tmp;
}

}  // namespace

partition_result partition(value_type* data, std::size_t size) {
  const value_type pivot = choose_pivot(data, size);

  // Hoare partition performs substantially fewer writes than a three-way
  // partition on random 32-bit data. Equal elements are swapped across the
  // split, so even an all-equal input is divided into two balanced ranges.
  std::size_t left = 0;
  std::size_t right = size - 1;
  for (;;) {
    while (data[left] < pivot) ++left;
    while (pivot < data[right]) --right;
    if (left >= right) return {left, left};
    swap_values(data[left], data[right]);
    ++left;
    --right;
  }
}

void seq_impl(value_type* data, std::size_t size) {
  // Recurse into the smaller part and iterate over the larger one. Thus the
  // call stack stays O(log n), even for adversarial partitions.
  while (size > kInsertionSortThreshold) {
    const partition_result split = partition(data, size);
    const std::size_t right_size = size - split.right_offset;

    if (split.left_size < right_size) {
      seq_impl(data, split.left_size);
      data += split.right_offset;
      size = right_size;
    } else {
      seq_impl(data + split.right_offset, right_size);
      size = split.left_size;
    }
  }
  insertion_sort(data, size);
}

}  // namespace detail

void seq(value_type* data, std::size_t size) {
  if (data == nullptr || size < 2) return;
  detail::seq_impl(data, size);
}

}  // namespace quicksort
