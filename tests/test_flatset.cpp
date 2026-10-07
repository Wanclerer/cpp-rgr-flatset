#include <flatset/FlatSet.hpp>
#include <gtest/gtest.h>

namespace custom {
class FlatSetTestPeer {
public:
    template <typename Set, typename T>
    static void push_back(Set& s, const T& val) {
        s.push_back_unchecked(val);
    }
};
}  // namespace custom

// Класс-шпион для проверки вызовов конструкторов, перемещений и деструкторов
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
        // Гарантия: после каждого теста все созданные объекты обязаны быть удалены
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
    // Выделена сырая память под 10 элементов, но ни один объект не сконструирован
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

    // Move constructor
    custom::FlatSet<Tracker> dst(std::move(src));
    EXPECT_EQ(dst.size(), 1);
    EXPECT_EQ(src.size(), 0);
    EXPECT_EQ(src.capacity(), 0);
    EXPECT_EQ(Tracker::copy_count, copies_before);  // Ноль лишних копирований

    // Move assignment
    custom::FlatSet<Tracker> dst2;
    dst2 = std::move(dst);
    EXPECT_EQ(dst2.size(), 1);
    EXPECT_EQ(dst.size(), 0);
    EXPECT_EQ(Tracker::copy_count, copies_before);
}