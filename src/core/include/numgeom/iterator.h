#ifndef NUMGEOM_CORE_ITERATOR_H
#define NUMGEOM_CORE_ITERATOR_H

#include <cassert>
#include <cstddef>
#include <iterator>

template <typename T>
class IteratorImpl {
 public:
  virtual ~IteratorImpl() {}
  virtual void advance() = 0;
  virtual T current() const = 0;
  virtual IteratorImpl<T>* clone() const = 0;
  virtual IteratorImpl<T>* last() const = 0;
  virtual bool end() const = 0;
  virtual bool equals(const IteratorImpl<T>&) const = 0;
};

/** \class IteratorImpl_Convert
\brief Converts `IteratorImpl<From>` to `IteratorImpl<To>` where `From` is
convertible to `To`. This enables const-correctness: `Iterator<const T*>` can
wrap `IteratorImpl<T*>`.
*/
template<typename From, typename To>
class IteratorImpl_Convert : public IteratorImpl<To> {
 public:
  typedef From from_value_type;
  typedef To to_value_type;

  explicit IteratorImpl_Convert(IteratorImpl<From>* impl) : impl_(impl) {}

  virtual ~IteratorImpl_Convert() { delete impl_; }

  void advance() override { impl_->advance(); }

  to_value_type current() const override {
    return static_cast<to_value_type>(impl_->current());
  }

  IteratorImpl<to_value_type>* clone() const override {
    return new IteratorImpl_Convert<From, To>(impl_->clone());
  }

  IteratorImpl<to_value_type>* last() const override {
    return new IteratorImpl_Convert<From, To>(impl_->last());
  }

  bool end() const override { return impl_->end(); }

  bool equals(const IteratorImpl<to_value_type>& other) const override {
    auto ptr = dynamic_cast<const IteratorImpl_Convert<From, To>*>(&other);
    if (!ptr) return false;
    return impl_->equals(*ptr->impl_);
  }

 private:
  IteratorImpl<From>* impl_;
};

template <typename T>
class Iterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using difference_type = ptrdiff_t;
  using pointer = T;
  using reference = T;

 public:
  Iterator() : impl_(nullptr) {}

  Iterator(IteratorImpl<T>* impl) : impl_(impl) {}

  // Converting constructor: allows `Iterator<To>` to be constructed
  // from `IteratorImpl<From>*` where `From` is convertible to `To`.
  template<typename U>
  Iterator(IteratorImpl<U>* impl,
           typename std::enable_if<std::is_convertible<U,T>::value>::type* = nullptr)
      : impl_(new IteratorImpl_Convert<U, T>(impl)) {}

  ~Iterator() { delete impl_; }

  Iterator(const Iterator& other) {
    impl_ = nullptr;
    if (other.impl_) impl_ = other.impl_->clone();
  }

  Iterator& operator++() {
    impl_->advance();
    return *this;
  }

  Iterator operator++(int) {
    auto clone = impl_->clone();
    impl_->advance();
    return Iterator<T>(clone);
  }

  bool operator==(const Iterator& other) const {
    if (impl_ == other.impl_)
      return true;
    if(!impl_ || !other.impl_)
      return false;
    return impl_->equals(*other.impl_);
  }

  bool operator!=(const Iterator& other) const { return !(*this == other); }

  reference operator*() const { return impl_->current(); }

  Iterator& operator=(const Iterator& other) {
    if (this != &other) {
      delete impl_;
      if(other.impl_ != nullptr)
        impl_ = other.impl_->clone();
    }
    return *this;
  }

  bool isEnd() const { return !impl_ || impl_->end(); }

  bool isEmpty() const { return !impl_; }

  Iterator begin() const { return *this; }

  Iterator end() const {
    if (!impl_) return Iterator();
    return Iterator(impl_->last());
  }

 private:
  IteratorImpl<T>* impl_;
};
#endif // !NUMGEOM_CORE_ITERATOR_H
