#include "numgeom/octree.h"

#include <array>
#include <cassert>
#include <queue>
#include <map>
#include <stack>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "iterator_ijk.h"


struct OcTree::Internal
{
    Bnd_Box boundBox;
    Standard_Integer nbByI, nbByJ, nbByK;
    //! Номера нетерминальных ячеек на уровне.
    std::vector<std::vector<uint32_t>> nonTerminalCells;
    //! Атрибуты ячеек.
    std::vector<std::map<Standard_Integer,size_t>> attrs;


    Standard_Integer Levels() const;

    Standard_Boolean IsTerminal(const Cell&) const;

    //! Ячейка существует в терминальном или нетреминальном виде?
    Standard_Boolean IsExists(const Cell&) const;

    Standard_Integer GetOrder(const Cell&) const;

    Cell GetChildCell(
        const Cell& parentCell,
        Standard_Integer childIndex
    ) const;

    void GetCellsOfNextLevel(
        const Cell& parentCell,
        std::array<Standard_Integer,8>& cells
    ) const;

    //! Размерность подложки уровня `level`.
    Ijk LevelSize(Standard_Integer level) const;

    Standard_Integer GetNbCellsOnLevel(
        Standard_Integer level
    ) const;

    Ijk GetCellCoords(const Cell& cell) const;

    Ijk NodeIndex2Coords(const Node& node) const;

    Cell GetParentCell(const Cell& cell) const;

    Cell GetFirstTerminalCell(const Cell& startCell) const;

    Cell Next(const Cell& startCell) const;

    gp_Pnt PointCoords(
        const Node& node
    ) const;

    Node Normalize(const Node& node) const;

    void Nodes(
        const Cell& cell,
        std::array<Node,8>& nodes
    ) const;
};


Standard_Integer OcTree::Internal::Levels() const
{
    return nonTerminalCells.size() + 1;
}


Standard_Boolean OcTree::Internal::IsTerminal(const Cell& cell) const
{
    Standard_Integer levels = this->Levels();
    assert(cell.level < levels);

    if(cell.level == levels - 1)
        return Standard_True;

    const auto& ntCells = nonTerminalCells[cell.level];
    return !std::binary_search(ntCells.begin(), ntCells.end(), cell.index);
}


Standard_Integer OcTree::Internal::GetOrder(const Cell& cell) const
{
    assert(cell.level != 0);
    Ijk ijk = this->GetCellCoords(cell);
    Standard_Boolean b1 = (ijk.i % 2 == 0);
    Standard_Boolean b2 = (ijk.j % 2 == 0);
    Standard_Boolean b3 = (ijk.k % 2 == 0);

    static const Standard_Integer s_cellOrder[2][2][2] =
    {
        {
            {0, 1},
            {3, 2}
        },
        {
            {4, 5},
            {7, 6}
        }
    };

    return s_cellOrder[b3?0:1][b2?0:1][b1?0:1];
}


OcTree::Cell OcTree::Internal::GetChildCell(
    const Cell& parentCell,
    Standard_Integer childIndex
) const
{
    assert(childIndex >= 0 && childIndex < 8);
    Ijk n = this->LevelSize(parentCell.level);
    Ijk c = Ijk::Coords(parentCell.index, n);
    n *= 2, c *= 2;
    switch(childIndex)
    {
    case 0: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i  ,c.j  ,c.k  ), n));
    case 1: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i+1,c.j  ,c.k  ), n));
    case 2: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i+1,c.j+1,c.k  ), n));
    case 3: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i  ,c.j+1,c.k  ), n));
    case 4: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i  ,c.j  ,c.k+1), n));
    case 5: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i+1,c.j  ,c.k+1), n));
    case 6: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i+1,c.j+1,c.k+1), n));
    case 7: return Cell(parentCell.level+1, Ijk::Index(Ijk(c.i  ,c.j+1,c.k+1), n));
    }
    return Cell();
}


void OcTree::Internal::GetCellsOfNextLevel(
    const Cell& parentCell,
    std::array<Standard_Integer,8>& cells
) const
{
    Ijk n = this->LevelSize(parentCell.level);
    Ijk c = Ijk::Coords(parentCell.index, n);

    n *= 2, c *= 2;
    cells[0] = Ijk::Index(c, n.i, n.j);
    cells[1] = cells[0] + 1;
    cells[2] = Ijk::Index(Ijk(c.i+1,c.j+1,c.k), n.i, n.j);
    cells[3] = cells[2] - 1;
    cells[4] = Ijk::Index(Ijk(c.i,c.j,c.k+1), n.i, n.j);
    cells[5] = cells[4] + 1;
    cells[6] = Ijk::Index(c+1, n.i, n.j);
    cells[7] = cells[6] - 1;
}


//! Размерность подложки уровня `level`.
Ijk OcTree::Internal::LevelSize(Standard_Integer level) const
{
    assert(level >= 0);
    Standard_Integer r = 0x01 << level;
    return Ijk(nbByI * r,
               nbByJ * r,
               nbByK * r);
}


Standard_Integer OcTree::Internal::GetNbCellsOnLevel(
    Standard_Integer level
) const
{
    Ijk n = this->LevelSize(level);
    return n.i * n.j * n.k;
}


Ijk OcTree::Internal::GetCellCoords(const Cell& cell) const
{
    Ijk n = this->LevelSize(cell.level);
    return Ijk::Coords(cell.index, n.i, n.j);
}


Ijk OcTree::Internal::NodeIndex2Coords(const Node& node) const
{
    Ijk n = this->LevelSize(node.level);
    n += 1;
    return Ijk::Coords(node.index, n);
}


OcTree::Cell OcTree::Internal::GetParentCell(const Cell& cell) const
{
    if(cell.level == 0)
        return Cell();
    Ijk c = this->GetCellCoords(cell);
    Ijk n = this->LevelSize(cell.level);
    return Cell(cell.level-1, Ijk::Index(c/2,n/2));
}


Standard_Boolean OcTree::Internal::IsExists(
    const Cell& cell
) const
{
    Standard_Integer nbLevels = this->Levels();
    if(cell.level >= nbLevels)
        return Standard_False;

    // Ячейка существует тогда, когда в подложке есть ячейка с таким номером и
    // ячейка входит в нулевой уровень или если нет, то ее родитель в списке
    // нетерминальных.

    Standard_Integer nc = this->GetNbCellsOnLevel(cell.level);
    if(cell.index >= nc)
        return Standard_False;

    if(cell.level == 0)
        return Standard_True;

    Cell parentCell = this->GetParentCell(cell);
    const auto& ntCells = nonTerminalCells[parentCell.level];
    return std::binary_search(ntCells.begin(),
                              ntCells.end(),
                              parentCell.index);
}


OcTree::Cell OcTree::Internal::GetFirstTerminalCell(
    const Cell& startCell
) const
{
    Standard_Boolean beNonTerminal = 
           startCell.level < nonTerminalCells.size()
        && std::binary_search(nonTerminalCells[startCell.level].begin(),
                              nonTerminalCells[startCell.level].end(),
                              startCell.index);

    Standard_Integer level = startCell.level;
    Standard_Integer ni = this->nbByI * (0x01 << level);
    Standard_Integer nj = this->nbByJ * (0x01 << level);
    Ijk ijk = Ijk::Coords(startCell.index, ni, nj);
    if(beNonTerminal)
    {
        // Если ячейка попадает в список нетерминальных,
        // то поднимаемся выше пока она оттуда не исключится.
        do
        {
            ijk *= 2;
            ni *= 2, nj *= 2;
            ++level;

            if(level >= nonTerminalCells.size())
                break;
        }
        while(std::binary_search(nonTerminalCells[level].begin(),
                                 nonTerminalCells[level].end(),
                                 Ijk::Index(ijk, ni, nj)));
    }
    else
    {
        // Если ячейки нет в списках нетерминальных ячеек, то спускаемся
        // пока родительская ячейка не окажется в списках нетерминальных.
        while(true)
        {
            if(level == 0)
                break;

            Standard_Boolean beNonTerminalParentCell =
                std::binary_search(nonTerminalCells[level-1].begin(),
                                   nonTerminalCells[level-1].end(),
                                   Ijk::Index(ijk/2,ni/2,nj/2));
            if(beNonTerminalParentCell)
                break;

            ijk /= 2;
            ni /= 2, nj /= 2;
            --level;
        }
    }

    return Cell(level, Ijk::Index(ijk,ni,nj));
}


OcTree::Cell OcTree::Internal::Next(const Cell& startCell) const
{
    assert(this->IsTerminal(startCell));
    Cell cell = startCell;
    while(true)
    {
        if(cell.level == 0)
        {
            Standard_Integer nextCellIndex = cell.index + 1;
            if(nextCellIndex == nbByI * nbByJ * nbByK)
                return Cell{0, nextCellIndex};
            cell = Cell{0, nextCellIndex};
            break;
        }

        Cell parentCell = this->GetParentCell(cell);
        Standard_Integer order = this->GetOrder(cell);
        if(order < 7)
        {
            cell = this->GetChildCell(parentCell, order + 1);
            break;
        }
        cell = parentCell;
    }
    return this->GetFirstTerminalCell(cell);
}


gp_Pnt OcTree::Internal::PointCoords(
    const Node& node
) const
{
    Ijk n = this->LevelSize(node.level);
    Ijk c = this->NodeIndex2Coords(node);
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    this->boundBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    Standard_Real x = xMin + (xMax - xMin) / n.i * c.i;
    Standard_Real y = yMin + (yMax - yMin) / n.j * c.j;
    Standard_Real z = zMin + (zMax - zMin) / n.k * c.k;
    return gp_Pnt(x, y, z);
}


OcTree::Node OcTree::Internal::Normalize(const Node& node) const
{
    Ijk c = this->NodeIndex2Coords(node);
    Ijk n = this->LevelSize(node.level);
    Standard_Integer levelDown = 0;
    if(node.level != 0)
    {
        while(c.i != 0 && c.j != 0 && c.k != 0
            && c.i%2 == 0 && c.j%2 == 0 && c.k%2 == 0
            && n.i%2 == 0 && n.j%2 == 0 && n.k%2 == 0)
        {
            c /= 2;
            n /= 2;
            ++levelDown;
        }
    }
    return Node(node.level - levelDown, Ijk::Index(c,n));
}


void OcTree::Internal::Nodes(
    const Cell& cell,
    std::array<Node,8>& nodes
) const
{
    Ijk c = this->GetCellCoords(cell);
    Ijk n = this->LevelSize(cell.level);
    n += 1;
    Standard_Integer nij = n.i * n.j;
    nodes[0] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i  ,c.j  ,c.k  ), n)));
    nodes[1] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i+1,c.j  ,c.k  ), n)));
    nodes[2] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i+1,c.j+1,c.k  ), n)));
    nodes[3] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i  ,c.j+1,c.k  ), n)));
    nodes[4] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i  ,c.j  ,c.k+1), n)));
    nodes[5] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i+1,c.j  ,c.k+1), n)));
    nodes[6] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i+1,c.j+1,c.k+1), n)));
    nodes[7] = this->Normalize(Node(cell.level, Ijk::Index(Ijk(c.i  ,c.j+1,c.k+1), n)));
}


OcTree::OcTree(
    const Bnd_Box& boundBox,
    Standard_Integer in,
    Standard_Integer jn,
    Standard_Integer kn
)
{
    pimpl = new Internal();
    pimpl->boundBox = boundBox;
    pimpl->nbByI = in;
    pimpl->nbByJ = jn;
    pimpl->nbByK = kn;
    pimpl->nonTerminalCells.reserve(32);
    pimpl->attrs.reserve(32);
}


OcTree::~OcTree()
{
    delete pimpl;
}


Standard_Integer OcTree::Levels() const
{
    return pimpl->Levels();
}


void OcTree::GetBox(
    Standard_Real& xMin,
    Standard_Real& yMin,
    Standard_Real& zMin,
    Standard_Real& xMax,
    Standard_Real& yMax,
    Standard_Real& zMax
) const
{
    pimpl->boundBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
}


namespace {;
Standard_Integer CellsCount(
    const OcTree::Internal* oTreeInternal,
    const OcTree::Cell& parentCell
)
{
    typedef OcTree::Cell Cell;

    Standard_Integer nbCells = 0;
    std::stack<Cell> stack;
    stack.push(parentCell);
    while(!stack.empty())
    {
        Cell cell = stack.top();
        stack.pop();

        Standard_Boolean isTerminalCell = Standard_True;
        if(cell.level < oTreeInternal->nonTerminalCells.size())
        {
            const auto& ntCells = oTreeInternal->nonTerminalCells[cell.level];
            if(std::binary_search(ntCells.begin(), ntCells.end(), cell.index))
                isTerminalCell = Standard_False;
        }

        if(isTerminalCell)
        {
            ++nbCells;
            continue;
        }

        std::array<Standard_Integer, 8> nextCellsIndex;
        oTreeInternal->GetCellsOfNextLevel(cell, nextCellsIndex);
        stack.push(Cell{cell.level+1,nextCellsIndex[0]});
        stack.push(Cell{cell.level+1,nextCellsIndex[1]});
        stack.push(Cell{cell.level+1,nextCellsIndex[2]});
        stack.push(Cell{cell.level+1,nextCellsIndex[3]});
        stack.push(Cell{cell.level+1,nextCellsIndex[4]});
        stack.push(Cell{cell.level+1,nextCellsIndex[5]});
        stack.push(Cell{cell.level+1,nextCellsIndex[6]});
        stack.push(Cell{cell.level+1,nextCellsIndex[7]});
    }

    return nbCells;
}
}


Standard_Integer OcTree::NbCells() const
{
    Standard_Integer nbCells = 0;
    Standard_Integer nbCellsOnLevel0 = pimpl->nbByI * pimpl->nbByJ * pimpl->nbByK;
    for(Standard_Integer i = 0; i < nbCellsOnLevel0; ++i)
    {
        Cell cell{0, i};
        nbCells += CellsCount(pimpl, cell);
    }
    return nbCells;
}


namespace {;
class IteratorImpl_OcTreeCells : public IteratorImpl<OcTree::Cell>
{
    const OcTree::Internal* myOTreeInternal;
    OcTree::Cell myCurrentCell; //!< Текущая терминальная ячейка.
public:

    IteratorImpl_OcTreeCells(
        const OcTree::Internal* oTreeInternal,
        const OcTree::Cell& startCell = OcTree::Cell()
    )
    {
        myOTreeInternal = oTreeInternal;
        myCurrentCell = startCell;
        if(!myCurrentCell)
            myCurrentCell = myOTreeInternal->GetFirstTerminalCell(OcTree::Cell{0,0});
    }

    virtual ~IteratorImpl_OcTreeCells() {}

    void advance() override
    {
        myCurrentCell = myOTreeInternal->Next(myCurrentCell);
    }

    OcTree::Cell current() const override
    {
        return myCurrentCell;
    }

    IteratorImpl<OcTree::Cell>* clone() const override
    {
        return new IteratorImpl_OcTreeCells(myOTreeInternal, myCurrentCell);
    }

    IteratorImpl<OcTree::Cell>* last() const override
    {
        Standard_Integer N = myOTreeInternal->nbByI * myOTreeInternal->nbByJ * myOTreeInternal->nbByK;
        return new IteratorImpl_OcTreeCells(myOTreeInternal, OcTree::Cell{0,N});
    }

    bool end() const override
    {
        return !myCurrentCell;
    }

    bool equals(const IteratorImpl<OcTree::Cell>& other) const override
    {
        auto it = dynamic_cast<const IteratorImpl_OcTreeCells*>(&other);
        if(!it)
            return false;
        return it->myOTreeInternal == this->myOTreeInternal
            && it->myCurrentCell == this->myCurrentCell;
    }
};
}


Iterator<OcTree::Cell> OcTree::TerminalCells() const
{
    auto itImpl = new IteratorImpl_OcTreeCells(pimpl);
    return Iterator<Cell>(itImpl);
}


namespace {;
class IteratorImpl_OcTreeCellsOfLevel : public IteratorImpl<OcTree::Cell>
{
    const OcTree::Internal* myOTreeInternal;
    Standard_Integer myLevel;
    Standard_Integer myCurrentIndex;
    Standard_Integer myNbCellsOnLevel;
    std::vector<uint32_t>::const_iterator itNtCells, itNtCellsEnd;
public:

    IteratorImpl_OcTreeCellsOfLevel(
        const OcTree::Internal* oTreeInternal,
        Standard_Integer level,
        Standard_Integer currentIndex = 0
    )
    {
        myOTreeInternal = oTreeInternal;
        myLevel = level;
        myCurrentIndex = currentIndex;
        myNbCellsOnLevel = myOTreeInternal->GetNbCellsOnLevel(level);

        static const std::vector<uint32_t> s_emptyVector;
        itNtCells = s_emptyVector.begin();
        itNtCellsEnd = s_emptyVector.end();
        if(level < myOTreeInternal->nonTerminalCells.size())
        {
            const auto& ntCells = myOTreeInternal->nonTerminalCells[level];
            itNtCells = ntCells.begin();
            itNtCellsEnd = ntCells.end();
        }

        // Настройка itNtCells
        while(itNtCells != itNtCellsEnd && myCurrentIndex > (*itNtCells))
            ++itNtCells;

        OcTree::Cell cell(myLevel, myCurrentIndex);
        if(!myOTreeInternal->IsTerminal(cell))
            GoToNextTerminalCell();
    }

    virtual ~IteratorImpl_OcTreeCellsOfLevel() {}


    void GoToNextTerminalCell()
    {
        assert(myCurrentIndex < myNbCellsOnLevel);

        for(++myCurrentIndex; myCurrentIndex < myNbCellsOnLevel; ++myCurrentIndex)
        {
            if(itNtCells == itNtCellsEnd)
                break;

            if(myCurrentIndex < (*itNtCells))
                break;

            if(myCurrentIndex == (*itNtCells))
            {
                ++itNtCells;
                continue;
            }
        }
    }


    void advance() override
    {
        this->GoToNextTerminalCell();
    }

    OcTree::Cell current() const override
    {
        return OcTree::Cell(myLevel, myCurrentIndex);
    }

    IteratorImpl<OcTree::Cell>* clone() const override
    {
        return new IteratorImpl_OcTreeCellsOfLevel(myOTreeInternal, myLevel, myCurrentIndex);
    }

    IteratorImpl<OcTree::Cell>* last() const override
    {
        return new IteratorImpl_OcTreeCellsOfLevel(myOTreeInternal,
                                                     myLevel,
                                                     myNbCellsOnLevel);
    }

    bool end() const override
    {
        return myCurrentIndex == myNbCellsOnLevel;
    }

    bool equals(const IteratorImpl<OcTree::Cell>& other) const override
    {
        auto it = dynamic_cast<const IteratorImpl_OcTreeCellsOfLevel*>(&other);
        if(!it)
            return false;
        return it->myOTreeInternal == this->myOTreeInternal
            && it->myLevel == this->myLevel
            && it->myCurrentIndex == this->myCurrentIndex;
    }
};
}


Iterator<OcTree::Cell> OcTree::TerminalCellsOfLevel(
    Standard_Integer level
) const
{
    auto itImpl = new IteratorImpl_OcTreeCellsOfLevel(pimpl,level);
    return Iterator<Cell>(itImpl);
}


namespace {;

/**
Итератор для обхода ячеек сетки уровня выше, связанные с ячейкой уровня ниже.
*/
class IndexIterator
{
    //! Координаты базовой ячейки.
    Ijk myCellCoords;
    //! Признак, что нижняя, верхняя, левая, правая,
    //! фронтальная и тыльная стороны упираются в границу.
    std::array<Standard_Boolean,6> myBlockedSize;
    //! Координаты текущей ячейки итератора.
    Iterator_Ijk myCurrentCellCoords;
    //! Уровень сетки, ячейки которой обходит итератор.
    Standard_Integer myUpLevel;

    Standard_Integer myQMultiple;
    Standard_Integer myISize, myJSize;

public:

    IndexIterator()
    {
        myBlockedSize = { Standard_True };
        myUpLevel = -1;
        myQMultiple = -1;
        myISize = myJSize = -1;
    }

    IndexIterator(
        const OcTree::Internal* oTreeInternal,
        const OcTree::Cell& cell,
        Standard_Integer upLevel
    )
    {
        myUpLevel = upLevel;
        myQMultiple = 0x01 << (upLevel - cell.level);

        myCellCoords = oTreeInternal->GetCellCoords(cell);

        Ijk n = oTreeInternal->LevelSize(cell.level);

        myBlockedSize[0] = (myCellCoords.k == 0);
        myBlockedSize[1] = (myCellCoords.k == n.k - 1);
        myBlockedSize[2] = (myCellCoords.i == 0);
        myBlockedSize[3] = (myCellCoords.i == n.i - 1);
        myBlockedSize[4] = (myCellCoords.j == 0);
        myBlockedSize[5] = (myCellCoords.j == n.j - 1);

        myCurrentCellCoords = Iterator_Ijk((myCellCoords + 1) * myQMultiple + 1,
                                           myCellCoords * myQMultiple - 1,
                                           myCellCoords * myQMultiple - 1);

        n = oTreeInternal->LevelSize(myUpLevel);
        myISize = n.i, myJSize = n.j;

        if(this->isBlockedCell())
            this->operator++();
    }

    Standard_Boolean isBlockedCell() const
    {
        return myCurrentCellCoords.nonBound()
            || myCurrentCellCoords.onBound(Side3d::iMin) && myBlockedSize[2]
            || myCurrentCellCoords.onBound(Side3d::iMax) && myBlockedSize[3]
            || myCurrentCellCoords.onBound(Side3d::jMin) && myBlockedSize[4]
            || myCurrentCellCoords.onBound(Side3d::jMax) && myBlockedSize[5]
            || myCurrentCellCoords.onBound(Side3d::kMin) && myBlockedSize[0]
            || myCurrentCellCoords.onBound(Side3d::kMax) && myBlockedSize[1];
    }

    IndexIterator& operator++()
    {
        assert(!myCurrentCellCoords.isEnd());
        for(++myCurrentCellCoords; !myCurrentCellCoords.isEnd(); ++myCurrentCellCoords)
        {
            if(this->isBlockedCell())
                continue;
            break;
        }
        return *this;
    }

    OcTree::Cell operator*() const
    {
        assert(!myCurrentCellCoords.isEnd());
        Standard_Integer index = Ijk::Index(*myCurrentCellCoords, myISize, myJSize);
        return OcTree::Cell(myUpLevel, index);
    }

    Standard_Boolean More() const
    {
        return !myCurrentCellCoords.isEnd();
    }

    Standard_Boolean Empty() const
    {
        return myUpLevel == -1;
    }

    void SetEnd()
    {
        myCurrentCellCoords.setEnd();
    }

    Standard_Boolean operator==(const IndexIterator& other) const
    {
        return myISize == other.myISize
            && myJSize == other.myJSize
            && myCellCoords == other.myCellCoords
            && myCurrentCellCoords == other.myCurrentCellCoords
            && myUpLevel == other.myUpLevel
            && myQMultiple == other.myQMultiple
            && myBlockedSize == other.myBlockedSize;
    }
};


class IteratorImpl_ConnectedCells : public IteratorImpl<OcTree::Cell>
{
    const OcTree::Internal* myOTreeInternal;
    OcTree::Cell myBaseCell;
    IndexIterator myIndexIterator;
    OcTree::Cell myCurrentCell;
public:

    IteratorImpl_ConnectedCells(
        const OcTree::Internal* oTreeInternal,
        const OcTree::Cell& cell,
        const IndexIterator& indexIterator = IndexIterator()
    )
    {
        myOTreeInternal = oTreeInternal;
        myBaseCell = cell;
        myIndexIterator = indexIterator;
        if(myIndexIterator.Empty())
            myIndexIterator = IndexIterator(oTreeInternal, cell, oTreeInternal->Levels() - 1);
        if(myIndexIterator.More())
        {
            auto upCell = *myIndexIterator;
            myCurrentCell = myOTreeInternal->GetFirstTerminalCell(upCell);
        }
    }

    virtual ~IteratorImpl_ConnectedCells() {}

    void advance() override
    {
        assert(myIndexIterator.More());

        while(myIndexIterator.More())
        {
            ++myIndexIterator;
            if(!myIndexIterator.More())
                break;

            OcTree::Cell upCell = *myIndexIterator;
            OcTree::Cell tCell = myOTreeInternal->GetFirstTerminalCell(upCell);
            if(myCurrentCell != tCell)
            {
                myCurrentCell = tCell;
                break;
            }
        }
    }

    OcTree::Cell current() const override
    {
        assert(myIndexIterator.More());
        return myCurrentCell;
    }

    IteratorImpl<OcTree::Cell>* clone() const override
    {
        return new IteratorImpl_ConnectedCells(myOTreeInternal, myBaseCell, myIndexIterator);
    }

    IteratorImpl<OcTree::Cell>* last() const override
    {
        IndexIterator ii = myIndexIterator;
        ii.SetEnd();
        return new IteratorImpl_ConnectedCells(myOTreeInternal, myBaseCell, ii);
    }

    bool end() const override
    {
        return !myIndexIterator.More();
    }

    bool equals(const IteratorImpl<OcTree::Cell>& other) const override
    {
        auto pOther = dynamic_cast<const IteratorImpl_ConnectedCells*>(&other);
        if(!pOther)
            return false;
        return myOTreeInternal == pOther->myOTreeInternal
            && myBaseCell == pOther->myBaseCell
            && myIndexIterator == pOther->myIndexIterator;
    }
};
}


Iterator<OcTree::Cell> OcTree::ConnectedCells(const Cell& cell) const
{
    auto itImpl = new IteratorImpl_ConnectedCells(pimpl, cell);
    return Iterator<Cell>(itImpl);
}


void OcTree::GetCellBox(
    const Cell& cell,
    Standard_Real& xMin,
    Standard_Real& yMin,
    Standard_Real& zMin,
    Standard_Real& xMax,
    Standard_Real& yMax,
    Standard_Real& zMax
) const
{
    Standard_Real xMinBox, yMinBox, zMinBox, xMaxBox, yMaxBox, zMaxBox;
    Ijk n = pimpl->LevelSize(cell.level);
    Ijk c = pimpl->GetCellCoords(cell);
    pimpl->boundBox.Get(xMinBox, yMinBox, zMinBox, xMaxBox, yMaxBox, zMaxBox);
    xMin = xMinBox + (xMaxBox - xMinBox) / n.i * c.i;
    xMax = xMinBox + (xMaxBox - xMinBox) / n.i * (c.i + 1);
    yMin = yMinBox + (yMaxBox - yMinBox) / n.j * c.j;
    yMax = yMinBox + (yMaxBox - yMinBox) / n.j * (c.j + 1);
    zMin = zMinBox + (zMaxBox - zMinBox) / n.k * c.k;
    zMax = zMinBox + (zMaxBox - zMinBox) / n.k * (c.k + 1);
}


Standard_Boolean OcTree::Dump(
    const std::filesystem::path& fileName
) const
{
    std::ofstream file(fileName);
    if(!file.is_open())
        return Standard_False;

    Standard_Integer nbCells = this->NbCells();
    Standard_Integer nbPoints = 8 * nbCells;

    file << "# vtk DataFile Version 3.0" << std::endl;
    file << "numgeom output" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;
    file << "POINTS " << nbPoints << " double" << std::endl;

    for(auto cell : this->TerminalCells())
    {
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        this->GetCellBox(cell, xMin, yMin, zMin, xMax, yMax, zMax);
        file << xMin << ' ' << yMin << ' ' << zMin << ' ';
        file << xMax << ' ' << yMin << ' ' << zMin << ' ';
        file << xMax << ' ' << yMax << ' ' << zMin << ' ';
        file << xMin << ' ' << yMax << ' ' << zMin << ' ';
        file << xMin << ' ' << yMin << ' ' << zMax << ' ';
        file << xMax << ' ' << yMin << ' ' << zMax << ' ';
        file << xMax << ' ' << yMax << ' ' << zMax << ' ';
        file << xMin << ' ' << yMax << ' ' << zMax << ' ';
        file << std::endl;
    }

    file << "CELLS " << nbCells << ' ' << 9 * nbCells << std::endl;
    for(Standard_Integer i = 0; i < nbCells; ++i)
        file << "8 "
             << 8*i+0 << ' '
             << 8*i+1 << ' '
             << 8*i+2 << ' '
             << 8*i+3 << ' '
             << 8*i+4 << ' '
             << 8*i+5 << ' '
             << 8*i+6 << ' '
             << 8*i+7
             << std::endl;
    file << std::endl;

    file << "CELL_TYPES " << nbCells << std::endl;
    for(size_t i = 0; i < nbCells; ++i)
        file << "12 "; //< VTK_HEXAHEDRON
    file << std::endl;
    return Standard_True;
}


Ijk OcTree::CellsNumberOnLevel(Standard_Integer level) const
{
    return pimpl->LevelSize(level);
}


void OcTree::Split(const Cell& cell)
{
    size_t N = pimpl->nonTerminalCells.size();
    assert(cell.level <= N);
    if(cell.level > N)
        return;

    if(!pimpl->IsExists(cell))
    {
        assert(false);
        return;
    }

    if(cell.level == N)
    {
        pimpl->nonTerminalCells.push_back({});
        auto& ntCells = pimpl->nonTerminalCells.back();
        Standard_Integer nc = pimpl->GetNbCellsOnLevel(cell.level);
        ntCells.reserve(std::min(32,nc));
    }

    auto& ntCells = pimpl->nonTerminalCells[cell.level];
    auto it = std::lower_bound(ntCells.begin(), ntCells.end(), cell.index);
    if(it == ntCells.end() || *it != cell.index)
        ntCells.insert(it, cell.index);
}


Ijk OcTree::GetCellCoords(const Cell& cell) const
{
    return pimpl->GetCellCoords(cell);
}


OcTree::Cell OcTree::GetCell(const gp_Pnt& Q) const
{
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    pimpl->boundBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real xQ = Q.X(), yQ = Q.Y(), zQ = Q.Z();

    // Точка расположена за пределами габаритной коробки дерева.
    if(xQ < xMin || xQ > xMax || yQ < yMin || yQ > yMax || zQ < zMin || zQ > zMax)
        return Cell();

    for(Standard_Integer level = 0; ; ++level)
    {
        Ijk n = pimpl->LevelSize(level);
        Standard_Real xStep = (xMax - xMin) / n.i;
        Standard_Real yStep = (yMax - yMin) / n.j;
        Standard_Real zStep = (zMax - zMin) / n.k;
        Ijk ijk(static_cast<Standard_Integer>(std::floor((xQ-xMin)/xStep)),
                static_cast<Standard_Integer>(std::floor((yQ-yMin)/yStep)),
                static_cast<Standard_Integer>(std::floor((zQ-zMin)/zStep)));
        Cell cell(level, Ijk::Index(ijk, n.i, n.j));
        if(pimpl->IsTerminal(cell))
            return cell;
    }
    return Cell();
}


size_t OcTree::GetAttr(const Cell& cell) const
{
    if(cell.level >= pimpl->attrs.size())
        return 0;

    const auto& map = pimpl->attrs[cell.level];
    auto it = map.find(cell.index);
    if(it == map.end())
        return 0;
    return it->second;
}


void OcTree::SetAttr(const Cell& cell, size_t attr) const
{
    assert(cell.level < this->Levels());
    while(cell.level >= pimpl->attrs.size())
        pimpl->attrs.push_back({});

    auto& map = pimpl->attrs[cell.level];
    map[cell.index] = attr;
}


Standard_Boolean OcTree::Equals(OcTree::CPtr other) const
{
    std::array<Standard_Real,6> bbCoords;
    pimpl->boundBox.Get(bbCoords[0],
                        bbCoords[1],
                        bbCoords[2],
                        bbCoords[3],
                        bbCoords[4],
                        bbCoords[5]);

    std::array<Standard_Real,6> bbCoords2;
    other->pimpl->boundBox.Get(bbCoords2[0],
                               bbCoords2[1],
                               bbCoords2[2],
                               bbCoords2[3],
                               bbCoords2[4],
                               bbCoords2[5]);

    for(int i = 0; i < 6; ++i)
    {
        Standard_Real dt = std::abs(bbCoords[i] - bbCoords2[i]);
        if(dt > 1.e-7)
            return Standard_False;
    }

    if(    pimpl->nbByI != other->pimpl->nbByI
        || pimpl->nbByJ != other->pimpl->nbByJ
        || pimpl->nbByK != other->pimpl->nbByK)
        return Standard_False;

    if(other->pimpl->nonTerminalCells != pimpl->nonTerminalCells)
        return Standard_False;

    return Standard_True;
}


void OcTree::Serialize(std::ostream& stream) const
{
    json data;
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    pimpl->boundBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    data["BoundBox"] = {xMin, yMin, zMin, xMax, yMax, zMax};
    data["ZeroLevelCellsNumbers"] = {pimpl->nbByI, pimpl->nbByJ, pimpl->nbByK};
    data["NonTerminalCells"] = pimpl->nonTerminalCells;

    if(!pimpl->attrs.empty())
        data["Attrs"] = pimpl->attrs;

    stream << data << std::endl;
}


OcTree::Ptr OcTree::Deserialize(std::istream& stream)
{
    json data = json::parse(stream);

    Bnd_Box boundBox;
    {
        const json& jBoundBox = data["BoundBox"];
        if(!jBoundBox.is_array() || jBoundBox.size() != 6)
            return OcTree::Ptr();
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        xMin = jBoundBox[0].get<Standard_Real>();
        yMin = jBoundBox[1].get<Standard_Real>();
        zMin = jBoundBox[2].get<Standard_Real>();
        xMax = jBoundBox[3].get<Standard_Real>();
        yMax = jBoundBox[4].get<Standard_Real>();
        zMax = jBoundBox[5].get<Standard_Real>();
        boundBox.Add(gp_Pnt(xMin,yMin,zMin));
        boundBox.Add(gp_Pnt(xMax,yMax,zMax));
    }

    Standard_Integer in, jn, kn;
    {
        const json& jZeroLevelCellsNumbers = data["ZeroLevelCellsNumbers"];
        if(!jZeroLevelCellsNumbers.is_array() || jZeroLevelCellsNumbers.size() != 3)
            return OcTree::Ptr();
        in = jZeroLevelCellsNumbers[0].get<Standard_Integer>();
        jn = jZeroLevelCellsNumbers[1].get<Standard_Integer>();
        kn = jZeroLevelCellsNumbers[2].get<Standard_Integer>();
    }

    auto oTree = OcTree::Create(boundBox, in, jn, kn);

    const json& jNonTerminalCells = data["NonTerminalCells"];
    if(!jNonTerminalCells.empty())
    {
        oTree->pimpl->nonTerminalCells =
            jNonTerminalCells.get<std::vector<std::vector<uint32_t>>>();
    }

    const json& jAttrs = data["Attrs"];
    if(!jAttrs.empty())
    {
        oTree->pimpl->attrs =
            jAttrs.get<std::vector<std::map<Standard_Integer,size_t>>>();
    }

    return oTree;
}


OcTree::Ptr OcTree::Create(
    const Bnd_Box& boundBox,
    Standard_Integer in,
    Standard_Integer jn,
    Standard_Integer kn
)
{
    if(in < 1 || jn < 1 || kn < 1)
        return OcTree::Ptr();

    return Ptr(new OcTree(boundBox, in, jn, kn));
}


namespace {;
OcTree::Cell GetParentCell(
    OcTree::Internal* qTreeIntenal,
    const OcTree::Cell& cell,
    Standard_Integer parentLevel
)
{
    assert(cell.level >= parentLevel);
    if(cell.level == parentLevel)
        return cell;

    Standard_Integer ni = qTreeIntenal->nbByI * (0x01 << cell.level);
    Standard_Integer nj = qTreeIntenal->nbByJ * (0x01 << cell.level);
    Ijk c = Ijk::Coords(cell.index, ni, nj);
    for(Standard_Integer l = cell.level; l > parentLevel; --l)
    {
        c /= 2;
        ni /= 2, nj /= 2;
    }

    return OcTree::Cell(parentLevel, Ijk::Index(c,ni,nj));
}
}


OcTree::CellConnection OcTree::GetConnectionType(
    const Cell& aCell,
    const Cell& bCell
) const
{
    // Если ячейки расположены на разных уровнях,
    // то приводим их к одному -- меньшему уровню.
    Standard_Integer oneLevel = std::min(aCell.level, bCell.level);
    Cell aCellNorm = GetParentCell(pimpl, aCell, oneLevel);
    Cell bCellNorm = GetParentCell(pimpl, bCell, oneLevel);
    Standard_Integer ni = pimpl->nbByI * (0x01 << oneLevel);
    Ijk ca = this->GetCellCoords(aCellNorm);
    Ijk cb = this->GetCellCoords(bCellNorm);

    if(ca.i == cb.i && ca.j == cb.j && ca.k == cb.k)
        return CellConnection::Inner;

    if(ca.i == cb.i && ca.j == cb.j && ca.k == cb.k+1)
        return CellConnection::DownSide;
    if(ca.i == cb.i && ca.j == cb.j && ca.k == cb.k-1)
        return CellConnection::UpSide;
    if(ca.i == cb.i+1 && ca.j == cb.j && ca.k == cb.k)
        return CellConnection::LeftSide;
    if(ca.i == cb.i-1 && ca.j == cb.j && ca.k == cb.k)
        return CellConnection::RightSide;
    if(ca.i == cb.i && ca.j == cb.j+1 && ca.k == cb.k)
        return CellConnection::FrontSide;
    if(ca.i == cb.i && ca.j == cb.j-1 && ca.k == cb.k)
        return CellConnection::BackSide;

    if(ca.i == cb.i+1 && ca.j == cb.j && ca.k == cb.k+1)
        return CellConnection::DownLeftRib;
    if(ca.i == cb.i-1 && ca.j == cb.j && ca.k == cb.k+1)
        return CellConnection::DownRightRib;
    if(ca.i == cb.i && ca.j == cb.j+1 && ca.k == cb.k+1)
        return CellConnection::DownFrontRib;
    if(ca.i == cb.i && ca.j == cb.j-1 && ca.k == cb.k+1)
        return CellConnection::DownBackRib;
    if(ca.i == cb.i+1 && ca.j == cb.j && ca.k == cb.k-1)
        return CellConnection::UpLeftRib;
    if(ca.i == cb.i-1 && ca.j == cb.j && ca.k == cb.k-1)
        return CellConnection::UpRightRib;
    if(ca.i == cb.i && ca.j == cb.j+1 && ca.k == cb.k-1)
        return CellConnection::UpFrontRib;
    if(ca.i == cb.i && ca.j == cb.j-1 && ca.k == cb.k-1)
        return CellConnection::UpBackRib;
    if(ca.i == cb.i+1 && ca.j == cb.j+1 && ca.k == cb.k)
        return CellConnection::LeftFrontRib;
    if(ca.i == cb.i-1 && ca.j == cb.j+1 && ca.k == cb.k)
        return CellConnection::RightFrontRib;
    if(ca.i == cb.i+1 && ca.j == cb.j-1 && ca.k == cb.k)
        return CellConnection::LeftBackRib;
    if(ca.i == cb.i-1 && ca.j == cb.j-1 && ca.k == cb.k)
        return CellConnection::RightBackRib;

    if(ca.i == cb.i+1 && ca.j == cb.j+1 && ca.k == cb.k+1)
        return CellConnection::DownLeftFrontCorner;
    if(ca.i == cb.i-1 && ca.j == cb.j+1 && ca.k == cb.k+1)
        return CellConnection::DownRightFrontCorner;
    if(ca.i == cb.i-1 && ca.j == cb.j-1 && ca.k == cb.k+1)
        return CellConnection::DownRightBackCorner;
    if(ca.i == cb.i+1 && ca.j == cb.j-1 && ca.k == cb.k+1)
        return CellConnection::DownLeftBackCorner;
    if(ca.i == cb.i+1 && ca.j == cb.j+1 && ca.k == cb.k-1)
        return CellConnection::UpLeftFrontCorner;
    if(ca.i == cb.i-1 && ca.j == cb.j+1 && ca.k == cb.k-1)
        return CellConnection::UpRightFrontCorner;
    if(ca.i == cb.i-1 && ca.j == cb.j-1 && ca.k == cb.k-1)
        return CellConnection::UpRightBackCorner;
    if(ca.i == cb.i+1 && ca.j == cb.j-1 && ca.k == cb.k-1)
        return CellConnection::UpLeftBackCorner;

    return CellConnection::Outer;
}


void OcTree::GetMinMaxDistances(
    const gp_Pnt& q,
    const Cell& cell,
    Standard_Real& minDistance,
    Standard_Real& maxDistance
) const
{
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    this->GetCellBox(cell, xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real xQ = q.X(), yQ = q.Y(), zQ = q.Z();
    if(    xQ >= xMin && xQ <= xMax
        && yQ >= yMin && yQ <= yMax
        && zQ >= zMin && zQ <= zMax)
    {
        minDistance = 0.0;
        maxDistance = 0.0;
        return ;
    }

    Standard_Real dxMin =
        std::min(std::abs(xQ - xMin),
                 std::abs(xQ - xMax));
    Standard_Real dxMax =
        std::max(std::abs(xQ - xMin),
                 std::abs(xQ - xMax));
    Standard_Real dyMin =
        std::min(std::abs(yQ - yMin),
                 std::abs(yQ - yMax));
    Standard_Real dyMax =
        std::max(std::abs(yQ - yMin),
                 std::abs(yQ - yMax));
    Standard_Real dzMin =
        std::min(std::abs(zQ - zMin),
                 std::abs(zQ - zMax));
    Standard_Real dzMax =
        std::max(std::abs(zQ - zMin),
                 std::abs(zQ - zMax));

    minDistance = std::sqrt(dxMin*dxMin + dyMin*dyMin + dzMin*dzMin);
    maxDistance = std::sqrt(dxMax*dxMax + dyMax*dyMax + dzMax*dzMax);
}


gp_Pnt OcTree::GetCenter(const Cell& cell) const
{
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    this->GetCellBox(cell, xMin, yMin, zMin, xMax, yMax, zMax);
    return gp_Pnt(0.5 * (xMin + xMax),
                  0.5 * (yMin + yMax),
                  0.5 * (zMin + zMax));
}


namespace {;

struct CellWithDistances
{
    OcTree::Cell cell;
    Standard_Real mn;
    Standard_Real mx;

    CellWithDistances(
        OcTree::CPtr tree,
        const OcTree::Cell& c,
        const gp_Pnt& q
    )
        : cell(c)
    {
        tree->GetMinMaxDistances(q, cell, mn, mx);
    }

    Standard_Boolean operator<(const CellWithDistances& other) const
    {
        assert(cell != other.cell);
        return mn < other.mn;
    }
};


void PushNonVisitedCellsToQueue(
    OcTree::CPtr tree,
    const OcTree::Cell& startCell,
    const gp_Pnt& Q,
    std::set<OcTree::Cell>& visited,
    std::priority_queue<CellWithDistances>& queue
)
{
    for(OcTree::Cell cell : tree->ConnectedCells(startCell))
    {
        if(!visited.insert(cell).second)
            continue;

        gp_Pnt C = tree->GetCenter(cell);
        Standard_Real d = C.Distance(Q);
        queue.push(CellWithDistances(tree,cell,Q));
    }
}
}


void SearchNearestCells(
    OcTree::CPtr tree,
    const gp_Pnt& Q,
    const std::function<Standard_Boolean(OcTree::CPtr,const OcTree::Cell&)>& isTaggedCell,
    Standard_Real maximumDistance,
    std::list<OcTree::Cell>& nearestCells
)
{
    nearestCells.clear();

    if(!tree)
        return;

    OcTree::Cell initCell = tree->GetCell(Q);
    if(isTaggedCell(tree,initCell))
    {
        nearestCells.push_back(initCell);
        return;
    }

    std::vector<CellWithDistances> result;
    std::set<OcTree::Cell> visited;
    std::priority_queue<CellWithDistances> queue;
    visited.insert(initCell);
    PushNonVisitedCellsToQueue(tree, initCell, Q, visited, queue);

    while(!queue.empty())
    {
        CellWithDistances cwd = queue.top();
        queue.pop();

        OcTree::Cell cell = cwd.cell;

        if(!isTaggedCell(tree,cell))
        {
            PushNonVisitedCellsToQueue(tree, cell, Q, visited, queue);
            continue;
        }

        if(cwd.mn > maximumDistance)
            continue;

        result.push_back(cwd);
    }

    std::sort(result.begin(), result.end());
    for(const auto& r : result)
        nearestCells.push_back(r.cell);
}
