#include "numgeom/trimeshconnectivity.h"

#include <cassert>


namespace {;
std::pair<size_t, size_t> GetMatchingElements(
    const size_t* arr1, size_t n1,
    const size_t* arr2, size_t n2
)
{
    size_t firstFoundElement = -1;
    for(size_t i1 = 0; i1 < n1; ++i1)
    {
        auto it = std::find(arr2, arr2 + n2, arr1[i1]);
        if(it != arr2 + n2)
        {
            if(firstFoundElement == -1)
                firstFoundElement = (*it);
            else
                return std::make_pair(firstFoundElement, *it);
        }
    }
    return std::make_pair(firstFoundElement, -1);
}


/**
\brief Сортировка треугольников `trs`, инцидентных вершине `node`.
\return true, если узел внутренний.
*/
Standard_Boolean SortTriangles(
    size_t* trs,
    size_t nbTrs,
    Standard_Integer node,
    const TriMesh::Cell* allTrias
)
{
    // Ищем треугольник с граничным ребром, исходящим из вершины `node`.
    size_t firstTriaIndex = 0;
    for(; firstTriaIndex < nbTrs; ++firstTriaIndex)
    {
        const TriMesh::Cell& tr = allTrias[trs[firstTriaIndex]];
        TriMesh::Edge e = tr.GetOutcomingEdge(node).Reversed();
        Standard_Boolean eIsBoundary = Standard_True;
        for(size_t i2 = 0; i2 < nbTrs; ++i2)
        {
            if(i2 == firstTriaIndex)
                continue;
            const TriMesh::Cell& tr2 = allTrias[trs[i2]];
            if(tr2.Has(e))
            {
                eIsBoundary = Standard_False;
                break;
            }
        }
        if(eIsBoundary)
            break;
    }

    if(firstTriaIndex != nbTrs && firstTriaIndex != 0)
        std::swap(trs[0], trs[firstTriaIndex]);

    // Сортируем треугольники по соседству через ребро.
    for(size_t i1 = 0; i1 < nbTrs - 1; ++i1)
    {
        const TriMesh::Cell& tr = allTrias[trs[i1]];
        TriMesh::Edge e = tr.GetIncomingEdge(node);
        e.Reverse();
        size_t i2 = i1 + 1;
        for(; i2 < nbTrs; ++i2)
        {
            const TriMesh::Cell& tr2 = allTrias[trs[i2]];
            if(tr2.Has(e))
                break;
        }
        if(i2 != nbTrs && i2 != i1 + 1)
            std::swap(trs[i1+1], trs[i2]);
    }

    return firstTriaIndex == nbTrs;
}
}


TriMeshConnectivity::TriMeshConnectivity(
    Standard_Integer nbNodes,
    Standard_Integer nbTrias,
    const Tria* trias
)
{
    myNbNodes = nbNodes;
    myNbTrias = nbTrias;
    myTrias = trias;

    std::vector<size_t> rowSizes(nbNodes, 0);
    Standard_Integer iTria = 0;
    for(const Tria* tptr = trias; iTria < nbTrias; ++tptr, ++iTria)
    {
        ++rowSizes[tptr->na];
        ++rowSizes[tptr->nb];
        ++rowSizes[tptr->nc];
    }

    myNode2Trias.Initialize(rowSizes);
    iTria = 0;
    for(const Tria* tptr = trias; iTria < nbTrias; ++tptr, ++iTria)
    {
        myNode2Trias.Append(tptr->na, iTria);
        myNode2Trias.Append(tptr->nb, iTria);
        myNode2Trias.Append(tptr->nc, iTria);
    }

    // Сортируем треугольники в списках по смежности.
    for(Standard_Integer iNode = 0; iNode < nbNodes; ++iNode)
    {
        Standard_Boolean nodeIsInner = 
            SortTriangles(myNode2Trias[iNode],
                          myNode2Trias.Size(iNode),
                          iNode,
                          myTrias);
        // У граничных вершин смежных вершин на одну
        // больше, чем связанных треугольников.
        if(!nodeIsInner)
            ++rowSizes[iNode];
    }

    myNode2Nodes.Initialize(rowSizes);
    iTria = 0;
    for(Standard_Integer iNode = 0; iNode < nbNodes; ++iNode)
    {
        size_t* trs = myNode2Trias[iNode];
        size_t* adjNodes = myNode2Nodes[iNode];
        size_t n = myNode2Trias.Size(iNode);
        for(size_t i = 0; i < n; ++i)
        {
            const Tria& tr = myTrias[trs[i]];
            adjNodes[i] = tr.GetOutcomingEdge(iNode).nb;
        }

        if(n == myNode2Nodes.Size(iNode) - 1)
            adjNodes[n] = myTrias[trs[n-1]].GetIncomingEdge(iNode).na;
    }

    myTria2Trias.resize(nbTrias);
    iTria = 0;
    for(const Tria* tptr = trias; iTria < nbTrias; ++tptr, ++iTria)
    {
        for(Standard_Integer i = 0; i < 3; ++i)
        {
            Edge e = tptr->GetEdge(i);
            size_t t1, t2;
            std::tie(t1,t2) = GetMatchingElements(myNode2Nodes[e.na],
                                                  myNode2Nodes.Size(e.na),
                                                  myNode2Nodes[e.nb],
                                                  myNode2Nodes.Size(e.nb));
            myTria2Trias[iTria][i] = (t1 == iTria ? t2 : t1);
        }
    }
}


Standard_Integer TriMeshConnectivity::NbNodes() const
{
    return myNode2Nodes.Size();
}


Standard_Integer TriMeshConnectivity::NbTrias() const
{
    return myTria2Trias.size();
}


Standard_Boolean TriMeshConnectivity::IsBoundaryNode(
    Standard_Integer iNode,
    Edge* incomingEdge,
    Edge* outcomingEdge
) const
{
    std::vector<Standard_Integer> nodes;
    this->Node2Nodes(iNode, nodes);
    Standard_Boolean isBoundary = Standard_False,
                     incomingEdgeSpecified = Standard_False,
                     outcomingEdgeSpecified = Standard_False;
    for(Standard_Integer iNode2 : nodes)
    {
        Standard_Integer tr1 = NONE_INDEX, tr2 = NONE_INDEX;
        Edge boundaryEdge(iNode2, iNode);
        this->Edge2Trias(boundaryEdge, tr1, tr2);

        if(tr1 != NONE_INDEX && tr2 != NONE_INDEX)
            continue;

        if(!incomingEdge && !outcomingEdge)
            return Standard_True;

        isBoundary = Standard_True;

        if(tr1 != NONE_INDEX)
        {
            assert(!incomingEdgeSpecified);
            incomingEdgeSpecified = Standard_True;
            if(incomingEdge)
                (*incomingEdge) = boundaryEdge;
        }
        else // tr2 != NONE_INDEX
        {
            assert(!outcomingEdgeSpecified);
            outcomingEdgeSpecified = Standard_True;
            if(outcomingEdge)
                (*outcomingEdge) = boundaryEdge.Reversed();
        }

        if(    (!incomingEdge || incomingEdgeSpecified)
            && (!outcomingEdge || outcomingEdgeSpecified))
            return Standard_True;
    }

    return isBoundary;
}


void TriMeshConnectivity::Node2Nodes(
    Standard_Integer iNode,
    std::vector<Standard_Integer>& nodes
) const
{
    auto n = myNode2Nodes.Size(iNode);
    nodes.resize(n);
    std::copy_n(myNode2Nodes[iNode], n, nodes.begin());
}


void TriMeshConnectivity::Node2Trias(
    Standard_Integer iNode,
    std::vector<Standard_Integer>& trias
) const
{
    auto n = myNode2Trias.Size(iNode);
    trias.resize(n);
    std::copy_n(myNode2Trias[iNode], n, trias.begin());
}


void TriMeshConnectivity::Edge2Trias(
    const Edge& edge,
    Standard_Integer& tr1,
    Standard_Integer& tr2
) const
{
    size_t n = myNode2Trias.Size(edge.na);
    Standard_Boolean tr1Found = Standard_False, tr2Found = Standard_False;
    for(const size_t* tptr = myNode2Trias[edge.na]; n != 0; ++tptr, --n)
    {
        Tria t = myTrias[*tptr];
        if(    edge.na == t.na && edge.nb == t.nb
            || edge.na == t.nb && edge.nb == t.nc
            || edge.na == t.nc && edge.nb == t.na)
        {
            tr1 = *tptr;
            tr1Found = Standard_True;
        }
        else if(edge.na == t.nb && edge.nb == t.na
             || edge.na == t.nc && edge.nb == t.nb
             || edge.na == t.na && edge.nb == t.nc)
        {
            tr2 = *tptr;
            tr2Found = Standard_True;
        }

        if(tr1Found && tr2Found)
            break;
    }
}


void TriMeshConnectivity::Tria2Trias(
    Standard_Integer iTria,
    std::array<Standard_Integer, 3>& trs
) const
{
    std::copy_n(myTria2Trias[iTria].begin(), 3, trs.begin());
}
