#pragma once

#include <cstddef>
#include <cstdint>

namespace quicksort {

using value_type = std::int32_t;

// Sorts [data, data + size) in the calling thread.
void seq(value_type* data, std::size_t size);

// Sorts [data, data + size) using at most thread_count threads, including the
// calling thread. The only synchronization operation is fork/join.
void par(value_type* data, std::size_t size, unsigned thread_count);

namespace detail {

struct partition_result {
  std::size_t left_size;
  std::size_t right_offset;
};

partition_result partition(value_type* data, std::size_t size);
void seq_impl(value_type* data, std::size_t size);

}  // namespace detail
}  // namespace quicksort
