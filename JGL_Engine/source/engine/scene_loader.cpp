#include "pch.h"

#include "engine/scene_loader.h"

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <sstream>

#include "elems/material.h"
#include "elems/tinyxml2.h"
#include "engine/resource_manager.h"
#include "engine/scene.h"
#include "utils/filesystem.h"

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

    bool parse_vec3_text(const char* text, glm::vec3* out_value)
    {
      if (!text || !out_value)
        return false;

      std::string normalized = text;
      for (char& c : normalized)
      {
        if (c == ',' || c == ';')
          c = ' ';
      }

      std::istringstream stream(normalized);
      glm::vec3 value { 0.0f, 0.0f, 0.0f };
      if (!(stream >> value.x >> value.y >> value.z))
        return false;

      *out_value = value;
      return true;
    }

    bool parse_vec2_text(const char* text, glm::vec2* out_value)
    {
      if (!text || !out_value)
        return false;

      std::string normalized = text;
      for (char& c : normalized)
      {
        if (c == ',' || c == ';')
          c = ' ';
      }

      std::istringstream stream(normalized);
      glm::vec2 value { 0.0f, 0.0f };
      if (!(stream >> value.x >> value.y))
        return false;

      *out_value = value;
      return true;
    }

    bool read_vec3_attr(
      const tinyxml2::XMLElement* element,
      const char* attr_name,
      glm::vec3* out_value,
      const glm::vec3& default_value)
    {
      if (!out_value)
        return false;

      *out_value = default_value;
      const char* attr_value = element ? element->Attribute(attr_name) : nullptr;
      if (!attr_value)
        return true;

      return parse_vec3_text(attr_value, out_value);
    }

    bool read_float_attr(
      const tinyxml2::XMLElement* element,
      const char* attr_name,
      float* out_value,
      float default_value)
    {
      if (!out_value)
        return false;

      *out_value = default_value;
      if (!element || !element->Attribute(attr_name))
        return true;

      return element->QueryFloatAttribute(attr_name, out_value) == tinyxml2::XML_SUCCESS;
    }

    bool read_int_attr(
      const tinyxml2::XMLElement* element,
      const char* attr_name,
      int* out_value,
      int default_value)
    {
      if (!out_value)
        return false;

      *out_value = default_value;
      if (!element || !element->Attribute(attr_name))
        return true;

      return element->QueryIntAttribute(attr_name, out_value) == tinyxml2::XML_SUCCESS;
    }

    LightComponent::LightType parse_light_type(const char* type_name)
    {
      if (!type_name)
        return LightComponent::LightType::Point;

      const std::string lower_type = to_lower_copy(type_name);
      if (lower_type == "directional")
        return LightComponent::LightType::Directional;
      return LightComponent::LightType::Point;
    }

    const char* light_type_name(LightComponent::LightType type)
    {
      switch (type)
      {
      case LightComponent::LightType::Directional:
        return "Directional";
      case LightComponent::LightType::Point:
      default:
        return "Point";
      }
    }

    std::string format_float(float value)
    {
      std::ostringstream stream;
      stream.setf(std::ios::fixed, std::ios::floatfield);
      stream.precision(4);
      stream << value;

      std::string text = stream.str();
      while (!text.empty() && text.back() == '0')
        text.pop_back();
      if (!text.empty() && text.back() == '.')
        text.push_back('0');
      if (text.empty())
        text = "0.0";
      return text;
    }

    std::string format_vec2(const glm::vec2& value)
    {
      return format_float(value.x) + "," + format_float(value.y);
    }

    std::string format_vec3(const glm::vec3& value)
    {
      return format_float(value.x) + "," + format_float(value.y) + "," + format_float(value.z);
    }

    std::string format_bool(bool value)
    {
      return value ? "true" : "false";
    }

    std::string normalize_ui_path(std::string path)
    {
      std::replace(path.begin(), path.end(), '\\', '/');
      return path;
    }

    std::string make_scene_resource_path(const std::string& path)
    {
      if (path.empty())
        return {};

      namespace fs = std::filesystem;
      fs::path resolved_path(path);
      if (!resolved_path.is_absolute())
        resolved_path = fs::path(FileSystem::getPath(path));
      return resolved_path.lexically_normal().string();
    }

    std::string to_scene_attr_path(const std::string& path)
    {
      if (path.empty())
        return {};

      namespace fs = std::filesystem;

      std::error_code ec;
      fs::path absolute_path(path);
      if (!absolute_path.is_absolute())
        absolute_path = fs::path(FileSystem::getPath(path));
      absolute_path = absolute_path.lexically_normal();

      const fs::path project_root = fs::path(FileSystem::getPath("Assets")).parent_path();
      const fs::path relative_path = fs::relative(absolute_path, project_root, ec);
      if (!ec && !relative_path.empty())
      {
        const std::string relative = normalize_ui_path(relative_path.generic_string());
        if (relative != "." && relative.rfind("../", 0) != 0)
          return relative;
      }

      return normalize_ui_path(absolute_path.generic_string());
    }

    void write_transform_attrs(tinyxml2::XMLElement* element, const TransformComponent* transform)
    {
      if (!element || !transform)
        return;

      element->SetAttribute("Position", format_vec3(transform->position).c_str());
      element->SetAttribute("Rotation", format_vec3(transform->rotation).c_str());
      element->SetAttribute("Scale", format_vec3(transform->scale).c_str());
    }

    bool apply_material_param(
      const tinyxml2::XMLElement* element,
      const std::shared_ptr<Material>& material,
      std::string* out_error)
    {
      if (!element)
        return true;

      if (!material)
      {
        assign_error(out_error, "Mesh material parameters require a loaded material instance.");
        return false;
      }

      const char* param_name = element->Attribute("Name");
      const char* param_type = element->Attribute("Type");
      const char* param_value = element->Attribute("Value");
      if (!param_name || !param_type || !param_value)
      {
        assign_error(out_error, "MaterialParam entry is missing Name, Type, or Value.");
        return false;
      }

      const std::string type_name = to_lower_copy(param_type);
      if (type_name == "float")
      {
        float value = 0.0f;
        if (element->QueryFloatAttribute("Value", &value) != tinyxml2::XML_SUCCESS)
        {
          assign_error(out_error, "MaterialParam float value is invalid.");
          return false;
        }
        material->getFloatMap()[param_name] = value;
        return true;
      }

      if (type_name == "float2")
      {
        glm::vec2 value { 0.0f, 0.0f };
        if (!parse_vec2_text(param_value, &value))
        {
          assign_error(out_error, "MaterialParam float2 value is invalid.");
          return false;
        }
        material->getFloat2Map()[param_name] = value;
        return true;
      }

      if (type_name == "float3")
      {
        glm::vec3 value { 0.0f, 0.0f, 0.0f };
        if (!parse_vec3_text(param_value, &value))
        {
          assign_error(out_error, "MaterialParam float3 value is invalid.");
          return false;
        }
        material->getFloat3Map()[param_name] = value;
        return true;
      }

      assign_error(out_error, std::string("Unsupported MaterialParam type: ") + param_type);
      return false;
    }

    void write_material_params(
      tinyxml2::XMLDocument* doc,
      tinyxml2::XMLElement* mesh_element,
      const std::shared_ptr<Material>& material)
    {
      if (!doc || !mesh_element || !material)
        return;

      auto color_it = material->getFloat3Map().find("color");
      if (color_it != material->getFloat3Map().end())
        mesh_element->SetAttribute("Color", format_vec3(color_it->second).c_str());

      for (const auto& entry : material->getFloatMap())
      {
        auto* param = doc->NewElement("MaterialParam");
        param->SetAttribute("Name", entry.first.c_str());
        param->SetAttribute("Type", "float");
        param->SetAttribute("Value", format_float(entry.second).c_str());
        mesh_element->InsertEndChild(param);
      }

      for (const auto& entry : material->getFloat2Map())
      {
        auto* param = doc->NewElement("MaterialParam");
        param->SetAttribute("Name", entry.first.c_str());
        param->SetAttribute("Type", "float2");
        param->SetAttribute("Value", format_vec2(entry.second).c_str());
        mesh_element->InsertEndChild(param);
      }

      for (const auto& entry : material->getFloat3Map())
      {
        if (to_lower_copy(entry.first) == "color")
          continue;

        auto* param = doc->NewElement("MaterialParam");
        param->SetAttribute("Name", entry.first.c_str());
        param->SetAttribute("Type", "float3");
        param->SetAttribute("Value", format_vec3(entry.second).c_str());
        mesh_element->InsertEndChild(param);
      }
    }

    bool apply_mesh_material_params(
      const tinyxml2::XMLElement* element,
      MeshComponent* mesh,
      std::string* out_error)
    {
      if (!element)
        return true;

      auto material = mesh ? mesh->material() : nullptr;
      for (const tinyxml2::XMLElement* child = element->FirstChildElement();
           child != nullptr;
           child = child->NextSiblingElement())
      {
        const std::string child_name = to_lower_copy(child->Name() ? child->Name() : "");
        if (child_name != "materialparam")
          continue;

        if (!apply_material_param(child, material, out_error))
          return false;
      }

      return true;
    }

    bool configure_transform(
      const tinyxml2::XMLElement* element,
      TransformComponent* transform,
      std::string* out_error)
    {
      if (!transform)
      {
        assign_error(out_error, "Scene resource created an entity without a Transform component.");
        return false;
      }

      glm::vec3 position { 0.0f, 0.0f, 0.0f };
      glm::vec3 rotation { 0.0f, 0.0f, 0.0f };
      glm::vec3 scale { 1.0f, 1.0f, 1.0f };
      if (!read_vec3_attr(element, "Position", &position, position) ||
          !read_vec3_attr(element, "Rotation", &rotation, rotation) ||
          !read_vec3_attr(element, "Scale", &scale, scale))
      {
        assign_error(out_error, "Scene resource contains an invalid transform vector.");
        return false;
      }

      transform->position = position;
      transform->rotation = rotation;
      transform->scale = scale;
      return true;
    }

    bool configure_entity(
      const tinyxml2::XMLElement* element,
      const std::shared_ptr<Scene>& scene,
      std::string* out_error)
    {
      const char* entity_name = element ? element->Attribute("Name") : nullptr;
      auto entity = scene->create_entity(entity_name ? entity_name : "Entity");
      auto* transform = entity->get_component<TransformComponent>();
      return configure_transform(element, transform, out_error);
    }

    bool configure_mesh(
      const tinyxml2::XMLElement* element,
      const std::shared_ptr<Scene>& scene,
      std::string* out_error)
    {
      const char* model_path = element ? element->Attribute("Model") : nullptr;
      if (!model_path || std::string(model_path).empty())
      {
        assign_error(out_error, "Mesh entry is missing a Model attribute.");
        return false;
      }

      const char* entity_name = element->Attribute("Name");
      const char* material_path = element->Attribute("Material");
      const char* shader_path = element->Attribute("Shader");
      const char* animation_path = element->Attribute("Animation");
      const char* animation_clip_name = element->Attribute("AnimationClip");

      auto entity = scene->create_mesh(entity_name ? entity_name : "Mesh");
      auto* transform = entity->get_component<TransformComponent>();
      auto* mesh = entity->get_component<MeshComponent>();
      if (!configure_transform(element, transform, out_error))
        return false;

      if (!mesh || !mesh->set_model(model_path))
      {
        assign_error(out_error, std::string("Failed to load mesh model: ") + model_path);
        return false;
      }

      if (animation_path && std::string(animation_path).size() > 0)
      {
        const bool animation_loaded = mesh->set_animation(
          animation_path,
          animation_clip_name ? animation_clip_name : "");
        if (!animation_loaded)
        {
          assign_error(out_error, std::string("Failed to load mesh animation source: ") + animation_path);
          return false;
        }
      }

      const char* material_to_use = (material_path && std::string(material_path).size() > 0)
        ? material_path
        : (mesh->is_skinned() ? "Assets/materials/Anim.xml" : "Assets/materials/PBR.xml");
      if (!mesh->set_material(material_to_use))
      {
        assign_error(out_error, std::string("Failed to load mesh material: ") + material_to_use);
        return false;
      }

      if (shader_path && std::string(shader_path).size() > 0 && !mesh->set_shader(shader_path))
      {
        assign_error(out_error, std::string("Failed to load mesh shader: ") + shader_path);
        return false;
      }

      if (const char* color_text = element->Attribute("Color"))
      {
        glm::vec3 color { 1.0f, 1.0f, 1.0f };
        if (!parse_vec3_text(color_text, &color))
        {
          assign_error(out_error, "Mesh entry contains an invalid Color attribute.");
          return false;
        }

        if (auto material = mesh->material())
          material->getFloat3Map()["color"] = color;
      }

      if (!apply_mesh_material_params(element, mesh, out_error))
        return false;

      if (animation_path && std::string(animation_path).size() > 0)
      {
        mesh->set_animation_looping(parse_bool_attr(element, "AnimationLoop", mesh->is_animation_looping()));

        float animation_speed = mesh->animation_speed();
        if (!read_float_attr(element, "AnimationSpeed", &animation_speed, animation_speed))
        {
          assign_error(out_error, "Mesh entry contains an invalid AnimationSpeed attribute.");
          return false;
        }
        mesh->set_animation_speed(animation_speed);
        mesh->set_animation_playing(parse_bool_attr(element, "AnimationAutoPlay", mesh->is_animation_playing()));
        if (!mesh->is_animation_playing())
          mesh->stop_animation();
      }

      return true;
    }

    bool configure_terrain(
      const tinyxml2::XMLElement* element,
      const std::shared_ptr<Scene>& scene,
      std::string* out_error)
    {
      const char* entity_name = element ? element->Attribute("Name") : nullptr;
      const char* material_path = element ? element->Attribute("Material") : nullptr;
      const char* shader_path = element ? element->Attribute("Shader") : nullptr;

      auto entity = scene->create_terrain(entity_name ? entity_name : "Terrain");
      auto* transform = entity->get_component<TransformComponent>();
      auto* mesh = entity->get_component<MeshComponent>();
      auto* terrain = entity->get_component<TerrainComponent>();
      if (!configure_transform(element, transform, out_error))
        return false;

      if (!mesh || !terrain)
      {
        assign_error(out_error, "Scene resource created a terrain entry without terrain components.");
        return false;
      }

      TerrainComponent::Settings settings = terrain->settings();
      if (!read_float_attr(element, "Width", &settings.width, settings.width) ||
          !read_float_attr(element, "Depth", &settings.depth, settings.depth) ||
          !read_int_attr(element, "ResolutionX", &settings.resolution_x, settings.resolution_x) ||
          !read_int_attr(element, "ResolutionZ", &settings.resolution_z, settings.resolution_z) ||
          !read_float_attr(element, "HeightScale", &settings.height_scale, settings.height_scale) ||
          !read_float_attr(element, "HeightOffset", &settings.height_offset, settings.height_offset) ||
          !read_float_attr(element, "UvScale", &settings.uv_scale, settings.uv_scale) ||
          !read_float_attr(element, "NoiseFrequency", &settings.noise_frequency, settings.noise_frequency) ||
          !read_int_attr(element, "NoiseOctaves", &settings.noise_octaves, settings.noise_octaves) ||
          !read_float_attr(element, "NoisePersistence", &settings.noise_persistence, settings.noise_persistence) ||
          !read_float_attr(element, "NoiseLacunarity", &settings.noise_lacunarity, settings.noise_lacunarity) ||
          !read_int_attr(element, "Seed", &settings.seed, settings.seed))
      {
        assign_error(out_error, "Terrain entry contains an invalid numeric attribute.");
        return false;
      }

      terrain->apply_settings(settings, true);

      if (material_path && std::string(material_path).size() > 0 && !mesh->set_material(material_path))
      {
        assign_error(out_error, std::string("Failed to load terrain material: ") + material_path);
        return false;
      }

      if (shader_path && std::string(shader_path).size() > 0 && !mesh->set_shader(shader_path))
      {
        assign_error(out_error, std::string("Failed to load terrain shader: ") + shader_path);
        return false;
      }

      if (const char* color_text = element->Attribute("Color"))
      {
        glm::vec3 color { 1.0f, 1.0f, 1.0f };
        if (!parse_vec3_text(color_text, &color))
        {
          assign_error(out_error, "Terrain entry contains an invalid Color attribute.");
          return false;
        }

        if (auto material = mesh->material())
          material->getFloat3Map()["color"] = color;
      }

      if (!apply_mesh_material_params(element, mesh, out_error))
        return false;

      return true;
    }

    bool configure_light(
      const tinyxml2::XMLElement* element,
      const std::shared_ptr<Scene>& scene,
      std::string* out_error)
    {
      const char* entity_name = element ? element->Attribute("Name") : nullptr;
      auto entity = scene->create_light(entity_name ? entity_name : "Light");
      auto* transform = entity->get_component<TransformComponent>();
      auto* light = entity->get_component<LightComponent>();
      if (!configure_transform(element, transform, out_error))
        return false;

      if (!light)
      {
        assign_error(out_error, "Scene resource created a light entry without a Light component.");
        return false;
      }

      light->set_type(parse_light_type(element->Attribute("Type")));

      glm::vec3 direction = light->direction();
      glm::vec3 color = light->color();
      float strength = light->strength();
      float bias_min = light->shadow_bias_min();
      float bias_max = light->shadow_bias_max();
      int filter_radius = light->shadow_filter_radius();

      if (!read_vec3_attr(element, "Direction", &direction, direction) ||
          !read_vec3_attr(element, "Color", &color, color) ||
          !read_float_attr(element, "Strength", &strength, strength) ||
          !read_float_attr(element, "ShadowBiasMin", &bias_min, bias_min) ||
          !read_float_attr(element, "ShadowBiasMax", &bias_max, bias_max) ||
          !read_int_attr(element, "ShadowFilterRadius", &filter_radius, filter_radius))
      {
        assign_error(out_error, "Light entry contains an invalid numeric attribute.");
        return false;
      }

      light->set_direction(direction);
      light->set_color(color);
      light->set_strength(strength);
      light->set_enabled(parse_bool_attr(element, "Enabled", true));
      light->set_casts_shadows(parse_bool_attr(element, "CastShadows", false));
      light->set_shadow_bias_min(bias_min);
      light->set_shadow_bias_max(bias_max);
      light->set_shadow_filter_radius(filter_radius);
      return true;
    }
  }

  bool load_scene_resource(
    const std::string& path,
    std::shared_ptr<IResourceManager> resources,
    SceneResourceDefinition* out_definition,
    std::string* out_error)
  {
    if (!out_definition)
    {
      assign_error(out_error, "No scene output definition was provided.");
      return false;
    }

    *out_definition = SceneResourceDefinition {};
    if (!resources)
    {
      assign_error(out_error, "Scene resource loading requires a valid resource manager.");
      return false;
    }

    const std::string resolved_path = resources->resolve_path(path);
    if (resolved_path.empty())
    {
      assign_error(out_error, std::string("Scene resource path could not be resolved: ") + path);
      return false;
    }

    tinyxml2::XMLDocument doc;
    const auto load_result = doc.LoadFile(resolved_path.c_str());
    if (load_result != tinyxml2::XML_SUCCESS)
    {
      assign_error(
        out_error,
        std::string("Failed to load scene resource: ") + resolved_path + " error: " + doc.ErrorStr());
      return false;
    }

    const tinyxml2::XMLElement* root = doc.FirstChildElement("Scene");
    if (!root)
    {
      assign_error(out_error, std::string("Scene resource is missing a <Scene> root: ") + resolved_path);
      return false;
    }

    const char* scene_name = root->Attribute("Name");
    auto scene = std::make_shared<Scene>(scene_name ? scene_name : "Scene", resources);
    scene->set_skybox_enabled(parse_bool_attr(root, "SkyboxEnabled", true));
    out_definition->show_plane = parse_bool_attr(root, "ShowPlane", false);
    if (const char* environment_map_path = root->Attribute("EnvironmentMap"))
      out_definition->environment_map_path = environment_map_path;

    for (const tinyxml2::XMLElement* child = root->FirstChildElement();
         child != nullptr;
         child = child->NextSiblingElement())
    {
      const std::string element_name = to_lower_copy(child->Name() ? child->Name() : "");
      if (element_name == "mesh")
      {
        if (!configure_mesh(child, scene, out_error))
          return false;
      }
      else if (element_name == "terrain")
      {
        if (!configure_terrain(child, scene, out_error))
          return false;
      }
      else if (element_name == "entity")
      {
        if (!configure_entity(child, scene, out_error))
          return false;
      }
      else if (element_name == "light")
      {
        if (!configure_light(child, scene, out_error))
          return false;
      }
    }

    out_definition->scene = std::move(scene);
    return true;
  }

  bool save_scene_resource(
    const std::string& path,
    const std::shared_ptr<Scene>& scene,
    bool show_plane,
    const std::string& environment_map_path,
    std::string* out_error)
  {
    if (!scene)
    {
      assign_error(out_error, "No scene was provided for export.");
      return false;
    }

    const std::string resolved_path = make_scene_resource_path(path);
    if (resolved_path.empty())
    {
      assign_error(out_error, "Scene export path is empty.");
      return false;
    }

    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path output_path(resolved_path);
    const fs::path parent_dir = output_path.parent_path();
    if (!parent_dir.empty())
      fs::create_directories(parent_dir, ec);
    if (ec)
    {
      assign_error(out_error, std::string("Failed to create scene export directory: ") + parent_dir.string());
      return false;
    }

    tinyxml2::XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration(R"(xml version="1.0" encoding="utf-8")"));

    auto* root = doc.NewElement("Scene");
    root->SetAttribute("Name", scene->name().c_str());
    root->SetAttribute("ShowPlane", format_bool(show_plane).c_str());
    root->SetAttribute("SkyboxEnabled", format_bool(scene->skybox_enabled()).c_str());
    if (!environment_map_path.empty())
      root->SetAttribute("EnvironmentMap", to_scene_attr_path(environment_map_path).c_str());
    doc.InsertEndChild(root);

    for (const auto& entity : scene->entities())
    {
      if (!entity)
        continue;

      auto* transform = entity->get_component<TransformComponent>();
      auto* mesh = entity->get_component<MeshComponent>();
      auto* terrain = entity->get_component<TerrainComponent>();
      auto* light = entity->get_component<LightComponent>();
      if ((mesh || terrain) && light)
      {
        assign_error(
          out_error,
          std::string("Scene export does not support entities with both renderable and Light components: ") + entity->name());
        return false;
      }

      tinyxml2::XMLElement* element = nullptr;
      if (terrain)
      {
        const auto& settings = terrain->settings();
        element = doc.NewElement("Terrain");
        element->SetAttribute("Width", format_float(settings.width).c_str());
        element->SetAttribute("Depth", format_float(settings.depth).c_str());
        element->SetAttribute("ResolutionX", settings.resolution_x);
        element->SetAttribute("ResolutionZ", settings.resolution_z);
        element->SetAttribute("HeightScale", format_float(settings.height_scale).c_str());
        element->SetAttribute("HeightOffset", format_float(settings.height_offset).c_str());
        element->SetAttribute("UvScale", format_float(settings.uv_scale).c_str());
        element->SetAttribute("NoiseFrequency", format_float(settings.noise_frequency).c_str());
        element->SetAttribute("NoiseOctaves", settings.noise_octaves);
        element->SetAttribute("NoisePersistence", format_float(settings.noise_persistence).c_str());
        element->SetAttribute("NoiseLacunarity", format_float(settings.noise_lacunarity).c_str());
        element->SetAttribute("Seed", settings.seed);

        if (mesh)
        {
          if (!mesh->material_path().empty())
            element->SetAttribute("Material", to_scene_attr_path(mesh->material_path()).c_str());
          if (!mesh->shader_path().empty())
            element->SetAttribute("Shader", to_scene_attr_path(mesh->shader_path()).c_str());
          write_material_params(&doc, element, mesh->material());
        }
      }
      else if (mesh)
      {
        if (mesh->model_path().empty())
        {
          assign_error(out_error, std::string("Mesh entity is missing a model path: ") + entity->name());
          return false;
        }

        element = doc.NewElement("Mesh");
        element->SetAttribute("Model", to_scene_attr_path(mesh->model_path()).c_str());
        if (!mesh->material_path().empty())
          element->SetAttribute("Material", to_scene_attr_path(mesh->material_path()).c_str());
        if (!mesh->shader_path().empty())
          element->SetAttribute("Shader", to_scene_attr_path(mesh->shader_path()).c_str());

        const std::string animation_path = mesh->animation_asset_path().empty()
          ? mesh->animation_path()
          : mesh->animation_asset_path();
        if (!animation_path.empty())
        {
          element->SetAttribute("Animation", to_scene_attr_path(animation_path).c_str());
          if (!mesh->animation_clip_name().empty())
            element->SetAttribute("AnimationClip", mesh->animation_clip_name().c_str());
          element->SetAttribute("AnimationLoop", format_bool(mesh->is_animation_looping()).c_str());
          element->SetAttribute("AnimationAutoPlay", format_bool(mesh->is_animation_playing()).c_str());
          element->SetAttribute("AnimationSpeed", format_float(mesh->animation_speed()).c_str());
        }

        write_material_params(&doc, element, mesh->material());
      }
      else if (light)
      {
        element = doc.NewElement("Light");
        element->SetAttribute("Type", light_type_name(light->type()));
        element->SetAttribute("Direction", format_vec3(light->direction()).c_str());
        element->SetAttribute("Color", format_vec3(light->color()).c_str());
        element->SetAttribute("Strength", format_float(light->strength()).c_str());
        element->SetAttribute("Enabled", format_bool(light->enabled()).c_str());
        element->SetAttribute("CastShadows", format_bool(light->casts_shadows()).c_str());
        element->SetAttribute("ShadowBiasMin", format_float(light->shadow_bias_min()).c_str());
        element->SetAttribute("ShadowBiasMax", format_float(light->shadow_bias_max()).c_str());
        element->SetAttribute("ShadowFilterRadius", light->shadow_filter_radius());
      }
      else
      {
        element = doc.NewElement("Entity");
      }

      element->SetAttribute("Name", entity->name().c_str());
      write_transform_attrs(element, transform);
      root->InsertEndChild(element);
    }

    const auto save_result = doc.SaveFile(resolved_path.c_str());
    if (save_result != tinyxml2::XML_SUCCESS)
    {
      assign_error(
        out_error,
        std::string("Failed to save scene resource: ") + resolved_path + " error: " + doc.ErrorStr());
      return false;
    }

    return true;
  }
}
