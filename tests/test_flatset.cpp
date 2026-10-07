#include <flatset/FlatSet.hpp>
#include <gtest/gtest.h>

#include <numeric>
#include <type_traits>
#include <vector>

namespace custom {
class FlatSetTestPeer {
public:
    template <typename Set, typename T>
    static void push_back(Set& s, const T& val) {
        s.push_back_unchecked(val);
    }
};
}  // namespace custom

// --- Тестовая структура RAII ---
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
    void TearDown() override {
        EXPECT_EQ(Tracker::live_objects, 0);
    }
};

TEST_F(FlatSetResourceTest, DefaultConstructor) {
    custom::FlatSet<Tracker> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(FlatSetResourceTest, ReserveSeparatesAllocationAndConstruction) {
    custom::FlatSet<Tracker> s;
    s.reserve(10);
    EXPECT_EQ(s.capacity(), 10);
    EXPECT_EQ(Tracker::live_objects, 0);
}

TEST_F(FlatSetResourceTest, DeepCopySemantics) {
    custom::FlatSet<Tracker> original;
    original.insert(Tracker(42));

    custom::FlatSet<Tracker> copy(original);
    EXPECT_EQ(copy.size(), 1);
    EXPECT_EQ(Tracker::live_objects, 2);
}

TEST_F(FlatSetResourceTest, MoveSemanticsTransferOwnership) {
    custom::FlatSet<Tracker> src;
    src.insert(Tracker(100));

    int copies_before = Tracker::copy_count;
    custom::FlatSet<Tracker> dst(std::move(src));
    EXPECT_EQ(dst.size(), 1);
    EXPECT_EQ(src.size(), 0);
    EXPECT_EQ(Tracker::copy_count, copies_before);
}

// --- Тесты итераторов (Этап 3) ---

TEST(FlatSetIteratorTest, TraitsAndCategory) {
    using Iter = custom::FlatSet<int>::iterator;
    using Traits = std::iterator_traits<Iter>;

    static_assert(std::is_same_v<Traits::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<Traits::value_type, int>);
    static_assert(std::is_same_v<Traits::reference, const int&>);
}

TEST(FlatSetIteratorTest, STLAlgorithmsCompatibility) {
    custom::FlatSet<int> s{1, 2, 3, 4, 5};
    EXPECT_EQ(std::distance(s.begin(), s.end()), 5);

    int sum = std::accumulate(s.begin(), s.end(), 0);
    EXPECT_EQ(sum, 15);
}

// --- Тесты операций множества (Этап 4) ---

TEST(FlatSetOperationsTest, InitializerListAndUniqueness) {
    // Вставка с дубликатами: 1 повторяется дважды
    custom::FlatSet<int> s{3, 1, 4, 1, 5};
    EXPECT_EQ(s.size(), 4);

    // Должны быть строго отсортированы
    std::vector<int> expected{1, 3, 4, 5};
    std::vector<int> actual(s.begin(), s.end());
    EXPECT_EQ(actual, expected);
}

TEST(FlatSetOperationsTest, InsertMaintainsSortAndRejectsDuplicates) {
    custom::FlatSet<int> s;
    auto [it1, ok1] = s.insert(20);
    EXPECT_TRUE(ok1);
    EXPECT_EQ(*it1, 20);

    auto [it2, ok2] = s.insert(10);
    EXPECT_TRUE(ok2);
    EXPECT_EQ(*it2, 10);

    auto [it3, ok3] = s.insert(30);
    EXPECT_TRUE(ok3);
    EXPECT_EQ(*it3, 30);

    // Повторная вставка
    auto [it_dup, ok_dup] = s.insert(20);
    EXPECT_FALSE(ok_dup);
    EXPECT_EQ(*it_dup, 20);
    EXPECT_EQ(s.size(), 3);

    std::vector<int> expected{10, 20, 30};
    std::vector<int> actual(s.begin(), s.end());
    EXPECT_EQ(actual, expected);
}

TEST(FlatSetOperationsTest, FindAndContains) {
    custom::FlatSet<int> s{10, 20, 30, 40};

    EXPECT_TRUE(s.contains(20));
    EXPECT_FALSE(s.contains(99));

    auto it = s.find(30);
    ASSERT_NE(it, s.end());
    EXPECT_EQ(*it, 30);

    EXPECT_EQ(s.find(55), s.end());
}

TEST(FlatSetOperationsTest, EraseByKeyAndIterator) {
    custom::FlatSet<int> s{1, 2, 3, 4, 5};

    // Удаление по существующему ключу
    EXPECT_EQ(s.erase(3), 1);
    EXPECT_FALSE(s.contains(3));
    EXPECT_EQ(s.size(), 4);

    // Удаление по несуществующему ключу
    EXPECT_EQ(s.erase(42), 0);
    EXPECT_EQ(s.size(), 4);

    // Удаление по итератору (первый элемент)
    auto it = s.erase(s.begin());
    EXPECT_EQ(*it, 2);
    EXPECT_FALSE(s.contains(1));
    EXPECT_EQ(s.size(), 3);

    // Удаление диапазона
    s.erase(s.begin(), s.end());
    EXPECT_TRUE(s.empty());
}

TEST(FlatSetOperationsTest, MergeSortedSets) {
    custom::FlatSet<int> s1{1, 3, 5};
    custom::FlatSet<int> s2{2, 3, 6};

    s1.merge(s2);

    // s1 должно объединить уникальные элементы
    std::vector<int> expected_s1{1, 2, 3, 5, 6};
    std::vector<int> actual_s1(s1.begin(), s1.end());
    EXPECT_EQ(actual_s1, expected_s1);

    // В s2 должен остаться только дубликат 3
    std::vector<int> expected_s2{3};
    std::vector<int> actual_s2(s2.begin(), s2.end());
    EXPECT_EQ(actual_s2, expected_s2);
}