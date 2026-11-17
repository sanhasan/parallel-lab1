#include "quicksort.hpp"
#include "utils.hpp"

#include <vector>

using namespace std;

static constexpr size_t BLOCK = 1024*8;

void quick_sort_par(vector<int>& data) {
    if (data.size() <= BLOCK) {
        quick_sort_seq(data, 0, data.size());
        return;
    }

    const size_t size = data.size();
    const int pivot = utils::median(data.front(), data[size / 2], data.back());

    vector<int> less;
    vector<int> equal;
    vector<int> greater;
    parlay::par_do(
        [&] {
            less = utils::filter(data, [=](int value) {
                return value < pivot;
            });
        },
        [&] {
            parlay::par_do(
                [&] {
                    equal = utils::filter(data, [=](int value) {
                        return value == pivot;
                    });
                },
                [&] {
                    greater = utils::filter(data, [=](int value) {
                        return value > pivot;
                    });
                });
        });

    // очищаем память, чтобы не плодить её на каждом уровне рекурсии
    vector<int>().swap(data);

    parlay::par_do(
        [&] { quick_sort_par(less); },
        [&] { quick_sort_par(greater); });

    data.resize(size);
    utils::parallel_for(0, size, [&](size_t i) {
        if (i < less.size()) {
            data[i] = less[i];
        } else if (i < less.size() + equal.size()) {
            data[i] = equal[i - less.size()];
        } else {
            data[i] = greater[i - less.size() - equal.size()];
        }
    });
}
