#ifndef numgeom_core_ijk_h
#define numgeom_core_ijk_h

#include <cassert>

#include <Standard_TypeDef.hxx>


class Ijk
{
public:

    static Standard_Integer Index(
        const Ijk& c,
        Standard_Integer ni,
        Standard_Integer nj
    )
    {
        return c.i + c.j*ni + c.k*ni*nj;
    }

    static Standard_Integer Index(
        const Ijk& c,
        const Ijk& n
    )
    {
        assert(c.k < n.k);
        return c.i + c.j*n.i + c.k*n.i*n.j;
    }

    static Ijk Coords(
        Standard_Integer index,
        Standard_Integer ni,
        Standard_Integer nj
    )
    {
        Standard_Integer k = index / (ni * nj);
        Standard_Integer r = index - k * ni * nj;
        Standard_Integer j = r / ni;
        Standard_Integer i = r % ni;
        return Ijk(i,j,k);
    }

    static Ijk Coords(
        Standard_Integer index,
        const Ijk& n
    )
    {
        Standard_Integer k = index / (n.i * n.j);
        Standard_Integer r = index - k * n.i * n.j;
        Standard_Integer j = r / n.i;
        Standard_Integer i = r % n.i;
        return Ijk(i,j,k);
    }


public:

    Ijk() : i(0), j(0), k(0) {}

    Ijk(Standard_Integer ii, Standard_Integer jj,  Standard_Integer kk)
        : i(ii), j(jj), k(kk) {}

    Ijk(const Standard_Integer* ijk)
        : i(ijk[0]), j(ijk[1]), k(ijk[2]) {}

    Standard_Boolean operator==(const Ijk& other) const
    {
        return i == other.i && j == other.j && k == other.k;
    }

    Standard_Boolean operator!=(const Ijk& other) const
    {
        return i != other.i || j != other.j || k != other.k;
    }

    Standard_Boolean operator<(const Ijk& other) const
    {
        return i < other.i && j < other.j && k < other.k;
    }

    Standard_Boolean operator<=(const Ijk& other) const
    {
        return i <= other.i && j <= other.j && k <= other.k;
    }

    Ijk& operator*=(Standard_Integer r)
    {
        i *= r, j *= r, k *= r;
        return (*this);
    }

    Ijk& operator/=(Standard_Integer r)
    {
        i /= r, j /= r, k /= r;
        return (*this);
    }

    Ijk& operator+=(Standard_Integer r)
    {
        i += r, j += r, k += r;
        return (*this);
    }

    Ijk operator/(Standard_Integer r) const
    {
        return Ijk(i/r, j/r, k/r);
    }

    Ijk operator*(Standard_Integer r) const
    {
        return Ijk(i*r, j*r, k*r);
    }

    Ijk operator+(Standard_Integer r) const
    {
        return Ijk(i+r, j+r, k+r);
    }

    Ijk operator-(Standard_Integer r) const
    {
        return Ijk(i-r, j-r, k-r);
    }

    Ijk operator+(const Ijk& o) const
    {
        return Ijk(i+o.i, j+o.j, k+o.k);
    }

    Ijk operator-(const Ijk& o) const
    {
        return Ijk(i-o.i, j-o.j, k-o.k);
    }

public:
    Standard_Integer i, j, k;
};
#endif // !numgeom_core_ijk_h
