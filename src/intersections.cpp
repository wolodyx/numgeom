#include "intersections.h"

#include <Geom_BoundedCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Extrema_ExtCC.hxx>


Standard_Boolean IsIntersected(
    const Handle(Geom_Curve)& C1, Standard_Real c1u0, Standard_Real c1u1,
    const Handle(Geom_Curve)& C2, Standard_Real c2u0, Standard_Real c2u1
)
{
    GeomAdaptor_Curve aC1(C1), aC2(C2);
    if(c1u0 > c1u1)
        std::swap(c1u0, c1u1);
    if(c2u0 > c2u1)
        std::swap(c2u0, c2u1);

    Extrema_ExtCC exCC(aC1, aC2, c1u0, c1u1, c2u0, c2u1);
    exCC.SetTolerance(1, 1.e-5);
    exCC.Perform();
    if(!exCC.IsDone())
        return Standard_False;

    Standard_Integer N = exCC.NbExt();
    for(Standard_Integer i = 1; i <= N; ++i)
    {
        Standard_Real sqDistance = exCC.SquareDistance(i);
        if(sqDistance > 1.e-8)
            continue;

        try
        {
            Extrema_POnCurv exP1, exP2;
            exCC.Points(i, exP1, exP2);
            Standard_Real u1 = exP1.Parameter(), u2 = exP2.Parameter();
            if((u1 == c1u0 || u1 == c1u1) && (u2 == c2u0 || u2 == c2u1))
                continue;
            return Standard_True;
        }
        catch(...)
        {
            return Standard_True;
        }
    }
    return Standard_False;
}
