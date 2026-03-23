#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "imgui.h"

class Material;

namespace nengine
{
  class Entity;
  class LightComponent;
  class MeshComponent;
  class TerrainComponent;
  class RenderEngine;
  class Scene;
  class TransformComponent;
}

namespace nui
{
  struct EditorPanelState
  {
    uint64_t selected_object_id = 0;
    std::string shader_reload_status;
    std::string animation_status;
    std::string environment_status;
    std::string scene_resource_status;
    std::string scene_resource_path;
    std::string new_mesh_model_path = "Assets/models/cube.fbx";
    std::string new_mesh_material_path = "Assets/materials/PBR.xml";
    int new_light_type = 1;
  };

  struct EditorPanelContext
  {
    nengine::RenderEngine* engine = nullptr;
    std::shared_ptr<nengine::Scene> scene;
    std::shared_ptr<nengine::Entity> selected_entity;
    nengine::MeshComponent* selected_mesh = nullptr;
    nengine::TerrainComponent* selected_terrain = nullptr;
    nengine::LightComponent* selected_light = nullptr;
    bool deferred_requested = false;
    bool deferred_available = false;
    std::vector<std::string> model_presets;
    std::vector<std::string> material_presets;
    std::vector<std::string> animation_presets;
    std::vector<std::string> shader_presets;
    std::vector<std::string> environment_maps;
    std::vector<std::string> screen_effect_materials;
  };

  EditorPanelContext build_editor_panel_context(
    nengine::RenderEngine* engine,
    EditorPanelState* state);

  std::optional<std::string> open_native_file_dialog(
    const wchar_t* title,
    const wchar_t* filter,
    const std::string& initial_relative_dir = "");

  std::optional<std::string> save_native_file_dialog(
    const wchar_t* title,
    const wchar_t* filter,
    const wchar_t* default_extension,
    const std::string& initial_relative_dir = "",
    const std::string& default_filename = "");

  std::string file_label(const std::string& path);
  std::string display_project_path(const std::string& path);
  std::vector<std::string> list_mesh_presets();
  std::vector<std::string> list_material_presets();
  std::vector<std::string> list_animation_presets();
  std::vector<std::string> list_environment_maps();
  std::vector<std::string> list_mesh_shader_programs();
  std::vector<std::string> list_screen_effect_materials();
  glm::vec3 direction_to_rotation(const glm::vec3& direction);
  glm::vec3 rotation_to_direction(const glm::vec3& rotation);
  void sync_directional_light_transform_from_direction(
    nengine::TransformComponent* transform,
    const nengine::LightComponent* light);
  void sync_directional_light_direction_from_transform(
    const nengine::TransformComponent* transform,
    nengine::LightComponent* light);
  void draw_material_parameter_editor(const std::shared_ptr<Material>& material);
  void begin_disabled(bool disabled);
  void end_disabled(bool disabled);

  template <typename Callback>
  void draw_path_combo(
    const char* label,
    const std::vector<std::string>& paths,
    const std::string& current_path,
    const char* empty_label,
    Callback&& on_select)
  {
    const std::string combo_label = current_path.empty()
      ? std::string(empty_label)
      : display_project_path(current_path);

    if (!ImGui::BeginCombo(label, combo_label.c_str()))
      return;

    for (const auto& path : paths)
    {
      const bool is_selected = path == current_path;
      const std::string path_label = display_project_path(path);
      if (ImGui::Selectable(path_label.c_str(), is_selected))
        on_select(path);
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }

    if (paths.empty())
      ImGui::TextDisabled("No presets found.");

    ImGui::EndCombo();
  }
}
