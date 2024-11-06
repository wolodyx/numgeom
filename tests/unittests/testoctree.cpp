#include "gtest/gtest.h"

#include "numgeom/octree.h"

#include "utilities.h"


namespace {;

OcTree::Ptr LoadOcTree(const std::filesystem::path& fileName)
{
    std::ifstream file(fileName);
    return OcTree::Deserialize(file);
}
}


TEST(OcTree, Example)
{
    Bnd_Box boundBox;
    boundBox.Add(gp_Pnt(0,0,0));
    boundBox.Add(gp_Pnt(3,2,1));

    auto oTree = OcTree::Create(boundBox, 3, 2, 1);
    oTree->Split(OcTree::Cell(0,1));
    oTree->Split(OcTree::Cell(0,2));
    oTree->Split(OcTree::Cell(0,3));
    oTree->Split(OcTree::Cell(1,2));
    oTree->Split(OcTree::Cell(1,3));
    oTree->Split(OcTree::Cell(1,9));
    oTree->Split(OcTree::Cell(1,10));

    ASSERT_EQ(oTree->NbCells(), 55);
}


TEST(OcTree, CreateOcTree0)
{
    Bnd_Box boundBox;
    boundBox.Add(gp_Pnt(0,0,0));
    boundBox.Add(gp_Pnt(3,2,1));
    OcTree::Ptr tree = OcTree::Create(boundBox, 3, 2, 1);

    tree->Split(OcTree::Cell(0,1));
    tree->Split(OcTree::Cell(0,3));
    tree->Split(OcTree::Cell(0,4));

    tree->Split(OcTree::Cell(1,3));
    tree->Split(OcTree::Cell(1,9));
    tree->Split(OcTree::Cell(1,15));
    tree->Split(OcTree::Cell(1,18));
    tree->Split(OcTree::Cell(1,19));
    tree->Split(OcTree::Cell(1,20));
    tree->Split(OcTree::Cell(1,21));

    tree->Split(OcTree::Cell(2,6));
    tree->Split(OcTree::Cell(2,18));
    tree->Split(OcTree::Cell(2,30));
    tree->Split(OcTree::Cell(2,42));
    tree->Split(OcTree::Cell(2,54));
    tree->Split(OcTree::Cell(2,66));
    tree->Split(OcTree::Cell(2,78));
    tree->Split(OcTree::Cell(2,77));
    tree->Split(OcTree::Cell(2,76));
    tree->Split(OcTree::Cell(2,75));
    tree->Split(OcTree::Cell(2,74));
    tree->Split(OcTree::Cell(2,73));
    tree->Split(OcTree::Cell(2,72));

    ASSERT_EQ(tree->NbCells(), 167);
}


TEST(OcTree, SaveLoad)
{
    OcTree::CPtr tree = LoadOcTree(TestData("octree-0.json"));
    ASSERT_TRUE(tree != OcTree::Ptr());

    std::string fileName = GetTestName();
    {
        std::ofstream file(fileName);
        tree->Serialize(file);
    }

    OcTree::CPtr qTreeCopy;
    {
        std::ifstream file(fileName);
        qTreeCopy = OcTree::Deserialize(file);
    }
    ASSERT_TRUE(qTreeCopy != OcTree::Ptr());
    ASSERT_TRUE(tree->Equals(qTreeCopy));

}


TEST(OcTree, IterateConnectedCells)
{
    auto fileName = TestData("octree-0.json");
    std::ifstream file(fileName);
    OcTree::Ptr tree = OcTree::Deserialize(file);
    ASSERT_TRUE(tree != nullptr);

    for(auto aCell : tree->TerminalCells())
    {
        int count = 0;
        for(auto bCell : tree->ConnectedCells(aCell))
        {
            ASSERT_TRUE(bCell != OcTree::Cell());
            auto ct = tree->GetConnectionType(aCell, bCell);
            ASSERT_TRUE(ct != OcTree::CellConnection::Inner
                     && ct != OcTree::CellConnection::Outer);
            ++count;
        }
        ASSERT_TRUE(count != 0);
    }
}


namespace {;
//! ѕоиск перебором наименьшего рассто€ни€ до отмеченных €чеек.
Standard_Real BruteForceCalcOfNearestDistance(
    OcTree::CPtr tree,
    const gp_Pnt& Q,
    const std::function<Standard_Boolean(OcTree::CPtr,const OcTree::Cell&)>& isTaggedCell
)
{
    Standard_Real minDistance = std::numeric_limits<double>::max();
    OcTree::Cell nearestCell;
    for(OcTree::Cell cell : tree->TerminalCells())
    {
        if(!isTaggedCell(tree,cell))
            continue;

        Standard_Real dMin, dMax;
        tree->GetMinMaxDistances(Q, cell, dMin, dMax);
        if(minDistance > dMin)
        {
            minDistance = dMin;
            nearestCell = cell;
        }
    }
    return minDistance;
}
}


//! »щем наименьшие рассто€ни€ от точки до €чеек наивысшего уровн€.
TEST(OcTree, SearchNearestCells)
{
    auto IsTaggedCell = [](OcTree::CPtr tree, const OcTree::Cell& cell)
    {
        return cell.level == tree->Levels() - 1;
    };

    OcTree::Ptr tree = LoadOcTree(TestData("OcTree-0.json"));
    ASSERT_TRUE(tree != OcTree::Ptr());

    for(OcTree::Cell cell : tree->TerminalCells())
    {
        gp_Pnt q = tree->GetCenter(cell);

        std::list<OcTree::Cell> nearestCells;
        SearchNearestCells(
            tree, q,
            IsTaggedCell,
            std::numeric_limits<double>::max(),
            nearestCells
        );

        ASSERT_FALSE(nearestCells.empty());
        Standard_Real distance = 0.0;
        for(OcTree::Cell cell : nearestCells)
        {
            ASSERT_TRUE(IsTaggedCell(tree,cell));
            Standard_Real dMin, dMax;
            tree->GetMinMaxDistances(q, cell, dMin, dMax);
            ASSERT_GE(dMin, distance);
            distance = dMin;
        }

        Standard_Real nearestDistance = BruteForceCalcOfNearestDistance(tree, q, IsTaggedCell);
        Standard_Real dMin, dMax;
        tree->GetMinMaxDistances(q, nearestCells.front(), dMin, dMax);
        ASSERT_EQ(nearestDistance, dMin);
    }
}
