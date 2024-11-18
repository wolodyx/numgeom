#ifndef numgeom_numgeom_staticjaggedarray_h
#define numgeom_numgeom_staticjaggedarray_h

#include <cstddef>
#include <vector>

#include "numgeom/numgeom_export.h"


/**\class StaticJaggedArray
\brief Static jagged array

Jagged arrays on [wiki](https://en.wikipedia.org/wiki/Jagged_array).
*/
class NUMGEOM_EXPORT StaticJaggedArray
{
public:

    StaticJaggedArray();

    StaticJaggedArray(const std::vector<size_t>& rowSizes);

    void Initialize(const std::vector<size_t>& rowSizes);

    size_t Size() const;

    size_t Size(size_t i) const;

    const size_t* operator[](size_t i) const;

    size_t* operator[](size_t i);

    void Append(size_t iRow, size_t element);

private:
    std::vector<size_t> m_data;
    std::vector<size_t> m_offsets;
};
#endif // !numgeom_numgeom_staticjaggedarray_h
