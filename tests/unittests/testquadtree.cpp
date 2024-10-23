#include "gtest/gtest.h"

#include "numgeom/quadtree.h"

#include "utilities.h"


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
    QuadTree::CPtr qTree;
    {
        std::ifstream file(TestData("quadtree-0.json"));
        qTree = QuadTree::Deserialize(file);
    }
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
