#pragma once

#include <algorithm>

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

}  // namespace utils
