#include "ffextintersections.h"

#include <cassert>
#include <list>

#include <Adaptor2d_Line2d.hxx>
#include <Adaptor3d_CurveOnSurface.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom_BoundedCurve.hxx>
#include <Geom_BoundedSurface.hxx>
#include <Geom2d_BoundedCurve.hxx>
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAPI_IntSS.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomLib.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopTools_ListOfShape.hxx>

#include "numgeom/utilities.h"


namespace {;


double GetIsolineCurveLength(
    Handle(Geom_BoundedSurface) surface,
    Standard_Boolean isUDirection,
    double param
)
{
    double u1, u2, v1, v2;
    surface->Bounds(u1, u2, v1, v2);

    gp_Pnt2d orig(
        isUDirection? u1 : param,
        isUDirection? param : v1
    );
    gp_Dir2d dir(
        isUDirection? 1.0 : 0.0,
        isUDirection? 0.0 : 1.0
    );
    Standard_Real uFirst = isUDirection? v1 : u1;
    Standard_Real uLast  = isUDirection? v2 : u2;

    Handle(Adaptor2d_Line2d) isoline = new Adaptor2d_Line2d(orig, dir, uFirst, uLast);
    Handle(Adaptor3d_Surface) s = new GeomAdaptor_Surface(surface);
    Adaptor3d_CurveOnSurface curveOnSurface(isoline, s);
    return GCPnts_AbscissaPoint::Length(curveOnSurface);
}


void ExtendSurfacesOfExternalFaces(
    const TopTools_MapOfShape& externalFaces,
    std::unordered_map<TopoDS_Face,Handle(Geom_Surface)>& face2extendedSurface
)
{
    // Функция BRepLib::ExtendFace(face, ...) расширяет грань. Пример
    // использования можно найти в BOPAlgo_RemoveFeatures.cxx:542.

    // \warning Несущие поверхности у двух граней могут совпадать,
    //          что позволяет ускорить этот код.

    for(auto it = externalFaces.cbegin(); it != externalFaces.cend(); ++it)
    {
        const TopoDS_Face& face = TopoDS::Face(*it);
        Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
        assert(!surface.IsNull());

        auto boundedSurface = Handle(Geom_BoundedSurface)::DownCast(surface);
        if(!boundedSurface)
        {
            face2extendedSurface.insert(std::make_pair(face,surface));
            continue;
        }

        // Расширяем ограниченную поверхность
        // на половину ее длины влево и вправо, вверх и вниз.
        Standard_Real u1, u2, v1, v2;
        boundedSurface->Bounds(u1, u2, v1, v2);
        Standard_Real extULength =
              0.25 * (  GetIsolineCurveLength(boundedSurface, Standard_True, u1)
                      + GetIsolineCurveLength(boundedSurface, Standard_True, u2)
                     );
        Standard_Real extVLength =
              0.25 * (  GetIsolineCurveLength(boundedSurface, Standard_False, v1)
                      + GetIsolineCurveLength(boundedSurface, Standard_False, v2)
                     );
        Standard_Integer cont = 2;
        for(int i = 0; i < 4; ++i)
        {
            Standard_Boolean inU = (i == 0 || i == 1);
            Standard_Boolean after = (i == 1 || i == 3);
            if(inU && boundedSurface->IsUPeriodic())
                continue;
            if(!inU && boundedSurface->IsVPeriodic())
                continue;
            Standard_Real extLength = inU ? extULength : extVLength;
            GeomLib::ExtendSurfByLength(boundedSurface, extLength, cont, inU, after);
        }

        face2extendedSurface.insert(std::make_pair(face, boundedSurface));
    }
}
}


FaceFaceIntersections::FaceFaceIntersections(
    const TopTools_MapOfShape& externalFaces
)
{
    ExtendSurfacesOfExternalFaces(externalFaces, myFace2extendedSurface);
}


namespace {;
void GetCommonEdges(
    const TopoDS_Face& f1,
    const TopoDS_Face& f2,
    std::list<TopoDS_Edge>& commonEdges)
{
    for(TopExp_Explorer it1(f1,TopAbs_EDGE); it1.More(); it1.Next())
    {
        const TopoDS_Shape& e1 = it1.Value();
        for(TopExp_Explorer it2(f2,TopAbs_EDGE); it2.More(); it2.Next())
        {
            const TopoDS_Shape& e2 = it2.Value();
            if(e1.IsSame(e2))
            {
                commonEdges.push_back(TopoDS::Edge(e1));
                break;
            }
        }
    }
}
}

FaceFaceIntersections::FFResult FaceFaceIntersections::GetIntersections(
    const TopoDS_Face& fFirst
) const
{
    auto it = myFace2intersections.find(fFirst);
    if(it != myFace2intersections.end())
        return FFResult(it->second);

    Handle(Geom_Surface) surface1 = BRep_Tool::Surface(fFirst);
    GeomAPI_IntSS intSS;
    auto& results = myFace2intersections[fFirst];
    for(const auto& v : myFace2extendedSurface)
    {
        const TopoDS_Face& fSecond = v.first;
        if(fSecond == fFirst)
            continue;

        Handle(Geom_Surface) surface2 = v.second;

        intSS.Perform(surface1, surface2, 1.0e-7);
        if(!intSS.IsDone())
            continue;
        Standard_Integer nbLines = intSS.NbLines();
        for(Standard_Integer l = 1; l <= nbLines; ++l)
        {
            Handle(Geom_Curve) line = intSS.Line(1);
            results.push_back(IntFF{fSecond, line});
        }
        if(nbLines == 0)
        {
            std::list<TopoDS_Edge> commonEdges;
            GetCommonEdges(fFirst, fSecond, commonEdges);
            for(const auto& e : commonEdges)
            {
                Standard_Real u1, u2;
                auto curve = BRep_Tool::Curve(e, u1, u2);
                if(auto boundedCurve = Handle(Geom_BoundedCurve)::DownCast(curve))
                {
                    assert(false);
                }
                results.push_back(IntFF{fSecond, curve});
            }
        }
    }
    return FFResult(results);
}


FaceFaceIntersections::CFResult FaceFaceIntersections::GetIntersections(
    const Handle(Geom_Curve)& curve
) const
{
    auto it = myCurve2intersections.find(curve);
    if(it != myCurve2intersections.end())
        return CFResult(it->second);

    GeomAPI_IntCS intCS;
    Standard_Real u, v, t;
    auto& results = myCurve2intersections[curve];
    for(const auto& fs : myFace2extendedSurface)
    {
        Handle(Geom_Surface) surface = fs.second;
        intCS.Perform(curve, surface);
        if(!intCS.IsDone())
            continue;
        Standard_Integer nbPoints = intCS.NbPoints();
        for(Standard_Integer i = 1; i <= nbPoints; ++i)
        {
            intCS.Parameters(i, u, v, t);
            results.push_back(IntCF{fs.first, t});
        }
    }
    return CFResult(results);
}


FaceFaceIntersections::FFResult::FFResult(const std::list<IntFF>& ints)
    : myIntersections(ints)
{
}


FaceFaceIntersections::CFResult::CFResult(const std::list<IntCF>& ints)
    : myIntersections(ints)
{
}


namespace {;
struct SurfaceDomain
{
    gp_Pnt2d uv;
    Standard_Real uRes, vRes;
};


void CalcSpecialPoints(
    const TopoDS_Face& F,
    std::vector<SurfaceDomain>& specialPoints
)
{
    Handle(Geom_Surface) S = BRep_Tool::Surface(F);
    GeomAdaptor_Surface Sa(S);
    for(TopExp_Explorer it(F,TopAbs_VERTEX); it.More(); it.Next())
    {
        const TopoDS_Vertex& V = TopoDS::Vertex(it.Value());
        gp_Pnt2d uv = BRep_Tool::Parameters(V, F);
        double tol = BRep_Tool::Tolerance(V);
        Standard_Real uRes = Sa.UResolution(tol);
        Standard_Real vRes = Sa.VResolution(tol);
        specialPoints.push_back({uv, uRes, vRes});
    }
}


Standard_Boolean Has(
    const gp_Pnt2d& uv,
    const std::vector<SurfaceDomain>& specialPoints
)
{
    for(const auto& sp : specialPoints)
    {
        gp_Vec2d dir(sp.uv, uv);
        if(    std::abs(dir.X()) <= sp.uRes
            && std::abs(dir.Y()) <= sp.vRes)
            return Standard_True;
    }
    return Standard_False;
}
}


Standard_Boolean IsIntersected(
    const Handle(Geom_BoundedCurve)& C,
    const TopTools_MapOfShape& faces
)
{
    GeomAPI_IntCS intCS;
    for(auto it = faces.cbegin(); it != faces.cend(); ++it)
    {
        const TopoDS_Face& F = TopoDS::Face(*it);
        Handle(Geom_Surface) S = BRep_Tool::Surface(F);

        // Определяем список точек на грани, касания с которыми разрешается концам кривой.
        std::vector<SurfaceDomain> specialPoints;
        CalcSpecialPoints(F, specialPoints);

        intCS.Perform(C, S);
        if(!intCS.IsDone())
            continue;

        Standard_Integer nbPoints = intCS.NbPoints();
        Standard_Real u, v, t;
        for(Standard_Integer i = 1; i <= nbPoints; ++i)
        {
            intCS.Parameters(i, u, v, t);
            gp_Pnt2d uv(u, v);
            if(OnFace(F,uv) && !Has(uv,specialPoints))
                return Standard_True;
        }

        Standard_Integer nbSegments = intCS.NbSegments();
        Standard_Real u1, v1, u2, v2;
        for(Standard_Integer i = 1; i <= nbSegments; ++i)
        {
            intCS.Parameters(i, u1, v1, u2, v2);
            gp_Pnt2d uv1(u1, v1), uv2(u2, v2);
            if(    OnFace(F,uv1) && !Has(uv1,specialPoints)
                || OnFace(F,uv2) && !Has(uv2,specialPoints))
                return Standard_True;

            // Проверяем, пересекает ли проекция кривой на поверхность контур грани.
            Handle(Geom_BoundedCurve) C2 = Handle(Geom_BoundedCurve)::DownCast(intCS.Segment(i));
            assert(!C2.IsNull());
            Handle(Geom2d_BoundedCurve) projCurve = Project(C2, S);
            if(IsIntersected(projCurve,F))
                return Standard_True;
        }
    }

    return Standard_False;
}


Standard_Boolean IsIntersected(
    const Handle(Geom2d_BoundedCurve)& curve,
    const TopoDS_Face& face
)
{
    Geom2dAPI_InterCurveCurve intCC;
    for(TopExp_Explorer it(face,TopAbs_EDGE); it.More(); it.Next())
    {
        const TopoDS_Edge& E = TopoDS::Edge(it.Value());
        Standard_Real u0, u1;
        auto C2 = BRep_Tool::CurveOnSurface(E, face, u0, u1);
        intCC.Init(curve, C2);
        for(Standard_Integer i = 1; i <= intCC.NbPoints(); ++i)
        {
            gp_Pnt2d pt = intCC.Point(i);
            Standard_Real t1 = Project(pt, curve);
            Standard_Real t2 = Project(pt, C2);
            if(    t1 >= curve->FirstParameter() && t1 <= curve->LastParameter()
                && t2 >= u0 && t2 <= u1)
                return Standard_True;
        }
        if(intCC.NbSegments() != 0)
            return Standard_True;
    }
    return Standard_False;
}
