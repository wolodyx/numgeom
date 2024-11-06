#include "numgeom/utilities.h"

#include <BndLib_Add2dCurve.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>


Bnd_Box2d ComputePCurvesBox(const TopoDS_Face& F)
{
    static const Standard_Real tol = Precision::Confusion();

    Bnd_Box2d box;
    BndLib_Add2dCurve boxComputer;
    for(TopoDS_Iterator itw(F); itw.More(); itw.Next())
    {
        const TopoDS_Shape& W = itw.Value();
        for(TopoDS_Iterator ite(W); ite.More(); ite.Next())
        {
            const TopoDS_Edge& E = TopoDS::Edge(ite.Value());
            Standard_Real uFirst, uLast;
            Handle(Geom2d_Curve) PC = BRep_Tool::CurveOnSurface(E, F, uFirst, uLast);
            boxComputer.Add(PC, uFirst, uLast, tol, box);
        }
    }
    return box;
}


Bnd_Box ComputeBoundBox(const TopoDS_Shape& shape)
{
    if(shape.IsNull())
        return Bnd_Box();

    Bnd_Box box;
    BRepBndLib().Add(shape, box);
    return box;
}
