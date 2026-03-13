#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "elems/material.h"
#include "elems/model.h"
#include "shader/shader_util.h"

namespace nengine
{
  class IResourceManager
  {
  public:
    virtual ~IResourceManager() = default;

    virtual std::string resolve_path(const std::string& path) const = 0;
    virtual std::shared_ptr<nelems::Model> load_model(const std::string& path) = 0;
    virtual std::shared_ptr<Material> load_material(const std::string& path) = 0;
    virtual std::unique_ptr<nshaders::Shader> load_shader_program(
      const std::string& vertex_path,
      const std::string& fragment_path,
      const std::string& geometry_path = "") = 0;
    virtual uint32_t load_texture_2d(const std::string& path, GLint tex_wrapping = GL_REPEAT) = 0;
    virtual uint32_t load_cubemap(const std::vector<std::string>& faces, bool is_flip) = 0;
  };

  class ResourceManager : public IResourceManager
  {
  public:
    using PathResolver = std::function<std::string(const std::string&)>;

    explicit ResourceManager(PathResolver resolver = {});

    std::string resolve_path(const std::string& path) const override;
    std::shared_ptr<nelems::Model> load_model(const std::string& path) override;
    std::shared_ptr<Material> load_material(const std::string& path) override;
    std::unique_ptr<nshaders::Shader> load_shader_program(
      const std::string& vertex_path,
      const std::string& fragment_path,
      const std::string& geometry_path = "") override;
    uint32_t load_texture_2d(const std::string& path, GLint tex_wrapping = GL_REPEAT) override;
    uint32_t load_cubemap(const std::vector<std::string>& faces, bool is_flip) override;

  private:
    PathResolver mPathResolver;
    std::unordered_map<std::string, std::weak_ptr<nelems::Model>> mModelCache;
    std::unordered_map<std::string, std::shared_ptr<Material>> mMaterialTemplates;
    std::unordered_map<std::string, uint32_t> mTextureCache;
  };
}
