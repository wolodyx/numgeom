#ifndef NUMGEOM_CORE_ITERATORIMPL_H
#define NUMGEOM_CORE_ITERATORIMPL_H

#include <cassert>

#include "numgeom/iterator.h"

template<typename IteratorType>
class IteratorImpl_StdIterator : public IteratorImpl<
    typename std::iterator_traits<IteratorType>::value_type> {
 public:
  typedef typename std::iterator_traits<IteratorType>::value_type value_type;

 public:
  IteratorImpl_StdIterator(const IteratorType& itBeg,
                           const IteratorType& itEnd)
    : it_(itBeg), it_end_(itEnd) {

  }
  virtual ~IteratorImpl_StdIterator() {}

  void advance() override {
    assert(it_ != it_end_);
    ++it_;
  }

  value_type current() const override {
    assert(it_ != it_end_);
    return *it_;
  }

  IteratorImpl_StdIterator<IteratorType>* clone() const override {
    return new IteratorImpl_StdIterator(it_, it_end_);
  }

  IteratorImpl_StdIterator<IteratorType>* last() const override {
    return new IteratorImpl_StdIterator(it_end_, it_end_);
  }

  bool end() const override { return it_ == it_end_; }

  bool equals(const IteratorImpl<value_type>& other) const override {
    auto ptr = dynamic_cast<const IteratorImpl_StdIterator<IteratorType>*>(&other);
    if(!ptr)
      return false;
    return it_ == ptr->it_;
  }

 private:
  IteratorType it_, it_end_;
};
#endif // !NUMGEOM_CORE_ITERATORIMPL_H
