#include "simple/simple.h"

#include <gtest/gtest.h>

namespace {

TEST(SimpleTest, AddReturnsSum) { EXPECT_EQ(simple::Add(2, 3), 5); }

TEST(SimpleTest, AddHandlesNegativeNumbers) {
  EXPECT_EQ(simple::Add(-2, -3), -5);
  EXPECT_EQ(simple::Add(-2, 3), 1);
}

}  // namespace
