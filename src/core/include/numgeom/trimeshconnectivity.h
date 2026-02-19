#ifndef numgeom_numgeom_trimeshconnectivity_h
#define numgeom_numgeom_trimeshconnectivity_h

#include <array>

#include "numgeom/staticjaggedarray.h"
#include "numgeom/trimesh.h"


class CORE_EXPORT TriMeshConnectivity
{
public:

    typedef CTriMesh::Edge Edge;
    typedef CTriMesh::Cell Tria;

public:

    TriMeshConnectivity(
        size_t nbNodes,
        size_t nbTrias,
        const Tria* trias
    );

    size_t NbNodes() const;

    size_t NbTrias() const;


    /** \brief Является ли узел граничным?
    \param iNode Классифицируемый узел.
    \param incomingEdge Входящее граничное ребро.
    \param outcomingEdge Исходящее граничное ребро.

    Узел граничный, если он входит в состав граничного ребра или никуда.
    Ребро граничное, если оно входит в состав только одной грани.
    */
    bool IsBoundaryNode(
        size_t iNode,
        Edge* incomingEdge = nullptr,
        Edge* outcomingEdge2 = nullptr
    ) const;


    void Node2Nodes(
        size_t iNode,
        std::vector<size_t>& nodes
    ) const;

    void Node2Trias(
        size_t iNode,
        std::vector<size_t>& trias
    ) const;


    /**\brief Извлечение связанных с ребром граней.
    \param edge Исходное ребро.
    \param tr1, tr2 Связанные с ребром грани. Первая грань включает ребро в
    исходном направлении, а вторая грань -- в обратном.
    */
    void Edge2Trias(
        const Edge& edge,
        size_t& tr1,
        size_t& tr2
    ) const;

    void Tria2Trias(
        size_t iTria,
        std::array<size_t, 3>& adjTrias
    ) const;

private:
    TriMeshConnectivity(const TriMeshConnectivity&) = delete;
    void operator=(const TriMeshConnectivity&) = delete;

private:
    const Tria* myTrias;
    size_t myNbNodes, myNbTrias;
    std::vector<std::array<size_t,3>> myTria2Trias;
    StaticJaggedArray myNode2Nodes;
    StaticJaggedArray myNode2Trias;
};
#endif // !numgeom_numgeom_trimeshconnectivity_h
