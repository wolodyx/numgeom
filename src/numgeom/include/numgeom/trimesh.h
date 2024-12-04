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
        Edge() : na(NONE_INDEX), nb(NONE_INDEX) {}

        Edge(Standard_Integer a, Standard_Integer b) : na(a), nb(b) {}

        Standard_Boolean empty() const { return na == NONE_INDEX; }

        Standard_Boolean operator!() const { return this->empty(); }

        Standard_Boolean operator==(const Edge& other) const
        {
            return na == other.na && nb == other.nb;
        }

        Standard_Boolean operator!=(const Edge& other) const
        {
            return !this->operator==(other);
        }

        Edge Reversed() const { return Edge(nb,na); }

        void Reverse() { std::swap(na,nb); }

        Standard_Integer na, nb;
    };

    struct Cell
    {
        Cell() : na(NONE_INDEX) {}

        Cell(const Standard_Integer* nodes)
            : na(nodes[0]), nb(nodes[1]), nc(nodes[2]) {}

        Cell(Standard_Integer a, Standard_Integer b, Standard_Integer c)
            : na(a), nb(b), nc(c) {}

        Edge GetEdge(Standard_Integer i) const
        {
            return Edge(*(&na + i%3), *(&na + (i+1)%3));
        }

        Edge GetIncomingEdge(Standard_Integer node) const
        {
            if(node == na)
                return Edge(nc, na);
            if(node == nb)
                return Edge(na, nb);
            if(node == nc)
                return Edge(nb, nc);
            return Edge();
        }

        Edge GetOutcomingEdge(Standard_Integer node) const
        {
            if(node == na)
                return Edge(na, nb);
            if(node == nb)
                return Edge(nb, nc);
            if(node == nc)
                return Edge(nc, na);
            return Edge();
        }

        Standard_Boolean Has(const Edge& edge) const
        {
            return edge.na == na && edge.nb == nb
                || edge.na == nb && edge.nb == nc
                || edge.na == nc && edge.nb == na;
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

    static Ptr Create(
        const std::vector<gp_Pnt>& nodes,
        const std::vector<Cell>& cells
    );

public:

    virtual ~TriMesh();

    gp_Pnt& GetNode(Standard_Integer);

    Cell& GetCell(Standard_Integer);

private:
    TriMesh(Standard_Integer nbNodes, Standard_Integer nbCells);
};
#endif // !numgeom_numgeom_trimesh_h
