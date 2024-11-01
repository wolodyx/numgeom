#include "numgeom/quadtree.h"

#include <array>
#include <cassert>
#include <queue>
#include <map>
#include <stack>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;


struct QuadTree::Internal
{
    Bnd_Box2d boundBox;
    Standard_Integer nbByI, nbByJ;
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
        std::array<Standard_Integer,4>& cells
    ) const;

    //! Размерность подложки уровня `level`.
    void LevelSize(
        Standard_Integer level,
        Standard_Integer& ni,
        Standard_Integer& nj
    ) const;

    Standard_Integer GetNbCellsOnLevel(
        Standard_Integer level
    ) const;

    void GetCellCoords(
        const Cell& cell,
        Standard_Integer& i,
        Standard_Integer& j
    ) const;

    void NodeIndex2Coords(
        const Node& node,
        Standard_Integer& i,
        Standard_Integer& j
    ) const;

    Cell GetParentCell(const Cell& cell) const;

    Cell GetFirstTerminalCell(const Cell& startCell) const;

    Cell Next(const Cell& startCell) const;

    gp_Pnt2d PointCoords(
        const Node& node
    ) const;

    Node Normalize(const Node& node) const;

    void Nodes(
        const Cell& cell,
        std::array<Node,4>& nodes
    ) const;
};


Standard_Integer QuadTree::Internal::Levels() const
{
    return nonTerminalCells.size() + 1;
}


Standard_Boolean QuadTree::Internal::IsTerminal(const Cell& cell) const
{
    Standard_Integer levels = this->Levels();
    assert(cell.level < levels);

    if(cell.level == levels - 1)
        return Standard_True;

    const auto& ntCells = nonTerminalCells[cell.level];
    return !std::binary_search(ntCells.begin(), ntCells.end(), cell.index);
}


Standard_Integer QuadTree::Internal::GetOrder(const Cell& cell) const
{
    assert(cell.level != 0);
    Standard_Integer ni, nj;
    this->LevelSize(cell.level, ni, nj);
    Standard_Integer i = cell.index % ni;
    Standard_Integer j = cell.index / ni;
    Standard_Boolean b1 = (i % 2 == 0);
    Standard_Boolean b2 = (j % 2 == 0);
    if(b1 && b2)
        return 0;
    if(!b1 && b2)
        return 1;
    if(!b1 && !b2)
        return 2;
    return 3;
}


QuadTree::Cell QuadTree::Internal::GetChildCell(
    const Cell& parentCell,
    Standard_Integer childIndex
) const
{
    assert(childIndex >= 0 && childIndex < 4);
    Standard_Integer ni, nj;
    this->LevelSize(parentCell.level, ni, nj);
    Standard_Integer nc = ni * nj;
    Standard_Integer i = parentCell.index % ni;
    Standard_Integer j = parentCell.index / ni;
    ni *= 2, nj *= 2, i *= 2, j *= 2;
    switch(childIndex)
    {
    case 0: return Cell{parentCell.level+1, j*ni+i};
    case 1: return Cell{parentCell.level+1, j*ni+i+1};
    case 2: return Cell{parentCell.level+1, (j+1)*ni+i+1};
    case 3: return Cell{parentCell.level+1, (j+1)*ni+i};
    }
    return Cell{-1, -1};
}


void QuadTree::Internal::GetCellsOfNextLevel(
    const Cell& parentCell,
    std::array<Standard_Integer,4>& cells
) const
{
    Standard_Integer ni, nj;
    this->LevelSize(parentCell.level, ni, nj);

    Standard_Integer nc = ni * nj;

    // От порядкового номера ячейки переходим к координатам.
    Standard_Integer i = parentCell.index % ni;
    Standard_Integer j = parentCell.index / ni;

    ni *= 2, nj *= 2, i *= 2, j *= 2;
    cells[0] = j * ni + i;
    cells[1] = cells[0] + 1;
    cells[2] = (j + 1) * ni + i + 1;
    cells[3] = cells[2] - 1;
}


//! Размерность подложки уровня `level`.
void QuadTree::Internal::LevelSize(
    Standard_Integer level,
    Standard_Integer& ni,
    Standard_Integer& nj
) const
{
    assert(level >= 0);
    ni = nbByI * (0x01 << level);
    nj = nbByJ * (0x01 << level);
}


Standard_Integer QuadTree::Internal::GetNbCellsOnLevel(
    Standard_Integer level
) const
{
    Standard_Integer ni, nj;
    this->LevelSize(level, ni, nj);
    return ni * nj;
}


void QuadTree::Internal::GetCellCoords(
    const Cell& cell,
    Standard_Integer& i,
    Standard_Integer& j
) const
{
    Standard_Integer ni, nj;
    this->LevelSize(cell.level, ni, nj);
    i = cell.index % ni;
    j = cell.index / ni;
}


void QuadTree::Internal::NodeIndex2Coords(
    const Node& node,
    Standard_Integer& i,
    Standard_Integer& j
) const
{
    Standard_Integer ni, nj;
    this->LevelSize(node.level, ni, nj);
    i = node.index % (ni + 1);
    j = node.index / (ni + 1);
}


QuadTree::Cell QuadTree::Internal::GetParentCell(const Cell& cell) const
{
    if(cell.level == 0)
        return Cell();
    Standard_Integer i, j;
    this->GetCellCoords(cell, i, j);
    Standard_Integer ni, nj;
    this->LevelSize(cell.level, ni, nj);
    ni /= 2, nj /= 2, i /= 2, j /= 2;
    return Cell{cell.level - 1, j * ni + i};
}


Standard_Boolean QuadTree::Internal::IsExists(
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


QuadTree::Cell QuadTree::Internal::GetFirstTerminalCell(
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
    Standard_Integer i = startCell.index % ni;
    Standard_Integer j = startCell.index / ni;
    if(beNonTerminal)
    {
        // Если ячейка попадает в список нетерминальных,
        // то поднимаемся выше пока она оттуда не исключится.
        do
        {
            i *= 2, j *= 2, ni *= 2;
            ++level;

            if(level >= nonTerminalCells.size())
                break;
        }
        while(std::binary_search(nonTerminalCells[level].begin(),
                                 nonTerminalCells[level].end(),
                                 i+j*ni));

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
                                   i/2 + j/2*ni/2);
            if(beNonTerminalParentCell)
                break;

            i /= 2, j /= 2, ni /= 2;
            --level;
        }
    }

    return Cell(level,i + j * ni);
}


QuadTree::Cell QuadTree::Internal::Next(const Cell& startCell) const
{
    assert(this->IsTerminal(startCell));
    Cell cell = startCell;
    while(true)
    {
        if(cell.level == 0)
        {
            Standard_Integer nextCellIndex = cell.index + 1;
            if(nextCellIndex == nbByI * nbByJ)
                return Cell{0, nextCellIndex};
            cell = Cell{0, nextCellIndex};
            break;
        }

        Cell parentCell = this->GetParentCell(cell);
        Standard_Integer order = this->GetOrder(cell);
        if(order < 3)
        {
            cell = this->GetChildCell(parentCell, order + 1);
            break;
        }
        cell = parentCell;
    }
    return this->GetFirstTerminalCell(cell);
}


gp_Pnt2d QuadTree::Internal::PointCoords(
    const Node& node
) const
{
    Standard_Integer ni, nj;
    this->LevelSize(node.level, ni, nj);
    Standard_Integer i, j;
    this->NodeIndex2Coords(node, i, j);
    Standard_Real xMin, yMin, xMax, yMax;
    this->boundBox.Get(xMin, yMin, xMax, yMax);
    Standard_Real x = xMin + (xMax - xMin) / ni * i;
    Standard_Real y = yMin + (yMax - yMin) / nj * j;
    return gp_Pnt2d(x, y);
}


QuadTree::Node QuadTree::Internal::Normalize(const Node& node) const
{
    Standard_Integer ni, nj, i, j;
    this->NodeIndex2Coords(node, i, j);
    this->LevelSize(node.level, ni, nj);
    Standard_Integer levelDown = 0;
    if(node.level != 0)
    {
        while(i != 0 && j != 0 && i%2 == 0 && j%2 == 0 && ni%2 == 0 && nj%2 == 0)
        {
            i /= 2, j /= 2, ni /= 2, nj /= 2;
            ++levelDown;
        }
    }
    return Node(node.level - levelDown, j * (ni+1) + i);
}


void QuadTree::Internal::Nodes(
    const Cell& cell,
    std::array<Node,4>& nodes
) const
{
    Standard_Integer ni, nj, i, j;
    this->GetCellCoords(cell, i, j);
    this->LevelSize(cell.level, ni, nj);
    ni += 1, nj += 1;
    nodes[0] = this->Normalize(Node(cell.level,j*ni+i));
    nodes[1] = this->Normalize(Node(cell.level,j*ni+i+1));
    nodes[2] = this->Normalize(Node(cell.level,(j+1)*ni+i+1));
    nodes[3] = this->Normalize(Node(cell.level,(j+1)*ni+i));
}


QuadTree::QuadTree(
    const Bnd_Box2d& boundBox,
    Standard_Integer in,
    Standard_Integer jn
)
{
    pimpl = new Internal();
    pimpl->boundBox = boundBox;
    pimpl->nbByI = in;
    pimpl->nbByJ = jn;
    pimpl->nonTerminalCells.reserve(16);
    pimpl->attrs.reserve(16);
}


QuadTree::~QuadTree()
{
    delete pimpl;
}


Standard_Integer QuadTree::Levels() const
{
    return pimpl->Levels();
}


void QuadTree::GetBox(
    Standard_Real& xMin,
    Standard_Real& yMin,
    Standard_Real& xMax,
    Standard_Real& yMax
) const
{
    pimpl->boundBox.Get(xMin, yMin, xMax, yMax);
}


namespace {;
Standard_Integer CellsCount(
    const QuadTree::Internal* qTreeInternal,
    const QuadTree::Cell& parentCell
)
{
    typedef QuadTree::Cell Cell;

    Standard_Integer nbCells = 0;
    std::stack<Cell> stack;
    stack.push(parentCell);
    while(!stack.empty())
    {
        auto cell = stack.top();
        stack.pop();

        Standard_Boolean isTerminalCell = Standard_True;
        if(cell.level < qTreeInternal->nonTerminalCells.size())
        {
            const auto& ntCells = qTreeInternal->nonTerminalCells[cell.level];
            if(std::binary_search(ntCells.begin(), ntCells.end(), cell.index))
                isTerminalCell = Standard_False;
        }

        if(isTerminalCell)
        {
            ++nbCells;
            continue;
        }

        std::array<Standard_Integer, 4> nextCellsIndex;
        qTreeInternal->GetCellsOfNextLevel(cell, nextCellsIndex);
        stack.push(Cell{cell.level+1,nextCellsIndex[0]});
        stack.push(Cell{cell.level+1,nextCellsIndex[1]});
        stack.push(Cell{cell.level+1,nextCellsIndex[2]});
        stack.push(Cell{cell.level+1,nextCellsIndex[3]});
    }

    return nbCells;
}
}


Standard_Integer QuadTree::NbCells() const
{
    Standard_Integer nbCells = 0;
    Standard_Integer nbCellsOnLevel0 = pimpl->nbByI * pimpl->nbByJ;
    for(Standard_Integer i = 0; i < nbCellsOnLevel0; ++i)
    {
        Cell cell{0, i};
        nbCells += CellsCount(pimpl, cell);
    }
    return nbCells;
}


namespace {;
class IteratorImpl_QuadTreeCells : public IteratorImpl<QuadTree::Cell>
{
    const QuadTree::Internal* myQTreeInternal;
    QuadTree::Cell myCurrentCell; //!< Текущая терминальная ячейка.
public:

    IteratorImpl_QuadTreeCells(
        const QuadTree::Internal* qTreeInternal,
        const QuadTree::Cell& startCell = QuadTree::Cell()
    )
    {
        myQTreeInternal = qTreeInternal;
        myCurrentCell = startCell;
        if(!myCurrentCell)
            myCurrentCell = myQTreeInternal->GetFirstTerminalCell(QuadTree::Cell{0,0});
    }

    virtual ~IteratorImpl_QuadTreeCells() {}

    void advance() override
    {
        myCurrentCell = myQTreeInternal->Next(myCurrentCell);
    }

    QuadTree::Cell current() const override
    {
        return myCurrentCell;
    }

    IteratorImpl<QuadTree::Cell>* clone() const override
    {
        return new IteratorImpl_QuadTreeCells(myQTreeInternal, myCurrentCell);
    }

    IteratorImpl<QuadTree::Cell>* last() const override
    {
        Standard_Integer N = myQTreeInternal->nbByI * myQTreeInternal->nbByJ;
        return new IteratorImpl_QuadTreeCells(myQTreeInternal, QuadTree::Cell{0,N});
    }

    bool end() const override
    {
        return !myCurrentCell;
    }

    bool equals(const IteratorImpl<QuadTree::Cell>& other) const override
    {
        auto it = dynamic_cast<const IteratorImpl_QuadTreeCells*>(&other);
        if(!it)
            return false;
        return it->myQTreeInternal == this->myQTreeInternal
            && it->myCurrentCell == this->myCurrentCell;
    }
};
}


Iterator<QuadTree::Cell> QuadTree::TerminalCells() const
{
    auto itImpl = new IteratorImpl_QuadTreeCells(pimpl);
    return Iterator<Cell>(itImpl);
}


namespace {;
class IteratorImpl_QuadTreeCellsOfLevel : public IteratorImpl<QuadTree::Cell>
{
    const QuadTree::Internal* myQTreeInternal;
    Standard_Integer myLevel;
    Standard_Integer myCurrentIndex;
    Standard_Integer myNbCellsOnLevel;
    std::vector<uint32_t>::const_iterator itNtCells, itNtCellsEnd;
public:

    IteratorImpl_QuadTreeCellsOfLevel(
        const QuadTree::Internal* qTreeInternal,
        Standard_Integer level,
        Standard_Integer currentIndex = 0
    )
    {
        myQTreeInternal = qTreeInternal;
        myLevel = level;
        myCurrentIndex = currentIndex;
        myNbCellsOnLevel = myQTreeInternal->GetNbCellsOnLevel(level);

        static const std::vector<uint32_t> s_emptyVector;
        itNtCells = s_emptyVector.begin();
        itNtCellsEnd = s_emptyVector.end();
        if(level < myQTreeInternal->nonTerminalCells.size())
        {
            const auto& ntCells = myQTreeInternal->nonTerminalCells[level];
            itNtCells = ntCells.begin();
            itNtCellsEnd = ntCells.end();
        }

        // Настройка itNtCells
        while(itNtCells != itNtCellsEnd && myCurrentIndex > (*itNtCells))
            ++itNtCells;

        QuadTree::Cell cell(myLevel, myCurrentIndex);
        if(!myQTreeInternal->IsTerminal(cell))
            GoToNextTerminalCell();
    }

    virtual ~IteratorImpl_QuadTreeCellsOfLevel() {}


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

    QuadTree::Cell current() const override
    {
        return QuadTree::Cell(myLevel, myCurrentIndex);
    }

    IteratorImpl<QuadTree::Cell>* clone() const override
    {
        return new IteratorImpl_QuadTreeCellsOfLevel(myQTreeInternal, myLevel, myCurrentIndex);
    }

    IteratorImpl<QuadTree::Cell>* last() const override
    {
        return new IteratorImpl_QuadTreeCellsOfLevel(myQTreeInternal,
                                                     myLevel,
                                                     myNbCellsOnLevel);
    }

    bool end() const override
    {
        return myCurrentIndex == myNbCellsOnLevel;
    }

    bool equals(const IteratorImpl<QuadTree::Cell>& other) const override
    {
        auto it = dynamic_cast<const IteratorImpl_QuadTreeCellsOfLevel*>(&other);
        if(!it)
            return false;
        return it->myQTreeInternal == this->myQTreeInternal
            && it->myLevel == this->myLevel
            && it->myCurrentIndex == this->myCurrentIndex;
    }
};
}


Iterator<QuadTree::Cell> QuadTree::TerminalCellsOfLevel(
    Standard_Integer level
) const
{
    auto itImpl = new IteratorImpl_QuadTreeCellsOfLevel(pimpl,level);
    return Iterator<Cell>(itImpl);
}


namespace {;

//!\brief Итератор для обхода ячеек сетки уровня выше вокруг ячейки уровня ниже.
class IndexIterator
{
    //! Координаты базовой ячейки.
    Standard_Integer myICell, myJCell;
    //! Признак, что нижняя, правая, верхняя и левая сторона упираются в границу.
    Standard_Boolean myBlockedSize[4];
    //! Координаты текущей ячейки итератора.
    Standard_Integer myCurrentI,myCurrentJ;
    //! Уровень сетки, ячейки которой обходит итератор.
    Standard_Integer myUpLevel;

    Standard_Integer myQMultiple;
    Standard_Integer myISize;

public:

    IndexIterator()
    {
        myICell = -1, myJCell = -1;
        myCurrentI = -1, myCurrentJ = -1;
        myUpLevel = -1;
        myQMultiple = -1;
        myISize = -1;
    }

    IndexIterator(
        const QuadTree::Internal* qTreeInternal,
        const QuadTree::Cell& cell,
        Standard_Integer upLevel
    )
    {
        myUpLevel = upLevel;
        myQMultiple = (0x01 << (upLevel - cell.level));
        qTreeInternal->GetCellCoords(cell, myICell, myJCell);
        Standard_Integer iSize, jSize;
        qTreeInternal->LevelSize(cell.level, iSize, jSize);
        myISize = iSize;
        myBlockedSize[0] = (myJCell == 0);
        myBlockedSize[1] = (myICell == iSize - 1);
        myBlockedSize[2] = (myJCell == jSize - 1);
        myBlockedSize[3] = (myICell == 0);
        if(!myBlockedSize[0])
        {
            myCurrentI = myQMultiple * myICell;
            myCurrentJ = myQMultiple * myJCell - 1;
        }
        else if(!myBlockedSize[1])
        {
            myCurrentI = myQMultiple * (myICell + 1);
            myCurrentJ = myQMultiple * myJCell;
        }
        else if(!myBlockedSize[2])
        {
            myCurrentI = myQMultiple * (myICell + 1) - 1;
            myCurrentJ = myQMultiple * (myJCell + 1);
        }
        else if(!myBlockedSize[3])
        {
            myCurrentI = myQMultiple * myICell - 1;
            myCurrentJ = myQMultiple * (myJCell + 1) - 1;
        }
        else
        {
            myCurrentI = myQMultiple * myICell - 1;
            myCurrentJ = myQMultiple * myJCell - 1;
        }
    }

    IndexIterator& operator++()
    {
        assert(myCurrentI != -1 || myCurrentJ != -1);

        Standard_Integer iIndexMin = myQMultiple * myICell - 1;
        Standard_Integer iIndexMax = myQMultiple * (myICell + 1);
        Standard_Integer jIndexMin = myQMultiple * myJCell - 1;
        Standard_Integer jIndexMax = myQMultiple * (myJCell + 1);
        Standard_Boolean beOnSide[4] =
        {
            myCurrentJ == jIndexMin,
            myCurrentI == iIndexMax,
            myCurrentJ == jIndexMax,
            myCurrentI == iIndexMin
        };

        if(beOnSide[0])
        {
            if(beOnSide[1])
                ++myCurrentJ;
            else if(beOnSide[3])
            {
                myCurrentI = -1;
                myCurrentJ = -1;
            }
            else
            {
                ++myCurrentI; //< Перемещаемся вправо.
                // Уперлись в правую сторону?
                if(myCurrentI == iIndexMax)
                {
                    if(myBlockedSize[1])
                    {
                        if(!myBlockedSize[2])
                        {
                            myCurrentI = iIndexMax - 1;
                            myCurrentJ = jIndexMax;
                        }
                        else if(!myBlockedSize[3])
                        {
                            myCurrentI = iIndexMin;
                            myCurrentJ = jIndexMax - 1;
                        }
                        else
                        {
                            myCurrentI = -1;
                            myCurrentJ = -1;
                        }
                    }
                }
            }
        }
        else if(beOnSide[1])
        {
            if(beOnSide[2])
                --myCurrentI;
            else
            {
                ++myCurrentJ;
                // Уперлись в верхнюю сторону?
                if(myCurrentJ == jIndexMax)
                {
                    if(myBlockedSize[2])
                    {
                        if(!myBlockedSize[3])
                        {
                            myCurrentI = iIndexMin;
                            myCurrentJ = jIndexMax - 1;
                        }
                        else
                        {
                            myCurrentI = -1;
                            myCurrentJ = -1;
                        }
                    }
                }
            }
        }
        else if(beOnSide[2])
        {
            if(beOnSide[3])
                --myCurrentJ;
            else
            {
                --myCurrentI;
                // Уперлись в левую сторону?
                if(myCurrentI == iIndexMin)
                {
                    if(myBlockedSize[3])
                    {
                        myCurrentI = -1;
                        myCurrentJ = -1;
                    }
                }
            }
        }
        else if(beOnSide[3])
        {
            --myCurrentJ;
            // Уперлись в нижнюю сторону?
            if(myCurrentJ == jIndexMin)
            {
                if(myBlockedSize[0])
                {
                    myCurrentI = -1;
                    myCurrentJ = -1;
                }
            }
        }

        return *this;
    }

    QuadTree::Cell operator*() const
    {
        Standard_Integer index = myCurrentI + myCurrentJ * myISize * myQMultiple;
        return QuadTree::Cell(myUpLevel, index);
    }

    Standard_Boolean More() const
    {
        return myCurrentI != -1 || myCurrentJ != -1;
    }

    Standard_Boolean Empty() const
    {
        return myICell == -1 && myJCell == -1;
    }

    void SetEnd()
    {
        myCurrentI = -1;
        myCurrentJ = -1;
    }

    Standard_Boolean operator==(const IndexIterator& other) const
    {
        return myICell == other.myICell
            && myJCell == other.myJCell
            && myCurrentI == other.myCurrentI
            && myCurrentJ == other.myCurrentJ
            && myUpLevel == other.myUpLevel
            && myQMultiple == other.myQMultiple
            && myISize == other.myISize;

    }
};


class IteratorImpl_ConnectedCells : public IteratorImpl<QuadTree::Cell>
{
    const QuadTree::Internal* myQTreeInternal;
    QuadTree::Cell myBaseCell;
    IndexIterator myIndexIterator;
    QuadTree::Cell myCurrentCell;
public:

    IteratorImpl_ConnectedCells(
        const QuadTree::Internal* qTreeInternal,
        const QuadTree::Cell& cell,
        const IndexIterator& indexIterator = IndexIterator()
    )
    {
        myQTreeInternal = qTreeInternal;
        myBaseCell = cell;
        myIndexIterator = indexIterator;
        if(myIndexIterator.Empty())
            myIndexIterator = IndexIterator(qTreeInternal, cell, qTreeInternal->Levels() - 1);
        if(myIndexIterator.More())
        {
            auto upCell = *myIndexIterator;
            myCurrentCell = myQTreeInternal->GetFirstTerminalCell(upCell);
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

            QuadTree::Cell upCell = *myIndexIterator;
            QuadTree::Cell tCell = myQTreeInternal->GetFirstTerminalCell(upCell);
            if(myCurrentCell != tCell)
            {
                myCurrentCell = tCell;
                break;
            }
        }
    }

    QuadTree::Cell current() const override
    {
        assert(myIndexIterator.More());
        return myCurrentCell;
    }

    IteratorImpl<QuadTree::Cell>* clone() const override
    {
        return new IteratorImpl_ConnectedCells(myQTreeInternal, myBaseCell, myIndexIterator);
    }

    IteratorImpl<QuadTree::Cell>* last() const override
    {
        IndexIterator ii = myIndexIterator;
        ii.SetEnd();
        return new IteratorImpl_ConnectedCells(myQTreeInternal, myBaseCell, ii);
    }

    bool end() const override
    {
        return !myIndexIterator.More();
    }

    bool equals(const IteratorImpl<QuadTree::Cell>& other) const override
    {
        auto pOther = dynamic_cast<const IteratorImpl_ConnectedCells*>(&other);
        if(!pOther)
            return false;
        return myQTreeInternal == pOther->myQTreeInternal
            && myBaseCell == pOther->myBaseCell
            && myIndexIterator == pOther->myIndexIterator;
    }
};
}


Iterator<QuadTree::Cell> QuadTree::ConnectedCells(const Cell& cell) const
{
    auto itImpl = new IteratorImpl_ConnectedCells(pimpl, cell);
    return Iterator<Cell>(itImpl);
}


void QuadTree::GetCellBox(
    const Cell& cell,
    Standard_Real& xMin,
    Standard_Real& yMin,
    Standard_Real& xMax,
    Standard_Real& yMax
) const
{
    Standard_Integer ni, nj, i, j;
    Standard_Real xMinBox, yMinBox, xMaxBox, yMaxBox;
    pimpl->LevelSize(cell.level, ni, nj);
    pimpl->GetCellCoords(cell, i, j);
    pimpl->boundBox.Get(xMinBox, yMinBox, xMaxBox, yMaxBox);

    xMin = xMinBox + (xMaxBox - xMinBox) / ni * i;
    xMax = xMinBox + (xMaxBox - xMinBox) / ni * (i + 1);
    yMin = yMinBox + (yMaxBox - yMinBox) / nj * j;
    yMax = yMinBox + (yMaxBox - yMinBox) / nj * (j + 1);
}


Standard_Boolean QuadTree::Dump(
    const std::filesystem::path& fileName
) const
{
    std::ofstream file(fileName);
    if(!file.is_open())
        return Standard_False;

    Standard_Integer nbCells = this->NbCells();
    Standard_Integer nbPoints = 4 * nbCells;

    file << "# vtk DataFile Version 3.0" << std::endl;
    file << "numgeom output" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;
    file << "POINTS " << nbPoints << " double" << std::endl;

    for(auto cell : this->TerminalCells())
    {
        Standard_Real xMin, yMin, xMax, yMax;
        this->GetCellBox(cell, xMin, yMin, xMax, yMax);
        file << xMin << ' ' << yMin << " 0 ";
        file << xMax << ' ' << yMin << " 0 ";
        file << xMax << ' ' << yMax << " 0 ";
        file << xMin << ' ' << yMax << " 0 ";
        file << std::endl;
    }

    file << "CELLS " << nbCells << ' ' << 5 * nbCells << std::endl;
    for(Standard_Integer i = 0; i < nbCells; ++i)
        file << "4 " << 4*i << ' ' << 4*i+1 << ' ' << 4*i+2 << ' ' << 4*i+3 << std::endl;
    file << std::endl;

    file << "CELL_TYPES " << nbCells << std::endl;
    for(size_t i = 0; i < nbCells; ++i)
        file << "9 ";
    file << std::endl;
    return Standard_True;
}


void QuadTree::CellsNumberOnLevel(
    Standard_Integer level,
    Standard_Integer& nbByI,
    Standard_Integer& nbByJ
) const
{
    pimpl->LevelSize(level, nbByI, nbByJ);
}


void QuadTree::Split(const Cell& cell)
{
    size_t N = pimpl->nonTerminalCells.size();
    assert(cell.level <= N);
    if(cell.level > N)
        return;

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


void QuadTree::GetCellCoords(
    const Cell& cell,
    Standard_Integer& i,
    Standard_Integer& j
) const
{
    pimpl->GetCellCoords(cell, i, j);
}


QuadTree::Cell QuadTree::GetCell(const gp_Pnt2d& Q) const
{
    Standard_Real xMin, yMin, xMax, yMax;
    pimpl->boundBox.Get(xMin, yMin, xMax, yMax);

    Standard_Real xQ = Q.X(), yQ = Q.Y();

    // Точка расположена за пределами габаритной коробки дерева.
    if(xQ < xMin || xQ > xMax || yQ < yMin || yQ > yMax)
        return Cell();

    for(Standard_Integer level = 0; ; ++level)
    {
        Standard_Integer ni, nj;
        pimpl->LevelSize(level, ni, nj);
        Standard_Real xStep = (xMax - xMin) / ni;
        Standard_Real yStep = (yMax - yMin) / nj;
        Standard_Integer i = static_cast<Standard_Integer>(std::floor((xQ-xMin)/xStep));
        Standard_Integer j = static_cast<Standard_Integer>(std::floor((yQ-yMin)/yStep));
        Cell cell(level, i + j*ni);
        if(pimpl->IsTerminal(cell))
            return cell;
    }
    return Cell();
}


size_t QuadTree::GetAttr(const Cell& cell) const
{
    if(cell.level >= pimpl->attrs.size())
        return 0;

    const auto& map = pimpl->attrs[cell.level];
    auto it = map.find(cell.index);
    if(it == map.end())
        return 0;
    return it->second;
}


void QuadTree::SetAttr(const Cell& cell, size_t attr) const
{
    assert(cell.level < this->Levels());
    while(cell.level >= pimpl->attrs.size())
        pimpl->attrs.push_back({});

    auto& map = pimpl->attrs[cell.level];
    map[cell.index] = attr;
}


Standard_Boolean QuadTree::Equals(QuadTree::CPtr other) const
{
    std::array<Standard_Real, 4> bbCoords;
    pimpl->boundBox.Get(bbCoords[0],
                        bbCoords[1],
                        bbCoords[2],
                        bbCoords[3]);

    std::array<Standard_Real, 4> bbCoords2;
    other->pimpl->boundBox.Get(bbCoords2[0],
                              bbCoords2[1],
                              bbCoords2[2],
                              bbCoords2[3]);

    for(int i = 0; i < 4; ++i)
    {
        Standard_Real dt = std::abs(bbCoords[i] - bbCoords2[i]);
        if(dt > 1.e-7)
            return Standard_False;
    }

    if(pimpl->nbByI != other->pimpl->nbByI || pimpl->nbByJ != other->pimpl->nbByJ)
        return Standard_False;

    if(other->pimpl->nonTerminalCells != pimpl->nonTerminalCells)
        return Standard_False;

    return Standard_True;
}


void QuadTree::Serialize(std::ostream& stream) const
{
    json data;
    Standard_Real xMin, yMin, xMax, yMax;
    pimpl->boundBox.Get(xMin, yMin, xMax, yMax);
    data["BoundBox"] = {xMin, yMin, xMax, yMax};
    data["ZeroLevelCellsNumbers"] = {pimpl->nbByI, pimpl->nbByJ};
    data["NonTerminalCells"] = pimpl->nonTerminalCells;

    if(!pimpl->attrs.empty())
        data["Attrs"] = pimpl->attrs;

    stream << data << std::endl;
}


QuadTree::Ptr QuadTree::Deserialize(std::istream& stream)
{
    json data = json::parse(stream);

    Bnd_Box2d boundBox;
    {
        const json& jBoundBox = data["BoundBox"];
        if(!jBoundBox.is_array() || jBoundBox.size() != 4)
            return QuadTree::Ptr();
        Standard_Real xMin, yMin, xMax, yMax;
        xMin = jBoundBox[0].get<Standard_Real>();
        yMin = jBoundBox[1].get<Standard_Real>();
        xMax = jBoundBox[2].get<Standard_Real>();
        yMax = jBoundBox[3].get<Standard_Real>();
        boundBox.Add(gp_Pnt2d(xMin,yMin));
        boundBox.Add(gp_Pnt2d(xMax,yMax));
    }

    Standard_Integer in, jn;
    {
        const json& jZeroLevelCellsNumbers = data["ZeroLevelCellsNumbers"];
        if(!jZeroLevelCellsNumbers.is_array() || jZeroLevelCellsNumbers.size() != 2)
            return QuadTree::Ptr();
        in = jZeroLevelCellsNumbers[0].get<Standard_Integer>();
        jn = jZeroLevelCellsNumbers[1].get<Standard_Integer>();
    }

    auto qTree = QuadTree::Create(boundBox, in, jn);

    const json& jNonTerminalCells = data["NonTerminalCells"];
    if(!jNonTerminalCells.empty())
    {
        qTree->pimpl->nonTerminalCells =
            jNonTerminalCells.get<std::vector<std::vector<uint32_t>>>();
    }

    const json& jAttrs = data["Attrs"];
    if(!jAttrs.empty())
    {
        qTree->pimpl->attrs =
            jAttrs.get<std::vector<std::map<Standard_Integer,size_t>>>();
    }

    return qTree;
}


QuadTree::Ptr QuadTree::Create(
    const Bnd_Box2d& boundBox,
    Standard_Integer in,
    Standard_Integer jn
)
{
    if(in < 1 || jn < 1)
        return QuadTree::Ptr();

    return Ptr(new QuadTree(boundBox, in, jn));
}


namespace {;
QuadTree::Cell GetParentCell(
    QuadTree::Internal* qTreeIntenal,
    const QuadTree::Cell& cell,
    Standard_Integer parentLevel
)
{
    assert(cell.level >= parentLevel);
    if(cell.level == parentLevel)
        return cell;

    Standard_Integer ni = qTreeIntenal->nbByI * (0x01 << cell.level);
    Standard_Integer i = cell.index % ni;
    Standard_Integer j = cell.index / ni;
    for(Standard_Integer l = cell.level; l > parentLevel; --l)
    {
        ni /= 2, i /= 2, j /= 2;
    }

    return QuadTree::Cell(parentLevel, i + j*ni);
}
}


QuadTree::CellConnection QuadTree::GetConnectionType(
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
    Standard_Integer ia = aCellNorm.index % ni;
    Standard_Integer ja = aCellNorm.index / ni;
    Standard_Integer ib = bCellNorm.index % ni;
    Standard_Integer jb = bCellNorm.index / ni;
    if(ja == jb)
    {
        if(ia == ib)
            return CellConnection::Inner;
        if(ia == ib - 1)
            return CellConnection::RightSide;
        if(ia == ib + 1)
            return CellConnection::LeftSide;
        return CellConnection::Outer;
    }
    else if(ja == jb - 1)
    {
        if(ia == ib)
            return CellConnection::UpSide;
        if(ia == ib - 1)
            return CellConnection::UpRightCorner;
        if(ia == ib + 1)
            return CellConnection::UpLefttCorner;
        return CellConnection::Outer;
    }
    else if(ja == jb + 1)
    {
        if(ia == ib)
            return CellConnection::DownSide;
        if(ia == ib - 1)
            return CellConnection::DownRightCorner;
        if(ia == ib + 1)
            return CellConnection::DownLeftCorner;
        return CellConnection::Outer;
    }
    return CellConnection::Outer;
}


void QuadTree::GetMinMaxDistances(
    const gp_Pnt2d& q,
    const Cell& cell,
    Standard_Real& minDistance,
    Standard_Real& maxDistance
) const
{
    Standard_Real xMin, yMin, xMax, yMax;
    this->GetCellBox(cell, xMin, yMin, xMax, yMax);

    Standard_Real xQ = q.X(), yQ = q.Y();
    if(xQ >= xMin && xQ <= xMax && yQ >= yMin && yQ <= yMax)
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

    minDistance = std::sqrt(dxMin*dxMin + dyMin*dyMin);
    maxDistance = std::sqrt(dxMax*dxMax + dyMax*dyMax);
}


gp_Pnt2d QuadTree::GetCenter(const Cell& cell) const
{
    Standard_Real xMin, yMin, xMax, yMax;
    this->GetCellBox(cell, xMin, yMin, xMax, yMax);
    return gp_Pnt2d(0.5 * (xMin + xMax),
                    0.5 * (yMin + yMax));
}


namespace {;

struct CellWithDistances
{
    QuadTree::Cell cell;
    Standard_Real mn;
    Standard_Real mx;

    CellWithDistances(
        QuadTree::CPtr qTree,
        const QuadTree::Cell& c,
        const gp_Pnt2d& q
    )
        : cell(c)
    {
        qTree->GetMinMaxDistances(q, cell, mn, mx);
    }

    Standard_Boolean operator<(const CellWithDistances& other) const
    {
        assert(cell != other.cell);
        return mn < other.mn;
    }
};


void PushNonVisitedCellsToQueue(
    QuadTree::CPtr qTree,
    const QuadTree::Cell& startCell,
    const gp_Pnt2d& Q,
    std::set<QuadTree::Cell>& visited,
    std::priority_queue<CellWithDistances>& queue
)
{
    for(QuadTree::Cell cell : qTree->ConnectedCells(startCell))
    {
        if(!visited.insert(cell).second)
            continue;

        gp_Pnt2d C = qTree->GetCenter(cell);
        Standard_Real d = C.Distance(Q);
        queue.push(CellWithDistances(qTree,cell,Q));
    }
}
}


void SearchNearestCells(
    QuadTree::CPtr qTree,
    const gp_Pnt2d& Q,
    const std::function<Standard_Boolean(QuadTree::CPtr,const QuadTree::Cell&)>& isTaggedCell,
    Standard_Real maximumDistance,
    std::list<QuadTree::Cell>& nearestCells
)
{
    nearestCells.clear();

    if(!qTree)
        return;

    QuadTree::Cell initCell = qTree->GetCell(Q);
    if(isTaggedCell(qTree,initCell))
    {
        nearestCells.push_back(initCell);
        return;
    }

    std::vector<CellWithDistances> result;
    std::set<QuadTree::Cell> visited;
    std::priority_queue<CellWithDistances> queue;
    visited.insert(initCell);
    PushNonVisitedCellsToQueue(qTree, initCell, Q, visited, queue);

    while(!queue.empty())
    {
        CellWithDistances cwd = queue.top();
        queue.pop();

        QuadTree::Cell cell = cwd.cell;

        if(!isTaggedCell(qTree,cell))
        {
            PushNonVisitedCellsToQueue(qTree, cell, Q, visited, queue);
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
