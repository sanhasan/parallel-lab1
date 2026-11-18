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
