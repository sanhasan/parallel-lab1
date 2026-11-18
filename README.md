# Parallel quicksort

[![Tests and benchmark](https://github.com/sanhasan/parallel-lab1/actions/workflows/benchmark.yml/badge.svg)](https://github.com/sanhasan/parallel-lab1/actions/workflows/benchmark.yml)

[Открыть последний запуск тестов и benchmark](https://github.com/sanhasan/parallel-lab1/actions/workflows/benchmark.yml)


## Структура проекта

```text
parallel-lab1/
├── .github/
│   └── workflows/
│       └── benchmark.yml       — CI: тесты, benchmark и проверка ускорения
├── baseline.cpp                — последовательная реализация
├── parallel_quicksort.cpp      — параллельная реализация
├── quicksort.hpp               — публичный интерфейс
├── utils.hpp                   — вспомогательные параллельные операции
├── tests.cpp                   — функциональные тесты и benchmark
├── benchmark.sh                — сборка и локальный запуск benchmark
```

Добавлю ещё пару слов про написание параллельной быстрой сортировки. Изначально пытался писать код аналогичный лекциям (через три фильтра, каждый из которых на сканах, а они на двух проходах похожим на построение до), однако это чудо получало ускорение 0,67-1,2. После чего пришлось отказаться от всех красивых функций и начинать выжимать из железа - самый яркий пример это отсутствие лишних аллокаций и переиспользование исходного массива и одного вспомогательного буфера между разными итерациями в рекурсии (без этого кажется лучший результат был около 2,7).
