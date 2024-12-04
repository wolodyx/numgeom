#ifndef numgeom_core_iterator_h
#define numgeom_core_iterator_h

#include <cassert>
#include <iterator>
#include <set>
#include <cstddef>


template<typename T>
class IteratorImpl
{
public:
    virtual ~IteratorImpl() {}
    virtual void advance() = 0;
    virtual T current() const = 0;
    virtual IteratorImpl<T>* clone() const = 0;
    virtual IteratorImpl<T>* last() const = 0;
    virtual bool end() const = 0;
    virtual bool equals(const IteratorImpl<T>&) const = 0;
};


template<typename T>
class Iterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type   = ptrdiff_t;
    using pointer           = T;
    using reference = T;

public:

    Iterator()
        : m_impl(nullptr)
    {}

    Iterator(IteratorImpl<T>* impl)
        : m_impl(impl)
    {}

    ~Iterator()
    {
        delete m_impl;
    }

    Iterator(const Iterator& other)
    {
        m_impl = nullptr;
        if(other.m_impl)
            m_impl = other.m_impl->clone();
    }

    Iterator& operator++()
    {
        m_impl->advance();
        return *this;
    }

    Iterator operator++(int)
    {
        auto clone = m_impl->clone();
        m_impl->advance();
        return Iterator<T>(clone);
    }

    bool operator==(const Iterator& other) const
    {
        return m_impl == other.m_impl
            || m_impl->equals(*other.m_impl);
    }

    bool operator!=(const Iterator& other) const
    {
        return !(*this == other);
    }

    reference operator*() const
    {
        return m_impl->current();
    }

    Iterator& operator=(const Iterator& other)
    {
        if(this != &other)
        {
            delete m_impl;
            m_impl = other.m_impl->clone();
        }
        return *this;
    }

    bool isEnd() const
    {
        return !m_impl || m_impl->end();
    }

    Iterator begin() const
    {
        return *this;
    }

    Iterator end() const
    {
        if(!m_impl)
            return Iterator();
        return Iterator(m_impl->last());
    }

private:
    IteratorImpl<T>* m_impl;
};


//! Количество элементов, которые пройдет итератор.
template<typename IT>
size_t Count(const IT& it)
{
    size_t n = 0;
    for(auto it2 = it; !it2.isEnd(); ++it2)
        ++n;
    return n;
}


template<typename IT>
bool Has(typename IT::value_type v, const IT& it)
{
    for(IT it2 = it; !it2.isEnd(); ++it2)
    {
        if(*it2 == v)
            return true;
    }
    return false;
}


template<typename IT>
std::set<typename IT::value_type> ToSet(const IT& it)
{
    std::set<typename IT::value_type> set;
    for(IT it2 = it; !it2.isEnd(); ++it2)
        set.insert(*it2);
    return set;
}

#endif // !numgeom_core_iterator_h
