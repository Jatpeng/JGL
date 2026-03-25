#include "pch.h"
#include "mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "render/device/render_device.h"
using namespace std;
namespace nelems
{
  namespace
  {
    std::unique_ptr<nrender::VertexIndexBuffer> create_vertex_index_buffer()
    {
      auto buffer = nrender::RenderDeviceManager::instance().create_vertex_index_buffer();
      if (!buffer)
      {
        std::cout << "[Mesh] Failed to create vertex/index buffer for backend "
                  << nrender::graphics_backend_name(nrender::RenderDeviceManager::instance().backend())
                  << "." << std::endl;
      }
      return buffer;
    }
  }

  Mesh::Mesh()
  {
      mRenderBufferMgr = create_vertex_index_buffer();
  }
  // constructor
  Mesh::Mesh(vector<VertexHolder> vertices, vector<unsigned int> indices)
  {
      mRenderBufferMgr = create_vertex_index_buffer();
      this->mVertices = vertices;
      this->mVertexIndices = indices;
      create_buffers();
  }
  Mesh::Mesh(float(&arr)[])
  {

  }
  Mesh::Mesh(const Mesh& other)
  {
      this->mVertices = other.mVertices;
      this->mVertexIndices = other.mVertexIndices;
      mRenderBufferMgr = create_vertex_index_buffer();
      create_buffers();
  }

  Mesh::~Mesh()
  {
    delete_buffers();
  }

  bool Mesh::load(const std::string& filepath)
  {
    const uint32_t cMeshImportFlags =
      aiProcess_CalcTangentSpace |
      aiProcess_Triangulate |
      aiProcess_SortByPType |
      aiProcess_GenNormals |
      aiProcess_GenUVCoords |
      aiProcess_OptimizeMeshes |
      aiProcess_ValidateDataStructure;
    Assimp::Importer Importer;
    const aiScene* pScene = Importer.ReadFile(filepath.c_str(),
      cMeshImportFlags);
    if (pScene && pScene->HasMeshes())
    {
      mVertexIndices.clear();
      mVertices.clear();

      auto* mesh = pScene->mMeshes[0];

      for (uint32_t i = 0; i < mesh->mNumVertices; i++)
      {
        VertexHolder vh;
        vh.mPos = { mesh->mVertices[i].x, mesh->mVertices[i].y ,mesh->mVertices[i].z };
        vh.mNormal = { mesh->mNormals[i].x, mesh->mNormals[i].y ,mesh->mNormals[i].z };
        vh.mTextureCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        add_vertex(vh);
      }
      for (size_t i = 0; i < mesh->mNumFaces; i++)
      {
        aiFace face = mesh->mFaces[i];

        for (size_t j = 0; j < face.mNumIndices; j++)
          add_vertex_index(face.mIndices[j]);
      }
      create_buffers();
      return true;
    }
    return false;
  }

  void Mesh::create_buffers()
  {
    if (!mRenderBufferMgr)
      mRenderBufferMgr = create_vertex_index_buffer();

    if (!mRenderBufferMgr)
      return;

    mRenderBufferMgr->create_buffers(mVertices, mVertexIndices);
  }

  void Mesh::delete_buffers()
  {
    if (mRenderBufferMgr)
      mRenderBufferMgr->delete_buffers();
  }

  void Mesh::bind()
  {
    if (mRenderBufferMgr)
      mRenderBufferMgr->bind();
  }

  void Mesh::unbind()
  {
    if (mRenderBufferMgr)
      mRenderBufferMgr->unbind();
  }

  void Mesh::render()
  {
    if (mRenderBufferMgr)
      mRenderBufferMgr->draw((int) mVertexIndices.size());
  }
}
