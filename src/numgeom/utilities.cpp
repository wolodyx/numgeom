#include "numgeom/utilities.h"

#include <BndLib_Add2dCurve.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
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
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
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
        std::string str = fileName.string();
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
    auto rc = writer.Write(fileName.string().c_str());
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


namespace {;
void ExtractTriangulableShapes(
    const TopoDS_Shape& initShape,
    std::vector<TopoDS_Shape>& resultShapes
)
{
    if(initShape.IsNull())
        return;

    std::queue<TopoDS_Shape> queue;
    queue.push(initShape);
    while(!queue.empty()) {
        TopoDS_Shape shape = queue.front();
        queue.pop();
        switch(shape.ShapeType()) {
        case TopAbs_COMPOUND:
        case TopAbs_COMPSOLID:
            for(TopoDS_Iterator it(shape); it.More(); it.Next()) {
                TopoDS_Shape subShape = it.Value();
                queue.push(subShape);
            }
            break;
        case TopAbs_SOLID:
        case TopAbs_SHELL:
        case TopAbs_FACE:
            resultShapes.push_back(shape);
            break;
        case TopAbs_WIRE:
        case TopAbs_EDGE:
        case TopAbs_VERTEX:
            break;
        default:
            std::exit(1);
        }
    }
}
}


TriMesh::Ptr ConvertToTriMesh(const TopoDS_Shape& initShape)
{
    std::unordered_map<TopoDS_Face,
                       std::pair<Handle(Poly_Triangulation),
                                 TopLoc_Location>
                      > face2triangulation;

    std::vector<TopoDS_Shape> triangulableShapes;
    ExtractTriangulableShapes(initShape, triangulableShapes);

    for(const TopoDS_Shape& shape : triangulableShapes) {
        for(TopExp_Explorer it(shape,TopAbs_FACE); it.More(); it.Next())
        {
            const TopoDS_Face& f = TopoDS::Face(it.Value());
            TopLoc_Location l;
            Handle(Poly_Triangulation) tr = BRep_Tool::Triangulation(f, l);
            if(!tr)
            {
                BRepMesh_IncrementalMesh(f, 0.6f);
                tr = BRep_Tool::Triangulation(f, l);
            }
            face2triangulation.insert(std::make_pair(f, std::make_pair(tr,l)));
        }
    }

    size_t nbNodes = 0, nbCells = 0;
    for(const auto& v : face2triangulation)
    {
        nbNodes += v.second.first->NbNodes();
        nbCells += v.second.first->NbTriangles();
    }

    TriMesh::Ptr mesh = TriMesh::Create(nbNodes, nbCells);
    Standard_Integer nodeIndex = 0, cellIndex = 0, nodeOffset = 0;
    for(const auto& v : face2triangulation)
    {
        Handle(Poly_Triangulation) triangulation = v.second.first;
        const TopLoc_Location& location = v.second.second;
        const gp_Trsf& tr = location.Transformation();
        Standard_Integer nbNodes = triangulation->NbNodes();
        for(Standard_Integer i = 1; i <= nbNodes; ++i)
        {
            gp_Pnt pt = triangulation->Node(i);
            pt.Transform(tr);
            mesh->GetNode(nodeIndex++) = pt;
        }

        Standard_Integer nbCells = triangulation->NbTriangles();
        for(Standard_Integer i = 1; i <= nbCells; ++i)
        {
            const Poly_Triangle& cell = triangulation->Triangle(i);
            Standard_Integer na, nb, nc;
            cell.Get(na, nb, nc);
            mesh->GetCell(cellIndex++) =
                TriMesh::Cell
                {
                    na + nodeOffset - 1,
                    nb + nodeOffset - 1,
                    nc + nodeOffset - 1
                };
        }

        nodeOffset += nbNodes;
    }

    return mesh;
}


namespace {;
Standard_Integer RoundToInt(Standard_Real value)
{
    if(value > 0.0)
        return static_cast<Standard_Integer>(std::floor(value + 0.5));
    if(value < 0.0)
        return static_cast<Standard_Integer>(std::ceil(value - 0.5));
    return 0;
}
}


void GetMeshSideSizes(
    const Bnd_Box2d& box,
    Standard_Integer nbCells,
    Standard_Integer& xN,
    Standard_Integer& yN
)
{
    Standard_Real xMin, yMin, xMax, yMax;
    box.Get(xMin, yMin, xMax, yMax);
    Standard_Real xWidth = xMax - xMin;
    Standard_Real yWidth = yMax - yMin;
    Standard_Real maxWidth = std::max(xWidth, yWidth);
    Standard_Real minWidth = std::min(xWidth, yWidth);
    Standard_Real ratio = maxWidth / minWidth;

    Standard_Integer n1Min = 5;
    Standard_Integer n1Max = 2 * nbCells / n1Min;
    Standard_Integer n1, n2;
    for(Standard_Integer iter = 0; iter < 16; ++iter)
    {
        n1 = (n1Min + n1Max) / 2;
        Standard_Real h1 = maxWidth / n1;
        n2 = RoundToInt(minWidth / h1);
        Standard_Integer n = n1 * n2;

        if(n == nbCells || n1Max - n1Min < 2)
            break;

        if(n > nbCells)
            n1Max = n1;
        else if(n < nbCells)
            n1Min = n1;
    }

    if(xWidth != maxWidth)
        std::swap(n1, n2);
    xN = n1, yN = n2;
}


void GetMeshSideSizes(
    const Bnd_Box& box,
    Standard_Integer nbCells,
    Standard_Integer& xN,
    Standard_Integer& yN,
    Standard_Integer& zN
)
{
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    box.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real width[3] = {
        xMax - xMin,
        yMax - yMin,
        zMax - zMin
    };

    Standard_Integer maxWidthIndex = 0;
    if(width[1] > width[0] && width[1] > width[2])
        maxWidthIndex = 1;
    else if(width[2] > width[0] && width[2] > width[1])
        maxWidthIndex = 2;

    Standard_Integer nMin = 5;
    Standard_Integer nMax = nbCells / nMin / nMin;
    Standard_Integer nMid, nn[3];
    for(Standard_Integer iter = 0; iter < 16; ++iter)
    {
        nMid = (nMin + nMax) / 2;

        Standard_Real h = width[maxWidthIndex] / nMid;

        nn[maxWidthIndex] = nMid;
        if(maxWidthIndex != 0)
            nn[0] = RoundToInt(width[0] / h);
        if(maxWidthIndex != 1)
            nn[1] = RoundToInt(width[1] / h);
        if(maxWidthIndex != 2)
            nn[2] = RoundToInt(width[2] / h);

        Standard_Integer vol = nn[0] * nn[1] * nn[2];

        if(vol == nbCells || nMax - nMin < 2)
            break;

        if(vol > nbCells)
            nMax = nMid;
        else if(vol < nbCells)
            nMin = nMid;
    }

    xN = nn[0], yN = nn[1], zN = nn[2];
}
