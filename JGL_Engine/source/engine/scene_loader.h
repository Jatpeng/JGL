#pragma once

#include <memory>
#include <string>

namespace nengine
{
  class IResourceManager;
  class Scene;

  struct SceneResourceDefinition
  {
    std::shared_ptr<Scene> scene;
    bool show_plane = false;
    std::string environment_map_path;
  };

  bool load_scene_resource(
    const std::string& path,
    std::shared_ptr<IResourceManager> resources,
    SceneResourceDefinition* out_definition,
    std::string* out_error = nullptr);

  bool save_scene_resource(
    const std::string& path,
    const std::shared_ptr<Scene>& scene,
    bool show_plane,
    const std::string& environment_map_path,
    std::string* out_error = nullptr);
}
