#include "pch.h"

#include "engine/resource_manager.h"

#include <cctype>
#include <filesystem>

#include "utils/filesystem.h"
#include "utils/texturessystem.h"

namespace nengine
{
  namespace
  {
    bool is_absolute_path(const std::string& path)
    {
      return !path.empty() && std::filesystem::path(path).is_absolute();
    }

    std::string to_lower_copy(std::string value)
    {
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
      {
        return static_cast<char>(std::tolower(c));
      });
      return value;
    }

    bool is_material_definition_path(const std::string& path)
    {
      const std::string ext = to_lower_copy(std::filesystem::path(path).extension().string());
      return ext == ".mtl" || ext == ".xml";
    }
  }

  ResourceManager::ResourceManager(PathResolver resolver)
    : mPathResolver(std::move(resolver))
  {
  }

  std::string ResourceManager::resolve_path(const std::string& path) const
  {
    if (path.empty() || is_absolute_path(path))
      return path;

    if (mPathResolver)
      return mPathResolver(path);

    return FileSystem::getPath(path);
  }

  std::shared_ptr<nelems::Model> ResourceManager::load_model(const std::string& path)
  {
    const std::string resolved_path = resolve_path(path);
    if (resolved_path.empty())
      return nullptr;

    const auto it = mModelCache.find(resolved_path);
    if (it != mModelCache.end())
    {
      if (auto cached = it->second.lock())
        return cached;
    }

    auto model = std::make_shared<nelems::Model>(resolved_path);
    mModelCache[resolved_path] = model;
    return model;
  }

  std::shared_ptr<Material> ResourceManager::load_material(const std::string& path)
  {
    const std::string resolved_path = resolve_path(path);
    if (resolved_path.empty())
      return nullptr;
    if (!is_material_definition_path(resolved_path))
    {
      std::cout << "[ResourceManager] Unsupported material definition extension: " << resolved_path << std::endl;
      return nullptr;
    }

    const auto it = mMaterialTemplates.find(resolved_path);
    if (it != mMaterialTemplates.end() && it->second)
      return std::make_shared<Material>(*it->second);

    MaterialLoadContext load_context;
    load_context.resolve_path = [this](const std::string& nested_path)
    {
      return resolve_path(nested_path);
    };
    load_context.load_texture_2d = [this](const std::string& texture_path, GLint tex_wrapping)
    {
      return load_texture_2d(texture_path, tex_wrapping);
    };

    auto material = std::make_shared<Material>();
    material->load(resolved_path.c_str(), &load_context);
    if (material->getshaderPath().empty())
      return nullptr;
    mMaterialTemplates[resolved_path] = material;
    return std::make_shared<Material>(*material);
  }

  std::unique_ptr<nshaders::Shader> ResourceManager::load_shader_program(
    const std::string& vertex_path,
    const std::string& fragment_path,
    const std::string& geometry_path)
  {
    const std::string resolved_vertex_path = resolve_path(vertex_path);
    const std::string resolved_fragment_path = resolve_path(fragment_path);
    const std::string resolved_geometry_path = geometry_path.empty()
      ? std::string()
      : resolve_path(geometry_path);

    const char* geometry_ptr = resolved_geometry_path.empty()
      ? nullptr
      : resolved_geometry_path.c_str();

    return std::make_unique<nshaders::Shader>(
      resolved_vertex_path.c_str(),
      resolved_fragment_path.c_str(),
      geometry_ptr);
  }

  uint32_t ResourceManager::load_texture_2d(const std::string& path, GLint tex_wrapping)
  {
    const std::string resolved_path = resolve_path(path);
    if (resolved_path.empty())
      return 0;

    const std::string cache_key = resolved_path + "#" + std::to_string(static_cast<int>(tex_wrapping));
    const auto it = mTextureCache.find(cache_key);
    if (it != mTextureCache.end())
      return it->second;

    const uint32_t texture_id = TextureSystem::getTextureId(resolved_path.c_str(), tex_wrapping);
    mTextureCache[cache_key] = texture_id;
    return texture_id;
  }

  uint32_t ResourceManager::load_cubemap(const std::vector<std::string>& faces, bool is_flip)
  {
    std::vector<std::string> resolved_faces;
    resolved_faces.reserve(faces.size());

    std::string cache_key = is_flip ? "cubemap#1" : "cubemap#0";
    for (const auto& face : faces)
    {
      const std::string resolved_face = resolve_path(face);
      resolved_faces.push_back(resolved_face);
      cache_key += "#" + resolved_face;
    }

    const auto it = mTextureCache.find(cache_key);
    if (it != mTextureCache.end())
      return it->second;

    const uint32_t texture_id = TextureSystem::loadCubemap(resolved_faces, is_flip);
    mTextureCache[cache_key] = texture_id;
    return texture_id;
  }
}
