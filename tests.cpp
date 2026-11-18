#include "quicksort.hpp"
#include "utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

using namespace std;

struct SortTestCase {
    string_view name;
    vector<int> input;
    vector<int> expected;
};

static bool sorted(const vector<int>& data) {
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] < data[i - 1]) {
            return false;
        }
    }
    return true;
}

static SortTestCase make_test_case(string_view name, vector<int> input) {
    vector<int> expected = input;
    sort(expected.begin(), expected.end());
    return {name, move(input), move(expected)};
}

static vector<int> random_data(size_t size, int minimum, int maximum) {
    random_device device;
    mt19937 generator(device());
    uniform_int_distribution<int> distribution(minimum, maximum);

    vector<int> data(size);
    for (int& value : data) {
        value = distribution(generator);
    }
    return data;
}

static bool check_sort(const SortTestCase& test, size_t threads) {
    vector<int> actual = test.input;
    quick_sort(actual, threads);

    if (!sorted(actual)) {
        cerr << test.name << " (" << threads
             << " потока): нарушен порядок элементов\n";
        return false;
    }
    if (actual != test.expected) {
        cerr << test.name << " (" << threads
             << " потока): изменился набор элементов\n";
        return false;
    }
    return true;
}

static bool run_tests() {
    vector<SortTestCase> small_cases;
    small_cases.push_back(make_test_case("пустой массив", {}));
    small_cases.push_back(make_test_case("один элемент", {42}));
    small_cases.push_back(make_test_case("два по возрастанию", {1, 2}));
    small_cases.push_back(make_test_case("два по убыванию", {2, 1}));
    small_cases.push_back(make_test_case("одинаковые элементы", {5, 5, 5}));
    small_cases.push_back(make_test_case(
        "смешанные числа", {5, -1, 4, 2, 2, 8, 0}));
    small_cases.push_back(make_test_case(
        "уже отсортирован", {-5, -2, 0, 1, 4, 9}));
    small_cases.push_back(make_test_case(
        "обратный порядок", {9, 4, 1, 0, -2, -5}));
    small_cases.push_back(make_test_case(
        "крайние значения",
        {numeric_limits<int>::max(), 3, 3, -1, 0,
         numeric_limits<int>::min(), 3, -1}));

    for (const SortTestCase& test : small_cases) {
        for (size_t threads : {size_t{1}, size_t{2}, size_t{4}}) {
            if (!check_sort(test, threads)) {
                return false;
            }
        }
    }

    constexpr size_t large_size = 500000;
    vector<SortTestCase> large_cases;

    vector<int> ascending(large_size);
    vector<int> descending(large_size);
    vector<int> equal(large_size, 7);
    vector<int> few_unique(large_size);
    vector<int> almost_sorted(large_size);

    for (size_t i = 0; i < large_size; ++i) {
        ascending[i] = static_cast<int>(i);
        descending[i] = static_cast<int>(large_size - i);
        few_unique[i] = static_cast<int>(i % 17) - 8;
        almost_sorted[i] = static_cast<int>(i);
    }
    for (size_t i = 1000; i < large_size; i += 1000) {
        swap(almost_sorted[i - 1], almost_sorted[i]);
    }

    large_cases.push_back(make_test_case(
        "большой отсортированный", move(ascending)));
    large_cases.push_back(make_test_case(
        "большой обратный", move(descending)));
    large_cases.push_back(make_test_case(
        "большой одинаковый", move(equal)));
    large_cases.push_back(make_test_case(
        "большой с повторами", move(few_unique)));
    large_cases.push_back(make_test_case(
        "почти отсортированный", move(almost_sorted)));
    large_cases.push_back(make_test_case(
        "случайный полный диапазон",
        random_data(large_size, numeric_limits<int>::min(),
                    numeric_limits<int>::max())));
    large_cases.push_back(make_test_case(
        "случайный с повторами", random_data(large_size, -100, 100)));

    for (const SortTestCase& test : large_cases) {
        if (!check_sort(test, 4)) {
            return false;
        }
    }

    return true;
}

static double benchmark_one(
    size_t size,
    mt19937::result_type seed,
    size_t threads
) {
    mt19937 generator(seed);
    uniform_int_distribution<int> distribution(
        numeric_limits<int>::min(), numeric_limits<int>::max());
    vector<int> data(size);
    for (int& value : data) {
        value = distribution(generator);
    }

    const auto begin = chrono::steady_clock::now();
    quick_sort(data, threads);
    const auto end = chrono::steady_clock::now();
    
    return chrono::duration<double>(end - begin).count();
}

static double benchmark_series(
    size_t size,
    size_t threads,
    const vector<mt19937::result_type>& seeds
) {
    double sum = 0.0;
    for (size_t run = 0; run < seeds.size(); ++run) {
        const double seconds = benchmark_one(size, seeds[run], threads);
        sum += seconds;
        cout << (threads == 1 ? "seq" : "par") << " запуск " << run + 1
                  << ": " << seconds << " с\n";
    }
    const double avg = sum / seeds.size();
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

    random_device device;
    vector<mt19937::result_type> seeds(runs);
    for (mt19937::result_type& seed : seeds) {
        seed = device();
    }

    const double seq = benchmark_series(size, 1, seeds);
    const double par = benchmark_series(size, threads, seeds);
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
