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

    myNode2Nodes.Initialize(rowSizes);
    myNode2Trias.Initialize(rowSizes);
    iTria = 0;
    for(const Tria* tptr = trias; iTria < nbTrias; ++tptr, ++iTria)
    {
        myNode2Nodes.Append(tptr->na, tptr->nb);
        myNode2Nodes.Append(tptr->nb, tptr->nc);
        myNode2Nodes.Append(tptr->nc, tptr->na);
        myNode2Trias.Append(tptr->na, iTria);
        myNode2Trias.Append(tptr->nb, iTria);
        myNode2Trias.Append(tptr->nc, iTria);
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
