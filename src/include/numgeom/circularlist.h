#ifndef numgeom_numgeom_circularlist_h
#define numgeom_numgeom_circularlist_h

#include "numgeom/numgeom_export.h"


template<typename T>
class NUMGEOM_EXPORT CircularList
{
public:

    class const_iterator;
    class iterator;

    struct Node
    {
        T data;
        Node* next;
        Node* prev;
    };

public:

    static const Node* Offset(const Node*, int);


public:

    CircularList();
    CircularList(const CircularList&);
    //CircularList(const std::vector<T>&);

    ~CircularList();

    bool empty() const;

    const_iterator begin() const;
    const_iterator end() const;

    iterator begin();
    iterator end();

    const T& front() const;

    const T& back() const;

    const Node* firstNode() const;

    size_t size() const;

    CircularList& operator=(const CircularList<T>&);

    void push_back(const T&);

    void clear();

    template<typename Iter>
    void assign(const Iter& first, const Iter& last);

private:
    Node* myFirstNode;
};


template<typename T>
class CircularList<T>::const_iterator
{
public:

    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = T;
    using difference_type   = ptrdiff_t;
    using pointer           = const value_type*;
    using reference         = const value_type&;

public:

    const_iterator();

    const_iterator(const Node* firstNode, bool beginIterator);

    bool operator!() const;

    bool operator==(const const_iterator& other) const;

    bool operator!=(const const_iterator& other) const;

    const T& operator*() const;
    const T* operator->() const;

    const_iterator& operator++();
    const_iterator& operator--();

    const Node* current_node() const;

private:
    const Node* myFirstNode;
    const Node* myCurrentNode;
    bool myFirstInitialized;
};


template<typename T>
class CircularList<T>::iterator
{
public:

    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = T;
    using difference_type   = ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;

public:

    iterator();

    iterator(Node* firstNode, bool beginIterator);

    bool operator!() const;

    bool operator==(const iterator& other) const;

    bool operator!=(const iterator& other) const;

    T& operator*() const;
    T* operator->() const;

    iterator& operator++();
    iterator& operator--();

private:
    Node* myFirstNode;
    Node* myCurrentNode;
    bool myFirstInitialized;
};


template<typename T>
CircularList<T>::CircularList()
{
    myFirstNode = nullptr;
}


template<typename T>
CircularList<T>::CircularList(const CircularList& other)
{
    myFirstNode = nullptr;
    this->assign(other.begin(), other.end());
}


template<typename T>
CircularList<T>& CircularList<T>::operator=(const CircularList& other)
{
    this->assign(other.begin(), other.end());
}


template<typename T>
template<typename Iter>
void CircularList<T>::assign(const Iter& first, const Iter& last)
{
    this->clear();

    Node* head = nullptr;
    for(Iter it = first; it != last; ++it)
    {
        const T& data = (*it);

        auto node = new Node{data};
        if(!this->myFirstNode)
        {
            myFirstNode = node;
            myFirstNode->next = node;
            myFirstNode->prev = node;
            head = node;
        }
        else
        {
            node->next = head->next;
            node->prev = head;
            head->next->prev = node;
            head->next = node;
            head = node;
        }
    }
}


template<typename T>
CircularList<T>::~CircularList()
{
    this->clear();
}


template<typename T>
void CircularList<T>::clear()
{
    if(!myFirstNode)
        return;

    Node* nextNode = myFirstNode;
    do
    {
        Node* node = nextNode;
        nextNode = node->next;
        delete node;
    }
    while(nextNode != myFirstNode);
    myFirstNode = nullptr;
}


template<typename T>
void CircularList<T>::push_back(const T& data)
{
    Node* node = new Node{data};
    if(!myFirstNode)
    {
        myFirstNode = node;
        myFirstNode->next = node;
        myFirstNode->prev = node;
        return;
    }

    myFirstNode->prev->next = node;
    node->next = myFirstNode;
    node->prev = myFirstNode->prev;
    myFirstNode->prev = node;

}


template<typename T>
size_t CircularList<T>::size() const
{
    if(!myFirstNode)
        return 0;

    size_t sz = 1;
    Node* node = myFirstNode->next;
    while(node != myFirstNode)
    {
        ++sz;
        node = node->next;
    }
    return sz;
}


template<typename T>
bool CircularList<T>::empty() const
{
    return myFirstNode == nullptr;
}


template<typename T>
const T& CircularList<T>::front() const
{
    return myFirstNode->data;
}


template<typename T>
const T& CircularList<T>::back() const
{
    return myFirstNode->prev->data;
}


template<typename T>
const typename CircularList<T>::Node* CircularList<T>::firstNode() const
{
    return myFirstNode;
}


template<typename T>
typename CircularList<T>::const_iterator CircularList<T>::begin() const
{
    return const_iterator(myFirstNode, myFirstNode != nullptr);
}


template<typename T>
typename CircularList<T>::const_iterator CircularList<T>::end() const
{
    return const_iterator(myFirstNode, false);
}


template<typename T>
typename CircularList<T>::iterator CircularList<T>::begin()
{
    return iterator(myFirstNode, myFirstNode != nullptr);
}


template<typename T>
typename CircularList<T>::iterator CircularList<T>::end()
{
    return iterator(myFirstNode, false);
}


template<typename T>
CircularList<T>::const_iterator::const_iterator()
{
    myFirstNode = nullptr;
    myCurrentNode = nullptr;
    myFirstInitialized = true;
}


template<typename T>
CircularList<T>::const_iterator::const_iterator(
    const Node* firstNode,
    bool beginIterator
)
{
    myFirstNode = firstNode;
    myCurrentNode = firstNode;
    myFirstInitialized = beginIterator;
}


template<typename T>
bool CircularList<T>::const_iterator::operator!() const
{
    return myFirstNode == nullptr;
}


template<typename T>
bool CircularList<T>::const_iterator::operator==(
    const const_iterator& other
) const
{
    return myFirstNode == other.myFirstNode
        && myCurrentNode == other.myCurrentNode
        && myFirstInitialized == other.myFirstInitialized;
}


template<typename T>
bool CircularList<T>::const_iterator::operator!=(
    const const_iterator& other
) const
{
    return !this->operator==(other);
}


template<typename T>
const T& CircularList<T>::const_iterator::operator*() const
{
    return myCurrentNode->data;
}


template<typename T>
const T* CircularList<T>::const_iterator::operator->() const
{
    return &myCurrentNode->data;
}


template<typename T>
typename CircularList<T>::const_iterator& CircularList<T>::const_iterator::operator++()
{
    myCurrentNode = myCurrentNode->next;
    myFirstInitialized = false;
    return *this;
}


template<typename T>
typename CircularList<T>::const_iterator& CircularList<T>::const_iterator::operator--()
{
    myCurrentNode = myCurrentNode->prev;
    myFirstInitialized = false;
    return *this;
}


template<typename T>
const typename CircularList<T>::Node* CircularList<T>::const_iterator::current_node() const
{
    return myCurrentNode;
}


template<typename T>
CircularList<T>::iterator::iterator()
{
    myFirstNode = nullptr;
    myCurrentNode = nullptr;
    myFirstInitialized = true;
}


template<typename T>
CircularList<T>::iterator::iterator(
    Node* firstNode,
    bool beginIterator
)
{
    myFirstNode = firstNode;
    myCurrentNode = firstNode;
    myFirstInitialized = beginIterator;
}


template<typename T>
bool CircularList<T>::iterator::operator!() const
{
    return myFirstNode == nullptr;
}


template<typename T>
bool CircularList<T>::iterator::operator==(
    const iterator& other
) const
{
    return myFirstNode == other.myFirstNode
        && myCurrentNode == other.myCurrentNode
        && myFirstInitialized == other.myFirstInitialized;
}


template<typename T>
bool CircularList<T>::iterator::operator!=(
    const iterator& other
) const
{
    return !this->operator==(other);
}


template<typename T>
T& CircularList<T>::iterator::operator*() const
{
    return myCurrentNode->data;
}


template<typename T>
T* CircularList<T>::iterator::operator->() const
{
    return &myCurrentNode->data;
}


template<typename T>
typename CircularList<T>::iterator& CircularList<T>::iterator::operator++()
{
    myCurrentNode = myCurrentNode->next;
    myFirstInitialized = false;
    return *this;
}


template<typename T>
typename CircularList<T>::iterator& CircularList<T>::iterator::operator--()
{
    myCurrentNode = myCurrentNode->prev;
    myFirstInitialized = false;
    return *this;
}


template<typename T>
const typename CircularList<T>::Node* CircularList<T>::Offset(
    const Node* startNode,
    int offset
)
{
    if(!startNode)
        return nullptr;

    if(offset == 0)
        return startNode;

    const Node* node = startNode;
    if(offset > 0)
    {
        for(int i = 0; i < offset; ++i)
            node = node->next;
    }
    else
    {
        for(int i = 0; i > offset; --i)
            node = node->prev;
    }
    return node;
}

#endif // !numgeom_numgeom_circularlist_h
