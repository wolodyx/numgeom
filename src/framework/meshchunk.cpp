#include "meshchunk.h"

#include "numgeom/gpumemory.h"

#include "vkutilities.h"


MeshChunk::MeshChunk(GpuMemory* mem, CTriMesh::Ptr mesh)
{
    m_memory = mem;
    if(!mesh) {
        m_indexOffset = 0;
        m_indexCount = 0;
        return;
    }

    size_t nodesCount = mesh->NbNodes();
    size_t cellsCount = mesh->NbCells();
    VkDeviceSize vertexBlockSize = mem->uniformBlockSize(3 * nodesCount * sizeof(float));
    VkDeviceSize indexBlockSize = mem->uniformBlockSize(3 * cellsCount * sizeof(uint32_t));
    m_indexOffset = vertexBlockSize;
    m_indexCount = 3 * cellsCount;
    mem->resize(vertexBlockSize + indexBlockSize);

    auto mapping = mem->access();
    std::byte* data = mapping.data();
    float* nodesData = reinterpret_cast<float*>(data);
    for(size_t i = 0; i < nodesCount; ++i) {
        auto& node = mesh->GetNode(i);
        *nodesData = node.x; ++nodesData;
        *nodesData = node.y; ++nodesData;
        *nodesData = node.z; ++nodesData;
    }

    uint32_t* cellsData = reinterpret_cast<uint32_t*>(data + vertexBlockSize);
    for(size_t i = 0; i < cellsCount; ++i) {
        auto& cell = mesh->GetCell(i);
        *cellsData = static_cast<uint32_t>(cell.na); ++cellsData;
        *cellsData = static_cast<uint32_t>(cell.nb); ++cellsData;
        *cellsData = static_cast<uint32_t>(cell.nc); ++cellsData;
    }
}


MeshChunk::~MeshChunk()
{
}


VkBuffer MeshChunk::buffer() const
{
    return m_memory->buffer();
}


uint32_t MeshChunk::indexOffset() const
{
    return m_indexOffset;
}


uint32_t MeshChunk::indexCount() const
{
    return m_indexCount;
}
