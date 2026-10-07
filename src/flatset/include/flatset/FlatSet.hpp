#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
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

    // --- Вложенный класс итератора произвольного доступа ---
    class ConstIterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = Key;
        using difference_type = std::ptrdiff_t;
        using pointer = const Key*;
        using reference = const Key&;

        constexpr ConstIterator() noexcept : ptr_(nullptr) {}
        constexpr explicit ConstIterator(const Key* ptr) noexcept : ptr_(ptr) {}

        [[nodiscard]] reference operator*() const noexcept { return *ptr_; }
        [[nodiscard]] pointer operator->() const noexcept { return ptr_; }

        ConstIterator& operator++() noexcept {
            ++ptr_;
            return *this;
        }

        ConstIterator operator++(int) noexcept {
            ConstIterator temp = *this;
            ++ptr_;
            return temp;
        }

        ConstIterator& operator--() noexcept {
            --ptr_;
            return *this;
        }

        ConstIterator operator--(int) noexcept {
            ConstIterator temp = *this;
            --ptr_;
            return temp;
        }

        ConstIterator& operator+=(difference_type n) noexcept {
            ptr_ += n;
            return *this;
        }

        [[nodiscard]] ConstIterator operator+(difference_type n) const noexcept {
            return ConstIterator(ptr_ + n);
        }

        [[nodiscard]] friend ConstIterator operator+(difference_type n, const ConstIterator& it) noexcept {
            return ConstIterator(it.ptr_ + n);
        }

        ConstIterator& operator-=(difference_type n) noexcept {
            ptr_ -= n;
            return *this;
        }

        [[nodiscard]] ConstIterator operator-(difference_type n) const noexcept {
            return ConstIterator(ptr_ - n);
        }

        [[nodiscard]] difference_type operator-(const ConstIterator& other) const noexcept {
            return ptr_ - other.ptr_;
        }

        [[nodiscard]] reference operator[](difference_type n) const noexcept {
            return *(ptr_ + n);
        }

        [[nodiscard]] bool operator==(const ConstIterator& other) const noexcept { return ptr_ == other.ptr_; }
        [[nodiscard]] bool operator!=(const ConstIterator& other) const noexcept { return ptr_ != other.ptr_; }
        [[nodiscard]] bool operator<(const ConstIterator& other) const noexcept { return ptr_ < other.ptr_; }
        [[nodiscard]] bool operator<=(const ConstIterator& other) const noexcept { return ptr_ <= other.ptr_; }
        [[nodiscard]] bool operator>(const ConstIterator& other) const noexcept { return ptr_ > other.ptr_; }
        [[nodiscard]] bool operator>=(const ConstIterator& other) const noexcept { return ptr_ >= other.ptr_; }

    private:
        const Key* ptr_{nullptr};
        friend class FlatSet;
    };

    using iterator = ConstIterator;
    using const_iterator = ConstIterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // --- Конструкторы ---
    FlatSet() : FlatSet(Compare()) {}

    explicit FlatSet(const Compare& comp)
        : data_(nullptr), size_(0), capacity_(0), comp_(comp) {}

    FlatSet(std::initializer_list<Key> init, const Compare& comp = Compare())
        : FlatSet(comp) {
        reserve(init.size());
        for (const auto& item : init) {
            insert(item);
        }
    }

    // --- Правило пяти (Rule of Five) ---
    ~FlatSet() {
        clear();
        deallocate(data_);
    }

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

    FlatSet(FlatSet&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          comp_(std::move(other.comp_)) {}

    FlatSet& operator=(const FlatSet& other) {
        if (this != &other) {
            FlatSet temp(other);
            swap(temp);
        }
        return *this;
    }

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

    // --- Навигация и итераторы ---
    [[nodiscard]] iterator begin() noexcept { return iterator(data_); }
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator(data_); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return const_iterator(data_); }

    [[nodiscard]] iterator end() noexcept { return iterator(data_ + size_); }
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator(data_ + size_); }
    [[nodiscard]] const_iterator cend() const noexcept { return const_iterator(data_ + size_); }

    [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    // --- Методы поиска ---
    [[nodiscard]] iterator lower_bound(const Key& key) {
        return iterator(c_lower_bound(key).ptr_);
    }

    [[nodiscard]] const_iterator lower_bound(const Key& key) const {
        return c_lower_bound(key);
    }

    [[nodiscard]] iterator upper_bound(const Key& key) {
        return iterator(c_upper_bound(key).ptr_);
    }

    [[nodiscard]] const_iterator upper_bound(const Key& key) const {
        return c_upper_bound(key);
    }

    [[nodiscard]] iterator find(const Key& key) {
        auto it = lower_bound(key);
        if (it != end() && !comp_(key, *it)) {
            return it;
        }
        return end();
    }

    [[nodiscard]] const_iterator find(const Key& key) const {
        auto it = lower_bound(key);
        if (it != end() && !comp_(key, *it)) {
            return it;
        }
        return end();
    }

    [[nodiscard]] bool contains(const Key& key) const {
        return find(key) != end();
    }

    // --- Методы модификации ---
    std::pair<iterator, bool> insert(const Key& value) {
        return insert_impl(value);
    }

    std::pair<iterator, bool> insert(Key&& value) {
        return insert_impl(std::move(value));
    }

    iterator erase(const_iterator pos) {
        size_type idx = pos - cbegin();
        for (size_type i = idx; i + 1 < size_; ++i) {
            data_[i] = std::move(data_[i + 1]);
        }
        data_[size_ - 1].~Key();
        --size_;
        return iterator(data_ + idx);
    }

    size_type erase(const Key& key) {
        auto it = find(key);
        if (it != end()) {
            erase(it);
            return 1;
        }
        return 0;
    }

    iterator erase(const_iterator first, const_iterator last) {
        size_type idx_first = first - cbegin();
        size_type idx_last = last - cbegin();
        size_type count = idx_last - idx_first;
        if (count == 0) {
            return iterator(data_ + idx_first);
        }

        for (size_type i = idx_last; i < size_; ++i) {
            data_[i - count] = std::move(data_[i]);
        }
        for (size_type i = size_ - count; i < size_; ++i) {
            data_[i].~Key();
        }
        size_ -= count;
        return iterator(data_ + idx_first);
    }

    // Слияние двух множеств за O(N + M)
    void merge(FlatSet& source) {
        if (this == &source || source.empty()) {
            return;
        }

        // 1. Первый проход: считаем, сколько уникальных элементов перейдет в *this
        size_type i = 0;
        size_type j = 0;
        size_type to_add = 0;

        while (i < size_ && j < source.size_) {
            if (comp_(data_[i], source.data_[j])) {
                ++i;
            } else if (comp_(source.data_[j], data_[i])) {
                ++to_add;
                ++j;
            } else {
                // Дубликат: остается в source
                ++i;
                ++j;
            }
        }
        to_add += (source.size_ - j);

        if (to_add == 0) {
            return;
        }

        // 2. Выделяем новый буфер под объединенные элементы
        size_type new_cap = size_ + to_add;
        Key* new_data = allocate(new_cap);
        size_type constructed = 0;

        i = 0;
        j = 0;
        size_type write_j = 0;  // позиция для уплотнения оставшихся элементов в source

        try {
            while (i < size_ && j < source.size_) {
                if (comp_(data_[i], source.data_[j])) {
                    new (static_cast<void*>(new_data + constructed)) Key(std::move_if_noexcept(data_[i]));
                    ++constructed;
                    ++i;
                } else if (comp_(source.data_[j], data_[i])) {
                    new (static_cast<void*>(new_data + constructed)) Key(std::move(source.data_[j]));
                    ++constructed;
                    ++j;
                } else {
                    // Дубликат переносим в *this из текущего множества, а в source сохраняем
                    new (static_cast<void*>(new_data + constructed)) Key(std::move_if_noexcept(data_[i]));
                    ++constructed;
                    ++i;
                    if (write_j != j) {
                        source.data_[write_j] = std::move(source.data_[j]);
                    }
                    ++write_j;
                    ++j;
                }
            }

            while (i < size_) {
                new (static_cast<void*>(new_data + constructed)) Key(std::move_if_noexcept(data_[i]));
                ++constructed;
                ++i;
            }

            while (j < source.size_) {
                new (static_cast<void*>(new_data + constructed)) Key(std::move(source.data_[j]));
                ++constructed;
                ++j;
            }
        } catch (...) {
            for (size_type k = 0; k < constructed; ++k) {
                new_data[k].~Key();
            }
            deallocate(new_data);
            throw;
        }

        // Уничтожаем старый буфер *this
        for (size_type k = 0; k < size_; ++k) {
            data_[k].~Key();
        }
        deallocate(data_);

        data_ = new_data;
        size_ = constructed;
        capacity_ = new_cap;

        // Уничтожаем перемещенные хвосты в source
        for (size_type k = write_j; k < source.size_; ++k) {
            source.data_[k].~Key();
        }
        source.size_ = write_j;
    }

    void merge(FlatSet&& source) {
        merge(source);
    }

    // --- Управление емкостью ---
    void shrink_to_fit() {
        if (size_ == capacity_) {
            return;
        }
        if (size_ == 0) {
            deallocate(data_);
            data_ = nullptr;
            capacity_ = 0;
            return;
        }

        Key* new_data = allocate(size_);
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
        capacity_ = size_;
    }
    
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

    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    [[nodiscard]] key_compare key_comp() const { return comp_; }

protected:
    void push_back_unchecked(const Key& val) {
        if (size_ == capacity_) {
            reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        new (static_cast<void*>(data_ + size_)) Key(val);
        ++size_;
    }

private:
    [[nodiscard]] const_iterator c_lower_bound(const Key& key) const {
        size_type count = size_;
        size_type first = 0;
        while (count > 0) {
            size_type step = count / 2;
            size_type mid = first + step;
            if (comp_(data_[mid], key)) {
                first = mid + 1;
                count -= step + 1;
            } else {
                count = step;
            }
        }
        return const_iterator(data_ + first);
    }

    [[nodiscard]] const_iterator c_upper_bound(const Key& key) const {
        size_type count = size_;
        size_type first = 0;
        while (count > 0) {
            size_type step = count / 2;
            size_type mid = first + step;
            if (!comp_(key, data_[mid])) {
                first = mid + 1;
                count -= step + 1;
            } else {
                count = step;
            }
        }
        return const_iterator(data_ + first);
    }

    template <typename U>
    std::pair<iterator, bool> insert_impl(U&& value) {
        auto it = lower_bound(value);
        size_type idx = it - begin();

        // Проверка эквивалентности: дубликаты не вставляются
        if (it != end() && !comp_(value, *it)) {
            return {it, false};
        }

        // Копируем/перемещаем значение в локальную переменную на случай реаллокации
        Key val_to_insert(std::forward<U>(value));

        if (size_ == capacity_) {
            size_type new_cap = (capacity_ == 0) ? 1 : capacity_ * 2;
            Key* new_data = allocate(new_cap);
            size_type constructed = 0;

            try {
                for (size_type i = 0; i < idx; ++i) {
                    new (static_cast<void*>(new_data + i)) Key(std::move_if_noexcept(data_[i]));
                    ++constructed;
                }
                new (static_cast<void*>(new_data + idx)) Key(std::move(val_to_insert));
                ++constructed;
                for (size_type i = idx; i < size_; ++i) {
                    new (static_cast<void*>(new_data + i + 1)) Key(std::move_if_noexcept(data_[i]));
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
            ++size_;
            return {iterator(data_ + idx), true};
        }

        // Емкости достаточно: сдвиг вправо
        if (idx == size_) {
            new (static_cast<void*>(data_ + size_)) Key(std::move(val_to_insert));
            ++size_;
        } else {
            new (static_cast<void*>(data_ + size_)) Key(std::move(data_[size_ - 1]));
            for (size_type i = size_ - 1; i > idx; --i) {
                data_[i] = std::move(data_[i - 1]);
            }
            data_[idx] = std::move(val_to_insert);
            ++size_;
        }

        return {iterator(data_ + idx), true};
    }

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