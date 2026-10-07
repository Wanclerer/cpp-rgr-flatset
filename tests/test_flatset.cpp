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

// --- Тестовая структура для проверки RAII ---
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
    EXPECT_EQ(s.capacity(), 0);
}

TEST_F(FlatSetResourceTest, ReserveSeparatesAllocationAndConstruction) {
    custom::FlatSet<Tracker> s;
    s.reserve(10);
    EXPECT_EQ(s.capacity(), 10);
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(Tracker::live_objects, 0);
}

TEST_F(FlatSetResourceTest, DestructionCleansUpObjects) {
    {
        custom::FlatSet<Tracker> s;
        custom::FlatSetTestPeer::push_back(s, Tracker(1));
        custom::FlatSetTestPeer::push_back(s, Tracker(2));
        EXPECT_EQ(s.size(), 2);
        EXPECT_EQ(Tracker::live_objects, 2);
    }
    EXPECT_EQ(Tracker::live_objects, 0);
}

TEST_F(FlatSetResourceTest, DeepCopySemantics) {
    custom::FlatSet<Tracker> original;
    custom::FlatSetTestPeer::push_back(original, Tracker(42));

    custom::FlatSet<Tracker> copy(original);
    EXPECT_EQ(copy.size(), 1);
    EXPECT_EQ(Tracker::live_objects, 2);

    custom::FlatSet<Tracker> assigned;
    assigned = copy;
    EXPECT_EQ(assigned.size(), 1);
    EXPECT_EQ(Tracker::live_objects, 3);
}

TEST_F(FlatSetResourceTest, MoveSemanticsTransferOwnership) {
    custom::FlatSet<Tracker> src;
    custom::FlatSetTestPeer::push_back(src, Tracker(100));
    EXPECT_EQ(Tracker::live_objects, 1);

    int copies_before = Tracker::copy_count;

    custom::FlatSet<Tracker> dst(std::move(src));
    EXPECT_EQ(dst.size(), 1);
    EXPECT_EQ(src.size(), 0);
    EXPECT_EQ(src.capacity(), 0);
    EXPECT_EQ(Tracker::copy_count, copies_before);

    custom::FlatSet<Tracker> dst2;
    dst2 = std::move(dst);
    EXPECT_EQ(dst2.size(), 1);
    EXPECT_EQ(dst.size(), 0);
    EXPECT_EQ(Tracker::copy_count, copies_before);
}

// --- Тесты итераторов (Этап 3) ---

TEST(FlatSetIteratorTest, TraitsAndCategory) {
    using Set = custom::FlatSet<int>;
    using Iter = Set::iterator;
    using Traits = std::iterator_traits<Iter>;

    static_assert(std::is_same_v<Traits::iterator_category, std::random_access_iterator_tag>,
                  "Iterator must be Random Access");
    static_assert(std::is_same_v<Traits::value_type, int>, "Value type mismatch");
    static_assert(std::is_same_v<Traits::reference, const int&>, "Iterator must yield const reference");
}

TEST(FlatSetIteratorTest, ArithmeticAndIndexing) {
    custom::FlatSet<int> s;
    for (int i = 10; i <= 50; i += 10) {
        custom::FlatSetTestPeer::push_back(s, i);  // 10, 20, 30, 40, 50
    }

    auto it = s.begin();
    EXPECT_EQ(*it, 10);
    EXPECT_EQ(it[2], 30);

    it += 3;
    EXPECT_EQ(*it, 40);

    it -= 2;
    EXPECT_EQ(*it, 20);

    auto it2 = it + 2;
    EXPECT_EQ(*it2, 40);
    EXPECT_EQ(it2 - it, 2);
    EXPECT_TRUE(it < it2);
}

TEST(FlatSetIteratorTest, STLAlgorithmsCompatibility) {
    custom::FlatSet<int> s;
    for (int i = 1; i <= 5; ++i) {
        custom::FlatSetTestPeer::push_back(s, i);  // 1, 2, 3, 4, 5
    }

    // std::distance
    EXPECT_EQ(std::distance(s.begin(), s.end()), 5);

    // Range-based for
    int sum = 0;
    for (int val : s) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);

    // std::accumulate
    int acc = std::accumulate(s.begin(), s.end(), 0);
    EXPECT_EQ(acc, 15);
}

TEST(FlatSetIteratorTest, ReverseIterators) {
    custom::FlatSet<int> s;
    for (int i = 1; i <= 3; ++i) {
        custom::FlatSetTestPeer::push_back(s, i);  // 1, 2, 3
    }

    std::vector<int> reversed;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        reversed.push_back(*it);
    }

    ASSERT_EQ(reversed.size(), 3);
    EXPECT_EQ(reversed[0], 3);
    EXPECT_EQ(reversed[1], 2);
    EXPECT_EQ(reversed[2], 1);
}