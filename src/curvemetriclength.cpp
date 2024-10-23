#include "curvemetriclength.h"

#include <cassert>

#include <Geom_Curve.hxx>
#include <Geom2d_Curve.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Pln.hxx>


namespace {;

Standard_Real PlaneFunction(
    const gp_Pnt& P,
    Standard_Real A,
    Standard_Real B,
    Standard_Real C,
    Standard_Real D
)
{
    return A*P.X() + B*P.Y() + C*P.Z() + D;
}


Standard_Real LineFunction(
    const gp_Pnt2d& P,
    Standard_Real A,
    Standard_Real B,
    Standard_Real C
)
{
    return A*P.X() + B*P.Y() + C;
}


//! Параметр пересечения кривой с плоскостью.
Standard_Real GetIntersection(
    const Handle(Geom_Curve)& C,
    Standard_Real uFirst,
    Standard_Real uSecond,
    const gp_Pln& plane
)
{
    Standard_Real a, b, c, d;
    plane.Coefficients(a, b, c, d);

    Standard_Real f0 = PlaneFunction(C->Value(uFirst), a, b, c, d);
    Standard_Real f1 = PlaneFunction(C->Value(uSecond), a, b, c, d);
    if(f0 < 0.0 && f1 < 0.0 || f0 > 0.0 && f1 > 0.0)
        return HUGE_VAL;

    Standard_Real u0 = uFirst, u1 = uSecond;
    if(f0 > 0.0)
        std::swap(u0, u1);

    Standard_Integer N = 10;
    Standard_Real u;
    for(Standard_Integer i = 0; i < N; ++i)
    {
        u = 0.5 * (u0 + u1);
        gp_Pnt pt = C->Value(u);
        Standard_Real f = PlaneFunction(pt, a, b, c, d);
        if(f > 0.0)
            u1 = u;
        else if(f < 0.0)
            u0 = u;
        else
            break;
    }
    return u;
}


//! Параметр пересечения кривой с плоскостью.
Standard_Real GetIntersection(
    const Handle(Geom2d_Curve)& C,
    Standard_Real uFirst,
    Standard_Real uSecond,
    const gp_Lin2d& line
)
{
    Standard_Real a, b, c;
    line.Coefficients(a, b, c);

    Standard_Real f0 = LineFunction(C->Value(uFirst), a, b, c);
    Standard_Real f1 = LineFunction(C->Value(uSecond), a, b, c);
    if(f0 < 0.0 && f1 < 0.0 || f0 > 0.0 && f1 > 0.0)
        return HUGE_VAL;

    Standard_Real u0 = uFirst, u1 = uSecond;
    if(f0 > 0.0)
        std::swap(u0, u1);

    Standard_Integer N = 10;
    Standard_Real u;
    for(Standard_Integer i = 0; i < N; ++i)
    {
        u = 0.5 * (u0 + u1);
        gp_Pnt2d pt = C->Value(u);
        Standard_Real f = LineFunction(pt, a, b, c);
        if(f > 0.0)
            u1 = u;
        else if(f < 0.0)
            u0 = u;
        else
            break;
    }
    return u;
}


gp_Pnt MiddlePoint(const gp_Pnt& P1, const gp_Pnt& P2, Standard_Real u)
{
    gp_Vec V(P1,P2);
    Standard_Real M = V.Magnitude();
    return P1.Translated(V.Multiplied(M*u));
}


gp_Pnt2d MiddlePoint(const gp_Pnt2d& P1, const gp_Pnt2d& P2, Standard_Real u)
{
    gp_Vec2d V(P1,P2);
    Standard_Real M = V.Magnitude();
    return P1.Translated(V.Multiplied(M*u));
}


//! Наибольшее расстояние между частью кривой и упирающейся в нее хордой.
Standard_Real MaximumGapParameter(
    const Handle(Geom_Curve)& C,
    Standard_Real u0,
    Standard_Real u1
)
{
    gp_Pnt pt0 = C->Value(u0), pt1 = C->Value(u1);
    gp_Dir planeNormal(gp_Vec(pt0, pt1));
    Standard_Real maxGap = 0.0;
    Standard_Real maxGapParam = HUGE_VAL;
    Standard_Integer N = 31;
    for(Standard_Integer i = 1; i <= N; ++i)
    {
        gp_Pnt pt = MiddlePoint(pt0, pt1, 1.0 / (N + 1) * i);
        gp_Pln plane(pt, planeNormal);
        Standard_Real u = GetIntersection(C, u0, u1, plane);
        Standard_Real gap = pt.Distance(C->Value(u));
        if(maxGap < gap)
        {
            maxGap = gap;
            maxGapParam = u;
        }
    }
    return maxGapParam;
}


//! Наибольшее расстояние между частью кривой и упирающейся в нее хордой.
Standard_Real MaximumGapParameter(
    const Handle(Geom2d_Curve)& C,
    Standard_Real u0,
    Standard_Real u1
)
{
    gp_Pnt2d pt0 = C->Value(u0), pt1 = C->Value(u1);
    gp_Dir2d chordDir(gp_Vec2d(pt0,pt1));
    gp_Dir2d lineDir(chordDir.Y(), -chordDir.X());
    Standard_Real maxGap = 0.0;
    Standard_Real maxGapParam = HUGE_VAL;
    Standard_Integer N = 31;
    for(Standard_Integer i = 1; i <= N; ++i)
    {
        gp_Pnt2d pt = MiddlePoint(pt0, pt1, 1.0 / (N + 1) * i);
        gp_Lin2d line(pt, lineDir);
        Standard_Real u = GetIntersection(C, u0, u1, line);
        Standard_Real gap = pt.Distance(C->Value(u));
        if(maxGap < gap)
        {
            maxGap = gap;
            maxGapParam = u;
        }
    }
    return maxGapParam;
}


Standard_Real pow2(double v)
{
    return v * v;
}


Standard_Real pow3(double v)
{
    return v * v * v;
}
}


CurveMetricLength::CurveMetricLength(
    const Handle(Geom_Curve)& C,
    Standard_Real uFirst,
    Standard_Real uLast,
    Standard_Real relAccuracy
)
{
    myCurve = C;
    myU0 = uFirst;
    myU1 = uLast;
    myRelAccuracy = relAccuracy;

    myL = 0.0;
    myPolyline = { myU0, myU1 };
    auto it = myPolyline.begin();
    Standard_Real u0 = *it;
    Standard_Real m0 = MetricOnCurvePoint(u0);
    while(Standard_True)
    {
        auto itNext = it; ++itNext;
        if(itNext == myPolyline.end())
            break;

        Standard_Real tA = *it;
        Standard_Real tB = *itNext;

        Standard_Real tC = MaximumGapParameter(C, tA, tB);
        gp_Pnt ptA = C->Value(tA);
        gp_Pnt ptB = C->Value(tB);
        gp_Pnt ptC = C->Value(tC);
        Standard_Real d = ptA.Distance(ptB);
        Standard_Real d1 = ptA.Distance(ptC);
        Standard_Real d2 = ptB.Distance(ptC);
        Standard_Real eps = std::abs(d / (d1 + d2) - 1.0);
        if(eps > relAccuracy)
            myPolyline.insert(itNext, tC);
        else
        {
            Standard_Real u1 = (*itNext);
            Standard_Real m1 = MetricOnCurvePoint(u1);
            Standard_Real subL = CalcCurveMetricLength(u0, u1, m0, m1);
            if(subL > 0.1)
            {
                myPolyline.insert(itNext, tC);
            }
            else
            {
                myL += subL;
                u0 = u1, m0 = m1;
                ++it;
            }
        }
    }
}


Standard_Real CurveMetricLength::Length() const
{
    return myL;
}


Standard_Real CurveMetricLength::ParameterByAbscissa(
    Standard_Real uStart,
    Standard_Real abscissa
) const
{
    assert(uStart >= myU0 && uStart <= myU1);

    gp_Pnt ptStart = myCurve->Value(uStart);
    auto it = myPolyline.begin();
    while(it != myPolyline.end() && *it <= uStart)
        ++it;
    Standard_Real u0 = uStart;
    Standard_Real m0 = MetricOnCurvePoint(u0);
    Standard_Real u1 = (*it);
    Standard_Real m1 = MetricOnCurvePoint(u1);
    Standard_Real allL = CalcCurveMetricLength(u0, u1, m0, m1);
    while(allL < abscissa)
    {
        ++it;
        if(it == myPolyline.end())
            return HUGE_VAL;
        u0 = u1, m0 = m1;
        u1 = (*it);
        m1 = MetricOnCurvePoint(u1);
        Standard_Real L = CalcCurveMetricLength(u0, u1, m0, m1);
        allL += L;
    }

    Standard_Real restL = allL - abscissa;
    if(restL < 1.e-8)
        return u0;

    Standard_Real u, m;
    Standard_Real us = u0, ms = m0;
    for(Standard_Integer iter = 0; iter < 10; ++iter)
    {
        u = 0.5 * (u0 + u1);
        m = MetricOnCurvePoint(u);
        Standard_Real L = CalcCurveMetricLength(us, u, ms, m);
        if(L < restL)
            u0 = u, m0 = m;
        else if(L > restL)
            u1 = u, m1 = m;
        else
            break;
    }

    return u;
}


Standard_Real CurveMetricLength::CalcCurveMetricLength(
    Standard_Real uA,
    Standard_Real uB,
    Standard_Real hA,
    Standard_Real hB
) const
{
    // Pascal Jean Frey, Paul-Louis George.
    // Mesh Generation Application to Finite Elements / p.465
    gp_Pnt ptA = myCurve->Value(uA);
    gp_Pnt ptB = myCurve->Value(uB);
    Standard_Real L = ptA.Distance(ptB);
    Standard_Real E = hB / hA - 1.0;
    if(E > 0.03)
        return L * (hA - hB)/(hA*hB*std::log(hA/hB));
    else
        return L * (2 - E) / (2 * hA);
}


Standard_Real CurveMetricLength::MetricOnCurvePoint(Standard_Real u) const
{
    Standard_Real dro = 0.0;
    Standard_Real alpha = std::sqrt(6*myRelAccuracy) / std::pow(1+dro*dro, 0.25);
    gp_Pnt p;
    gp_Vec v1, v2;
    myCurve->D2(u, p, v1, v2);
    Standard_Real ro = pow3(v1.Magnitude()) / v1.Crossed(v2).Magnitude();
    //return 1.0 / pow2(alpha * ro);
    return alpha * ro;
}


Curve2dMetricLength::Curve2dMetricLength(
    const Handle(Geom2d_Curve)& C,
    Standard_Real uFirst,
    Standard_Real uLast,
    Standard_Real relAccuracy
)
{
    myCurve = C;
    myU0 = uFirst;
    myU1 = uLast;
    myRelAccuracy = relAccuracy;

    myL = 0.0;
    myPolyline = { myU0, myU1 };
    auto it = myPolyline.begin();
    Standard_Real u0 = *it;
    Standard_Real m0 = MetricOnCurvePoint(u0);
    while(Standard_True)
    {
        auto itNext = it; ++itNext;
        if(itNext == myPolyline.end())
            break;

        Standard_Real tA = *it;
        Standard_Real tB = *itNext;

        Standard_Real tC = MaximumGapParameter(C, tA, tB);
        gp_Pnt2d ptA = C->Value(tA);
        gp_Pnt2d ptB = C->Value(tB);
        gp_Pnt2d ptC = C->Value(tC);
        Standard_Real d = ptA.Distance(ptB);
        Standard_Real d1 = ptA.Distance(ptC);
        Standard_Real d2 = ptB.Distance(ptC);
        Standard_Real eps = std::abs(d / (d1 + d2) - 1.0);
        if(eps > relAccuracy)
            myPolyline.insert(itNext, tC);
        else
        {
            Standard_Real u1 = (*itNext);
            Standard_Real m1 = MetricOnCurvePoint(u1);
            Standard_Real subL = CalcCurveMetricLength(u0, u1, m0, m1);
            if(subL > 0.1)
            {
                myPolyline.insert(itNext, tC);
            }
            else
            {
                myL += subL;
                u0 = u1, m0 = m1;
                ++it;
            }
        }
    }
}


Standard_Real Curve2dMetricLength::Length() const
{
    return myL;
}


Standard_Real Curve2dMetricLength::ParameterByAbscissa(
    Standard_Real uStart,
    Standard_Real abscissa
) const
{
    assert(uStart >= myU0 && uStart <= myU1);

    gp_Pnt2d ptStart = myCurve->Value(uStart);
    auto it = myPolyline.begin();
    while(it != myPolyline.end() && *it <= uStart)
        ++it;
    Standard_Real u0 = uStart;
    Standard_Real m0 = MetricOnCurvePoint(u0);
    Standard_Real u1 = (*it);
    Standard_Real m1 = MetricOnCurvePoint(u1);
    Standard_Real allL = CalcCurveMetricLength(u0, u1, m0, m1);
    while(allL < abscissa)
    {
        ++it;
        if(it == myPolyline.end())
            return HUGE_VAL;
        u0 = u1, m0 = m1;
        u1 = (*it);
        m1 = MetricOnCurvePoint(u1);
        Standard_Real L = CalcCurveMetricLength(u0, u1, m0, m1);
        allL += L;
    }

    Standard_Real restL = allL - abscissa;
    if(restL < 1.e-8)
        return u0;

    Standard_Real u, m;
    Standard_Real us = u0, ms = m0;
    for(Standard_Integer iter = 0; iter < 10; ++iter)
    {
        u = 0.5 * (u0 + u1);
        m = MetricOnCurvePoint(u);
        Standard_Real L = CalcCurveMetricLength(us, u, ms, m);
        if(L < restL)
            u0 = u, m0 = m;
        else if(L > restL)
            u1 = u, m1 = m;
        else
            break;
    }

    return u;
}


Standard_Real Curve2dMetricLength::CalcCurveMetricLength(
    Standard_Real uA,
    Standard_Real uB,
    Standard_Real hA,
    Standard_Real hB
) const
{
    // Pascal Jean Frey, Paul-Louis George.
    // Mesh Generation Application to Finite Elements / p.465
    gp_Pnt2d ptA = myCurve->Value(uA);
    gp_Pnt2d ptB = myCurve->Value(uB);
    Standard_Real L = ptA.Distance(ptB);
    Standard_Real E = hB / hA - 1.0;
    if(E > 0.03)
        return L * (hA - hB)/(hA*hB*std::log(hA/hB));
    else
        return L * (2 - E) / (2 * hA);
}


Standard_Real Curve2dMetricLength::MetricOnCurvePoint(Standard_Real u) const
{
    Standard_Real dro = 0.0;
    Standard_Real alpha = std::sqrt(6*myRelAccuracy) / std::pow(1+dro*dro, 0.25);
    gp_Pnt2d p;
    gp_Vec2d v1, v2;
    myCurve->D2(u, p, v1, v2);
    Standard_Real ro = pow3(v1.Magnitude()) / v1.Crossed(v2);
    //return 1.0 / pow2(alpha * ro);
    return alpha * ro;
}
