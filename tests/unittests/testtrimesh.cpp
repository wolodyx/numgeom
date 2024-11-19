#include "gtest/gtest.h"

#include "numgeom/loadfromvtk.h"
#include "numgeom/trimeshconnectivity.h"

#include "utilities.h"


TEST(TriMesh, LoadFromVtk)
{
    TriMesh::Ptr mesh = LoadTriMeshFromVtk(TestData("k.vtk"));
    ASSERT_TRUE(mesh != TriMesh::Ptr());
}


TEST(TriMesh, IsBoundaryNode)
{
    TriMesh::Ptr mesh = LoadTriMeshFromVtk(TestData("k.vtk"));
    ASSERT_TRUE(mesh != TriMesh::Ptr());

    mesh->Dump("trimesh.vtk");

    auto connectivity = mesh->Connectivity();
    ASSERT_TRUE(connectivity != nullptr);

    ASSERT_EQ(connectivity->NbNodes(), mesh->NbNodes());
    ASSERT_EQ(connectivity->NbTrias(), mesh->NbCells());

    Standard_Integer nbNodes = mesh->NbNodes();
    std::vector<Standard_Integer> nodes;
    for(Standard_Integer iNode = 0; iNode < nbNodes; ++iNode)
    {
        TriMesh::Edge eIncoming, eOutcoming;
        Standard_Boolean isBoundaryNode =
            connectivity->IsBoundaryNode(iNode,
                                         &eIncoming,
                                         &eOutcoming);

        if(!isBoundaryNode)
            continue;

        ASSERT_TRUE(!eIncoming.empty());
        ASSERT_TRUE(!eOutcoming.empty());
        ASSERT_NE(eIncoming, eOutcoming);
        ASSERT_NE(eIncoming, eOutcoming.Reversed());
        ASSERT_EQ(eIncoming.nb, iNode);
        ASSERT_EQ(eOutcoming.na, iNode);
        ASSERT_TRUE(connectivity->IsBoundaryNode(eIncoming.na));
        ASSERT_TRUE(connectivity->IsBoundaryNode(eOutcoming.nb));
        connectivity->Node2Nodes(iNode, nodes);
        ASSERT_EQ(nodes.front(), eOutcoming.nb);
        ASSERT_EQ(nodes.back(), eIncoming.na);
    }
}
