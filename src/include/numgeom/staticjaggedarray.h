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

    StaticJaggedArray(
        size_t rows,
        size_t elems
    );

    StaticJaggedArray(
        const std::vector<size_t>& data,
        const std::vector<size_t>& offsets
    );

    StaticJaggedArray(
        const std::vector<size_t>& rowSizes
    );

    void Initialize(
        const std::vector<size_t>& rowSizes
    );

    size_t Size() const;

    size_t Size(size_t i) const;

    const size_t* operator[](size_t i) const;

    size_t* operator[](size_t i);

    const size_t* Data() const;

    size_t* Data();

    const size_t* Offsets() const;

    size_t* Offsets();

    void Append(size_t iRow,size_t element);

private:
    StaticJaggedArray(const StaticJaggedArray&) = delete;
    void operator=(const StaticJaggedArray&) = delete;

private:
    std::vector<size_t> myData;
    std::vector<size_t> myOffsets;
};
#endif // !numgeom_numgeom_staticjaggedarray_h
