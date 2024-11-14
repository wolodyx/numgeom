#include "numgeom/utilities.h"

#include <BndLib_Add2dCurve.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepTools.hxx>
#include <Geom_BoundedCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Interface_Static.hxx>
#include <Precision.hxx>
#include <ProjLib_ProjectedCurve.hxx>
#include <ShapeFix_Shape.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>


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


Standard_Boolean ReadFromFile(
    const std::filesystem::path& fileName,
    TopoDS_Shape& shape
)
{
    if(!std::filesystem::exists(fileName))
        return Standard_False;

    std::string ext = fileName.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return std::tolower(c); });
    if(ext == ".step" || ext == ".stp")
    {
        STEPControl_Reader reader;
        std::string str = fileName.u8string();
        if(reader.ReadFile(str.c_str()) != IFSelect_RetDone)
            return Standard_False;
        Standard_Integer NbRoots = reader.NbRootsForTransfer();
        Standard_Integer NbTrans = reader.TransferRoots();
        shape = reader.OneShape();

        Standard_Boolean doHealing = Standard_False;
        if(doHealing)
        {
            ShapeFix_Shape shapeHealer(shape);
            if(shapeHealer.Perform())
                shape = shapeHealer.Shape();
        }

        return Standard_True;
    } else if(ext == ".brep")
    {
        BRep_Builder builder;
        return BRepTools::Read(shape, fileName.string().c_str(), builder);
    }

    return Standard_False;
}


Standard_Boolean WriteToStep(
    const TopoDS_Shape& shape,
    const std::filesystem::path& fileName
)
{
    STEPControl_Writer writer;
    writer.Transfer(shape, STEPControl_AsIs, Standard_True);
    auto rc = writer.Write(fileName.u8string().c_str());
    if(rc != IFSelect_RetDone)
        return Standard_False;
    return Standard_True;
}


Standard_Boolean IsWaterproof(const TopoDS_Shape& shape)
{
    TopTools_IndexedDataMapOfShapeListOfShape edge2faces;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edge2faces);
    Standard_Integer nEdges = edge2faces.Extent();
    for(Standard_Integer i = 1; i <= nEdges; ++i)
    {
        const TopoDS_Edge& edge = TopoDS::Edge(edge2faces.FindKey(i));
        if(BRep_Tool::Degenerated(edge))
            continue;

        const int nOwningFaces = edge2faces.FindFromIndex(i).Extent();
        if(nOwningFaces != 2)
            return false;
    }
    return true;
}


bool IsClosed(const TopoDS_Edge& E)
{
    TopoDS_Vertex V1 = TopExp::FirstVertex(E, true);
    TopoDS_Vertex V2 = TopExp::LastVertex(E, true);
    return V1.IsSame(V2);
}


gp_Pnt2d Project(const gp_Pnt& P, const Handle(Geom_Surface)& S)
{
    GeomAPI_ProjectPointOnSurf project(P, S);
    Standard_Real u,v;
    project.LowerDistanceParameters(u,v);
    return gp_Pnt2d(u,v);
}


Standard_Boolean OnFace(
    const TopoDS_Face& F,
    const gp_Pnt2d& uv
)
{
    Standard_Real tol = Min(BRep_Tool::Tolerance(F), Precision::Confusion());
    BRepClass_FaceClassifier classifier;
    classifier.Perform(F, uv, tol);
    TopAbs_State state = classifier.State();
    if(state == TopAbs_ON || state == TopAbs_IN)
        return Standard_True;
    return Standard_False;
}


Handle(Geom2d_BoundedCurve) Project(
    const Handle(Geom_BoundedCurve)& curve,
    const Handle(Geom_Surface)& surface
)
{
    Handle(Adaptor3d_Curve) C = new GeomAdaptor_Curve(curve);
    Handle(Adaptor3d_Surface) S = new GeomAdaptor_Surface(surface);
    ProjLib_ProjectedCurve C2d(S, C);
    return C2d.BSpline();
}


Standard_Real Project(const gp_Pnt2d& P, const Handle(Geom2d_Curve)& C)
{
    Geom2dAPI_ProjectPointOnCurve project(P, C);
    if(project.NbPoints() == 0)
        return HUGE_VAL;
    return project.LowerDistanceParameter();
}


Standard_Real Project(const gp_Pnt& P, const Handle(Geom_Curve)& C)
{
    GeomAPI_ProjectPointOnCurve project(P, C);
    if(project.NbPoints() == 0)
        return HUGE_VAL;
    return project.LowerDistanceParameter();
}
