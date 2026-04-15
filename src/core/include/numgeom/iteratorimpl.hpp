#ifndef NUMGEOM_CORE_ITERATORIMPL_HPP
#define NUMGEOM_CORE_ITERATORIMPL_HPP

#include "numgeom/iteratorimpl.h"

template<typename IteratorType>
IteratorImpl_StdIterator<IteratorType>::IteratorImpl_StdIterator(
    const IteratorType& itBeg,
    const IteratorType& itEnd)
: it_(itBeg), it_end_(itEnd) {
}

template<typename IteratorType>
IteratorImpl_StdIterator<IteratorType>::~IteratorImpl_StdIterator() {}

template<typename IteratorType>
void IteratorImpl_StdIterator<IteratorType>::advance() {
  assert(it_ != it_end_);
  ++it_;
}

template<typename IteratorType>
IteratorImpl_StdIterator<IteratorType>::value_type
IteratorImpl_StdIterator<IteratorType>::current() const {
  assert(it_ != it_end_);
  return *it_;
}

template<typename IteratorType>
IteratorImpl_StdIterator<IteratorType>*
IteratorImpl_StdIterator<IteratorType>::clone() const {
  return new IteratorImpl_StdIterator(it_, it_end_);
}

template<typename IteratorType>
IteratorImpl_StdIterator<IteratorType>*
IteratorImpl_StdIterator<IteratorType>::last() const {
  return new IteratorImpl_StdIterator(it_end_, it_end_);
}

template<typename IteratorType>
bool IteratorImpl_StdIterator<IteratorType>::end() const {
  return it_ == it_end_;
}

template<typename IteratorType>
bool IteratorImpl_StdIterator<IteratorType>::equals(
    const IteratorImpl<value_type>& other) const {
  auto ptr = dynamic_cast<const IteratorImpl_StdIterator<IteratorType>*>(&other);
  if(!ptr)
    return false;
  return it_ == ptr->it_;
}

template<typename KeyType, typename ValueType>
IteratorImpl_StdMapValue<KeyType,ValueType>::IteratorImpl_StdMapValue(
    const IteratorImpl_StdMapValue<KeyType,ValueType>::map_const_iterator& itBeg,
    const IteratorImpl_StdMapValue<KeyType,ValueType>::map_const_iterator& itEnd)
    : m_it(itBeg), m_itEnd(itEnd) {
}

template<typename KeyType, typename ValueType>
IteratorImpl_StdMapValue<KeyType,ValueType>::~IteratorImpl_StdMapValue() {
}

template<typename KeyType, typename ValueType>
void IteratorImpl_StdMapValue<KeyType,ValueType>::advance() {
  assert(m_it != m_itEnd);
  ++m_it;
}

template<typename KeyType, typename ValueType>
IteratorImpl_StdMapValue<KeyType,ValueType>::value_type
IteratorImpl_StdMapValue<KeyType,ValueType>::current() const {
  return m_it->second;
}

template<typename KeyType, typename ValueType>
IteratorImpl<typename IteratorImpl_StdMapValue<KeyType,ValueType>::value_type>*
IteratorImpl_StdMapValue<KeyType,ValueType>::clone() const {
  return new IteratorImpl_StdMapValue(m_it, m_itEnd);
}

template<typename KeyType, typename ValueType>
IteratorImpl<typename IteratorImpl_StdMapValue<KeyType,ValueType>::value_type>*
IteratorImpl_StdMapValue<KeyType,ValueType>::last() const {
  return new IteratorImpl_StdMapValue(m_itEnd, m_itEnd);
}

template<typename KeyType, typename ValueType>
bool IteratorImpl_StdMapValue<KeyType,ValueType>::end() const {
  return m_it == m_itEnd;
}

template<typename KeyType, typename ValueType>
bool IteratorImpl_StdMapValue<KeyType,ValueType>::equals(
    const IteratorImpl<value_type>& other) const {
  auto pOther = dynamic_cast<const IteratorImpl_StdMapValue*>(&other);
  if(!pOther)
    return false;
  return m_it == pOther->m_it;
}

template<typename BaseItValueType, typename Filter, typename Transform>
IteratorImpl_Filter<BaseItValueType,Filter,Transform>::IteratorImpl_Filter(
    BaseIteratorType* baseIt) : m_baseIt(baseIt) {
  while(!m_baseIt->end() && !Filter()(m_baseIt->current()))
        m_baseIt->advance();
}

template<typename BaseItValueType, typename Filter, typename Transform>
IteratorImpl_Filter<BaseItValueType,Filter,Transform>::~IteratorImpl_Filter() {
}

template<typename BaseItValueType, typename Filter, typename Transform>
void IteratorImpl_Filter<BaseItValueType,Filter,Transform>::advance() {
  assert(!this->end());
  m_baseIt->advance();
  while(!m_baseIt->end() && !Filter()(m_baseIt->current()))
        m_baseIt->advance();
}

template<typename BaseItValueType, typename Filter, typename Transform>
IteratorImpl_Filter<BaseItValueType,Filter,Transform>::value_type
IteratorImpl_Filter<BaseItValueType,Filter,Transform>::current() const {
  assert(!this->end());
  return Transform()(m_baseIt->current());
}

template<typename BaseItValueType, typename Filter, typename Transform>
IteratorImpl<typename IteratorImpl_Filter<BaseItValueType,Filter,Transform>::value_type>*
IteratorImpl_Filter<BaseItValueType,Filter,Transform>::clone() const {
  return new IteratorImpl_Filter(m_baseIt->clone());
}

template<typename BaseItValueType, typename Filter, typename Transform>
IteratorImpl<typename IteratorImpl_Filter<BaseItValueType,Filter,Transform>::value_type>*
IteratorImpl_Filter<BaseItValueType,Filter,Transform>::last() const {
  return new IteratorImpl_Filter(m_baseIt->last());
}

template<typename BaseItValueType, typename Filter, typename Transform>
bool IteratorImpl_Filter<BaseItValueType,Filter,Transform>::end() const {
  return m_baseIt->end();
}

template<typename BaseItValueType, typename Filter, typename Transform>
bool IteratorImpl_Filter<BaseItValueType,Filter,Transform>::equals(
    const IteratorImpl<value_type>& other) const {
  auto pOther = dynamic_cast<const IteratorImpl_Filter<BaseItValueType,Filter,Transform>*>(&other);
  if(!pOther)
    return false;
  return m_baseIt->equals(*pOther->m_baseIt);
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::IteratorImpl_Transform(
    IteratorImpl<InputValueType>* it,
    const Transformer& tr) : tr_(tr), it_(it) {
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::~IteratorImpl_Transform() {
  delete it_;
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
void IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::advance() {
  ++it_;
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
typename IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::value_type
IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::current() const {
  return tr_(it_->current());
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
IteratorImpl<typename IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::value_type>*
IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::clone() const {
  return new IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>(it_);
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
IteratorImpl<typename IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::value_type>*
IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::last() const {
  return new IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>(it_->last());
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
bool IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::end() const {
  return it_->end();
}

template<typename InputValueType, typename OutputValueType, typename Transformer>
bool IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>::equals(
    const IteratorImpl<value_type>& other) const {
  auto ptr = dynamic_cast<const IteratorImpl_Transform<InputValueType,OutputValueType,Transformer>*>(&other);
  if (!ptr)
    return false;
  return ptr->it_ == it_;
}

#endif // !NUMGEOM_CORE_ITERATORIMPL_HPP
