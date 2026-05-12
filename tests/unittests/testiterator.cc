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

TEST(Iterator, ConvertPointerToConstPointer) {
  typedef std::list<int*> StdContainerType;
  int values[3] = {1, 2, 3};
  StdContainerType v = {&values[0], &values[1], &values[2]};
  auto impl = new IteratorImpl_StdIterator<StdContainerType::const_iterator>(
      v.begin(), v.end());
  Iterator<const int*> it(impl);
  ASSERT_EQ(*it, &values[0]); ++it;
  ASSERT_EQ(*it, &values[1]); ++it;
  ASSERT_EQ(*it, &values[2]); ++it;
  ASSERT_TRUE(it.isEnd());
}
