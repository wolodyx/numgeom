#ifndef numgeom_numgeom_trimesh_h
#define numgeom_numgeom_trimesh_h

#include <filesystem>
#include <memory>
#include <vector>

#include "glm/glm.hpp"

#include "numgeom/numgeomcore_export.h"

#define NONE_INDEX -1

class TriMeshConnectivity;


class CORE_EXPORT CTriMesh
{
public:

    typedef glm::dvec3 NodeType;
    typedef size_t IndexType;
    typedef std::shared_ptr<CTriMesh> Ptr;

    struct Edge
    {
        Edge() : na(NONE_INDEX), nb(NONE_INDEX) {}

        Edge(size_t a, size_t b) : na(a), nb(b) {}

        bool empty() const { return na == NONE_INDEX; }

        bool operator!() const { return this->empty(); }

        bool operator==(const Edge& other) const
        {
            return na == other.na && nb == other.nb;
        }

        bool operator!=(const Edge& other) const
        {
            return !this->operator==(other);
        }

        Edge Reversed() const { return Edge(nb,na); }

        void Reverse() { std::swap(na,nb); }

        size_t na, nb;
    };

    struct Cell
    {
        Cell() : na(NONE_INDEX) {}

        Cell(const size_t* nodes)
            : na(nodes[0]), nb(nodes[1]), nc(nodes[2]) {}

        Cell(size_t a, size_t b, size_t c)
            : na(a), nb(b), nc(c) {}

        Edge GetEdge(size_t i) const
        {
            return Edge(*(&na + i%3), *(&na + (i+1)%3));
        }

        Edge GetIncomingEdge(size_t node) const
        {
            if(node == na)
                return Edge(nc, na);
            if(node == nb)
                return Edge(na, nb);
            if(node == nc)
                return Edge(nb, nc);
            return Edge();
        }

        Edge GetOutcomingEdge(size_t node) const
        {
            if(node == na)
                return Edge(na, nb);
            if(node == nb)
                return Edge(nb, nc);
            if(node == nc)
                return Edge(nc, na);
            return Edge();
        }

        bool Has(const Edge& edge) const
        {
            return edge.na == na && edge.nb == nb
                || edge.na == nb && edge.nb == nc
                || edge.na == nc && edge.nb == na;
        }

        size_t na, nb, nc;
    };

public:

    virtual ~CTriMesh();

    size_t NbNodes() const;

    size_t NbCells() const;

    const NodeType& GetNode(size_t) const;

    const Cell& GetCell(size_t) const;

    bool Dump(const std::filesystem::path&) const;

    TriMeshConnectivity* Connectivity() const;

protected:
    CTriMesh(size_t nbNodes, size_t nbCells);

private:
    CTriMesh(const CTriMesh&) = delete;
    void operator=(const CTriMesh&) = delete;

protected:
    std::vector<NodeType> myNodes;
    std::vector<Cell> myCells;
    mutable TriMeshConnectivity* myConnectivity;
};


class TriMesh : public CTriMesh
{
public:

    typedef std::shared_ptr<TriMesh> Ptr;

public:

    static Ptr Create(
        size_t nbNodes,
        size_t nbCells
    );

    static Ptr Create(
        const std::vector<NodeType>& nodes,
        const std::vector<Cell>& cells
    );

public:

    virtual ~TriMesh();

    NodeType& GetNode(size_t);

    Cell& GetCell(size_t);

    void Transform(const glm::dmat4&);

private:
    TriMesh(size_t nbNodes, size_t nbCells);
};
#endif // !numgeom_numgeom_trimesh_h
