#ifndef numgeom_core_iterator_ijk_h
#define numgeom_core_iterator_ijk_h

#include "numgeom/enums.h"
#include "numgeom/ijk.h"
#include "numgeom/numgeom_export.h"


class NUMGEOM_EXPORT Iterator_Ijk
{
public:

    Iterator_Ijk();

    Iterator_Ijk(
        const Ijk& end,
        const Ijk& init = Ijk(),
        const Ijk& begin = Ijk());

    Iterator_Ijk& operator++();

    Ijk operator*() const;

    Standard_Boolean operator==(const Iterator_Ijk& other) const;
    Standard_Boolean operator!=(const Iterator_Ijk& other) const;

    Standard_Boolean isEnd() const;

    Standard_Boolean onBound(Side3d) const;
    Standard_Boolean nonBound() const;

    void setEnd();

private:
    Ijk myCurrent;
    Ijk myBegin;
    Ijk myEnd;
};
#endif // !numgeom_core_iterator_ijk_h
