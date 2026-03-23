#include "pch.h"

#include "engine/animation_asset.h"

#include <algorithm>
#include <filesystem>

#include "elems/tinyxml2.h"
#include "engine/resource_manager.h"

namespace nengine
{
  namespace
  {
    void assign_error(std::string* out_error, const std::string& value)
    {
      if (out_error)
        *out_error = value;
    }

    std::string to_lower_copy(std::string value)
    {
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
      {
        return static_cast<char>(std::tolower(c));
      });
      return value;
    }

    bool parse_bool_attr(const tinyxml2::XMLElement* element, const char* attr_name, bool default_value)
    {
      const char* attr_value = element ? element->Attribute(attr_name) : nullptr;
      if (!attr_value)
        return default_value;

      const std::string lower_value = to_lower_copy(attr_value);
      if (lower_value == "1" || lower_value == "true" || lower_value == "yes" || lower_value == "on")
        return true;
      if (lower_value == "0" || lower_value == "false" || lower_value == "no" || lower_value == "off")
        return false;
      return default_value;
    }

    bool parse_float_attr(
      const tinyxml2::XMLElement* element,
      const char* attr_name,
      float default_value,
      float* out_value)
    {
      if (!out_value)
        return false;

      *out_value = default_value;
      if (!element || !element->Attribute(attr_name))
        return true;

      return element->QueryFloatAttribute(attr_name, out_value) == tinyxml2::XML_SUCCESS;
    }

    std::string read_resolved_attr(
      const tinyxml2::XMLElement* element,
      const char* attr_name,
      const std::shared_ptr<IResourceManager>& resources)
    {
      if (!element || !resources)
        return {};

      const char* attr_value = element->Attribute(attr_name);
      if (!attr_value || std::string(attr_value).empty())
        return {};

      return resources->resolve_path(attr_value);
    }
  }

  bool load_animation_asset_resource(
    const std::string& path,
    std::shared_ptr<IResourceManager> resources,
    AnimationAssetDefinition* out_definition,
    std::string* out_error)
  {
    if (!out_definition)
    {
      assign_error(out_error, "No animation asset output definition was provided.");
      return false;
    }

    *out_definition = AnimationAssetDefinition {};
    if (!resources)
    {
      assign_error(out_error, "Animation asset loading requires a valid resource manager.");
      return false;
    }

    const std::string resolved_path = resources->resolve_path(path);
    if (resolved_path.empty())
    {
      assign_error(out_error, std::string("Animation asset path could not be resolved: ") + path);
      return false;
    }

    tinyxml2::XMLDocument doc;
    const auto load_result = doc.LoadFile(resolved_path.c_str());
    if (load_result != tinyxml2::XML_SUCCESS)
    {
      assign_error(
        out_error,
        std::string("Failed to load animation asset: ") + resolved_path + " error: " + doc.ErrorStr());
      return false;
    }

    const tinyxml2::XMLElement* root = doc.FirstChildElement("JAnim");
    if (!root)
    {
      assign_error(out_error, std::string("Animation asset is missing a <JAnim> root: ") + resolved_path);
      return false;
    }

    AnimationAssetDefinition definition;
    const char* asset_name = root->Attribute("Name");
    definition.name = asset_name && std::string(asset_name).size() > 0
      ? asset_name
      : std::filesystem::path(resolved_path).stem().string();
    definition.skeleton_path = read_resolved_attr(root, "Skeleton", resources);
    definition.source_path = read_resolved_attr(root, "Source", resources);
    definition.clip_name = root->Attribute("Clip") ? root->Attribute("Clip") : "";
    definition.loop = parse_bool_attr(root, "Loop", true);
    definition.auto_play = parse_bool_attr(root, "AutoPlay", true);
    if (!parse_float_attr(root, "Speed", 1.0f, &definition.speed))
    {
      assign_error(out_error, std::string("Animation asset contains an invalid Speed value: ") + resolved_path);
      return false;
    }

    if (definition.source_path.empty())
      definition.source_path = definition.skeleton_path;

    if (definition.source_path.empty())
    {
      assign_error(out_error, std::string("Animation asset is missing a Source path: ") + resolved_path);
      return false;
    }

    definition.speed = std::max(0.0f, definition.speed);
    *out_definition = std::move(definition);
    return true;
  }
}
