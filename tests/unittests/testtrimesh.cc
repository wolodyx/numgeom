#include <format>

#include "gtest/gtest.h"

#include "numgeom/loadfromvtk.h"
#include "numgeom/trimeshconnectivity.h"
#include "numgeom/writetovtk.h"

#include "utilities.h"

TEST(TriMesh, LoadFromVtk) {
  TriMesh::Ptr mesh = LoadTriMeshFromVtk(TestData("polydata-cube.vtk"));
  ASSERT_TRUE(mesh != TriMesh::Ptr());
}

TEST(TriMesh, IsBoundaryNode) {
  TriMesh::Ptr mesh = LoadTriMeshFromVtk(TestData("k.vtk"));
  ASSERT_TRUE(mesh != TriMesh::Ptr());

  mesh->Dump("trimesh.vtk");

  auto connectivity = mesh->Connectivity();
  ASSERT_TRUE(connectivity != nullptr);

  ASSERT_EQ(connectivity->NbNodes(), mesh->NbNodes());
  ASSERT_EQ(connectivity->NbTrias(), mesh->NbCells());

  size_t nbNodes = mesh->NbNodes();
  std::vector<size_t> nodes;
  for (size_t iNode = 0; iNode < nbNodes; ++iNode) {
    TriMesh::Edge eIncoming, eOutcoming;
    bool isBoundaryNode =
        connectivity->IsBoundaryNode(iNode, &eIncoming, &eOutcoming);

    if (!isBoundaryNode) continue;

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

float Deviation(CTriMesh::Ptr mesh1, CTriMesh::Ptr mesh2) {
  float max_deviation = 0.0;
  size_t nodes_count = mesh1->NbNodes();
  for(size_t node_index = 0; node_index < nodes_count; ++node_index) {
    auto pt1 = mesh1->GetNode(node_index);
    auto pt2 = mesh2->GetNode(node_index);
    double d = glm::length(pt1 - pt2);
    if(d > max_deviation)
      max_deviation = d;
  }
  return max_deviation;
}

TEST(TriMesh, LoadAndWrite) {
  TriMesh::Ptr mesh = LoadTriMeshFromVtk(TestData("polydata-cube.vtk"));
  ASSERT_TRUE(mesh != TriMesh::Ptr());
  std::filesystem::path out_filename = GetTestName() + ".vtk";
  ASSERT_TRUE(WriteToPolydataVtk(mesh,out_filename));
  TriMesh::Ptr mesh_next = LoadTriMeshFromVtk(out_filename);
  ASSERT_EQ(mesh->NbNodes(), mesh_next->NbNodes());
  ASSERT_EQ(mesh->NbCells(), mesh_next->NbCells());
  ASSERT_LT(Deviation(mesh,mesh_next), 1.e-6);
}
