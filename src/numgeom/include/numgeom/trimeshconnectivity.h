#ifndef numgeom_numgeom_trimeshconnectivity_h
#define numgeom_numgeom_trimeshconnectivity_h

#include <array>

#include "numgeom/staticjaggedarray.h"
#include "numgeom/trimesh.h"


class NUMGEOM_EXPORT TriMeshConnectivity
{
public:

    typedef CTriMesh::Edge Edge;
    typedef CTriMesh::Cell Tria;

public:

    TriMeshConnectivity(
        Standard_Integer nbNodes,
        Standard_Integer nbTrias,
        const Tria* trias
    );

    Standard_Integer NbNodes() const;

    Standard_Integer NbTrias() const;


    /** \brief Является ли узел граничным?
    \param iNode Классифицируемый узел.
    \param incomingEdge Входящее граничное ребро.
    \param outcomingEdge Исходящее граничное ребро.

    Узел граничный, если он входит в состав граничного ребра или никуда.
    Ребро граничное, если оно входит в состав только одной грани.
    */
    Standard_Boolean IsBoundaryNode(
        Standard_Integer iNode,
        Edge* incomingEdge = nullptr,
        Edge* outcomingEdge2 = nullptr
    ) const;


    void Node2Nodes(
        Standard_Integer iNode,
        std::vector<Standard_Integer>& nodes
    ) const;

    void Node2Trias(
        Standard_Integer iNode,
        std::vector<Standard_Integer>& trias
    ) const;


    /**\brief Извлечение связанных с ребром граней.
    \param edge Исходное ребро.
    \param tr1, tr2 Связанные с ребром грани. Первая грань включает ребро в
    исходном направлении, а вторая грань -- в обратном.
    */
    void Edge2Trias(
        const Edge& edge,
        Standard_Integer& tr1,
        Standard_Integer& tr2
    ) const;

    void Tria2Trias(
        Standard_Integer iTria,
        std::array<Standard_Integer, 3>& adjTrias
    ) const;

private:
    TriMeshConnectivity(const TriMeshConnectivity&) = delete;
    void operator=(const TriMeshConnectivity&) = delete;

private:
    const Tria* myTrias;
    Standard_Integer myNbNodes, myNbTrias;
    std::vector<std::array<Standard_Integer,3>> myTria2Trias;
    StaticJaggedArray myNode2Nodes;
    StaticJaggedArray myNode2Trias;
};
#endif // !numgeom_numgeom_trimeshconnectivity_h
