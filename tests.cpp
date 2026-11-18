#include "quicksort.hpp"
#include "utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

using namespace std;

static uint64_t random_value(uint64_t& state) {
    uint64_t value = (state += 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

static bool run_tests() {
    {
        // Arrange
        vector<int> actual = {5, -1, 4, 2, 2, 8, 0};
        const vector<int> expected = {-1, 0, 2, 2, 4, 5, 8};

        // Act: параметр не указан, поэтому должен использоваться baseline.
        quick_sort(actual);

        // Assert
        if (actual != expected) {
            cerr << "baseline quicksort: неверный результат\n";
            return false;
        }
    }

    {
        // Arrange
        const vector<int> input = {
            numeric_limits<int>::max(), 3, 3, -1, 0,
            numeric_limits<int>::min(), 3, -1};
        const vector<int> expected = {
            numeric_limits<int>::min(), -1, -1, 0, 3, 3, 3,
            numeric_limits<int>::max()};

        for (size_t threads : {size_t{2}, size_t{4}}) {
            // Act
            vector<int> actual = input;
            quick_sort(actual, threads);

            // Assert
            if (actual != expected) {
                cerr << "parallel quicksort (" << threads
                          << " потока): неверный результат\n";
                return false;
            }
        }
    }

    {
        // Arrange
        uint64_t state = 123456789;
        vector<int> original(100000);
        for (int& value : original) {
            value = static_cast<int>(random_value(state));
        }
        vector<int> expected = original;
        sort(expected.begin(), expected.end());  // Эталон только для теста.

        for (size_t threads : {size_t{1}, size_t{4}}) {
            // Act
            vector<int> actual = original;
            quick_sort(actual, threads);

            // Assert
            if (actual != expected) {
                cerr << "random quicksort (" << threads
                          << " потока): неверный результат\n";
                return false;
            }
        }
    }

    for (int pattern = 0; pattern < 4; ++pattern) {
        // Размер больше SEQ_BLOCK: проверяем не только
        // последовательную ветку, но и чередование двух буферов.
        vector<int> input(500000);
        for (size_t i = 0; i < input.size(); ++i) {
            if (pattern == 0) input[i] = static_cast<int>(i);
            if (pattern == 1) input[i] = static_cast<int>(input.size() - i);
            if (pattern == 2) input[i] = 7;
            if (pattern == 3) input[i] = static_cast<int>(i % 17) - 8;
        }

        vector<int> expected = input;
        sort(expected.begin(), expected.end());

        quick_sort(input, 4);
        if (input != expected) {
            cerr << "parallel quicksort: ошибка на шаблоне "
                      << pattern << '\n';
            return false;
        }
    }

    return true;
}

static bool sorted(const vector<int>& data) {
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] < data[i - 1]) return false;
    }
    return true;
}

static double benchmark_one(size_t size, uint64_t seed, size_t threads) {
    vector<int> data(size);
    for (int& value : data) {
        value = static_cast<int>(random_value(seed));
    }

    const auto begin = chrono::steady_clock::now();
    quick_sort(data, threads);
    const auto end = chrono::steady_clock::now();
    
    return chrono::duration<double>(end - begin).count();
}

static double benchmark_series(size_t size, size_t runs, size_t threads) {
    double sum = 0.0;
    for (size_t run = 0; run < runs; ++run) {
        const uint64_t seed =
            0x243f6a8885a308d3ULL + run * 0x9e3779b97f4a7c15ULL;
        const double seconds = benchmark_one(size, seed, threads);
        sum += seconds;
        cout << (threads == 1 ? "seq" : "par") << " запуск " << run + 1
                  << ": " << seconds << " с\n";
    }
    const double avg = sum / runs;
    cout << (threads == 1 ? "seq" : "par") << " среднее: " << avg << " с\n";
    return avg;
}

static int run_benchmark(int argc, char** argv) {
    const size_t size =
        argc > 2 ? strtoull(argv[2], nullptr, 10) : 100000000;
    const size_t runs =
        argc > 3 ? strtoull(argv[3], nullptr, 10) : 5;
    const size_t threads =
        argc > 4 ? strtoull(argv[4], nullptr, 10) : 4;
    const double required_speedup =
        argc > 5 ? strtod(argv[5], nullptr) : 0.0;

    if (size == 0 || runs == 0 || threads <= 1) return 2;
    cout << "Элементов: " << size << ", запусков: " << runs
              << ", потоков: " << threads << '\n';
    
    const double seq = benchmark_series(size, runs, 1);
    const double par = benchmark_series(size, runs, threads);
    const double speedup = seq / par;
    cout << "Ускорение: " << speedup << "\n";

    if (speedup < required_speedup) {
        cerr << "Ошибка: ускорение меньше требуемых "
                  << required_speedup << "\n";
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1 && string_view(argv[1]) == "--benchmark") {
        return run_benchmark(argc, argv);
    }

    if (!run_tests()) return 1;
    cout << "Все тесты пройдены\n";
}
