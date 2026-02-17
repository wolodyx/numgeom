#include "numgeom/trimesh.h"

#include <algorithm>
#include <fstream>

#include "numgeom/trimeshconnectivity.h"


CTriMesh::~CTriMesh()
{
    delete myConnectivity;
}


CTriMesh::CTriMesh(size_t nbNodes, size_t nbCells)
    : myNodes(nbNodes), myCells(nbCells)
{
    myConnectivity = nullptr;
}


size_t CTriMesh::NbNodes() const
{
    return myNodes.size();
}


size_t CTriMesh::NbCells() const
{
    return myCells.size();
}


const glm::dvec3& CTriMesh::GetNode(size_t index) const
{
    return myNodes[index];
}


const CTriMesh::Cell& CTriMesh::GetCell(size_t index) const
{
    return myCells[index];
}


glm::dvec3& TriMesh::GetNode(size_t index)
{
    return myNodes[index];
}


CTriMesh::Cell& TriMesh::GetCell(size_t index)
{
    return myCells[index];
}


TriMesh::Ptr TriMesh::Create(size_t nbNodes, size_t nbCells)
{
    if(nbNodes == 0 || nbCells == 0)
        return Ptr();

    return Ptr(new TriMesh(nbNodes,nbCells));
}


TriMeshConnectivity* CTriMesh::Connectivity() const
{
    if(!myConnectivity)
        myConnectivity = new TriMeshConnectivity(this->NbNodes(),
                                                 this->NbCells(),
                                                 myCells.data());
    return myConnectivity;
}


TriMesh::TriMesh(size_t nbNodes, size_t nbCells)
    : CTriMesh(nbNodes, nbCells)
{
}


TriMesh::~TriMesh()
{
}


bool CTriMesh::Dump(
    const std::filesystem::path& fileName
) const
{
    std::ofstream file(fileName);
    if(!file.is_open())
        return false;

    file << "# vtk DataFile Version 3.0" << std::endl;
    file << "numgeom output" << std::endl;
    file << "ASCII" << std::endl;
    file << "DATASET UNSTRUCTURED_GRID" << std::endl;

    size_t nbNodes = this->NbNodes();
    file << "POINTS " << nbNodes << " double" << std::endl;
    for(size_t i = 0; i < nbNodes; ++i)
    {
        const glm::dvec3& pt = this->GetNode(i);
        file << pt.x << ' ' << pt.y << ' ' << pt.z << ' ';
    }
    file << std::endl;

    size_t nbCells = this->NbCells();
    file << "CELLS " << nbCells << ' ' << 4 * nbCells << std::endl;
    for(size_t i = 0; i < nbCells; ++i)
    {
        const CTriMesh::Cell& cell = this->GetCell(i);
        file << "3 " << cell.na << ' ' << cell.nb << ' ' << cell.nc << ' ';
    }
    file << std::endl;

    file << "CELL_TYPES " << nbCells << std::endl;
    for(size_t i = 0; i < nbCells; ++i)
        file << "5 "; //< VTK_TRIANGLE
    file << std::endl;

    return true;
}


TriMesh::Ptr TriMesh::Create(
    const std::vector<glm::dvec3>& nodes,
    const std::vector<Cell>& cells
)
{
    auto mesh = Ptr(new TriMesh(0,0));
    mesh->myNodes = std::move(nodes);
    mesh->myCells = std::move(cells);
    return mesh;
}


void TriMesh::Transform(const glm::dmat4& tr)
{
    std::transform(
        myNodes.begin(),
        myNodes.end(),
        myNodes.begin(),
        [&](const glm::dvec3& pt)
        {
            return tr * glm::dvec4(pt,0.0);
        }
    );
}
