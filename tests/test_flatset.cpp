#include <gtest/gtest.h>

#include <algorithm>
#include <flatset/FlatSet.hpp>
#include <numeric>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

// ============================================================================
// 1. Тестирование управления ресурсами (RAII) и Rule of Five
// ============================================================================

struct Tracker {
    static inline int live_objects = 0;
    static inline int copy_count = 0;
    static inline int move_count = 0;

    int value{0};

    explicit Tracker(int v = 0) : value(v) { ++live_objects; }
    Tracker(const Tracker& other) : value(other.value) {
        ++live_objects;
        ++copy_count;
    }
    Tracker(Tracker&& other) noexcept : value(other.value) {
        ++live_objects;
        ++move_count;
    }
    Tracker& operator=(const Tracker& other) {
        value = other.value;
        ++copy_count;
        return *this;
    }
    Tracker& operator=(Tracker&& other) noexcept {
        value = other.value;
        ++move_count;
        return *this;
    }
    ~Tracker() { --live_objects; }

    bool operator<(const Tracker& other) const { return value < other.value; }
};

class FlatSetResourceTest : public ::testing::Test {
   protected:
    void SetUp() override {
        Tracker::live_objects = 0;
        Tracker::copy_count = 0;
        Tracker::move_count = 0;
    }
    void TearDown() override { EXPECT_EQ(Tracker::live_objects, 0); }
};

TEST_F(FlatSetResourceTest, DefaultConstructorAndShrinkToFit) {
    custom::FlatSet<Tracker> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.capacity(), 0);

    s.reserve(50);
    EXPECT_EQ(s.capacity(), 50);
    EXPECT_EQ(Tracker::live_objects, 0);

    s.shrink_to_fit();
    EXPECT_EQ(s.capacity(), 0);
}

TEST_F(FlatSetResourceTest, DeepCopyAndAssignment) {
    custom::FlatSet<Tracker> original;
    original.insert(Tracker(10));
    original.insert(Tracker(20));

    custom::FlatSet<Tracker> copy(original);
    EXPECT_EQ(copy.size(), 2);
    EXPECT_EQ(Tracker::live_objects, 4);

    custom::FlatSet<Tracker> assigned;
    assigned = copy;
    EXPECT_EQ(assigned.size(), 2);
    EXPECT_EQ(Tracker::live_objects, 6);
}

TEST_F(FlatSetResourceTest, MoveConstructorAndMoveAssignment) {
    custom::FlatSet<Tracker> src;
    src.insert(Tracker(100));
    src.insert(Tracker(200));

    int copies_before = Tracker::copy_count;

    custom::FlatSet<Tracker> dst(std::move(src));
    EXPECT_EQ(dst.size(), 2);
    EXPECT_EQ(src.size(), 0);      // NOLINT
    EXPECT_EQ(src.capacity(), 0);  // NOLINT
    EXPECT_EQ(Tracker::copy_count, copies_before);

    custom::FlatSet<Tracker> dst2;
    dst2 = std::move(dst);
    EXPECT_EQ(dst2.size(), 2);
    EXPECT_EQ(dst.size(), 0);  // NOLINT
    EXPECT_EQ(Tracker::copy_count, copies_before);
}

// ============================================================================
// 2. Тестирование строгой безопасности исключений
// ============================================================================

struct ThrowOnCopy {
    static inline int live_objects = 0;
    static inline int throw_after = -1;
    static inline int copy_attempts = 0;

    int id{0};

    explicit ThrowOnCopy(int val = 0) : id(val) { ++live_objects; }
    ThrowOnCopy(const ThrowOnCopy& other) : id(other.id) {
        ++copy_attempts;
        if (throw_after > 0 && copy_attempts >= throw_after) {
            throw std::runtime_error("Simulated copy exception");
        }
        ++live_objects;
    }
    ThrowOnCopy(ThrowOnCopy&& other) noexcept(false) : ThrowOnCopy(other) {}  // NOLINT
    ThrowOnCopy& operator=(const ThrowOnCopy& other) = default;
    ThrowOnCopy& operator=(ThrowOnCopy&& other) noexcept(false) = default;
    ~ThrowOnCopy() { --live_objects; }

    bool operator<(const ThrowOnCopy& other) const { return id < other.id; }
};

TEST(FlatSetExceptionTest, StrongExceptionGuaranteeOnRealloc) {
    ThrowOnCopy::live_objects = 0;
    ThrowOnCopy::copy_attempts = 0;
    ThrowOnCopy::throw_after = -1;

    custom::FlatSet<ThrowOnCopy> s;
    s.insert(ThrowOnCopy(1));
    s.insert(ThrowOnCopy(2));

    ThrowOnCopy::copy_attempts = 0;
    ThrowOnCopy::throw_after = 2;

    EXPECT_THROW(s.reserve(100), std::runtime_error);

    EXPECT_EQ(s.size(), 2);
    s.clear();
    EXPECT_EQ(ThrowOnCopy::live_objects, 0);
}

// ============================================================================
// 3. Тестирование итераторов и алгоритмов STL
// ============================================================================

TEST(FlatSetIteratorTest, TraitsAndConstness) {
    using Set = custom::FlatSet<int>;
    using Iter = Set::iterator;
    using Traits = std::iterator_traits<Iter>;

    static_assert(std::is_same_v<Traits::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<Traits::value_type, int>);
    static_assert(std::is_same_v<Traits::reference, const int&>);
}

TEST(FlatSetIteratorTest, AlgorithmsIntegration) {
    custom::FlatSet<int> s{10, 20, 30, 40, 50};

    EXPECT_TRUE(std::binary_search(s.begin(), s.end(), 30));
    EXPECT_FALSE(std::binary_search(s.begin(), s.end(), 99));

    auto it = std::lower_bound(s.begin(), s.end(), 25);
    ASSERT_NE(it, s.end());
    EXPECT_EQ(*it, 30);

    bool all_positive = std::all_of(s.begin(), s.end(), [](int x) { return x > 0; });
    EXPECT_TRUE(all_positive);
}

TEST(FlatSetIteratorTest, ReverseIteration) {
    custom::FlatSet<int> s{1, 2, 3, 4};
    std::vector<int> rev(s.rbegin(), s.rend());
    EXPECT_EQ(rev, (std::vector<int>{4, 3, 2, 1}));
}

// ============================================================================
// 4. Тестирование операций множества и краевых случаев
// ============================================================================

TEST(FlatSetOperationsTest, DuplicateHandling) {
    custom::FlatSet<int> s{5, 1, 5, 3, 1, 2, 3};
    EXPECT_EQ(s.size(), 4);

    std::vector<int> expected{1, 2, 3, 5};
    EXPECT_TRUE(std::equal(s.begin(), s.end(), expected.begin()));
}

TEST(FlatSetOperationsTest, CustomComparatorGreater) {
    custom::FlatSet<int, std::greater<>> s{10, 50, 20, 40, 30};

    std::vector<int> expected{50, 40, 30, 20, 10};
    std::vector<int> actual(s.begin(), s.end());
    EXPECT_EQ(actual, expected);

    EXPECT_TRUE(s.contains(40));
    EXPECT_FALSE(s.contains(999));
    EXPECT_EQ(*s.find(20), 20);
}

TEST(FlatSetOperationsTest, EraseEdgeCases) {
    custom::FlatSet<int> s{10, 20, 30, 40, 50};

    s.erase(s.begin());
    EXPECT_FALSE(s.contains(10));
    EXPECT_EQ(s.size(), 4);

    s.erase(--s.end());
    EXPECT_FALSE(s.contains(50));
    EXPECT_EQ(s.size(), 3);

    EXPECT_EQ(s.erase(999), 0);
    EXPECT_EQ(s.size(), 3);

    auto it_first = s.find(20);
    auto it_last = s.find(40);
    s.erase(it_first, it_last);
    EXPECT_EQ(s.size(), 1);
    EXPECT_TRUE(s.contains(40));

    s.erase(s.begin(), s.end());
    EXPECT_TRUE(s.empty());
}

TEST(FlatSetOperationsTest, MergeAdvancedCases) {
    {
        custom::FlatSet<int> a{1, 2, 3};
        custom::FlatSet<int> b{1, 2, 3};
        a.merge(b);
        EXPECT_EQ(a.size(), 3);
        EXPECT_EQ(b.size(), 3);
    }

    {
        custom::FlatSet<int> a{1, 3, 5};
        custom::FlatSet<int> b{2, 4, 6};
        a.merge(b);
        EXPECT_EQ(a.size(), 6);
        EXPECT_TRUE(b.empty());
        std::vector<int> exp{1, 2, 3, 4, 5, 6};
        EXPECT_TRUE(std::equal(a.begin(), a.end(), exp.begin()));
    }

    {
        custom::FlatSet<int> a{1, 2, 3};
        a.merge(a);
        EXPECT_EQ(a.size(), 3);
    }
}

TEST(FlatSetOperationsTest, StressTestRandomData) {
    custom::FlatSet<int> s;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 500);

    for (int i = 0; i < 1000; ++i) {
        s.insert(dist(rng));
    }

    ASSERT_FALSE(s.empty());
    for (auto it = s.begin(); it + 1 != s.end(); ++it) {
        EXPECT_LT(*it, *(it + 1));
    }
}