#include "numgeom/removefaces.h"

#include <cassert>
#include <list>
#include <map>
#include <queue>
#include <stack>

#include <BOPTools_AlgoTools.hxx>
#include <BOPTools_AlgoTools2D.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepLib.hxx>
#include <BRepLib_MakeEdge.hxx>
#include <BRepLib_MakeFace.hxx>
#include <BRepLib_MakeWire.hxx>
#include <BRepLib_MakeVertex.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom_BoundedCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAPI_IntCS.hxx>
#include <TopExp.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopExp_Explorer.hxx>

#include "numgeom/utilities.h"

#include "ffextintersections.h"
#include "intersections.h"


#ifndef NDEBUG
  #define STATE_DIAGNOSTIC(cond) if(!(cond)) { abort(); }
#else
  #define STATE_DIAGNOSTIC(cond);
#endif
#undef STATE_DUMP


namespace {;

Standard_Boolean Dump(
    const std::string& fileName,
    const TopoDS_Shape& initShape,
    const TopTools_MapOfShape& externalFaces,
    const TopTools_ListOfShape& contour
);


void ExtractBoundaryShapes(
    const TopTools_ListOfShape& faces,
    const TopTools_IndexedDataMapOfShapeListOfShape& verex2edges,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    std::list<TopTools_ListOfShape>& boundaryEdges,
    TopTools_MapOfShape& externalFaces,
    TopTools_IndexedDataMapOfShapeListOfShape& bvertex2eedges
);


TopTools_ListOfShape::iterator GetNextContourPiece(
    const TopTools_ListOfShape::iterator& itCurrent,
    const TopTools_ListOfShape::iterator& itEnd,
    const TopTools_MapOfShape& externalFaces,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    TopoDS_Face& exf
);


std::pair<TopoDS_Vertex,TopoDS_Vertex> GetBoundaryVertices(
    const TopTools_ListOfShape::iterator& itBegin,
    const TopTools_ListOfShape::iterator& itEnd
);


TopoDS_Vertex CreateVertex(const gp_Pnt&);


TopoDS_Edge CreateEdge(
    Handle(Geom_Curve) curve,
    const TopoDS_Vertex& vBeg,
    const TopoDS_Vertex& vEnd
);


void RemoveFaces(
    const TopoDS_Shape& initShape,
    const BOPTools_ConnexityBlock& cblock,
    const TopTools_IndexedDataMapOfShapeListOfShape& vertex2edges,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    TopTools_ListOfShape& visibleFaces
);


TopoDS_Shape CreateResult(
    const TopTools_ListOfShape& visibleFaces
);


TopoDS_Vertex GetBegVertex(const TopoDS_Edge& edge)
{
    return TopExp::FirstVertex(edge, true);
}


TopoDS_Vertex GetEndVertex(const TopoDS_Edge& edge)
{
    return TopExp::LastVertex(edge, true);
}
}


TopoDS_Shape RemoveFaces(
    const TopoDS_Shape& initShape,
    const TopTools_ListOfShape& facesToRemove
)
{
    if(initShape.IsNull())
        return TopoDS_Shape();

    TopTools_IndexedDataMapOfShapeListOfShape vertex2edges;
    TopExp::MapShapesAndAncestors(initShape, TopAbs_VERTEX, TopAbs_EDGE, vertex2edges);

    TopTools_IndexedDataMapOfShapeListOfShape edge2faces;
    TopExp::MapShapesAndAncestors(initShape, TopAbs_EDGE, TopAbs_FACE, edge2faces);

    // Разделение граней на сильно связанные компоненты.
    BOPTools_ListOfConnexityBlock features;
    BOPTools_AlgoTools::MakeConnexityBlocks(facesToRemove, TopAbs_EDGE, TopAbs_FACE, features);

    TopTools_ListOfShape visibleFaces;
    for(const BOPTools_ConnexityBlock& cblock : features)
    {
        RemoveFaces(initShape,
                    cblock,
                    vertex2edges,
                    edge2faces,
                    visibleFaces
        );
    }

    return CreateResult(visibleFaces);
}


namespace {;

struct VisibleContour
{
    /**\struct Piece
    \brief Участок контура восстанавливаемой грани, по которому
           будут созданы недостающие ребра и вершины.
    */
    struct Piece
    {
        Piece(const TopoDS_Face& f, const Handle(Geom_Curve)& c, Standard_Real t)
            : F(f), C(c), tMin(t), tMax(HUGE_VAL) {}
        TopoDS_Face F;
        Handle(Geom_Curve) C;
        // Параметры точек на кривой, где участок контура начинается и заканчивается.
        Standard_Real tMin, tMax;

        gp_Pnt BegPoint() const { return C->Value(tMin); }
        gp_Pnt EndPoint() const { return C->Value(tMax); }
    };

    VisibleContour(bool isClosedContour, const gp_Pnt& ptFinish)
    {
        m_isClosedContour = isClosedContour;
        m_ptFinish = ptFinish;
        solution.reserve(32);
    }

    void Add(const Piece& piece)
    {
        if(!solution.empty())
        {
            Handle(Geom_Curve) preC = solution.back().C;
            Standard_Real u = Project(piece.C->Value(piece.tMin), preC);
            solution.back().tMax = u;
        }
        solution.push_back(piece);
        STATE_DIAGNOSTIC(this->IsValid());
    }

    void Close()
    {
        gp_Pnt pt = m_ptFinish;
        if(m_isClosedContour)
            pt = solution.front().BegPoint();
        solution.back().tMax = Project(pt, solution.back().C);
    }

    Piece PopBack()
    {
        auto p = solution.back();
        solution.pop_back();
        if(!solution.empty())
            solution.back().tMax = HUGE_VAL;
        return p;
    }

    Standard_Boolean IsEmpty() const { return solution.empty(); }

    bool Dump(const std::string& fileName) const;

    std::vector<Piece> solution;
    Standard_Boolean m_isClosedContour;
    gp_Pnt m_ptFinish;

private:

    Standard_Boolean IsValid() const
    {
        if(solution.size() < 2)
            return Standard_True;

        size_t n = solution.size();
        size_t N = n;
        if(!m_isClosedContour || solution.back().tMax == HUGE_VAL)
            n = n - 1;
        for(size_t i = 0; i < n; ++i)
        {
            const Piece& pCur = solution[i];
            const Piece& pNext = solution[(i+1)%N];
            gp_Pnt pt1 = pCur.EndPoint();
            gp_Pnt pt2 = pNext.BegPoint();
            Standard_Real d2 = pt1.SquareDistance(pt2);
            if(d2 > 1.0e-7)
                return Standard_False;
        }
        return Standard_True;
    }

    gp_Pnt Intersect(
        const Handle(Geom_Curve)& C,
        const Handle(Geom_Surface)& S
    ) const
    {
        GeomAPI_IntCS csInt(C,S);
        if(!csInt.IsDone())
            return gp_Pnt(HUGE_VAL,HUGE_VAL,HUGE_VAL);
        return csInt.Point(1);
    }
};


// Создаем восстанавливающую грань по найденному решению.
TopoDS_Face CreateVisibleFace(
    const Handle(Geom_Surface)& sEx,
    bool isClosedContour,
    const VisibleContour& visibleContour,
    const TopTools_ListOfShape::iterator& itc,
    const TopTools_ListOfShape::iterator& itc2
)
{
    TopoDS_Wire innerWire;
    if(isClosedContour)
    {
        BRepLib_MakeWire wireMaker;
        for(auto it = itc; it != itc2; ++it)
            wireMaker.Add(TopoDS::Edge(*it));
        STATE_DIAGNOSTIC(wireMaker.Error() == BRepLib_WireDone);
        innerWire = wireMaker.Wire();
        innerWire.Reverse();
    }

    TopoDS_Wire outerWire;
    {
        BRepLib_MakeWire wireMaker;
        if(!isClosedContour)
        {
            for(auto it = itc; it != itc2; ++it)
                wireMaker.Add(TopoDS::Edge(*it));
        }

        TopoDS_Vertex vStart;
        if(isClosedContour)
        {
            const auto& lastSolPiece = visibleContour.solution.back();
            gp_Pnt pt = lastSolPiece.EndPoint();
            vStart = CreateVertex(pt);
        }
        else
        {
            const TopoDS_Edge& e = TopoDS::Edge(*itc);
            vStart = GetBegVertex(e);
        }

        TopoDS_Vertex vFinish = vStart;
        if(!isClosedContour)
        {
            // Итератор itc2 указывает на следующую позицию, идущую за
            // последним ребром. Итератор не поддерживает декремента, поэтому,
            // чтобы найти последнее ребро, проходим от начала (itc) до конца.
            TopoDS_Edge lastContourEdge;
            STATE_DIAGNOSTIC(itc != itc2);
            auto it = itc;
            while(it != itc2)
            {
                lastContourEdge = TopoDS::Edge(*it);
                ++it;
            }
            vFinish = GetEndVertex(lastContourEdge);
        }

        TopoDS_Vertex vPred = vStart;
        size_t numSolPiece = visibleContour.solution.size();
        for(size_t i = 0; i < numSolPiece; ++i)
        {
            const auto& solPiece = visibleContour.solution[i];
            TopoDS_Vertex vNext;
            if(i != numSolPiece - 1)
                vNext = CreateVertex(solPiece.EndPoint());
            else
                vNext = vFinish;
            TopoDS_Edge edge = CreateEdge(solPiece.C, vPred, vNext);
            STATE_DIAGNOSTIC(!edge.IsNull());
            wireMaker.Add(edge);
            vPred = vNext;
        }

        STATE_DIAGNOSTIC(wireMaker.Error() == BRepLib_WireDone);
        outerWire = wireMaker.Wire();
    }

    BRepLib_MakeFace faceMaker(sEx, outerWire);
    if(!innerWire.IsNull())
        faceMaker.Add(innerWire);
    STATE_DIAGNOSTIC(faceMaker.Error() == BRepLib_FaceDone);
    return faceMaker.Face();
}


bool PointLieOnCurve(const gp_Pnt& P, const Handle(Geom_Curve)& C)
{
    Standard_Real t = Project(P, C);
    return P.SquareDistance(C->Value(t)) <= 1.e-7;
}


TopoDS_Face GetAdjFace(
    const TopoDS_Face& face,
    const TopoDS_Edge& edge,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces
)
{
    STATE_DIAGNOSTIC(!BRep_Tool::Degenerated(edge));
    const TopTools_ListOfShape& faces = edge2faces.FindFromKey(edge);
    STATE_DIAGNOSTIC(faces.Size() == 2);
    const TopoDS_Shape& face1 = faces.First();
    const TopoDS_Shape& face2 = faces.Last();
    if(face1 == face)
        return TopoDS::Face(face2);
    return TopoDS::Face(face1);
}


TopoDS_Face GetNextExternalFace(
    const TopoDS_Face& fEx,
    const TopoDS_Vertex& vertex,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    const TopTools_IndexedDataMapOfShapeListOfShape& bvertex2eedges
)
{
    const auto& externalEdges = bvertex2eedges.FindFromKey(vertex);
    STATE_DIAGNOSTIC(externalEdges.Size() == 1);
    auto externalEdge = TopoDS::Edge(externalEdges.First());
    return GetAdjFace(fEx, externalEdge, edge2faces);
}


Standard_Boolean IsFaceInnerWire(
    const TopoDS_Face& F,
    const VisibleContour& wire
)
{
    Handle(Geom_Surface) S = BRep_Tool::Surface(F);
    for(const auto& s : wire.solution)
    {
        gp_Pnt pt = s.BegPoint();
        gp_Pnt2d uv = Project(pt, S);
        if(OnFace(F,uv))
            return Standard_False;
    }

    for(const auto& s : wire.solution)
    {
        Standard_Real t0 = s.tMin;
        Standard_Real t1 = s.tMax;
        if(t0 > t1)
            std::swap(t0, t1);

        Handle(Geom2d_Curve) C2;
        Standard_Real tol;
        BOPTools_AlgoTools2D::MakePCurveOnFace(F, s.C, C2, tol);
        Handle(Geom2d_BoundedCurve) bC2 = Handle(Geom2d_BoundedCurve)::DownCast(C2);
        if(!bC2)
        {
            Standard_Real u0 = Project(Project(s.C->Value(t0),S), C2);
            Standard_Real u1 = Project(Project(s.C->Value(t1),S), C2);
            bC2 = new Geom2d_TrimmedCurve(C2, u0, u1);
        }
        if(IsIntersected(bC2,F))
            return Standard_False;
    }

    return Standard_True;
}


Standard_Boolean IsValidClosedContour(
    const TopoDS_Face& exFace,
    const VisibleContour& contour
)
{
    return IsFaceInnerWire(exFace, contour);
}


Standard_Boolean IsSelfIntersected(
    const TopoDS_Face& F,
    const VisibleContour& solution
)
{
    // Не пересекается ли последний участок контура с предыдущими?
    const auto& s1 = solution.solution.back();
    Handle(Geom_TrimmedCurve) C1 = new Geom_TrimmedCurve(s1.C, s1.tMin, s1.tMax);
    for(auto it = solution.solution.begin(); it != solution.solution.end() - 1; ++it)
    {
        const auto& s = (*it);
        Handle(Geom_TrimmedCurve) C2 = new Geom_TrimmedCurve(s.C, s.tMin, s.tMax);
        if(IsIntersected(s1.C,s1.tMin,s1.tMax,s.C,s.tMin,s.tMax))
            return Standard_True;
    }

    return Standard_False;
}


//! Затравка для вычисления следующего участка контура: внешняя грань `F` и
//! точка `P` на этой грани, через которые должен пройти участок контура.
struct ContourSeed
{
    TopoDS_Face F;
    gp_Pnt P;
};


//! Структура для поддержки поиска с возвратом.
class SearchStack
{
public:

    SearchStack() {}

    void AddNextState()
    {
        stack.push({});
    }

    void RemoveState()
    {
        assert(stack.top().empty());
        stack.pop();
    }

    bool IsStateEmpty() const { return stack.top().empty(); }

    bool IsEmpty() const { return stack.empty(); }

    void Append(const ContourSeed& seed)
    {
        stack.top().push(seed);
    }

    ContourSeed PopSeed()
    {
        auto& seeds = stack.top();
        assert(!seeds.empty());
        auto s = seeds.front();
        seeds.pop();
        return s;
    }

    bool HasSingleState() const { return stack.size() == 1; }

private:
    std::stack<std::queue<ContourSeed>> stack;
};


//! Воспроизводим восстанавливающую грань.
TopoDS_Face RecoverVisibleFace(
    const TopoDS_Shape& initShape,
    const TopoDS_Face& fEx,
    const TopTools_MapOfShape& externalFaces,
    const FaceFaceIntersections& ffIntersections,
    const TopTools_ListOfShape& contour,
    const TopTools_ListOfShape::iterator& itc,
    const TopTools_ListOfShape::iterator& itc2,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    const TopTools_IndexedDataMapOfShapeListOfShape& bvertex2eedges
)
{
    Handle(Geom_Surface) sEx = BRep_Tool::Surface(fEx);
    bool isClosedContour = (itc == contour.begin() && itc2 == contour.end());
    TopoDS_Vertex vBeg, vEnd;
    if(!isClosedContour)
        std::tie(vBeg,vEnd) = GetBoundaryVertices(itc, itc2);

    SearchStack sStack;
    VisibleContour solution(isClosedContour, isClosedContour? gp_Pnt() : BRep_Tool::Pnt(vEnd));

    if(isClosedContour)
    {
        // Вычисляем внешние грани, пересекающиеся с `fEx`.
        sStack.AddNextState();
        for(auto ffInt : ffIntersections.GetIntersections(fEx))
        {
            for(auto cfInt : ffIntersections.GetIntersections(ffInt.C))
            {
                if(cfInt.F == fEx || cfInt.F == ffInt.F)
                    continue;
                gp_Pnt P = ffInt.C->Value(cfInt.t);
                sStack.Append({cfInt.F, P});
            }
        }

        if(sStack.IsStateEmpty())
        {
            sStack.RemoveState();
            return TopoDS_Face();
        }
    }
    else
    {
        const gp_Pnt& p = BRep_Tool::Pnt(vBeg);
        sStack.AddNextState();
        TopoDS_Face fexStart = GetNextExternalFace(fEx, vBeg, edge2faces, bvertex2eedges);
        STATE_DIAGNOSTIC(!fexStart.IsNull() && fEx != fexStart);
        sStack.Append({ContourSeed{fexStart, p}});
    }

    TopoDS_Face fexFinish;
    if(!isClosedContour)
    {
        fexFinish = GetNextExternalFace(fEx, vEnd, edge2faces, bvertex2eedges);
        STATE_DIAGNOSTIC(!fexFinish.IsNull() && fEx != fexFinish);
    }

#ifdef STATE_DUMP
    Dump("init.vtk", initShape, externalFaces, contour);
#endif

    // Вычисляем контур восстанавливающей грани, которая есть продолжение `fEx`.
    TopTools_MapOfShape exfsProcessed;
    exfsProcessed.Add(fEx);
    Standard_Integer iterationCount = 0;
    while(!sStack.IsEmpty())
    {
        STATE_DIAGNOSTIC(!sStack.HasSingleState() || solution.IsEmpty());

        if(sStack.IsStateEmpty())
        {
            // Откатываемся на один шаг назад.
            sStack.RemoveState();
            if(!solution.IsEmpty())
            {
                auto sol = solution.PopBack();
                exfsProcessed.Remove(sol.F);
            }
            continue;
        }

        ContourSeed seed = sStack.PopSeed();

        if(!solution.IsEmpty())
        {
            auto& s = solution.solution.back();
            Standard_Real tMax = Project(seed.P, s.C);
            s.tMax = tMax;
            if(IsSelfIntersected(fEx,solution))
            {
                s.tMax = HUGE_VAL;
                continue;
            }
            s.tMax = HUGE_VAL;
        }

        // Если поиск начинается с самого начала для замкнутного контура, то
        // настраиваем параметры условия выхода: а именно грань `fexFinish`.
        if(isClosedContour && solution.IsEmpty())
            fexFinish = seed.F;

        // Ищем кривую пересечения граней `fEx` и `seed.F`,
        // проходящую через точку `seed.P`.
        // Такая кривая может быть только одна.
        Handle(Geom_Curve) ffCurve;
        for(const auto& ff : ffIntersections.GetIntersections(fEx))
        {
            if(ff.F == seed.F && PointLieOnCurve(seed.P,ff.C))
            {
                ffCurve = ff.C;
                break;
            }
        }

        // Если не нашли кривую пересечения, то откатываемся назад.
        if(ffCurve.IsNull())
            continue;

        bool emptySolution = solution.IsEmpty();

        // Extend solution.
        Standard_Real uSeedPoint = Project(seed.P, ffCurve);
        VisibleContour::Piece sol(seed.F, ffCurve, uSeedPoint);
        solution.Add(sol);

#ifdef STATE_DUMP
        std::stringstream ss;
        ss << "solution-" << iterationCount++ << ".vtk";
        solution.Dump(ss.str());
#endif

        // Условие успешного выхода из поиска.
        if(!emptySolution && seed.F == fexFinish)
        {
            if(isClosedContour)
                solution.PopBack();

            solution.Close();
            if(isClosedContour)
            {
                if(IsValidClosedContour(fEx,solution))
                    break;
            }
            else
            {
                break;
            }
            continue;
        }

        // Подготавливаем состояния на следующий шаг:
        // ищем пересечение кривой `ffCurve` с внешними гранями.
        sStack.AddNextState();
        for(const auto& ints : ffIntersections.GetIntersections(ffCurve))
        {
            if(ints.F == seed.F)
                continue;

            gp_Pnt P = ffCurve->Value(ints.t);
            if(seed.P.Distance(P) <= 1.e-7)
                continue;

            if(exfsProcessed.Contains(ints.F))
                continue;

            sStack.Append({ints.F,P});
        }

        if(sStack.IsStateEmpty())
            continue;

        if(seed.F != fexFinish)
            exfsProcessed.Add(seed.F);
    }

    if(solution.IsEmpty())
        return TopoDS_Face();

    // Создаем восстанавливающую грань по найденному решению.
    TopoDS_Face fSol = CreateVisibleFace(sEx,
                                         isClosedContour,
                                         solution,
                                         itc,
                                         itc2
    );
    return fSol;
}


void RemoveFaces(
    const TopoDS_Shape& initShape,
    const BOPTools_ConnexityBlock& cblock,
    const TopTools_IndexedDataMapOfShapeListOfShape& vertex2edges,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    TopTools_ListOfShape& visibleFaces
)
{
    std::list<TopTools_ListOfShape> contours;
    TopTools_MapOfShape externalFaces;
    TopTools_IndexedDataMapOfShapeListOfShape bvertex2eedges;
    ExtractBoundaryShapes(cblock.Shapes(),
                          vertex2edges,
                          edge2faces,
                          contours,
                          externalFaces,
                          bvertex2eedges);

    FaceFaceIntersections ffIntersections(externalFaces);

    for(const auto& contour : contours)
    {
        auto itc = contour.begin();
        while(itc != contour.end())
        {
            TopoDS_Face fEx;
            auto itc2 = GetNextContourPiece(itc, contour.end(), externalFaces, edge2faces, fEx);
            STATE_DIAGNOSTIC(!fEx.IsNull());
            STATE_DIAGNOSTIC(itc != itc2);

            TopoDS_Face slf = RecoverVisibleFace(initShape,
                                                 fEx,
                                                 externalFaces,
                                                 ffIntersections,
                                                 contour,
                                                 itc,
                                                 itc2,
                                                 edge2faces,
                                                 bvertex2eedges);
            STATE_DIAGNOSTIC(!slf.IsNull());
            visibleFaces.Append(slf);
            itc = itc2;
        }
    }
}


bool IsBoundary(
    const TopoDS_Edge& edge,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    const TopTools_ListOfShape& faces,
    TopoDS_Face& externalFace
)
{
    const auto& adjFaces = edge2faces.FindFromKey(edge);
    STATE_DIAGNOSTIC(adjFaces.Size() == 2);
    const auto& firstFace = adjFaces.First();
    const auto& secondFace = adjFaces.Last();
    if(!faces.Contains(firstFace))
        externalFace = TopoDS::Face(firstFace);
    else if(!faces.Contains(secondFace))
        externalFace = TopoDS::Face(secondFace);
    return !externalFace.IsNull();
}


TopoDS_Edge GetNextEdge(const TopoDS_Face& face, const TopoDS_Edge& prevEdge)
{
    for(TopExp_Explorer itw(face,TopAbs_WIRE); itw.More(); itw.Next())
    {
        const TopoDS_Wire& wire = TopoDS::Wire(itw.Value());
        for(BRepTools_WireExplorer ite(wire); ite.More(); ite.Next())
        {
            const TopoDS_Edge& edge = TopoDS::Edge(ite.Current());
            if(BRep_Tool::Degenerated(edge))
                continue;

            if(prevEdge == edge)
            {
                while(true)
                {
                    ite.Next();
                    if(!ite.More())
                        ite.Init(wire);
                    TopoDS_Edge eNext = TopoDS::Edge(ite.Current());
                    if(!BRep_Tool::Degenerated(eNext))
                        return eNext;
                }
            }
        }
    }
    return TopoDS_Edge();
}


TopoDS_Edge GetOutgoingEdge(
    const TopoDS_Face& face,
    const TopoDS_Vertex& vertex
)
{
    for(TopExp_Explorer itw(face,TopAbs_WIRE); itw.More(); itw.Next())
    {
        const TopoDS_Wire& wire = TopoDS::Wire(itw.Value());
        for(BRepTools_WireExplorer ite(wire); ite.More(); ite.Next())
        {
            const TopoDS_Edge& edge = ite.Current();
            if(BRep_Tool::Degenerated(edge))
                continue;
            if(vertex.IsSame(GetBegVertex(edge)))
                return edge;
        }
    }
    return TopoDS_Edge();
}


void Rotate(TopoDS_ListOfShape& list, int n)
{
    int i = 0;
    while(i++ != n)
    {
        TopoDS_Shape el = list.First();
        list.RemoveFirst();
        list.Append(el);
    }
}


TopoDS_Vertex FirstVertexFromContour(const TopTools_ListOfShape& contour)
{
    TopoDS_Edge firstEdge = TopoDS::Edge(contour.First());
    return GetBegVertex(firstEdge);
}


Standard_Boolean CheckContours(
    const std::list<TopTools_ListOfShape>& contours,
    const TopTools_MapOfShape& boundaryEdges,
    const TopTools_IndexedDataMapOfShapeListOfShape& bvertex2eedges
)
{
    for(const TopTools_ListOfShape& contour : contours)
    {
        if(contour.Size() == 1)
        {
            TopoDS_Edge edge = TopoDS::Edge(contour.First());
            if(IsClosed(edge))
                return Standard_False;
            continue;
        }

        TopoDS_Vertex vPre = GetEndVertex(TopoDS::Edge(contour.Last()));
        for(const TopoDS_Shape& E : contour)
        {
            TopoDS_Edge edge = TopoDS::Edge(E);
            TopoDS_Vertex vBeg = GetBegVertex(edge);
            TopoDS_Vertex vEnd = GetEndVertex(edge);
            if(!vBeg.IsSame(vPre))
                return Standard_False;
            vPre = vEnd;
        }
    }

    // Контуры должны состоять только из граничных ребер.
    TopTools_MapOfShape contourEdges;
    for(const TopTools_ListOfShape& contour : contours)
    {
        for(const TopoDS_Shape& edge : contour)
            contourEdges.Add(edge);
    }
    if(    !boundaryEdges.Contains(contourEdges)
        || !contourEdges.Contains(boundaryEdges))
        return Standard_False;

    for(const TopTools_ListOfShape& contour : contours)
    {
        TopoDS_Vertex vFirst = FirstVertexFromContour(contour);
        if(bvertex2eedges.Contains(vFirst))
            continue;

        // Если первая вершина контура не содержит внешних ребер, то
        // остальные узлы контура также не должны содержать внешние ребра.
        auto it = contour.cbegin();
        for(++it; it != contour.cend(); ++it)
        {
            TopoDS_Edge e = TopoDS::Edge(*it);
            TopoDS_Vertex v = GetBegVertex(e);
            if(bvertex2eedges.Contains(v))
                return Standard_False;
        }
    }

    return Standard_True;
}


Standard_Boolean Joinable(
    const TopTools_ListOfShape& contour,
    const TopoDS_Edge& nextEdge
)
{
    if(contour.IsEmpty())
        return Standard_True;
    TopoDS_Edge lastEdge = TopoDS::Edge(contour.Last());
    TopoDS_Vertex lastVertex = GetEndVertex(lastEdge);
    TopoDS_Vertex nextVertex = GetBegVertex(nextEdge);
    return lastVertex.IsSame(nextVertex);
}


Standard_Boolean ComesFrom(const TopoDS_Edge& E, const TopoDS_Vertex& V)
{
    return GetBegVertex(E).IsSame(V);
}

void GetAdjacentEntities(
    const TopTools_ListOfShape& inputFaces,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    TopTools_MapOfShape& boundaryEdges,
    TopTools_MapOfShape& internalEdges,
    TopTools_MapOfShape& externalFaces
)
{
    for(const auto& face : inputFaces)
    {
        for(TopExp_Explorer itw(face,TopAbs_WIRE); itw.More(); itw.Next())
        {
            const TopoDS_Wire& wire = TopoDS::Wire(itw.Value());
            for(BRepTools_WireExplorer ite(wire); ite.More(); ite.Next())
            {
                const TopoDS_Edge& edge = ite.Current();
                if(BRep_Tool::Degenerated(edge))
                    continue;

                TopoDS_Face externalFace;
                if(IsBoundary(edge,edge2faces,inputFaces,externalFace))
                {
                    boundaryEdges.Add(edge);
                    externalFaces.Add(externalFace);
                }
                else
                {
                    internalEdges.Add(edge);
                }
            }
        }
    }
}


void ExtractBoundaryShapes(
    const TopTools_ListOfShape& inputFaces,
    const TopTools_IndexedDataMapOfShapeListOfShape& verex2edges,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    std::list<TopTools_ListOfShape>& contours,
    TopTools_MapOfShape& externalFaces,
    TopTools_IndexedDataMapOfShapeListOfShape& bvertex2eedges
)
{
    // Вычисляем внешние и внутренние ребра связанных граней `inputFaces`.
    TopTools_MapOfShape boundaryEdges, internalEdges;
    GetAdjacentEntities(inputFaces,
                        edge2faces,
                        boundaryEdges,
                        internalEdges,
                        externalFaces);

    // Вычисляем граничные и внешние ребра и грани.
    TopTools_MapOfShape nonProcessedFaces;
    TopTools_MapOfShape processedBoundaryEdges;
    for(const auto& f : inputFaces)
        nonProcessedFaces.Add(f);
    while(!nonProcessedFaces.IsEmpty())
    {
        TopoDS_Face cFace = TopoDS::Face(*nonProcessedFaces.cbegin());
        TopoDS_Edge cEdge;
        for(TopExp_Explorer it(cFace,TopAbs_EDGE); it.More(); it.Next())
        {
            const TopoDS_Edge& edge = TopoDS::Edge(it.Value());
            if(!boundaryEdges.Contains(edge))
                continue;

            if(processedBoundaryEdges.Contains(edge))
                continue;

            cEdge = edge;
            break;
        }
        if(cEdge.IsNull())
        {
            nonProcessedFaces.Remove(cFace);
            continue;
        }

        TopTools_ListOfShape contour;
        while(true)
        {
            if(!contour.IsEmpty() && contour.First() == cEdge)
            {
                // Контур замкнулся.
                contours.push_back(contour);
                break;
            }

            STATE_DIAGNOSTIC(!BRep_Tool::Degenerated(cEdge));
            STATE_DIAGNOSTIC(Joinable(contour,cEdge));

            contour.Append(cEdge);
            processedBoundaryEdges.Add(cEdge);

            if(IsClosed(cEdge))
                continue;

            TopoDS_Vertex boundaryVertex = GetEndVertex(cEdge);
            TopoDS_Edge nextEdge = GetNextEdge(cFace, cEdge);
            TopoDS_Face nextFace = cFace;
            if(!boundaryEdges.Contains(nextEdge))
            {
                do
                {
                    nextFace = GetAdjFace(nextFace, nextEdge, edge2faces);
                    nextEdge = GetOutgoingEdge(nextFace, boundaryVertex);
                }
                while(!boundaryEdges.Contains(nextEdge));
            }
            STATE_DIAGNOSTIC(ComesFrom(nextEdge,boundaryVertex));
            cEdge = nextEdge;
            cFace = nextFace;
        }
    }

    // Вычисляем внешние ребра.
    for(const TopTools_ListOfShape& contour : contours)
    {
        for(const TopoDS_Shape& edge : contour)
        {
            TopoDS_Vertex boundaryVertex = GetEndVertex(TopoDS::Edge(edge));

            // Для граничной вершины вычисляем внешние ребра.
            const auto& edges = verex2edges.FindFromKey(boundaryVertex);
            TopTools_ListOfShape externalEdges;
            for(auto it = edges.begin(); it != edges.end(); ++it)
            {
                const TopoDS_Edge& e = TopoDS::Edge(*it);

                if(BRep_Tool::Degenerated(e))
                    continue;

                if(boundaryEdges.Contains(e) || internalEdges.Contains(e))
                    continue;

                TopoDS_Edge eExternal = TopoDS::Edge(e);
                if(!GetEndVertex(eExternal).IsSame(boundaryVertex))
                    eExternal.Reverse();
                if(!externalEdges.Contains(eExternal))
                    externalEdges.Append(eExternal);
            }
            if(!externalEdges.IsEmpty())
                bvertex2eedges.Add(boundaryVertex, externalEdges);
        }
    }

    for(TopTools_ListOfShape& contour : contours)
    {
        // Правим контур так, чтобы первая
        // вершина контура была связана с внешними ребрами.
        int n = 0;
        bool has = false;
        for(TopTools_ListOfShape::Iterator it(contour); it.More(); it.Next(), ++n)
        {
            const TopoDS_Edge& e = TopoDS::Edge(it.Value());
            const TopoDS_Vertex& v = GetBegVertex(e);
            if(bvertex2eedges.Contains(v))
            {
                has = true;
                break;
            }
        }

        if(n != 0 && contour.Size() != n)
            Rotate(contour, n);
    }

    STATE_DIAGNOSTIC(
        CheckContours(contours,
                      boundaryEdges,
                      bvertex2eedges)
    );
}


TopTools_ListOfShape::iterator GetNextContourPiece(
    const TopTools_ListOfShape::iterator& itContourStart,
    const TopTools_ListOfShape::iterator& itContourEnd,
    const TopTools_MapOfShape& externalFaces,
    const TopTools_IndexedDataMapOfShapeListOfShape& edge2faces,
    TopoDS_Face& externalFace
)
{
    auto itContour = itContourStart;
    bool firstIteration = true;
    while(itContour != itContourEnd)
    {
        const auto& e = (*itContour);
        const auto& faces = edge2faces.FindFromKey(e);
        STATE_DIAGNOSTIC(faces.Size() == 2);
        TopoDS_Face exf = TopoDS::Face(faces.First());
        if(!externalFaces.Contains(exf))
        {
            exf = TopoDS::Face(faces.Last());
            STATE_DIAGNOSTIC(externalFaces.Contains(exf));
        }

        if(firstIteration)
        {
            externalFace = exf;
            firstIteration = false;
        }

        if(externalFace != exf)
            break;

        ++itContour;
    }
    return itContour;
}


std::pair<TopoDS_Vertex,TopoDS_Vertex> GetBoundaryVertices(
    const TopTools_ListOfShape::iterator& itBegin,
    const TopTools_ListOfShape::iterator& itEnd
)
{
    TopoDS_Edge firstEdge = TopoDS::Edge(*itBegin);
    TopoDS_Vertex vBeg = GetBegVertex(firstEdge);

    auto itLast = itBegin;
    for(auto it = itBegin; it != itEnd; ++it)
        itLast = it;

    TopoDS_Edge lastEdge = TopoDS::Edge(*itLast);
    TopoDS_Vertex vEnd = GetEndVertex(lastEdge);

    return std::make_pair(vBeg, vEnd);
}


TopoDS_Edge CreateEdge(
    Handle(Geom_Curve) curve,
    const TopoDS_Vertex& vBeg,
    const TopoDS_Vertex& vEnd
)
{
    BRepLib_MakeEdge edgeMaker(curve, vBeg, vEnd);
    STATE_DIAGNOSTIC(edgeMaker.Error() == BRepLib_EdgeDone);
    return edgeMaker.Edge();
}


TopoDS_Vertex CreateVertex(const gp_Pnt& pnt)
{
    return BRepLib_MakeVertex(pnt).Vertex();
}


TopoDS_Shape CreateResult(
    const TopTools_ListOfShape& visibleFaces
)
{
    TopoDS_Compound result;
    BRep_Builder builder;
    builder.MakeCompound(result);
    for(const TopoDS_Shape& face : visibleFaces)
        builder.Add(result, face);
    return result;
}


Standard_Boolean Dump(
    const std::string& fileName,
    const TopoDS_Shape& initShape,
    const TopTools_MapOfShape& externalFaces,
    const TopTools_ListOfShape& contour
)
{
    std::ofstream file(fileName);
    if(!file.is_open())
        return Standard_False;

    TopTools_IndexedMapOfShape allFaces;
    TopExp::MapShapes(initShape, TopAbs_FACE, allFaces);
    std::unordered_map<TopoDS_Shape, std::pair<Handle(Poly_Triangulation),TopLoc_Location>> face2trng;
    for(Standard_Integer i = 1; i <= allFaces.Size(); ++i)
    {
        const TopoDS_Face& F = TopoDS::Face(allFaces.FindKey(i));
        TopLoc_Location loc;
        auto trng = BRep_Tool::Triangulation(F, loc);
        if(!trng)
        {
            BRepMesh_IncrementalMesh(F, 0.6f);
            trng = BRep_Tool::Triangulation(F, loc);
        }
        assert(!trng.IsNull());
        face2trng.insert(std::make_pair(F,std::make_pair(trng,loc)));
    }

    TopTools_IndexedMapOfShape allEdges;
    TopExp::MapShapes(initShape, TopAbs_EDGE, allEdges);
    std::unordered_map<TopoDS_Shape, Poly_ArrayOfNodes> edge2trng;
    for(Standard_Integer i = 1; i <= allEdges.Size(); ++i)
    {
        const TopoDS_Edge& E = TopoDS::Edge(allEdges.FindKey(i));
        Poly_ArrayOfNodes& nodes = edge2trng[E];
        if(BRep_Tool::Degenerated(E))
            continue;
        Standard_Integer N = 50;
        nodes.Resize(N, Standard_False);
        Standard_Real u0, u1;
        Handle(Geom_Curve) C = BRep_Tool::Curve(E, u0, u1);
        for(Standard_Integer i = 0; i < N; ++i)
        {
            Standard_Real u = u0 + (u1 - u0) / (N-1) * i;
            gp_Pnt pt = C->Value(u);
            nodes.SetValue(i, pt);
        }
    }

    TopTools_IndexedMapOfShape allVertices;
    TopExp::MapShapes(initShape, TopAbs_VERTEX, allVertices);

    Standard_Integer nbNodes = 0;
    Standard_Integer nbTrias = 0;
    Standard_Integer nbLines = 0;
    Standard_Integer nbVerts = allVertices.Size();
    for(const auto& v : face2trng)
    {
        nbNodes += v.second.first->NbNodes();
        nbTrias += v.second.first->NbTriangles();
    }
    for(const auto& v : edge2trng)
    {
        if(v.second.Size() == 0)
            continue;
        nbNodes += v.second.Size();
        nbLines += (v.second.Size() - 1);
    }
    nbNodes += nbVerts;
    Standard_Integer nbCells = nbTrias + nbLines + nbVerts;

    file << "# vtk DataFile Version 2.0" << std::endl;
    file << "My polydata" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;

    file << "POINTS " << nbNodes << " double" << std::endl;
    for(auto it = allFaces.cbegin(); it != allFaces.cend(); ++it)
    {
        Handle(Poly_Triangulation) trng;
        TopLoc_Location loc;
        std::tie(trng, loc) = face2trng.at(*it);
        for(Standard_Integer i = 1; i <= trng->NbNodes(); ++i)
        {
            gp_Pnt pt = trng->Node(i);
            file << pt.X() << ' ' << pt.Y() << ' ' << pt.Z() << ' ';
        }
        file << std::endl;
    }
    for(auto it = allEdges.cbegin(); it != allEdges.cend(); ++it)
    {
        const Poly_ArrayOfNodes& polyline = edge2trng.at(*it);
        for(Standard_Integer i = 0; i < polyline.Size(); ++i)
        {
            gp_Pnt pt = polyline.Value(i);
            file << pt.X() << ' ' << pt.Y() << ' ' << pt.Z() << ' ';
        }
        file << std::endl;
    }
    for(auto it = allVertices.cbegin(); it != allVertices.cend(); ++it)
    {
        const TopoDS_Vertex& V = TopoDS::Vertex(*it);
        gp_Pnt pt = BRep_Tool::Pnt(V);
        file << pt.X() << ' ' << pt.Y() << ' ' << pt.Z() << ' ';
    }
    file << std::endl << std::endl;

    file << "CELLS " << nbCells << ' ' << 4 * nbTrias + 3 * nbLines + 2 * nbVerts << std::endl;
    Standard_Integer offset = 0;
    for(auto it = allFaces.cbegin(); it != allFaces.cend(); ++it)
    {
        Handle(Poly_Triangulation) trng = face2trng.at(*it).first;
        Standard_Integer nbTrias = trng->NbTriangles();
        for(Standard_Integer i = 1; i <= nbTrias; ++i)
        {
            const auto& tr = trng->Triangle(i);
            file << "3 "
                 << tr.Value(1) - 1 + offset << ' '
                 << tr.Value(2) - 1 + offset << ' '
                 << tr.Value(3) - 1 + offset << ' ';
        }
        file << std::endl;
        offset += trng->NbNodes();
    }
    file << std::endl;

    for(auto it = allEdges.cbegin(); it != allEdges.cend(); ++it)
    {
        const Poly_ArrayOfNodes& polyline = edge2trng.at(*it);
        Standard_Integer nbLines = polyline.Size() - 1;
        for(Standard_Integer i = 0; i < nbLines; ++i)
        {
            file << "2 "
                 << i + 0 + offset << ' '
                 << i + 1 + offset << ' ';
        }
        offset += polyline.Size();
    }
    file << std::endl;

    for(Standard_Integer i = 0; i < nbVerts; ++i)
        file << "1 " << offset++ << ' ';
    file << std::endl;

    file << "CELL_TYPES " << nbCells << std::endl;
    for(Standard_Integer i = 0; i < nbTrias; ++i)
        file << "5 ";
    file << std::endl;
    for(Standard_Integer i = 0; i < nbLines; ++i)
        file << "3 ";
    for(Standard_Integer i = 0; i < nbVerts; ++i)
        file << "1 ";
    file << std::endl;

    file << "CELL_DATA " << nbCells << std::endl;
    file << "SCALARS shapes int 1" << std::endl;
    file << "LOOKUP_TABLE default" << std::endl;
    Standard_Integer id = 0;
    for(auto it = allFaces.cbegin(); it != allFaces.cend(); ++it)
    {
        Handle(Poly_Triangulation) trng = face2trng.at(*it).first;
        Standard_Integer N = trng->NbTriangles();
        for(Standard_Integer i = 0; i < N; ++i)
            file << id << ' ';
        ++id;
        file << std::endl;
    }
    for(auto it = allEdges.cbegin(); it != allEdges.cend(); ++it)
    {
        const Poly_ArrayOfNodes& polyline = edge2trng.at(*it);
        Standard_Integer nbLines = polyline.Size() - 1;
        for(Standard_Integer i = 0; i < nbLines; ++i)
            file << id << ' ';
        ++id;
        file << std::endl;
    }
    for(Standard_Integer i = 0; i < nbVerts; ++i)
        file << id << ' ';
    file << std::endl;
    file << std::endl;

    file << "SCALARS externalFaces char 1" << std::endl;
    file << "LOOKUP_TABLE default" << std::endl;
    for(auto it = allFaces.cbegin(); it != allFaces.cend(); ++it)
    {
        const auto& F = (*it);
        char ch = (externalFaces.Contains(F) ? '1' : '0');
        Handle(Poly_Triangulation) trng = face2trng.at(F).first;
        Standard_Integer N = trng->NbTriangles();
        for(Standard_Integer i = 1; i <= N; ++i)
            file << ch << ' ';
    }
    for(Standard_Integer i = 0; i < nbLines + nbVerts; ++i)
        file << '0' << ' ';
    file << std::endl;

    file << "SCALARS contour char 1" << std::endl;
    file << "LOOKUP_TABLE default" << std::endl;
    for(Standard_Integer i = 0; i < nbTrias; ++i)
        file << '0' << ' ';
    for(auto it = allEdges.cbegin(); it != allEdges.cend(); ++it)
    {
        const auto& E = (*it);
        char ch = (contour.Contains(E) || contour.Contains(E.Reversed()) ? '1' : '0');
        const Poly_ArrayOfNodes& polyline = edge2trng.at(E);
        Standard_Integer nbLines = polyline.Size() - 1;
        for(Standard_Integer i = 0; i < nbLines; ++i)
            file << ch << ' ';
    }
    for(Standard_Integer i = 0; i < nbVerts; ++i)
        file << '0' << ' ';

    file.close();
    return Standard_True;
}


bool VisibleContour::Dump(const std::string& fileName) const
{
    std::ofstream file(fileName);
    if(!file.is_open())
        return Standard_False;

    file << "# vtk DataFile Version 2.0" << std::endl;
    file << "My polydata" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;

    Standard_Integer N = 50;
    Standard_Integer nbNodes = N * this->solution.size();
    file << "POINTS " << nbNodes << " double" << std::endl;
    for(const auto& s : this->solution)
    {
        Standard_Real tNext = s.tMax;
        if(s.tMax == HUGE_VAL)
        {
            auto boundedCurve = Handle(Geom_BoundedCurve)::DownCast(s.C);
            if(boundedCurve)
                tNext = boundedCurve->LastParameter();
            else
            {
                Standard_Real length = 33.0;
                GCPnts_AbscissaPoint abscissaPoint(GeomAdaptor_Curve(s.C), length, s.tMin);
                tNext = abscissaPoint.Parameter();
            }
        }

        for(Standard_Integer i = 0; i < N; ++i)
        {
            Standard_Real t = s.tMin + (tNext - s.tMin) / (N-1) * i;
            gp_Pnt pt = s.C->Value(t);
            file << pt.X() << ' ' << pt.Y() << ' ' << pt.Z() << ' ';
        }
    }

    Standard_Integer nbCells = (N - 1) * this->solution.size();
    file << "CELLS " << nbCells << ' ' << 3 * nbCells << std::endl;
    Standard_Integer offset = 0;
    for(Standard_Integer i = 0; i < this->solution.size(); ++i)
    {
        for(Standard_Integer j = 0; j < N - 1; ++j)
        {
            file << "2 "<< offset << ' ' << offset + 1 << ' ';
            ++offset;
        }
    }

    file << "CELL_TYPES " << nbCells << std::endl;
    for(Standard_Integer i = 0; i < nbCells; ++i)
        file << '3' << ' ';

    return true;
}
}
