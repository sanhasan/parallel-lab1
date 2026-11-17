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

static std::uint64_t random_value(std::uint64_t& state) {
    std::uint64_t value = (state += 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

static bool run_tests() {
    {
        // Arrange
        const std::vector<std::size_t> input = {2, 3, 4};
        const std::vector<std::size_t> expected_offsets = {1, 2, 6};

        // Act
        const auto actual = utils::exclusive_scan(
            input, std::size_t{1}, std::multiplies<std::size_t>{});

        // Assert
        if (actual.offsets != expected_offsets || actual.total != 24) {
            std::cerr << "exclusive_scan: неверный результат\n";
            return false;
        }
    }

    {
        // Arrange
        const std::vector<int> input = {5, 2, 7, 4, 1, 6};
        const std::vector<int> expected = {2, 4, 6};

        // Act
        const std::vector<int> actual = utils::filter(input, [](int value) {
            return value % 2 == 0;
        });

        // Assert
        if (actual != expected) {
            std::cerr << "filter: неверный результат\n";
            return false;
        }
    }

    {
        // Arrange
        std::vector<int> actual = {5, -1, 4, 2, 2, 8, 0};
        const std::vector<int> expected = {-1, 0, 2, 2, 4, 5, 8};

        // Act: параметр не указан, поэтому должен использоваться baseline.
        quick_sort(actual);

        // Assert
        if (actual != expected) {
            std::cerr << "baseline quicksort: неверный результат\n";
            return false;
        }
    }

    {
        // Arrange
        const std::vector<int> input = {
            std::numeric_limits<int>::max(), 3, 3, -1, 0,
            std::numeric_limits<int>::min(), 3, -1};
        const std::vector<int> expected = {
            std::numeric_limits<int>::min(), -1, -1, 0, 3, 3, 3,
            std::numeric_limits<int>::max()};

        for (std::size_t threads : {std::size_t{2}, std::size_t{4}}) {
            // Act
            std::vector<int> actual = input;
            quick_sort(actual, threads);

            // Assert
            if (actual != expected) {
                std::cerr << "parallel quicksort (" << threads
                          << " потока): неверный результат\n";
                return false;
            }
        }
    }

    {
        // Arrange
        std::uint64_t state = 123456789;
        std::vector<int> original(100000);
        for (int& value : original) {
            value = static_cast<int>(random_value(state));
        }
        std::vector<int> expected = original;
        std::sort(expected.begin(), expected.end());  // Эталон только для теста.

        for (std::size_t threads : {std::size_t{1}, std::size_t{4}}) {
            // Act
            std::vector<int> actual = original;
            quick_sort(actual, threads);

            // Assert
            if (actual != expected) {
                std::cerr << "random quicksort (" << threads
                          << " потока): неверный результат\n";
                return false;
            }
        }
    }

    return true;
}

static bool sorted(const std::vector<int>& data) {
    for (std::size_t i = 1; i < data.size(); ++i) {
        if (data[i] < data[i - 1]) return false;
    }
    return true;
}

static double benchmark_one(std::size_t size, std::uint64_t seed,
                            std::size_t threads) {
    std::vector<int> data(size);
    for (int& value : data) value = static_cast<int>(random_value(seed));

    const auto begin = std::chrono::steady_clock::now();
    quick_sort(data, threads);
    const auto end = std::chrono::steady_clock::now();

    if (!sorted(data)) {
        std::cerr << "Сортировка вернула неверный результат\n";
        std::exit(1);
    }
    return std::chrono::duration<double>(end - begin).count();
}

static double benchmark_series(std::size_t size, std::size_t runs,
                               std::size_t threads) {
    double sum = 0.0;
    for (std::size_t run = 0; run < runs; ++run) {
        const std::uint64_t seed =
            0x243f6a8885a308d3ULL + run * 0x9e3779b97f4a7c15ULL;
        const double seconds = benchmark_one(size, seed, threads);
        sum += seconds;
        std::cout << (threads == 1 ? "seq" : "par") << " запуск " << run + 1
                  << ": " << seconds << " с\n";
    }
    const double average = sum / runs;
    std::cout << (threads == 1 ? "seq" : "par")
              << " среднее: " << average << " с\n";
    return average;
}

static int run_benchmark(int argc, char** argv) {
    const std::size_t size =
        argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 100000000;
    const std::size_t runs =
        argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 5;
    const std::size_t threads =
        argc > 4 ? std::strtoull(argv[4], nullptr, 10) : 4;
    const double required_speedup =
        argc > 5 ? std::strtod(argv[5], nullptr) : 0.0;
    if (size == 0 || runs == 0 || threads <= 1) return 2;

    std::cout << "Элементов: " << size << ", запусков: " << runs
              << ", потоков: " << threads << '\n';
    const double seq = benchmark_series(size, runs, 1);
    const double par = benchmark_series(size, runs, threads);
    const double speedup = seq / par;
    std::cout << "Ускорение: " << speedup << "x\n";

    if (speedup < required_speedup) {
        std::cerr << "Ошибка: ускорение меньше требуемых "
                  << required_speedup << "x\n";
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view(argv[1]) == "--benchmark") {
        return run_benchmark(argc, argv);
    }

    if (!run_tests()) return 1;
    std::cout << "Все тесты пройдены\n";
}
