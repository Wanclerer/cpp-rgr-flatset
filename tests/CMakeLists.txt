find_package(GTest REQUIRED)

add_executable(flatset_tests test_flatset.cpp)

# Подключение нашей библиотеки flatset и системного GTest
target_link_libraries(flatset_tests PRIVATE flatset GTest::gtest_main)

# Строгие флаги предупреждений компилятора
if(MSVC)
  target_compile_options(flatset_tests PRIVATE /W4 /WX)
else()
  target_compile_options(flatset_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(GoogleTest)
gtest_discover_tests(flatset_tests)