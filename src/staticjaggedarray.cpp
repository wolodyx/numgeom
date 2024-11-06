#include "numgeom/staticjaggedarray.h"

#include <cassert>
#include <numeric>


StaticJaggedArray::StaticJaggedArray()
{
}


StaticJaggedArray::StaticJaggedArray(const std::vector<size_t>& rowSizes)
{
    this->Initialize(rowSizes);
}


void StaticJaggedArray::Initialize(const std::vector<size_t>& rowSizes)
{
    m_offsets.clear();
    m_data.clear();
    size_t nElements = std::accumulate(rowSizes.begin(), rowSizes.end(), static_cast<size_t>(0));
    m_data.resize(nElements, -1);
    m_offsets.resize(rowSizes.size() + 1);
    auto ito = m_offsets.begin();
    (*ito) = 0;
    size_t offset = 0;
    for(auto rowSz : rowSizes)
    {
        offset += rowSz;
        (*++ito) = offset;
    }
}


size_t StaticJaggedArray::Size() const
{
    return m_offsets.size() - 1;
}


size_t StaticJaggedArray::Size(size_t i) const
{
    return m_offsets[i + 1] - m_offsets[i];
}


const size_t* StaticJaggedArray::operator[](size_t i) const
{
    return &m_data[m_offsets[i]];
}


size_t* StaticJaggedArray::operator[](size_t i)
{
    return &m_data[m_offsets[i]];
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
