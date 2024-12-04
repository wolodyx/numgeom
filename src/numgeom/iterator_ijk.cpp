#include "iterator_ijk.h"

#include <cassert>


Iterator_Ijk::Iterator_Ijk()
{
}


Iterator_Ijk::Iterator_Ijk(
    const Ijk& end,
    const Ijk& init,
    const Ijk& begin
)
{
    assert(begin <= end && init <= begin);
    myBegin = begin;
    myEnd = end;
    myCurrent = init;
}


Iterator_Ijk& Iterator_Ijk::operator++()
{
    ++myCurrent.i;
    if(myCurrent.i == myEnd.i)
    {
        myCurrent.i = myBegin.i;
        ++myCurrent.j;
        if(myCurrent.j == myEnd.j)
        {
            myCurrent.j = myBegin.j;
            ++myCurrent.k;
            if(myCurrent.k == myEnd.k)
                myCurrent = myEnd;
        }
    }
    return (*this);
}


Ijk Iterator_Ijk::operator*() const
{
    return myCurrent;
}


Standard_Boolean Iterator_Ijk::isEnd() const
{
    return myCurrent == myEnd;
}


Standard_Boolean Iterator_Ijk::operator==(
    const Iterator_Ijk& other
) const
{
    return myCurrent == other.myCurrent
        && myBegin == other.myBegin
        && myEnd == other.myEnd;
}


Standard_Boolean Iterator_Ijk::operator!=(
    const Iterator_Ijk& other
) const
{
    return !this->operator==(other);
}


void Iterator_Ijk::setEnd()
{
    myCurrent = myEnd;
}


Standard_Boolean Iterator_Ijk::onBound(Side3d s) const
{
    switch(s)
    {
    case Side3d::iMin: return myCurrent.i == myBegin.i;
    case Side3d::iMax: return myCurrent.i == myEnd.i - 1;
    case Side3d::jMin: return myCurrent.j == myBegin.j;
    case Side3d::jMax: return myCurrent.j == myEnd.j - 1;
    case Side3d::kMin: return myCurrent.k == myBegin.k;
    case Side3d::kMax: return myCurrent.k == myEnd.k - 1;
    }
    return Standard_False;
}


Standard_Boolean Iterator_Ijk::nonBound() const
{
    return myCurrent.i != myBegin.i
        && myCurrent.i != myEnd.i - 1
        && myCurrent.j != myBegin.j
        && myCurrent.j != myEnd.j - 1
        && myCurrent.k != myBegin.k
        && myCurrent.k != myEnd.k - 1;
}
