# FlatSet (C++17) [![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17) [![CMake](https://img.shields.io/badge/CMake-3.22+-brightgreen.svg)](https://cmake.org/) [![Tests](https://img.shields.io/badge/Tests-GoogleTest%2012%2F12%20Passed-success.svg)](https://github.com/google/googletest) [![Sanitizers](https://img.shields.io/badge/Sanitizers-ASan%20%7C%20UBSan%20Clean-success.svg)]() [![Valgrind](https://img.shields.io/badge/Valgrind-0%20leaks%20%2F%200%20errors-brightgreen.svg)]()

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
| `find(key)` / `contains(key)` | \(O(\log N)\) | Двоичный поиск позиции ключа |
| `lower_bound` / `upper_bound` | \(O(\log N)\) | Поиск границы диапазона |
| `insert(val)` | \(O(\log N) + O(N)\) | Двоичный поиск позиции + сдвиг элементов буфера |
| `erase(it)` / `erase(key)` | O(N) | Сдвиг оставшихся элементов влево |
| `merge(source)` | O(N + M) | Линейное слияние двух отсортированных последовательностей |
| `reserve(n)` / `shrink_to_fit()` | O(N) | Реаллокация буфера и перемещение элементов |
| `size()` / `empty()` | O(1) | Проверка состояния контейнера |

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
```

---

## Сборка и запуск

Проект использует систему мета-сборки CMake и строго следует концепции **Out-of-source build** — артефакты компиляции изолированы в отдельных директориях (`build/debug/`, `build/release/`) и не засоряют дерево исходных кодов.

### Системные требования

* **Компилятор C++:** GCC (>= 11) или Clang (>= 13) с поддержкой стандарта C++17
* **Система сборки:** CMake (>= 3.22)
* **Тестовый фреймворк:** GoogleTest (`libgtest-dev`)
* **Анализаторы и линтеры:** Valgrind, Clang-Tools (`clang-format`, `clang-tidy`)

Установка зависимостей в Ubuntu / Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake libgtest-dev valgrind clang-format clang-tidy
```

### Вариант 1. Сборка через CMake Presets (рекомендуемый)

Файл `CMakePresets.json` инкапсулирует параметры генератора, флаги компилятора и пути, исключая длинные ручные команды в терминале и обеспечивая совместимость с IDE (VS Code, CLion).

**Конфигурация Debug (с санитайзерами ASan + UBSan):**

```bash
# 1. Этап генерации файлов сборки
cmake --preset debug

# 2. Этап сборки (не вызываем make напрямую, используем абстракцию cmake --build)
cmake --build --preset debug

# 3. Запуск модульных тестов
ctest --preset debug
```

**Конфигурация Release (оптимизация -O3, санитайзеры отключены):**

```bash
cmake --preset release
cmake --build --preset release
```

### Вариант 2. Классическая сборка через CMake CLI

Если пресеты не используются, сборка выполняется стандартным двухэтапным вызовом CMake с явным указанием каталогов исходников (`-S`) и сборки (`-B`):

```bash
# Конфигурация и сборка Debug-версии
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build/debug

# Запуск тестов через CTest с выводом деталей при ошибках
ctest --test-dir build/debug --output-on-failure
```

---

## Проверка динамическими и статическими анализаторами

### 1. AddressSanitizer и UndefinedBehaviorSanitizer

Санитайзеры активны в профиле debug (флаги `-fsanitize=address,undefined -fno-omit-frame-pointer`). Запуск тестов под санитайзерами:

```bash
./build/debug/tests/flatset_tests
```

### 2. Valgrind Memcheck (проверка утечек сырой памяти)

Анализ выполняется на бинарном файле конфигурации Release (без санитайзеров):

```bash
valgrind --tool=memcheck \
         --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --error-exitcode=1 \
         ./build/release/tests/flatset_tests
```

### 3. Статический анализ (clang-tidy)

Проверка кода на соответствие Core Guidelines и поиск потенциальных багов:

```bash
clang-tidy -p build/debug tests/test_flatset.cpp
```

### 4. Автоматическое форматирование (clang-format)

Приведение исходных текстов к Google C++ Style:

```bash
clang-format -i src/flatset/include/flatset/FlatSet.hpp tests/test_flatset.cpp
```

---

## Очистка проекта

Очистка артефактов текущей сборки (clean target):

```bash
cmake --build --preset debug --target clean
```

Полное удаление сгенерированных файлов и кэша CMake:

```bash
rm -rf build
```
