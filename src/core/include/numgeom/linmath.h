#ifndef numgeom_core_linmath_h
#define numgeom_core_linmath_h

#include <cstddef>


template<typename T>
class Vec2
{
public:

    Vec2() {}

    Vec2(T xx, T yy) : x(xx), y(yy) {}

    Vec2(const T* xy) : x(xy[0]), y(xy[1]) {}

public:
    T x, y;
};


template<typename T>
class Vec3
{
public:

    Vec3() {}

    Vec3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {}

    Vec3(const T* xyz) : x(xyz[0]), y(xyz[1]), z(xyz[2]) {}

public:
    T x, y, z;
};


template<typename T>
class Tuple2
{
public:
    Tuple2() {}

    Tuple2(T v1, T v2) : a(v1), b(v2) {}

public:
    T a, b;
};


template<typename T>
class Tuple3
{
public:
    Tuple3() {}

    Tuple3(T v1, T v2, T v3) : a(v1), b(v2), c(v3) {}

public:
    T a, b, c;
};


typedef Vec2<double> Vec2d;
typedef Vec2<unsigned int> Vec2ui;

typedef Vec3<double> Vec3d;
typedef Vec3<float> Vec3f;
typedef Vec3<unsigned int> Vec3ui;

typedef Tuple2<unsigned int> Tuple2ui;
typedef Tuple2<size_t> Tuple2s;

typedef Tuple3<unsigned int> Tuple3ui;
typedef Tuple3<size_t> Tuple3s;

#endif // !numgeom_core_linmath_h
