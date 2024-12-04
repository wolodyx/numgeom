#ifndef NUMGEOM_NUMGEOM_CURVEMETRICLENGTH_H
#define NUMGEOM_NUMGEOM_CURVEMETRICLENGTH_H

#include <list>

#include "Standard_Handle.hxx"

class Geom_Curve;
class Geom2d_Curve;


class CurveMetricLength
{
public:
    CurveMetricLength(
        const Handle(Geom_Curve)& C,
        Standard_Real uFirst,
        Standard_Real uLast,
        Standard_Real relAccuracy
    );

    Standard_Real Length() const;

    Standard_Real ParameterByAbscissa(
        Standard_Real u0,
        Standard_Real abscissa
    ) const;

private:
    Standard_Real MetricOnCurvePoint(Standard_Real u) const;

    Standard_Real CalcCurveMetricLength(
        Standard_Real uA,
        Standard_Real uB,
        Standard_Real hA,
        Standard_Real hB
    ) const;

private:
    Handle(Geom_Curve) myCurve;
    Standard_Real myU0,myU1;
    Standard_Real myL;
    Standard_Real myRelAccuracy;
    std::list<Standard_Real> myPolyline;
};


class Curve2dMetricLength
{
public:
    Curve2dMetricLength(
        const Handle(Geom2d_Curve)& C,
        Standard_Real uFirst,
        Standard_Real uLast,
        Standard_Real relAccuracy
    );

    Standard_Real Length() const;

    Standard_Real ParameterByAbscissa(
        Standard_Real u0,
        Standard_Real abscissa
    ) const;

private:
    Standard_Real MetricOnCurvePoint(Standard_Real u) const;

    Standard_Real CalcCurveMetricLength(
        Standard_Real uA,
        Standard_Real uB,
        Standard_Real hA,
        Standard_Real hB
    ) const;

private:
    Handle(Geom2d_Curve) myCurve;
    Standard_Real myU0,myU1;
    Standard_Real myL;
    Standard_Real myRelAccuracy;
    std::list<Standard_Real> myPolyline;
};
#endif // !NUMGEOM_NUMGEOM_CURVEMETRICLENGTH_H
