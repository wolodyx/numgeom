#ifndef numgeom_framework_meshchunk_h
#define numgeom_framework_meshchunk_h

#include <cstdint>

#include "vulkan/vulkan.h"

#include "numgeom/trimesh.h"

class GpuMemory;


class MeshChunk
{
public:

    MeshChunk(GpuMemory* mem, CTriMesh::Ptr);

    ~MeshChunk();

    VkBuffer buffer() const;

    uint32_t indexOffset() const;

    uint32_t indexCount() const;

private:
    GpuMemory* m_memory;
    uint32_t m_indexOffset;
    uint32_t m_indexCount;
};
#endif // !numgeom_framework_meshchunk_h
