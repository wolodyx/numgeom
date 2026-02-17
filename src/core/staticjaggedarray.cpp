#include "numgeom/staticjaggedarray.h"

#include <cassert>
#include <numeric>


StaticJaggedArray::StaticJaggedArray()
{
}


StaticJaggedArray::StaticJaggedArray(
    size_t rows,
    size_t elems
)
    : myData(elems, -1), myOffsets(rows + 1, 0)
{
}


StaticJaggedArray::StaticJaggedArray(
    const std::vector<size_t>& data,
    const std::vector<size_t>& offsets
)
: myData(data), myOffsets(offsets)
{
}


StaticJaggedArray::StaticJaggedArray(
    const std::vector<size_t>& rowSizes
)
{
    this->Initialize(rowSizes);
}


void StaticJaggedArray::Initialize(
    const std::vector<size_t>& rowSizes
)
{
    myOffsets.clear();
    myData.clear();
    size_t nElements = std::accumulate(rowSizes.begin(), rowSizes.end(), static_cast<size_t>(0));
    myData.resize(nElements, -1);
    myOffsets.resize(rowSizes.size() + 1);
    auto ito = myOffsets.begin();
    (*ito) = 0;
    size_t offset = 0;
    for(auto rowSz : rowSizes)
    {
        offset += rowSz;
        (*++ito) = offset;
    }
}


void StaticJaggedArray::Initialize(
    size_t rows,
    size_t elems
)
{
    myData.resize(elems, -1);
    myOffsets.resize(rows + 1, 0);
}


size_t StaticJaggedArray::Size() const
{
    return myOffsets.size() - 1;
}


size_t StaticJaggedArray::Size(size_t i) const
{
    return myOffsets[i + 1] - myOffsets[i];
}


const size_t* StaticJaggedArray::operator[](size_t i) const
{
    return &myData[myOffsets[i]];
}


size_t* StaticJaggedArray::operator[](size_t i)
{
    return &myData[myOffsets[i]];
}


void StaticJaggedArray::Append(size_t iRow, size_t element)
{
    size_t* row = (*this)[iRow];
    size_t rowSz = this->Size(iRow);
    size_t i = 0;
    while(i < rowSz && row[i] != -1)
        ++i;
    assert(i < rowSz);
    row[i] = element;
}


const size_t* StaticJaggedArray::Data() const
{
    return myData.data();
}


size_t* StaticJaggedArray::Data()
{
    return myData.data();
}


const size_t* StaticJaggedArray::Offsets() const
{
    return myOffsets.data();
}


size_t* StaticJaggedArray::Offsets()
{
    return myOffsets.data();
}


void StaticJaggedArray::Clear()
{
    myData.clear();
    myOffsets.clear();
}
