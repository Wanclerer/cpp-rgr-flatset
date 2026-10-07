add_library(flatset INTERFACE)

# Путь для включения: #include <flatset/FlatSet.hpp>
target_include_directories(flatset INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(flatset INTERFACE cxx_std_17)