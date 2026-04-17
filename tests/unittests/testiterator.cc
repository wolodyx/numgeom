#include "gtest/gtest.h"

#include "numgeom/iterator.h"
#include "numgeom/iteratorimpl.hpp"

TEST(Iterator, AssignEmptyObject) {
  Iterator<int> it;
  ASSERT_TRUE(it.isEmpty());
  it = Iterator<int>();
}

TEST(Iterator, EqualityComparision) {
  Iterator<int> it;
  Iterator<int> it2 = it;
  ASSERT_TRUE(it2 == it);
  Iterator<int> it3 = new IteratorImpl_OneValue<int>(42);
  ASSERT_FALSE(it == it3);
}
