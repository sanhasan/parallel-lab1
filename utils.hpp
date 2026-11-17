#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

#include <parlay/parallel.h>

using namespace std;

namespace utils {

static constexpr size_t BLOCK = 1024;

inline int median(int a, int b, int c) {
    if (min(a, b) <= c && c <= max(a, b)) {
        return c;
    }
    if (min(a, c) <= b && b <= max(a, c)) {
        return b;
    }
    return a;
}

template <class Function>
void parallel_for(size_t begin, size_t end,
                  const Function& function) {
    if (end - begin <= BLOCK) {
        for (size_t i = begin; i < end; ++i) {
            function(i);
        }
        return;
    }

    size_t mid = (end + begin) / 2;
    parlay::par_do(
        [&] { parallel_for(begin, mid, function); },
        [&] { parallel_for(mid, end, function); });
}

template <class T>
struct ScanResult {
    vector<T> offsets;
    T total;

    ScanResult(size_t n, T zero)
        : offsets(n), total(zero) {}
};

template <class Value, class T, class Operation>
ScanResult<T> exclusive_scan(const vector<Value>& values, T zero, const Operation& sum) {
    ScanResult<T> result(values.size(), zero);
    if (values.empty()) {
        return result;
    }

    size_t blocks = (values.size() + BLOCK - 1) / BLOCK;
    vector<T> tree(blocks * 4, zero);

    const auto build = [&](const auto& self, size_t node,
                           size_t left, size_t right) -> void {
        if (right - left == 1) {
            size_t begin = left * BLOCK;
            size_t end = min(begin + BLOCK, values.size());
            T value = zero;

            for (size_t i = begin; i < end; ++i) {
                value = sum(value, values[i]);
            }
            tree[node] = value;
            return;
        }

        size_t mid = (right + left) / 2;
        parlay::par_do(
            [&] { self(self, node * 2, left, mid); },
            [&] { self(self, node * 2 + 1, mid, right); });
        tree[node] = sum(tree[node * 2], tree[node * 2 + 1]);
    };

    const auto write = [&](const auto& self, size_t node,
                           size_t left, size_t right,
                           T prefix) -> void {
        if (right - left == 1) {
            size_t begin = left * BLOCK;
            size_t end = min(begin + BLOCK, values.size());

            for (size_t i = begin; i < end; ++i) {
                result.offsets[i] = prefix;
                prefix = sum(prefix, values[i]);
            }
            return;
        }

        size_t mid = (right + left) / 2;
        T right_prefix = sum(prefix, tree[node * 2]);
        parlay::par_do(
            [&] { self(self, node * 2, left, mid, prefix); },
            [&] { self(self, node * 2 + 1, mid, right, right_prefix); });
    };

    build(build, 1, 0, blocks);
    write(write, 1, 0, blocks, zero);

    result.total = tree[1];
    return result;
}

template <class T, class Predicate>
vector<T> filter(const vector<T>& input, const Predicate& predicate) {
    vector<unsigned char> flags(input.size());
    parallel_for(0, input.size(), [&](size_t i) {
        flags[i] = predicate(input[i]);
    });

    ScanResult<size_t> scan = exclusive_scan(flags, size_t{0}, plus<size_t>{});

    vector<T> output(scan.total);
    parallel_for(0, input.size(), [&](size_t i) {
        if (flags[i] != 0) {
            output[scan.offsets[i]] = input[i];
        }
    });
    return output;
}

}  // namespace utils
