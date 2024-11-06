#include "gtest/gtest.h"

#include "numgeom/quadtree.h"

#include "utilities.h"


namespace {;

QuadTree::Ptr LoadQuadTree(const std::filesystem::path& fileName)
{
    std::ifstream file(fileName);
    return QuadTree::Deserialize(file);
}
}


TEST(QuadTree, Example)
{
    Bnd_Box2d boundBox;
    boundBox.Add(gp_Pnt2d(0,0));
    boundBox.Add(gp_Pnt2d(3,2));

    QuadTree::Ptr qTree = QuadTree::Create(boundBox, 3, 2);
    qTree->Split(QuadTree::Cell(0,1));
    qTree->Split(QuadTree::Cell(0,2));
    qTree->Split(QuadTree::Cell(0,3));
    qTree->Split(QuadTree::Cell(1,2));
    qTree->Split(QuadTree::Cell(1,3));
    qTree->Split(QuadTree::Cell(1,9));
    qTree->Split(QuadTree::Cell(1,10));

    ASSERT_EQ(qTree->NbCells(), 27);
}


TEST(QuadTest, CreateQuadTree0)
{
    Bnd_Box2d boundBox;
    boundBox.Add(gp_Pnt2d(0.0,0.0));
    boundBox.Add(gp_Pnt2d(3.0,2.0));
    QuadTree::Ptr qTree = QuadTree::Create(boundBox, 3, 2);

    qTree->Split(QuadTree::Cell(0,1));
    qTree->Split(QuadTree::Cell(0,3));
    qTree->Split(QuadTree::Cell(0,4));

    qTree->Split(QuadTree::Cell(1,3));
    qTree->Split(QuadTree::Cell(1,9));
    qTree->Split(QuadTree::Cell(1,15));
    qTree->Split(QuadTree::Cell(1,18));
    qTree->Split(QuadTree::Cell(1,19));
    qTree->Split(QuadTree::Cell(1,20));
    qTree->Split(QuadTree::Cell(1,21));

    qTree->Split(QuadTree::Cell(2,6));
    qTree->Split(QuadTree::Cell(2,18));
    qTree->Split(QuadTree::Cell(2,30));
    qTree->Split(QuadTree::Cell(2,42));
    qTree->Split(QuadTree::Cell(2,54));
    qTree->Split(QuadTree::Cell(2,66));
    qTree->Split(QuadTree::Cell(2,78));
    qTree->Split(QuadTree::Cell(2,77));
    qTree->Split(QuadTree::Cell(2,76));
    qTree->Split(QuadTree::Cell(2,75));
    qTree->Split(QuadTree::Cell(2,74));
    qTree->Split(QuadTree::Cell(2,73));
    qTree->Split(QuadTree::Cell(2,72));

    ASSERT_EQ(qTree->NbCells(), 71);
}


TEST(QuadTree, SaveLoad)
{
    QuadTree::CPtr qTree = LoadQuadTree(TestData("quadtree-0.json"));
    ASSERT_TRUE(qTree != QuadTree::Ptr());

    std::string fileName = GetTestName();
    {
        std::ofstream file(fileName);
        qTree->Serialize(file);
    }

    QuadTree::CPtr qTreeCopy;
    {
        std::ifstream file(fileName);
        qTreeCopy = QuadTree::Deserialize(file);
    }
    ASSERT_TRUE(qTreeCopy != QuadTree::Ptr());
    ASSERT_TRUE(qTree->Equals(qTreeCopy));

}


TEST(QuadTree, IterateConnectedCells)
{
    auto fileName = TestData("quadtree-0.json");
    std::ifstream file(fileName);
    QuadTree::Ptr qTree = QuadTree::Deserialize(file);
    ASSERT_TRUE(qTree != nullptr);

    for(auto aCell : qTree->TerminalCells())
    {
        int count = 0;
        for(auto bCell : qTree->ConnectedCells(aCell))
        {
            ASSERT_TRUE(bCell != QuadTree::Cell());
            auto ct = qTree->GetConnectionType(aCell, bCell);
            ASSERT_TRUE(ct != QuadTree::CellConnection::Inner
                     && ct != QuadTree::CellConnection::Outer);
            ++count;
        }
        ASSERT_TRUE(count != 0);
    }
}


namespace {;
//! Поиск перебором наименьшего расстояния до отмеченных ячеек.
Standard_Real BruteForceCalcOfNearestDistance(
    QuadTree::CPtr qTree,
    const gp_Pnt2d& Q,
    const std::function<Standard_Boolean(QuadTree::CPtr,const QuadTree::Cell&)>& isTaggedCell
)
{
    Standard_Real minDistance = std::numeric_limits<double>::max();
    QuadTree::Cell nearestCell;
    for(QuadTree::Cell cell : qTree->TerminalCells())
    {
        if(!isTaggedCell(qTree,cell))
            continue;

        Standard_Real dMin, dMax;
        qTree->GetMinMaxDistances(Q, cell, dMin, dMax);
        if(minDistance > dMin)
        {
            minDistance = dMin;
            nearestCell = cell;
        }
    }
    return minDistance;
}
}


//! Ищем наименьшие расстояния от точки до ячеек наивысшего уровня.
TEST(QuadTree, SearchNearestCells)
{
    auto IsTaggedCell = [](QuadTree::CPtr qTree, const QuadTree::Cell& cell)
    {
        return cell.level == qTree->Levels() - 1;
    };

    QuadTree::Ptr qTree = LoadQuadTree(TestData("quadtree-0.json"));
    ASSERT_TRUE(qTree != QuadTree::Ptr());

    for(QuadTree::Cell cell : qTree->TerminalCells())
    {
        gp_Pnt2d q = qTree->GetCenter(cell);

        std::list<QuadTree::Cell> nearestCells;
        SearchNearestCells(
            qTree, q,
            IsTaggedCell,
            std::numeric_limits<double>::max(),
            nearestCells
        );

        ASSERT_FALSE(nearestCells.empty());
        Standard_Real distance = 0.0;
        for(QuadTree::Cell cell : nearestCells)
        {
            ASSERT_TRUE(IsTaggedCell(qTree,cell));
            Standard_Real dMin, dMax;
            qTree->GetMinMaxDistances(q, cell, dMin, dMax);
            ASSERT_GE(dMin, distance);
            distance = dMin;
        }

        Standard_Real nearestDistance = BruteForceCalcOfNearestDistance(qTree, q, IsTaggedCell);
        Standard_Real dMin, dMax;
        qTree->GetMinMaxDistances(q, nearestCells.front(), dMin, dMax);
        ASSERT_EQ(nearestDistance, dMin);
    }
}
