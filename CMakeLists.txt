cmake_minimum_required(VERSION 3.22)
project(FlatSetProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опция для включения санитайзеров (ASan + UBSan)
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

if(ENABLE_SANITIZERS)
  if(MSVC)
    add_compile_options(/fsanitize=address)
  else()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()

# Опция сборки тестов
option(BUILD_TESTS "Build unit tests" ON)

# Подключение библиотеки
add_subdirectory(src/flatset)

# Подключение модульных тестов
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()