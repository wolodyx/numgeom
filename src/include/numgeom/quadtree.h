#ifndef numgeom_numgeom_quadtree_h
#define numgeom_numgeom_quadtree_h

#include <filesystem>
#include <memory>

#include <Bnd_Box2d.hxx>

#include "numgeom/iterator.h"
#include "numgeom/numgeom_export.h"

class Bnd_Box2d;
class gp_Pnt2d;


class NUMGEOM_EXPORT QuadTree
{
public:

    typedef std::shared_ptr<QuadTree> Ptr;
    typedef std::shared_ptr<const QuadTree> CPtr;

    enum class CellConnection
    {
        DownSide,
        RightSide,
        UpSide,
        LeftSide,
        DownLeftCorner,
        DownRightCorner,
        UpRightCorner,
        UpLefttCorner,
        Inner,
        Outer
    };

    struct Internal;
    struct Cell;
    struct Node;

public:

    static QuadTree::Ptr Deserialize(std::istream&);

    static QuadTree::Ptr Create(
        const Bnd_Box2d&,
        Standard_Integer in = 1,
        Standard_Integer jn = 1
    );

public:

    ~QuadTree();

    //! Количество уровней в дереве.
    Standard_Integer Levels() const;

    //! Количество терминальных квадрантов.
    Standard_Integer NbCells() const;

    //! Габаритная коробка всех ячеек нулевого уровня.
    void GetBox(
        Standard_Real& xMin,
        Standard_Real& yMin,
        Standard_Real& xMax,
        Standard_Real& yMax
    ) const;

    void CellsNumberOnLevel(
        Standard_Integer level,
        Standard_Integer& nbByI,
        Standard_Integer& nbByJ
    ) const;

    void GetCellCoords(
        const Cell& cell,
        Standard_Integer& i,
        Standard_Integer& j
    ) const;

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
        Standard_Real& xMax,
        Standard_Real& yMax
    ) const;

    Standard_Boolean Equals(QuadTree::CPtr) const;

    void Split(const Cell&);

    size_t GetAttr(const Cell&) const;
    void SetAttr(const Cell&, size_t) const;

    Cell GetCell(const gp_Pnt2d&) const;

    Standard_Boolean Dump(const std::filesystem::path&) const;

    void Serialize(std::ostream&) const;

private:
    QuadTree(
        const Bnd_Box2d&,
        Standard_Integer in,
        Standard_Integer jn
    );
    QuadTree(const QuadTree&) = delete;
    QuadTree& operator=(const QuadTree&) = delete;

private:
    Internal* pimpl;
};


struct QuadTree::Cell
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
};


struct QuadTree::Node
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
};
#endif // !numgeom_numgeom_quadtree_h
