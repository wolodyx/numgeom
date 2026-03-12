#include "meshchunk.h"

#include "numgeom/gpumemory.h"
#include "numgeom/trimeshconnectivity.h"

#include "vkutilities.h"

namespace {
glm::vec3 computeTriangleNormal(const glm::vec3& a, const glm::vec3& b,
                                const glm::vec3& c) {
  glm::vec3 ab = b - a;
  glm::vec3 ac = c - a;
  glm::vec3 normal = glm::cross(ab, ac);
  float length = glm::length(normal);
  if (length < 1e-6f)
    return glm::vec3();
  return normal / length;
}

std::vector<glm::vec3> computeNormals(CTriMesh::Ptr mesh) {
  size_t nodes_count = mesh->NbNodes();
  std::vector<glm::vec3> normals(nodes_count);
  auto* con = mesh->Connectivity();
  std::vector<size_t> adjTrias;
  for (size_t node_i = 0; node_i < nodes_count; ++node_i) {
    glm::vec3 node_n(0.0f, 0.0f, 0.0f);
    con->Node2Trias(node_i, adjTrias);
    for (size_t tria_i : adjTrias) {
      const auto& tria = mesh->GetCell(tria_i);
      glm::vec3 tria_n = computeTriangleNormal(mesh->GetNode(tria.na),
                                               mesh->GetNode(tria.nb),
                                               mesh->GetNode(tria.nc));
      node_n += tria_n;
    }
    normals[node_i] = glm::normalize(node_n);
  }
  return normals;
}

} // namespace

MeshChunk::MeshChunk(GpuMemory* mem, CTriMesh::Ptr mesh) {
  m_memory = mem;
  if (!mesh) {
    m_indexOffset = 0;
    m_indexCount = 0;
    return;
  }

  size_t nodesCount = mesh->NbNodes();
  size_t cellsCount = mesh->NbCells();
  VkDeviceSize vertexBlockSize =
      mem->uniformBlockSize(6 * nodesCount * sizeof(float));
  VkDeviceSize indexBlockSize =
      mem->uniformBlockSize(3 * cellsCount * sizeof(uint32_t));
  m_indexOffset = vertexBlockSize;
  m_indexCount = 3 * cellsCount;
  mem->resize(vertexBlockSize + indexBlockSize);

  auto mapping = mem->access();
  std::byte* data = mapping.data();
  float* nodesData = reinterpret_cast<float*>(data);
  auto normals = computeNormals(mesh);
  for (size_t i = 0; i < nodesCount; ++i) {
    auto& node = mesh->GetNode(i);
    *nodesData++ = node.x;
    *nodesData++ = node.y;
    *nodesData++ = node.z;
    const auto& n = normals[i];
    *nodesData++ = n.x;
    *nodesData++ = n.y;
    *nodesData++ = n.z;
  }

  uint32_t* cellsData = reinterpret_cast<uint32_t*>(data + vertexBlockSize);
  for (size_t i = 0; i < cellsCount; ++i) {
    auto& cell = mesh->GetCell(i);
    *cellsData++ = static_cast<uint32_t>(cell.na);
    *cellsData++ = static_cast<uint32_t>(cell.nb);
    *cellsData++ = static_cast<uint32_t>(cell.nc);
  }
}

MeshChunk::~MeshChunk() {}

VkBuffer MeshChunk::buffer() const { return m_memory->buffer(); }

uint32_t MeshChunk::indexOffset() const { return m_indexOffset; }

uint32_t MeshChunk::indexCount() const { return m_indexCount; }
