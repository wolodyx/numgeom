#include "numgeom/classificate.h"

#include <map>

#include <Bnd_Box2d.hxx>
#include <BRepTools.hxx>
#include <Geom2d_Curve.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Wire.hxx>

#include "numgeom/circularlist.h"
#include "numgeom/quadtree.h"
#include "numgeom/tesselate.h"
#include "numgeom/utilities.h"

#include "writevtk.h"


namespace {;

enum PositionType
{
    OuterPosition,
    InnerPosition,
    BoundaryPosition
};


struct ContourPoint
{
    gp_Pnt2d p;
};


class FaceContours
{
public:

    FaceContours(const TopoDS_Face&);

    Standard_Boolean Dump(const std::filesystem::path&) const;

public:
    TopoDS_Face myFace;
    std::vector<CircularList<ContourPoint>> myWires;
    std::vector<
        std::tuple<
            Standard_Real,
            Standard_Real,
            Standard_Real,
            Standard_Real
        >
    > myBoundBoxes;
};


class MeshLinesAndContoursIntersections
{
public:

    struct Cross
    {
        //! Координата точки пересечения сеточной линии с контуром.
        Standard_Real u;
        //! Начало сегмента контура, пересекающего сеточную линию.
        const CircularList<ContourPoint>::Node* seg;
    };

    mutable std::map<size_t,std::vector<Cross>> vInts, hInts;

public:
    MeshLinesAndContoursIntersections(
        const QuadTree&,
        const FaceContours&,
        Standard_Integer maxLevelsNumber
    );

    Standard_Integer levelIndex() const { return myLevelIndex; }

    const std::vector<Cross>& GetVInts(size_t) const;
    const std::vector<Cross>& GetHInts(size_t) const;

private:

    //! Intersect contours by vertical segment.
    //! Vertical segment ((xConst,yMin),(xConst,yMax)).
    void IntersectVertical(Standard_Real xConst, std::vector<Cross>&) const;

    void IntersectHorizontal(Standard_Real yConst, std::vector<Cross>&) const;

private:
    const FaceContours& myContours;
    Standard_Real xMin, yMin, xMax, yMax;
    Standard_Integer nbVLines, nbHLines;
    Standard_Integer myLevelIndex;
};


void Enlarge(Bnd_Box2d&, Standard_Real);


void Quadratize(Bnd_Box2d&);


void CalcLineCoefficients(
    const gp_Pnt2d&,
    const gp_Pnt2d&,
    Standard_Real& A,
    Standard_Real& B,
    Standard_Real& C
);


Standard_Boolean PointInsideBox(
    const gp_Pnt2d& P,
    Standard_Real xMin,
    Standard_Real yMin,
    Standard_Real xMax,
    Standard_Real yMax
)
{
    Standard_Real xP = P.X();
    Standard_Real yP = P.Y();
    return xP > xMin && xP < xMax && yP > yMin && yP < yMax;
}


Standard_Boolean ContourLieInnerCell(
    const FaceContours&,
    Standard_Real xMin,
    Standard_Real yMin,
    Standard_Real xMax,
    Standard_Real yMax
);


PositionType ClassificateCell(
    const QuadTree& qTree,
    const QuadTree::Cell& cell,
    const FaceContours& fContours,
    const MeshLinesAndContoursIntersections& mlacInts,
    const CircularList<ContourPoint>::Node** attr
);
}


class AuxClassificateData2d
{
public:

    AuxClassificateData2d(const TopoDS_Face&, Standard_Real tol);
    ~AuxClassificateData2d();

    const TopoDS_Face face;
    QuadTree::Ptr qTree;
    FaceContours fContours;
    Standard_Real tolerance;

private:
    AuxClassificateData2d(const AuxClassificateData2d&) = delete;
    void operator=(const AuxClassificateData2d&) = delete;
};


Standard_Boolean IsBoundaryCell(
    QuadTree::CPtr qTree,
    const QuadTree::Cell& cell
)
{
    size_t attr = qTree->GetAttr(cell);
    return attr != 0 && attr != 1;
}


/**
\brief Наименьшее расстояние от точки до участка контура.
\warning НЕ РЕАЛИЗОВАНО! В описании контура не хватает указателя на кривую и
         параметров в точках дискретизации.

Голованов Н.Н., Геометрическое моделирование / 2002.
Раздел 4.5 "Проекция точки на линию".
*/
Standard_Real NearestDistance(
    const gp_Pnt2d& p,
    const CircularList<ContourPoint>::Node* cptr
)
{
    // Ищем начальное приближение для вычисления проекции.
    const CircularList<ContourPoint>::Node* c1ptr = nullptr;
    const CircularList<ContourPoint>::Node* c2ptr = nullptr;
    Handle(Geom2d_Curve) curve;
    Standard_Real tA = 0.0, tB = 0.0;
    for(Standard_Integer i = 0; i < 6; ++i)
    {
        Standard_Integer i1 = (i % 2 == 0 ? i/2 : -i/2-1);
        Standard_Integer i2 = (i % 2 == 0 ? i/2+1 : -i/2);
        auto n1 = CircularList<ContourPoint>::Offset(cptr, i1);
        auto n2 = CircularList<ContourPoint>::Offset(cptr, i2);
        //tA = n1->data.t;
        //tB = n2->data.t;
        gp_Pnt2d pA, pB;
        gp_Vec2d dA, dB;
        curve->D1(tA, pA, dA);
        curve->D1(tB, pB, dB);
        Standard_Real a = gp_Vec2d(pA,p) * dA;
        Standard_Real b = gp_Vec2d(pA,p) * dB;
        if(a <= 0.0 && b >= 0.0 || a >= 0.0 && b <= 0.0)
        {
            c1ptr = n1;
            c2ptr = n2;
            break;
        }
    }

    assert(c1ptr != nullptr && c2ptr != nullptr);
    if(!c1ptr || !c2ptr)
        return std::numeric_limits<Standard_Real>::max();

    // Отрезок контура, который задает начальное приближение для
    // вычисления проекции, задают указатели `c1ptr` и `c2ptr`.
    Standard_Real t = 0.5 * (tA + tB);
    gp_Pnt2d c;
    gp_Vec2d dc, d2c;
    const Standard_Integer maxIters = 10;
    for(Standard_Integer iter = 0; iter < maxIters; ++iter)
    {
        curve->D2(t, c, dc, d2c);
        gp_Vec2d cp(c, p);
        Standard_Real dt = cp*dc / (-cp * d2c - dc*dc);
        t = t + dt;
        if(std::abs(dt) <= 1.e-7)
            break;
    }

    gp_Vec2d pc(p, c);
    Standard_Real distance = pc.Magnitude();
    if(pc.Crossed(dc) >= 0.0) //< Точка `p` слева от контура?
        distance = -distance;
    return distance;
}


/**
\brief Наименьшее знаковое расстояние от точки до участка контура.
Если точка слева от контура, то расстояние отрицательно.
*/
Standard_Real NearestDistance2(
    const gp_Pnt2d& p,
    const CircularList<ContourPoint>::Node* cptr
)
{
    static const Standard_Real tol = 1.e-7;
    Standard_Real minDistance = std::numeric_limits<Standard_Real>::max();
    for(Standard_Integer i = 0; i < 2; ++i)
    {
        //Standard_Integer i1 = (i % 2 == 0 ? i/2 : -i/2-1);
        //Standard_Integer i2 = (i % 2 == 0 ? i/2+1 : -i/2);
        gp_Pnt2d pA = CircularList<ContourPoint>::Offset(cptr, i)->data.p;
        gp_Pnt2d pB = CircularList<ContourPoint>::Offset(cptr, i+1)->data.p;
        gp_Vec2d w(pA, pB);
        gp_Vec2d ap(pA, p);
        Standard_Real t = (ap * w) / w.SquareMagnitude();
        if(t < 0.0 - tol || t > 1.0 + tol)
            continue;
        Standard_Real dist = ap.Crossed(w) / w.Magnitude();
        if(std::abs(dist) < std::abs(minDistance))
            minDistance = dist;
    }
    return minDistance;
}


TopAbs_State Classificate(
    const TopoDS_Face& F,
    const gp_Pnt2d& Q,
    const AuxClassificateData2d** pAux
)
{
    if(F.IsNull())
        return TopAbs_UNKNOWN;

    if(pAux != nullptr && (*pAux) != nullptr && (*pAux)->face != F)
        return TopAbs_UNKNOWN;

    static const Standard_Real Tol = 1.e-7;
    static const AuxClassificateData2d* s_aux = nullptr;
    if(pAux)
    {
        if(!(*pAux))
            (*pAux) = new AuxClassificateData2d(F, Tol);
    }
    else
    {
        if(!s_aux || s_aux->face != F)
            s_aux = new AuxClassificateData2d(F, Tol);
    }

    const AuxClassificateData2d* aux = pAux? *pAux : s_aux;
    assert(aux && aux->face == F);

#if 0
    aux->qTree->Dump("quadtree.vtk");
    aux->fContours.Dump("contours.vtk");
    Dumper dumper;
    dumper.AddPoints({Q});
    dumper.Save("point.vtk");
#endif

    QuadTree::Cell cell = aux->qTree->GetCell(Q);
    if(!cell)
        return TopAbs_OUT;

    size_t attr = aux->qTree->GetAttr(cell);
    std::list<QuadTree::Cell> boundaryCells;
    TopAbs_State qPointState = TopAbs_UNKNOWN;
    if(attr == 0 || attr == 1)
    {
        SearchNearestCells(aux->qTree,
                           Q,
                           IsBoundaryCell,
                           aux->tolerance,
                           boundaryCells);
        qPointState = (attr == 0 ? TopAbs_IN : TopAbs_OUT);
        if(boundaryCells.empty())
            return qPointState;
    }
    else
        boundaryCells.push_back(cell);

    for(const QuadTree::Cell& cell : boundaryCells)
    {
        size_t attr = aux->qTree->GetAttr(cell);
        assert(attr != 0 && attr != 1);
        // Превращаем атрибут в указатель на участок контура.
        auto cptr = reinterpret_cast<const CircularList<ContourPoint>::Node*>(attr);
        // Вычисляем расстояние от точки до контура,
        // используя `cptr` как начальное приближение.
        Standard_Real qcDistance = NearestDistance2(Q, cptr);
        if(std::abs(qcDistance) <= aux->tolerance)
            return TopAbs_ON;

        if(qcDistance < 0.0 )
        {
            assert(qPointState == TopAbs_UNKNOWN || qPointState == TopAbs_IN);
            qPointState = TopAbs_IN;
        }
        else
        {
            assert(qPointState == TopAbs_UNKNOWN || qPointState == TopAbs_OUT);
            qPointState = TopAbs_OUT;
        }
    }

    return qPointState;
}


namespace {;

void Enlarge(Bnd_Box2d& box, Standard_Real k)
{
    Standard_Real xMin, yMin, xMax, yMax;
    box.Get(xMin, yMin, xMax, yMax);
    Standard_Real xCenter = 0.5 * (xMin + xMax);
    Standard_Real yCenter = 0.5 * (yMin + yMax);
    Standard_Real xHalfWidth = 0.5 * (xMax - xMin);
    Standard_Real yHalfWidth = 0.5 * (yMax - yMin);
    xMin = xCenter - k * xHalfWidth;
    yMin = yCenter - k * yHalfWidth;
    xMax = xCenter + k * xHalfWidth;
    yMax = yCenter + k * yHalfWidth;
    box.Update(xMin, yMin, xMax, yMax);
}


void Quadratize(Bnd_Box2d& box)
{
    Standard_Real xMin, yMin, xMax, yMax;
    box.Get(xMin, yMin, xMax, yMax);
    Standard_Real xWidth = xMax - xMin;
    Standard_Real yWidth = yMax - yMin;
    Standard_Real xCenter = 0.5 * (xMin + xMax);
    Standard_Real yCenter = 0.5 * (yMin + yMax);
    Standard_Real width = std::max(xWidth, yWidth);
    xMin = xCenter - 0.5 * width;
    yMin = yCenter - 0.5 * width;
    xMax = xCenter + 0.5 * width;
    yMax = yCenter + 0.5 * width;
    box.Update(xMin, yMin, xMax, yMax);
}


FaceContours::FaceContours(const TopoDS_Face& F)
{
    myFace = F;
    TopoDS_Wire wOuter = BRepTools::OuterWire(F);
    myWires.push_back({});
    myBoundBoxes.push_back({});
    for(TopoDS_Iterator itw(F); itw.More(); itw.Next())
    {
        const TopoDS_Wire& W = TopoDS::Wire(itw.Value());
        auto* contour = &myWires.front();
        auto* bbox = &myBoundBoxes.front();
        if(W != wOuter)
        {
            myWires.push_back({});
            myBoundBoxes.push_back({});
            contour = &myWires.back();
            bbox = &myBoundBoxes.back();
        }
        std::vector<gp_Pnt2d> pl;
        Tesselate(F, W, pl);
        for(auto it = pl.begin(); it != std::prev(pl.end()); ++it)
        {
            const gp_Pnt2d& p = (*it);
            contour->push_back(ContourPoint{p});
        }

        Standard_Real xMin = +std::numeric_limits<Standard_Real>::max(),
                      yMin = +std::numeric_limits<Standard_Real>::max(),
                      xMax = -std::numeric_limits<Standard_Real>::max(),
                      yMax = -std::numeric_limits<Standard_Real>::max();
        for(const gp_Pnt2d& pt : pl)
        {
            xMin = std::min(xMin, pt.X());
            yMin = std::min(yMin, pt.Y());
            xMax = std::max(xMax, pt.X());
            yMax = std::max(yMax, pt.Y());
        }
        std::get<0>(*bbox) = xMin;
        std::get<1>(*bbox) = yMin;
        std::get<2>(*bbox) = xMax;
        std::get<3>(*bbox) = yMax;
    }
}


Standard_Boolean FaceContours::Dump(
    const std::filesystem::path& fileName
) const
{
    Dumper dumper;
    for(const auto& pl : myWires)
    {
        std::vector<gp_Pnt2d> polyline;
        for(const auto& cp : pl)
            polyline.push_back(cp.p);
        polyline.push_back(polyline.front());
        dumper.AddPolyline(polyline);
    }
    return dumper.Save(fileName);
}


Standard_Boolean IntersectSegmentSegment(
    const gp_Pnt2d& seg1p1,
    const gp_Pnt2d& seg1p2,
    const gp_Pnt2d& seg2p1,
    const gp_Pnt2d& seg2p2,
    Standard_Real& t1
)
{
    // Коэффициенты общего уравнения двух прямых.
    Standard_Real A1, B1, C1;
    CalcLineCoefficients(seg1p1, seg1p2, A1, B1, C1);
    Standard_Real A2, B2, C2;
    CalcLineCoefficients(seg2p1, seg2p2, A2, B2, C2);

    /*
    |A1 B1| |x|   |C1|
    |     | | | = |  |
    |A2 B2| |y|   |C2|
    */
    Standard_Real D = A1 * B2 - A2 * B1;
    if(std::abs(D) <= 1.e-7)
        return Standard_False;
    Standard_Real invD = 1.0 / D;

    Standard_Real xCross = invD * (B2 * C1 - B1 * C2);
    Standard_Real yCross = invD * (A1 * C1 - A2 * C2);

    Standard_Real absA1 = std::abs(A1);
    Standard_Real absB1 = std::abs(B1);
    if(absA1 > absB1)
        t1 = (yCross - seg1p1.Y()) / -A1;
    else
        t1 = (xCross - seg1p1.X()) / B1;

    return t1 >= 0.0 && t1 <= 1.0;
}


const std::vector<MeshLinesAndContoursIntersections::Cross>&
MeshLinesAndContoursIntersections::GetVInts(size_t i) const
{
    auto it = vInts.find(i);
    if(it == vInts.end())
    {
        auto itb = vInts.insert(std::make_pair(i,std::vector<Cross>()));
        it = itb.first;
        Standard_Real xConst = xMin + (xMax - xMin) / (nbVLines-1) * i;
        this->IntersectVertical(xConst, it->second);
    }
    return it->second;
}


const std::vector<MeshLinesAndContoursIntersections::Cross>&
MeshLinesAndContoursIntersections::GetHInts(size_t j) const
{
    auto it = hInts.find(j);
    if(it == hInts.end())
    {
        auto itb = hInts.insert(std::make_pair(j,std::vector<Cross>()));
        it = itb.first;
        Standard_Real yConst = yMin + (yMax - yMin) / (nbHLines-1) * j;
        this->IntersectHorizontal(yConst, it->second);
    }
    return it->second;
}


void MeshLinesAndContoursIntersections::IntersectVertical(
    Standard_Real xConst,
    std::vector<Cross>& inter
) const
{
    gp_Pnt2d vSegP1(xConst,yMin), vSegP2(xConst,yMax);
    for(const CircularList<ContourPoint>& contour : myContours.myWires)
    {
        for(auto it = contour.begin(); it != contour.end(); ++it)
        {
            gp_Pnt2d p0 = (*it).p;
            gp_Pnt2d p1 = (*std::next(it)).p;
            // Пересекается ли отрезок (p0,p1) с вертикальной прямой?
            Standard_Real x0 = p0.X(), x1 = p1.X();
            Standard_Real y0 = p0.Y(), y1 = p1.Y();
            Standard_Boolean b1 = (xConst >= x0 || xConst >= x1) && (xConst <= x0 || xConst <= x1);
            Standard_Boolean b2 = (y0 >= yMin && y0 <= yMax || y1 >= yMin && y1 <= yMax);
            if(b1 && b2)
            {
                Standard_Real u = (xConst - x0) / (x1 - x0);
                Standard_Real y = y0 + u * (y1 - y0);
                inter.push_back({y,it.current_node()});
            }
        }
    }

    struct Less
    {
        bool operator()(const Cross& a, const Cross& b) const
        {
            return a.u < b.u;
        }
    };
    std::sort(inter.begin(), inter.end(), Less());
}


void MeshLinesAndContoursIntersections::IntersectHorizontal(
    Standard_Real yConst,
    std::vector<Cross>& inter
) const
{
    gp_Pnt2d vSegP1(yConst,xMin), vSegP2(yConst,xMax);
    for(const auto& polyline : myContours.myWires)
    {
        for(auto it = polyline.begin(); it != polyline.end(); ++it)
        {
            gp_Pnt2d p0 = (*it).p;
            gp_Pnt2d p1 = (*std::next(it)).p;
            // Пересекается ли отрезок (p0,p1) с горизонтальной прямой?
            Standard_Real x0 = p0.X(), x1 = p1.X();
            Standard_Real y0 = p0.Y(), y1 = p1.Y();
            Standard_Boolean b1 = (yConst >= y0 || yConst >= y1) && (yConst <= y0 || yConst <= y1);
            Standard_Boolean b2 = (x0 >= xMin && x0 <= xMax || x1 >= xMin && x1 <= xMax);
            if(b1 && b2)
            {
                Standard_Real u = (yConst - y0) / (y1 - y0);
                Standard_Real x = x0 + u * (x1 - x0);
                inter.push_back({x,it.current_node()});
            }
        }
    }

    struct Less
    {
        bool operator()(const Cross& a, const Cross& b) const
        {
            return a.u < b.u;
        }
    };
    std::sort(inter.begin(), inter.end(), Less());
}


PositionType Reverse(PositionType t)
{
    if(t == OuterPosition)
        return InnerPosition;
    if(t == InnerPosition)
        return OuterPosition;
    return t;
}


void GetEdgeInfo(
    const std::vector<MeshLinesAndContoursIntersections::Cross>& lints,
    Standard_Real uBeg,
    Standard_Real uEnd,
    PositionType& t1,
    PositionType& t2,
    const CircularList<ContourPoint>::Node** seg1,
    const CircularList<ContourPoint>::Node** seg2
)
{
    (*seg1) = (*seg2) = nullptr;
    t1 = t2 = OuterPosition;

    Standard_Real eps = 1.e-7;

    // Пропускаем пересечения, расположенные слева от отрезка (uBeg,uEnd).
    PositionType tt1 = OuterPosition;
    auto it = lints.begin();
    for(; it != lints.end(); ++it)
    {
        Standard_Real u = it->u;
        if(u >= uBeg - eps)
        {
            if(u >= uBeg - eps && u <= uBeg + eps)
                t1 = BoundaryPosition;
            break;
        }
        tt1 = Reverse(tt1);
    }

    if(it == lints.end())
        return;

    PositionType tt2 = tt1;
    for(; it != lints.end(); ++it)
    {
        Standard_Real u = it->u;
        if(u >= uEnd - eps )
        {
            if(u >= uEnd - eps && u <= uEnd + eps)
                t2 = BoundaryPosition;
            break;
        }
        tt2 = Reverse(tt2);
        if(!(*seg1))
        {
            (*seg1) = it->seg;
        }
        else if(!(*seg2))
        {
            auto seg = it->seg;
            if(seg != (*seg1) && seg != (*seg1)->next && seg != (*seg1)->prev)
                (*seg2) = seg;
        }
    }

    if(t1 != BoundaryPosition)
        t1 = tt1;
    if(t2 != BoundaryPosition)
        t2 = tt2;
}


void GetCellLineIndices(
    const MeshLinesAndContoursIntersections& mlacInts,
    const QuadTree& qTree,
    const QuadTree::Cell& cell,
    Standard_Integer& i1Line,
    Standard_Integer& j1Line,
    Standard_Integer& i2Line,
    Standard_Integer& j2Line
    )
{
    Standard_Integer mlacLevel = mlacInts.levelIndex();
    Standard_Integer ni, nj;
    qTree.CellsNumberOnLevel(mlacLevel, ni, nj);
    Standard_Integer iCell, jCell;
    qTree.GetCellCoords(cell, iCell, jCell);
    i1Line = iCell * (0x01 << (mlacLevel - cell.level));
    j1Line = jCell * (0x01 << (mlacLevel - cell.level));
    i2Line = (iCell+1) * (0x01 << (mlacLevel - cell.level));
    j2Line = (jCell+1) * (0x01 << (mlacLevel - cell.level));
}

/**
\brief Классифицирует ячейку относительно контуров грани: внешняя, внутренняя
или граничная.
\param attr Атрибут указывает на первую вершину отрезка контура, который
пересекает ячейку. Первая вершина отрезка всегда располагается вне ячейки.
Если вторая вершина отрезка внутри ячейки, то ячейку пересекает следующий
отрезок. Если attr нулевой для пограничной ячейки, то она требует разделения.

Условия, при которых ячейка `cell` разобъется на части:
1) ребра ячейки пересекают контуры грани;
2) ячейка покрывает весь контур.
*/
PositionType ClassificateCell(
    const QuadTree& qTree,
    const QuadTree::Cell& cell,
    const FaceContours& fContours,
    const MeshLinesAndContoursIntersections& mlacInts,
    const CircularList<ContourPoint>::Node** attr
)
{
    Standard_Real xMin, yMin, xMax, yMax;
    qTree.GetCellBox(cell, xMin, yMin, xMax, yMax);
    /*
    C    D
    x----x
    |    |
    x----x
    A    B
    */
    // Признак положения вершин ячейки относительно контуров грани.
    PositionType avType, bvType, cvType, dvType;
    // Указатель на начало сегмента контура, который пересекает ячейку.
    const CircularList<ContourPoint>::Node* intSegment1 = nullptr;
    const CircularList<ContourPoint>::Node* intSegment2 = nullptr;

    // Запросы на пересечение сторон ячейки с контурами грани.
    Standard_Integer i1Line, j1Line, i2Line, j2Line;
    GetCellLineIndices(mlacInts, qTree, cell, i1Line, j1Line, i2Line, j2Line);

    const auto& abli = mlacInts.GetHInts(j1Line);
    const auto& bdli = mlacInts.GetVInts(i2Line);
    const auto& cdli = mlacInts.GetHInts(j2Line);
    const auto& acli = mlacInts.GetVInts(i1Line);
    PositionType a1Type, a2Type, b1Type, b2Type, c1Type, c2Type, d1Type, d2Type;
    const CircularList<ContourPoint>::Node* segments[8];
    GetEdgeInfo(abli, xMin, xMax, a1Type, b1Type, &segments[0], &segments[1]);
    GetEdgeInfo(cdli, xMin, xMax, c1Type, d1Type, &segments[2], &segments[3]);
    GetEdgeInfo(acli, yMin, yMax, a2Type, c2Type, &segments[4], &segments[5]);
    GetEdgeInfo(bdli, yMin, yMax, b2Type, d2Type, &segments[6], &segments[7]);

    avType = a1Type; assert(a1Type == a2Type);
    bvType = b1Type; assert(b1Type == b2Type);
    cvType = c1Type; assert(c1Type == c2Type);
    dvType = d1Type; assert(d1Type == d2Type);

    for(const auto* seg : segments)
    {
        if(!seg)
            continue;

        if(!intSegment1)
        {
            intSegment1 = seg;
            continue;
        }

        if(  intSegment1 == seg
          || intSegment1 == seg->prev)
            continue;


        if(intSegment1 == seg->next)
        {
            intSegment1 = seg;
            continue;
        }

        if(!intSegment2)
        {
            intSegment2 = seg;
            continue;
        }

        break;
    }

    Standard_Boolean isIntersected = (intSegment1 != nullptr);

    Standard_Boolean hasInnerContour = ContourLieInnerCell(fContours,
                                                           xMin,
                                                           yMin,
                                                           xMax,
                                                           yMax);

    Standard_Boolean isBoundaryCell = isIntersected || hasInnerContour;

    if(!isBoundaryCell)
    {
        if(    (avType == OuterPosition || avType == BoundaryPosition)
            && (bvType == OuterPosition || bvType == BoundaryPosition)
            && (cvType == OuterPosition || cvType == BoundaryPosition)
            && (dvType == OuterPosition || dvType == BoundaryPosition)
          )
            return OuterPosition;

        assert((avType == InnerPosition || avType == BoundaryPosition)
            && (bvType == InnerPosition || bvType == BoundaryPosition)
            && (cvType == InnerPosition || cvType == BoundaryPosition)
            && (dvType == InnerPosition || dvType == BoundaryPosition)
        );
        return InnerPosition;
    }

    assert(intSegment1 != nullptr);
    (*attr) = nullptr;
    if(!intSegment2)
    {
        (*attr) = intSegment1;
    }

    return BoundaryPosition;
}


void CalcLineCoefficients(
    const gp_Pnt2d& p1,
    const gp_Pnt2d& p2,
    Standard_Real& A,
    Standard_Real& B,
    Standard_Real& C
)
{
    A = p1.Y() - p2.Y();
    B = p2.X() - p1.X();
    C = p1.X() * p2.Y() - p2.X()*p1.Y();
}


Standard_Boolean ContourLieInnerCell(
    const FaceContours& fContours,
    Standard_Real xMinCell,
    Standard_Real yMinCell,
    Standard_Real xMaxCell,
    Standard_Real yMaxCell
)
{
    for(const auto& bb : fContours.myBoundBoxes)
    {
        Standard_Real xMinBox = std::get<0>(bb);
        Standard_Real yMinBox = std::get<1>(bb);
        Standard_Real xMaxBox = std::get<2>(bb);
        Standard_Real yMaxBox = std::get<3>(bb);

        if(    xMinCell <= xMinBox
            && xMaxCell >= xMaxBox
            && yMinCell <= yMinBox
            && yMaxCell <= yMaxBox)
            return Standard_True;
    }
    return Standard_False;
}


MeshLinesAndContoursIntersections::MeshLinesAndContoursIntersections(
    const QuadTree& qTree,
    const FaceContours& fContours,
    Standard_Integer levelIndex
) : myContours(fContours)
{
    qTree.GetBox(xMin, yMin, xMax, yMax);
    myLevelIndex = levelIndex;
    qTree.CellsNumberOnLevel(levelIndex, nbVLines, nbHLines);
    nbVLines += 1, nbHLines += 1;
}
}


AuxClassificateData2d::AuxClassificateData2d(
    const TopoDS_Face& F,
    Standard_Real tol
) : face(F), fContours(F), tolerance(tol)
{
    Bnd_Box2d box = ComputePCurvesBox(F);
    Enlarge(box, 1.1);
    Quadratize(box);

    Standard_Integer maxLevelsNumber = 16;
    qTree = QuadTree::Create(box, 3, 3);
    Standard_Real xMinBox, yMinBox, xMaxBox, yMaxBox;
    box.Get(xMinBox, yMinBox, xMaxBox, yMaxBox);
    MeshLinesAndContoursIntersections mlacInts(*qTree, fContours, maxLevelsNumber);
    std::vector<Standard_Integer> splitCells;
    for(Standard_Integer level = 0; level < maxLevelsNumber; ++level)
    {
        splitCells.clear();
        for(QuadTree::Cell cell : qTree->TerminalCellsOfLevel(level))
        {
            const CircularList<ContourPoint>::Node* node = nullptr;
            PositionType type = ClassificateCell(*qTree,
                                                 cell,
                                                 fContours,
                                                 mlacInts,
                                                 &node);

            if(type == BoundaryPosition && !node)
                splitCells.push_back(cell.index);
            else
            {
                size_t attr;
                if(type == OuterPosition)
                    attr = 1;
                else if(type == InnerPosition)
                    attr = 0;
                else // type == BoundaryPosition
                    attr = reinterpret_cast<size_t>(node);
                qTree->SetAttr(cell, attr);
            }
        }

        if(splitCells.empty())
            break;

        for(Standard_Integer cellIndex : splitCells)
            qTree->Split(QuadTree::Cell(level,cellIndex));
    }
}


AuxClassificateData2d::~AuxClassificateData2d()
{
}
