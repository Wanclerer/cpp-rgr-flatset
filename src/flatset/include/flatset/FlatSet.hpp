#pragma once

#include <cstddef>
#include <functional>
#include <new>
#include <utility>

namespace custom {

template <typename Key, typename Compare = std::less<Key>>
class FlatSet {
public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using value_compare = Compare;

    // --- Конструкторы ---
    FlatSet() : FlatSet(Compare()) {}

    explicit FlatSet(const Compare& comp)
        : data_(nullptr), size_(0), capacity_(0), comp_(comp) {}

    // --- 1. Деструктор ---
    ~FlatSet() {
        clear();
        deallocate(data_);
    }

    // --- 2. Конструктор копирования (Deep Copy) ---
    FlatSet(const FlatSet& other)
        : data_(nullptr), size_(0), capacity_(0), comp_(other.comp_) {
        if (other.size_ > 0) {
            data_ = allocate(other.size_);
            capacity_ = other.size_;
            try {
                for (size_type i = 0; i < other.size_; ++i) {
                    new (static_cast<void*>(data_ + i)) Key(other.data_[i]);
                    ++size_;
                }
            } catch (...) {
                clear();
                deallocate(data_);
                data_ = nullptr;
                capacity_ = 0;
                throw;
            }
        }
    }

    // --- 3. Перемещающий конструктор ---
    FlatSet(FlatSet&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          comp_(std::move(other.comp_)) {}

    // --- 4. Копирующее присваивание (Copy-and-Swap) ---
    FlatSet& operator=(const FlatSet& other) {
        if (this != &other) {
            FlatSet temp(other);
            swap(temp);
        }
        return *this;
    }

    // --- 5. Перемещающее присваивание ---
    FlatSet& operator=(FlatSet&& other) noexcept {
        if (this != &other) {
            clear();
            deallocate(data_);
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
            comp_ = std::move(other.comp_);
        }
        return *this;
    }

    // --- Управление емкостью и памятью ---
    void reserve(size_type new_cap) {
        if (new_cap <= capacity_) {
            return;
        }

        Key* new_data = allocate(new_cap);
        size_type constructed = 0;

        try {
            for (size_type i = 0; i < size_; ++i) {
                new (static_cast<void*>(new_data + i)) Key(std::move_if_noexcept(data_[i]));
                ++constructed;
            }
        } catch (...) {
            for (size_type i = 0; i < constructed; ++i) {
                new_data[i].~Key();
            }
            deallocate(new_data);
            throw;
        }

        for (size_type i = 0; i < size_; ++i) {
            data_[i].~Key();
        }
        deallocate(data_);

        data_ = new_data;
        capacity_ = new_cap;
    }

    void clear() noexcept {
        for (size_type i = 0; i < size_; ++i) {
            data_[i].~Key();
        }
        size_ = 0;
    }

    void swap(FlatSet& other) noexcept {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
        swap(capacity_, other.capacity_);
        swap(comp_, other.comp_);
    }

    // --- Базовые методы ---
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    [[nodiscard]] key_compare key_comp() const { return comp_; }

protected:
    // Вспомогательный метод для тестирования памяти до реализации insert
    void push_back_unchecked(const Key& val) {
        if (size_ == capacity_) {
            reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        new (static_cast<void*>(data_ + size_)) Key(val);
        ++size_;
    }

private:
    static Key* allocate(size_type n) {
        if (n == 0) {
            return nullptr;
        }
        return static_cast<Key*>(::operator new(n * sizeof(Key)));
    }

    static void deallocate(Key* ptr) noexcept {
        ::operator delete(static_cast<void*>(ptr));
    }

    Key* data_{nullptr};
    size_type size_{0};
    size_type capacity_{0};
    Compare comp_{};

    friend class FlatSetTestPeer;
};

template <typename Key, typename Compare>
void swap(FlatSet<Key, Compare>& lhs, FlatSet<Key, Compare>& rhs) noexcept {
    lhs.swap(rhs);
}

}  // namespace custom