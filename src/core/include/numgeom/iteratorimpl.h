#ifndef NUMGEOM_CORE_ITERATORIMPL_H
#define NUMGEOM_CORE_ITERATORIMPL_H

#include <cassert>
#include <map>
#include <memory>

#include "numgeom/iterator.h"

template<typename IteratorType>
class IteratorImpl_StdIterator : public IteratorImpl<
    typename std::iterator_traits<IteratorType>::value_type> {
 public:
  typedef typename std::iterator_traits<IteratorType>::value_type value_type;

 public:
  IteratorImpl_StdIterator(const IteratorType& itBeg,
                           const IteratorType& itEnd);
  virtual ~IteratorImpl_StdIterator();
  void advance() override;
  value_type current() const override;
  IteratorImpl_StdIterator<IteratorType>* clone() const override;
  IteratorImpl_StdIterator<IteratorType>* last() const override;
  bool end() const override;
  bool equals(const IteratorImpl<value_type>& other) const override;

 private:
  IteratorType it_, it_end_;
};

template<typename KeyType, typename ValueType>
class IteratorImpl_StdMapValue : public IteratorImpl<ValueType>
{
public:
  typedef typename std::map<KeyType,ValueType>::const_iterator map_const_iterator;
  typedef ValueType value_type;

public:
  IteratorImpl_StdMapValue(
      const map_const_iterator& itBeg,
      const map_const_iterator& itEnd);
  virtual ~IteratorImpl_StdMapValue();
  void advance() override;
  value_type current() const override;
  IteratorImpl<value_type>* clone() const override;
  IteratorImpl<value_type>* last() const override;
  bool end() const override;
  bool equals(const IteratorImpl<value_type>& other) const override;

private:
  map_const_iterator m_it, m_itEnd;
};

template<typename T>
struct NonTransformFunctor {
  typedef T out_value_type;
  typedef T in_value_type;
  T operator()(const T& val) const { return val; }
};

/**
\class IteratorImpl_Filter
\brief Фильтрующий итератор с выходным преобразованием значений.
\tparam ItType Тип базового итератора.
\tparam Filter Класс фильтра с действующим методом
        `bool operator()(base_value_type)`, которая возвращает true для пропуска
        значений с базового итератора.
\tparam Transform Класс преобразователя с действующим методом
        `out_value_type operator()(in_value_type)`,
        которая преобразует значение (также возможно преобразование типа
        значения).
*/
template<typename BaseItValueType,
         typename Filter,
         typename Transform=NonTransformFunctor<BaseItValueType>>
class IteratorImpl_Filter : public IteratorImpl<typename Transform::out_value_type>
{
public:
  typedef BaseItValueType base_value_type;
  typedef IteratorImpl<base_value_type> BaseIteratorType;
  typedef typename Transform::out_value_type value_type;

public:
  IteratorImpl_Filter(BaseIteratorType* baseIt);
  virtual ~IteratorImpl_Filter();
  void advance() override;
  value_type current() const override;
  IteratorImpl<value_type>* clone() const override;
  IteratorImpl<value_type>* last() const override;
  bool end() const override;
  bool equals(const IteratorImpl<value_type>& other) const override;

private:
  std::unique_ptr<BaseIteratorType> m_baseIt;
};

/**
\class IteratorImpl_Transform 
\brief Итератор, преобразующий элементы.
*/
template<typename InputValueType, typename OutputValueType, typename Transformer>
class IteratorImpl_Transform : public IteratorImpl<OutputValueType> {
 public:
  typedef OutputValueType value_type;

 public:
  IteratorImpl_Transform(IteratorImpl<InputValueType>*,
                         const Transformer& tr = Transformer());
  virtual ~IteratorImpl_Transform();
  void advance() override;
  value_type current() const override;
  IteratorImpl<value_type>* clone() const override;
  IteratorImpl<value_type>* last() const override;
  bool end() const override;
  bool equals(const IteratorImpl<value_type>&) const override;

 private:
  IteratorImpl<InputValueType>* it_;
  Transformer tr_;
};

template<typename T>
class IteratorImpl_ByEnum : public IteratorImpl<T>
{
public:

  IteratorImpl_ByEnum(const std::vector<T>& values) {
    m_values = std::make_shared<std::vector<T>>(values);
    m_it = m_values->begin();
  }
  IteratorImpl_ByEnum(const std::shared_ptr<std::vector<T>> values,
                      const typename std::vector<T>::const_iterator& it) {
    m_values = values;
    m_it = it;
  }
  virtual ~IteratorImpl_ByEnum() {}
  void advance() override {
    assert(m_it != m_values->end());
    ++m_it;
  }
  T current() const override {
    return (*m_it);
  }
  IteratorImpl<T>* clone() const override {
    return new IteratorImpl_ByEnum<T>(m_values, m_it);
  }
  IteratorImpl<T>* last() const override {
    return new IteratorImpl_ByEnum<T>(m_values, m_values->end());
  }
  bool end() const override {
    return m_it == m_values->end();
  }
  bool equals(const IteratorImpl<T>& other) const override {
    auto pOther = dynamic_cast<const IteratorImpl_ByEnum<T>*>(&other);
    if(!pOther)
      return false;
    return m_it == pOther->m_it;
  }

private:
  std::shared_ptr<std::vector<T>> m_values;
  typename std::vector<T>::const_iterator m_it;
};


template<typename T>
class IteratorImpl_OneValue : public IteratorImpl<T>
{
public:

  IteratorImpl_OneValue(const T& value, bool begin = true) {
    m_value = value;
    m_begin = begin;
  }
  virtual ~IteratorImpl_OneValue() {}
  void advance() override {
    assert(m_begin);
    m_begin = false;
  }
  T current() const override {
    assert(m_begin);
    return m_value;
  }
  IteratorImpl<T>* clone() const override {
    return new IteratorImpl_OneValue<T>(m_value, m_begin);
  }
  IteratorImpl<T>* last() const override {
    return new IteratorImpl_OneValue<T>(m_value, false);
  }
  bool end() const override {
    return !m_begin;
  }
  bool equals(const IteratorImpl<T>& other) const override {
    auto pOther = dynamic_cast<const IteratorImpl_OneValue<T>*>(&other);
    if(!pOther)
      return false;
    return m_value == pOther->m_value && m_begin == pOther->m_begin;
  }

private:
  T m_value;
  bool m_begin;
};

template<typename T>
class IteratorImpl_ByStaticPointer : public IteratorImpl<T>
{
public:

  IteratorImpl_ByStaticPointer(const T* itBegin, const T* itEnd) {
    m_it = itBegin;
    m_itEnd = itEnd;
  }
  virtual ~IteratorImpl_ByStaticPointer() {}
  void advance() override {
    assert(m_it != m_itEnd);
    ++m_it;
  }
  T current() const override {
    assert(m_it != m_itEnd);
    return (*m_it);
  }
  IteratorImpl<T>* clone() const override {
    return new IteratorImpl_ByStaticPointer<T>(m_it, m_itEnd);
  }
  IteratorImpl<T>* last() const override {
    return new IteratorImpl_ByStaticPointer<T>(m_itEnd, m_itEnd);
  }
  bool end() const override {
    return m_it == m_itEnd;
  }
  bool equals(const IteratorImpl<T>& other) const override {
    auto pOther = dynamic_cast<const IteratorImpl_ByStaticPointer<T>*>(&other);
    if(!pOther)
      return false;
    return m_it == pOther->m_it;
  }

private:
  const T* m_it;
  const T* m_itEnd;
};

template<
    typename ContType,
    typename ContElemType,
    size_t(ContType::*Size)() const,
    const ContElemType&(ContType::*Get)(size_t) const>
class IteratorImpl_ByIndex : public IteratorImpl<ContElemType>
{
public:
  IteratorImpl_ByIndex(const ContType* cont, size_t index = 0)
  {
    m_cont = cont;
    m_num = (cont->*Size)();
    m_index = index;
  }
  virtual ~IteratorImpl_ByIndex() {}
  virtual void advance() {
    assert(m_index != m_num);
    ++m_index;
  }
  virtual ContElemType current() const
  {
    assert(m_index < m_num);
    return (m_cont->*Get)(m_index);
  }
  virtual IteratorImpl<ContElemType>* clone() const
  {
    return new IteratorImpl_ByIndex(m_cont, m_index);
  }
  virtual IteratorImpl<ContElemType>* last() const
  {
    return new IteratorImpl_ByIndex(m_cont, m_num);
  }
  virtual bool end() const
  {
    return m_index == m_num;
  }
  virtual bool equals(const IteratorImpl<ContElemType>& other) const
  {
    auto pOther = dynamic_cast<
        const IteratorImpl_ByIndex<ContType,ContElemType,Size,Get>*>(&other);
    if(!pOther)
      return false;
    return m_cont == pOther->m_cont && m_index == pOther->m_index;
  }

private:
  const ContType* m_cont;
  size_t m_num;
  size_t m_index;
};
#endif // !NUMGEOM_CORE_ITERATORIMPL_H
