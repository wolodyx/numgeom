#ifndef numgeom_numgeom_intersections_h
#define numgeom_numgeom_intersections_h

#include "Standard_Handle.hxx"

class Geom_BoundedCurve;
class Geom_Curve;


Standard_Boolean IsIntersected(
    const Handle(Geom_Curve)& C1, Standard_Real u0C1, Standard_Real u1C1,
    const Handle(Geom_Curve)& C2, Standard_Real u0C2, Standard_Real u1C2
);

#endif // !numgeom_numgeom_intersections_h
