#ifndef numgeom_numgeom_trimesh_h
#define numgeom_numgeom_trimesh_h

#include <filesystem>
#include <memory>
#include <vector>

#include <gp_Pnt.hxx>

#include "numgeom/numgeom_export.h"


class NUMGEOM_EXPORT TriMesh
{
public:

    typedef std::shared_ptr<TriMesh> Ptr;
    typedef std::shared_ptr<const TriMesh> CPtr;

    struct Cell
    {
        Standard_Integer na, nb, nc;
    };

public:

    static Ptr Create(Standard_Integer nbNodes, Standard_Integer nbCells);

public:

    TriMesh();

    Standard_Integer NbNodes() const;

    Standard_Integer NbCells() const;

    const gp_Pnt& GetNode(Standard_Integer) const;

    const Cell& GetCell(Standard_Integer) const;

    gp_Pnt& GetNode(Standard_Integer);

    Cell& GetCell(Standard_Integer);

    Standard_Boolean Dump(const std::filesystem::path&) const;

private:
    TriMesh(Standard_Integer nbNodes, Standard_Integer nbCells);
    TriMesh(const TriMesh&) = delete;
    void operator=(const TriMesh&) = delete;

private:
    std::vector<gp_Pnt> myNodes;
    std::vector<Cell> myCells;
};
#endif // !numgeom_numgeom_trimesh_h
