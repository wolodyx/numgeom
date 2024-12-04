#ifndef numgeom_numgeom_octree_h
#define numgeom_numgeom_octree_h

#include <filesystem>
#include <list>
#include <memory>

#include <Bnd_Box.hxx>

#include "numgeom/ijk.h"
#include "numgeom/iterator.h"
#include "numgeom/numgeom_export.h"


class NUMGEOM_EXPORT OcTree
{
public:

    typedef std::shared_ptr<OcTree> Ptr;
    typedef std::shared_ptr<const OcTree> CPtr;

    enum class CellConnection
    {
        // Sides
        DownSide,
        UpSide,
        LeftSide,
        RightSide,
        FrontSide,
        BackSide,
        // Ribs
        DownLeftRib,
        DownRightRib,
        DownFrontRib,
        DownBackRib,
        UpLeftRib,
        UpRightRib,
        UpFrontRib,
        UpBackRib,
        LeftFrontRib,
        RightFrontRib,
        LeftBackRib,
        RightBackRib,
        // Corners
        DownLeftFrontCorner,
        DownRightFrontCorner,
        DownRightBackCorner,
        DownLeftBackCorner,
        UpLeftFrontCorner,
        UpRightFrontCorner,
        UpRightBackCorner,
        UpLeftBackCorner,
        // Others
        Inner,
        Outer
    };

    struct Internal;
    struct Cell;
    struct Node;

public:

    static OcTree::Ptr Deserialize(std::istream&);

    static OcTree::Ptr Create(
        const Bnd_Box&,
        Standard_Integer in = 1,
        Standard_Integer jn = 1,
        Standard_Integer kn = 1
    );

public:

    ~OcTree();

    //! Количество уровней в дереве.
    Standard_Integer Levels() const;

    //! Количество терминальных квадрантов.
    Standard_Integer NbCells() const;

    //! Габаритная коробка всех ячеек нулевого уровня.
    void GetBox(
        Standard_Real& xMin,
        Standard_Real& yMin,
        Standard_Real& zMin,
        Standard_Real& xMax,
        Standard_Real& yMax,
        Standard_Real& zMax
    ) const;

    Ijk CellsNumberOnLevel(Standard_Integer level) const;

    Ijk GetCellCoords(const Cell& cell) const;

    //! Возвращает тип связи второй ячейки относительно первой.
    CellConnection GetConnectionType(const Cell&, const Cell&) const;

    //! Итератор по терминальным ячейкам.
    Iterator<Cell> TerminalCells() const;

    //! Итератор по терминальным ячейкам заданного уровня.
    Iterator<Cell> TerminalCellsOfLevel(Standard_Integer level) const;

    //! Итерирование по связанным (по ребру и вершине) ячейкам.
    Iterator<Cell> ConnectedCells(const Cell&) const;

    void GetCellBox(
        const Cell& cell,
        Standard_Real& xMin,
        Standard_Real& yMin,
        Standard_Real& zMin,
        Standard_Real& xMax,
        Standard_Real& yMax,
        Standard_Real& zMax
    ) const;


    //! Наименьшее и наибольшее расстояния от точки `q` до ячейки `cell`.
    //! Если точка попадает внутрь ячейки, то расстояния нулевые.
    void GetMinMaxDistances(
        const gp_Pnt& q,
        const Cell& cell,
        Standard_Real& minDistance,
        Standard_Real& maxDistance
    ) const;


    gp_Pnt GetCenter(const Cell& cell) const;


    Standard_Boolean Equals(OcTree::CPtr) const;

    void Split(const Cell&);

    size_t GetAttr(const Cell&) const;
    void SetAttr(const Cell&, size_t) const;

    Cell GetCell(const gp_Pnt&) const;

    Standard_Boolean Dump(const std::filesystem::path&) const;

    void Serialize(std::ostream&) const;

private:
    OcTree(
        const Bnd_Box&,
        Standard_Integer in,
        Standard_Integer jn,
        Standard_Integer kn
    );
    OcTree(const OcTree&) = delete;
    OcTree& operator=(const OcTree&) = delete;

private:
    Internal* pimpl;
};


struct OcTree::Cell
{
    Standard_Integer level;
    Standard_Integer index;

    Cell() : level(-1), index(0) {}

    Cell(Standard_Integer lvl, Standard_Integer idx)
        : level(lvl), index(idx) {}

    Standard_Boolean operator!() const { return level < 0;}

    Standard_Boolean operator==(const Cell& other) const
    {
        return other.level == this->level
            && other.index == this->index;
    }

    Standard_Boolean operator!=(const Cell& other) const
    {
        return other.level != this->level
            || other.index != this->index;
    }

    Standard_Boolean operator<(const Cell& other) const
    {
        if(level < other.level)
            return Standard_True;
        if(level > other.level)
            return Standard_False;
        return index < other.index;
    }
};


struct OcTree::Node
{
    Standard_Integer level;
    Standard_Integer index;

    Node() : level(-1), index(0) {}

    Node(Standard_Integer lvl, Standard_Integer idx)
        : level(lvl), index(idx) {}

    Standard_Boolean operator!() const { return level < 0;}

    Standard_Boolean operator==(const Node& other) const
    {
        if(other.level == this->level)
            return other.index == this->index;
        return Standard_False;
    }

    Standard_Boolean operator!=(const Node& other) const
    {
        return !this->operator==(other);
    }

    Standard_Boolean operator<(const Cell& other) const
    {
        if(level < other.level)
            return Standard_True;
        if(level > other.level)
            return Standard_False;
        return index < other.index;
    }
};


/**
\brief Поиск в октодереве ячеек (входящих в список отмеченных), которые
       наиболее близко расположены к точке.
\param tree Октодерево.
\param Q Искомая точка.
\param isTaggedCell Функция, возвращающая признак отмеченности ячейки.
\param maximumDistance Радиус, вокруг которого происходит поиск.
\param nearestCells Ближайшие к точке терминальные отмеченные ячейки.
*/
void NUMGEOM_EXPORT SearchNearestCells(
    OcTree::CPtr tree,
    const gp_Pnt& Q,
    const std::function<Standard_Boolean(OcTree::CPtr, const OcTree::Cell&)>& isTaggedCell,
    Standard_Real maximumDistance,
    std::list<OcTree::Cell>& nearestCells
);
#endif // !numgeom_numgeom_octree_h
