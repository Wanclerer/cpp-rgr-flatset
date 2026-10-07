#include <flatset/FlatSet.hpp>
#include <gtest/gtest.h>

TEST(FlatSetInitTest, BasicEmptyCheck) {
    custom::FlatSet<int> set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);
}