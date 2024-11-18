#ifndef numgeom_numgeom_trimesh_h
#define numgeom_numgeom_trimesh_h

#include <filesystem>
#include <memory>
#include <vector>

#include <gp_Pnt.hxx>

#include "numgeom/numgeom_export.h"

#define NONE_INDEX -1

class TriMeshConnectivity;


class NUMGEOM_EXPORT CTriMesh
{
public:

    typedef std::shared_ptr<CTriMesh> Ptr;

    struct Edge
    {
        Edge() : na(NONE_INDEX) {}
        Edge(Standard_Integer a, Standard_Integer b) : na(a), nb(b) {}
        Standard_Boolean operator!() const { return na == NONE_INDEX; }
        Edge Reversed() const { return Edge(nb,na); }
        void Reverse() { std::swap(na,nb); }

        Standard_Integer na, nb;
    };

    struct Cell
    {
        Cell() : na(NONE_INDEX) {}
        Cell(Standard_Integer a, Standard_Integer b, Standard_Integer c)
            : na(a), nb(b), nc(c) {}

        Edge GetEdge(Standard_Integer i) const
        {
            return Edge(*(&na + i%3), *(&na + (i+1)%3));
        }

        Standard_Integer na, nb, nc;
    };

public:

    virtual ~CTriMesh();

    Standard_Integer NbNodes() const;

    Standard_Integer NbCells() const;

    const gp_Pnt& GetNode(Standard_Integer) const;

    const Cell& GetCell(Standard_Integer) const;

    Standard_Boolean Dump(const std::filesystem::path&) const;

    TriMeshConnectivity* Connectivity() const;

protected:
    CTriMesh(Standard_Integer nbNodes, Standard_Integer nbCells);

private:
    CTriMesh(const CTriMesh&) = delete;
    void operator=(const CTriMesh&) = delete;

protected:
    std::vector<gp_Pnt> myNodes;
    std::vector<Cell> myCells;
    mutable TriMeshConnectivity* myConnectivity;
};


class TriMesh : public CTriMesh
{
public:

    typedef std::shared_ptr<TriMesh> Ptr;

public:

    static Ptr Create(
        Standard_Integer nbNodes,
        Standard_Integer nbCells
    );

public:

    virtual ~TriMesh();

    gp_Pnt& GetNode(Standard_Integer);

    Cell& GetCell(Standard_Integer);

private:
    TriMesh(Standard_Integer nbNodes, Standard_Integer nbCells);
};
#endif // !numgeom_numgeom_trimesh_h
