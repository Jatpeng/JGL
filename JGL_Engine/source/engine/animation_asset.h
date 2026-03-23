#pragma once

#include <memory>
#include <string>

namespace nengine
{
  class IResourceManager;

  struct AnimationAssetDefinition
  {
    std::string name;
    std::string skeleton_path;
    std::string source_path;
    std::string clip_name;
    bool loop = true;
    bool auto_play = true;
    float speed = 1.0f;
  };

  bool load_animation_asset_resource(
    const std::string& path,
    std::shared_ptr<IResourceManager> resources,
    AnimationAssetDefinition* out_definition,
    std::string* out_error = nullptr);
}
