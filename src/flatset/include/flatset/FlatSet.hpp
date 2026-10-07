#pragma once

#include <cstddef>

namespace custom {

template <typename Key>
class FlatSet {
public:
    FlatSet() = default;
    [[nodiscard]] bool empty() const noexcept { return true; }
    [[nodiscard]] std::size_t size() const noexcept { return 0; }
};

}  // namespace custom