# FlatSet (C++17)

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![CMake](https://img.shields.io/badge/CMake-3.22+-brightgreen.svg)](https://cmake.org/)
[![Tests](https://img.shields.io/badge/Tests-GoogleTest%2012%2F12%20Passed-success.svg)](https://github.com/google/googletest)
[![Sanitizers](https://img.shields.io/badge/Sanitizers-ASan%20%7C%20UBSan%20Clean-success.svg)]()
[![Valgrind](https://img.shields.io/badge/Valgrind-0%20leaks%20%2F%200%20errors-brightgreen.svg)]()

Реализация шаблонного ассоциативного контейнера множества `FlatSet` на языке C++17 в рамках расчетно-графической работы (РГР) по дисциплине **«Современные технологии программирования»** (СибГУТИ, Кафедра вычислительных систем).

---

## Архитектура и особенности

В отличие от стандартного узлового контейнера `std::set` (традиционно основанного на красно-черных деревьях), `FlatSet` хранит элементы в **едином непрерывном динамическом буфере** в строго упорядоченном виде:

* **Cache Locality:** непрерывное размещение в оперативной памяти минимизирует промахи кэша процессора (L1/L2/L3) при обходе и бинарном поиске.
* **Нулевой оверхед на метаданные:** отсутствуют служебные указатели узлов дерева (экономия до 24–32 байт на каждый элемент).
* **Управление ресурсами (RAII):** разделены этапы выделения сырой памяти (`::operator new`) и конструирования объектов (`placement new`), что снимает требование наличия конструктора по умолчанию у типа `Key`.
* **Правило пяти (Rule of Five):** реализованы глубокое копирование (Deep Copy) и перемещение без аллокаций (Move-семантика).
* **Strong Exception Guarantee:** строгая гарантия безопасности исключений при реаллокации буфера за счет `std::move_if_noexcept`.
* **Random Access Iterators:** итераторы удовлетворяют концепту `std::random_access_iterator_tag`, полностью совместимы с алгоритмами STL (`std::lower_bound`, `std::binary_search`, `std::distance`) и защищают инвариант сортировки за счет константности разыменования (`const Key&`).

---

## Асимптотическая сложность

| Операция | Сложность | Описание |
| :--- | :--- | :--- |
| `find(key)` / `contains(key)` | $O(\log N)$ | Двоичный поиск позиции ключа |
| `lower_bound` / `upper_bound` | $O(\log N)$ | Поиск границы диапазона |
| `insert(val)` | $O(\log N) + O(N)$ | Двоичный поиск позиции + сдвиг элементов буфера |
| `erase(it)` / `erase(key)` | $O(N)$ | Сдвиг оставшихся элементов влево |
| `merge(source)` | $O(N + M)$ | Линейное слияние двух отсортированных последовательностей |
| `reserve(n)` / `shrink_to_fit()` | $O(N)$ | Реаллокация буфера и перемещение элементов |
| `size()` / `empty()` | $O(1)$ | Проверка состояния контейнера |

---

## Структура проекта

Проект организован по стандарту **Canonical Project Structure (P1204R0)**:

```text
.
├── CMakeLists.txt              # Корневая конфигурация сборки
├── CMakePresets.json           # Пресеты сборки (Debug, Release)
├── .clang-format               # Правила форматирования (Google Style)
├── .clang-tidy                 # Конфигурация статического анализатора
├── src/
│   └── flatset/
│       ├── CMakeLists.txt      # INTERFACE library цель
│       └── include/flatset/
│           └── FlatSet.hpp     # Шаблонный класс FlatSet
└── tests/
    ├── CMakeLists.txt          # Интеграция GoogleTest
    └── test_flatset.cpp        # 12 модульных тестов
