#include "numgeom/tesselate.h"

#include <BRep_Tool.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <Geom_Curve.hxx>
#include <Geom2d_Curve.hxx>

#include "curvemetriclength.h"


void Tesselate(
    const Handle(Geom_Curve)& C,
    Standard_Real uFirst,
    Standard_Real uLast,
    std::vector<gp_Pnt>& polyline,
    Standard_Real relAccuracy
)
{
    CurveMetricLength cl(C, uFirst, uLast, relAccuracy);
    Standard_Real L = cl.Length();
    Standard_Integer N = std::floor(L + 0.5);
    N = std::max(N, 2);
    polyline.resize(N);
    polyline[0] = C->Value(uFirst);
    polyline[N-1] = C->Value(uLast);
    for(Standard_Integer i = 1; i < N-1; ++i)
    {
        Standard_Real l = L / (N-1) * i;
        Standard_Real u = cl.ParameterByAbscissa(uFirst, l);
        polyline[i] = C->Value(u);
    }
}


void Tesselate(
    const Handle(Geom2d_Curve)& C,
    Standard_Real uFirst,
    Standard_Real uLast,
    std::vector<gp_Pnt2d>& polyline,
    Standard_Real relAccuracy
)
{
    Curve2dMetricLength cl(C, uFirst, uLast, relAccuracy);
    Standard_Real L = cl.Length();
    Standard_Integer N = std::floor(L + 0.5);
    N = std::max(N, 2);
    polyline.resize(N);
    polyline[0] = C->Value(uFirst);
    polyline[N-1] = C->Value(uLast);
    for(Standard_Integer i = 1; i < N-1; ++i)
    {
        Standard_Real l = L / (N-1) * i;
        Standard_Real u = cl.ParameterByAbscissa(uFirst, l);
        polyline[i] = C->Value(u);
    }
}


void Tesselate(
    const TopoDS_Face& F,
    const TopoDS_Wire& W,
    std::vector<gp_Pnt2d>& polyline,
    Standard_Real relAccuracy
)
{
    std::vector<gp_Pnt2d> pl;
    for(BRepTools_WireExplorer it(W); it.More(); it.Next())
    {
        const TopoDS_Edge& E = it.Current();
        Standard_Real u0, u1;
        Handle(Geom2d_Curve) C = BRep_Tool::CurveOnSurface(E, F, u0, u1);
        if(E.Orientation() == TopAbs_REVERSED)
            std::swap(u0, u1);
        Tesselate(C, u0, u1, pl);
        auto itBeg = pl.begin();
        if(!polyline.empty())
            itBeg = std::next(itBeg);
        polyline.insert(polyline.end(), itBeg, pl.end());
    }
}
