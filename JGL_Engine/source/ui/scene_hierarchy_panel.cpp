#include "pch.h"

#include "ui/scene_hierarchy_panel.h"

#include <cctype>

#include "engine/render_engine.h"
#include "engine/scene.h"
#include "engine/scene_loader.h"
#include "misc/cpp/imgui_stdlib.h"

namespace nui
{
  namespace
  {
    std::string default_scene_resource_filename(const std::string& scene_name)
    {
      std::string filename = scene_name.empty() ? "scene" : scene_name;
      for (char& ch : filename)
      {
        if (std::iscntrl(static_cast<unsigned char>(ch)) ||
            ch == '<' || ch == '>' || ch == ':' || ch == '"' ||
            ch == '/' || ch == '\\' || ch == '|' || ch == '?' || ch == '*')
        {
          ch = '_';
        }
      }

      if (filename.empty())
        filename = "scene";
      if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".xml")
        filename += ".xml";
      return filename;
    }
  }

  void SceneHierarchyPanel::render(const EditorPanelContext& context, EditorPanelState& state)
  {
    if (!context.engine)
      return;

    ImGui::SetNextWindowSize(ImVec2(340.0f, 680.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Hierarchy");

    if (ImGui::Button("Reset Camera"))
      context.engine->reset_view();

    ImGui::SameLine();
    bool show_plane = context.engine->is_plane_show();
    if (ImGui::Checkbox("Plane##HierarchyQuick", &show_plane))
      context.engine->set_plane_show(show_plane);

    ImGui::SameLine();
    ImGui::TextDisabled("RMB Orbit | MMB Pan | W/S Zoom | F Reset");

    if (!context.scene)
    {
      ImGui::Separator();
      ImGui::TextDisabled("No active scene.");
      ImGui::End();
      return;
    }

    std::string scene_name = context.scene->name();
    if (ImGui::InputText("Scene Name", &scene_name))
      context.scene->set_name(scene_name);

    bool skybox_enabled = context.scene->skybox_enabled();
    if (ImGui::Checkbox("Skybox Visible", &skybox_enabled))
      context.scene->set_skybox_enabled(skybox_enabled);

    if (ImGui::TreeNodeEx("Scene Resource", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const std::string resource_path_label = state.scene_resource_path.empty()
        ? std::string("Unsaved scene")
        : display_project_path(state.scene_resource_path);

      ImGui::TextDisabled("Current");
      ImGui::SameLine();
      ImGui::TextWrapped("%s", resource_path_label.c_str());

      if (ImGui::Button("Open Scene..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Scene Resource",
          L"Scene Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0",
          "Assets/scenes");
        if (file_path)
        {
          nengine::SceneResourceDefinition definition;
          std::string error;
          if (nengine::load_scene_resource(*file_path, context.engine->get_resource_manager(), &definition, &error) &&
              definition.scene)
          {
            context.engine->set_scene(definition.scene);
            context.engine->set_plane_show(definition.show_plane);

            bool environment_ok = true;
            if (!definition.environment_map_path.empty())
              environment_ok = context.engine->load_environment_map(definition.environment_map_path);
            else
              context.engine->reset_environment_map();

            state.selected_object_id = 0;
            state.scene_resource_path = *file_path;
            state.scene_resource_status = environment_ok
              ? "Scene resource loaded."
              : "Scene loaded, but the environment map could not be applied. See console for details.";
          }
          else
          {
            state.scene_resource_status = error.empty()
              ? "Scene resource load failed."
              : "Scene resource load failed: " + error;
          }
        }
      }

      ImGui::SameLine();
      if (ImGui::Button("Export Scene..."))
      {
        const auto file_path = save_native_file_dialog(
          L"Export Scene Resource",
          L"Scene Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0",
          L"xml",
          "Assets/scenes",
          default_scene_resource_filename(context.scene->name()));
        if (file_path)
        {
          std::string error;
          const std::string environment_path = context.engine->has_custom_environment_map()
            ? context.engine->get_environment_map_path()
            : std::string();
          const bool saved = nengine::save_scene_resource(
            *file_path,
            context.scene,
            context.engine->is_plane_show(),
            environment_path,
            &error);
          if (saved)
          {
            state.scene_resource_path = *file_path;
            state.scene_resource_status = "Scene resource exported.";
          }
          else
          {
            state.scene_resource_status = error.empty()
              ? "Scene resource export failed."
              : "Scene resource export failed: " + error;
          }
        }
      }

      if (!state.scene_resource_status.empty())
        ImGui::TextWrapped("%s", state.scene_resource_status.c_str());

      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Create", ImGuiTreeNodeFlags_DefaultOpen))
    {
      draw_path_combo(
        "Mesh Preset##Create",
        context.model_presets,
        state.new_mesh_model_path,
        "Select mesh preset...",
        [&state](const std::string& path)
        {
          state.new_mesh_model_path = path;
        });

      draw_path_combo(
        "Material Preset##Create",
        context.material_presets,
        state.new_mesh_material_path,
        "Select material preset...",
        [&state](const std::string& path)
        {
          state.new_mesh_material_path = path;
        });

      ImGui::Combo("Light Type##Create", &state.new_light_type, "Point\0Directional\0");

      if (ImGui::Button("Add Mesh"))
      {
        auto mesh = context.scene->create_mesh("mesh_" + std::to_string(context.scene->entities().size() + 1));
        auto* mesh_component = mesh->get_component<nengine::MeshComponent>();
        if (mesh_component)
        {
          if (!state.new_mesh_model_path.empty())
            mesh_component->set_model(state.new_mesh_model_path);
          if (!state.new_mesh_material_path.empty())
            mesh_component->set_material(state.new_mesh_material_path);
        }
        state.selected_object_id = mesh->id();
      }

      ImGui::SameLine();
      if (ImGui::Button("Add Terrain"))
      {
        auto terrain = context.scene->create_terrain("terrain_" + std::to_string(context.scene->entities().size() + 1));
        auto* mesh_component = terrain->get_component<nengine::MeshComponent>();
        if (mesh_component && !state.new_mesh_material_path.empty())
          mesh_component->set_material(state.new_mesh_material_path);
        state.selected_object_id = terrain->id();
      }

      ImGui::SameLine();
      if (ImGui::Button("Add Light"))
      {
        auto light = context.scene->create_light("light_" + std::to_string(context.scene->entities().size() + 1));
        auto* transform = light->get_component<nengine::TransformComponent>();
        auto* light_component = light->get_component<nengine::LightComponent>();
        if (transform)
          transform->position = glm::vec3(1.5f, 3.5f, 3.0f);
        if (light_component)
        {
          const auto light_type = static_cast<nengine::LightComponent::LightType>(state.new_light_type);
          light_component->set_type(light_type);
          if (light_type == nengine::LightComponent::LightType::Directional)
          {
            light_component->set_strength(3.5f);
            light_component->set_direction(glm::vec3(-0.35f, -1.0f, -0.25f));
            light_component->set_casts_shadows(true);
            sync_directional_light_transform_from_direction(transform, light_component);
          }
          else
          {
            light_component->set_strength(100.0f);
            light_component->set_casts_shadows(false);
          }
        }
        state.selected_object_id = light->id();
      }

      ImGui::SameLine();
      begin_disabled(!context.selected_entity);
      if (ImGui::Button("Remove Selected") && context.selected_entity)
      {
        context.scene->remove_entity(context.selected_entity->id());
        state.selected_object_id = 0;
      }
      end_disabled(!context.selected_entity);

      ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::TextDisabled("Objects");

    for (const auto& entity : context.scene->entities())
    {
      const bool is_selected = context.selected_entity && entity->id() == context.selected_entity->id();
      const char* type_name = "Entity";
      if (entity->get_component<nengine::TerrainComponent>()) type_name = "Terrain";
      else if (entity->get_component<nengine::MeshComponent>()) type_name = "Mesh";
      else if (entity->get_component<nengine::LightComponent>()) type_name = "Light";

      std::string label = std::string(type_name) + "##" + std::to_string(entity->id());
      if (ImGui::Selectable((entity->name() + "###" + label).c_str(), is_selected))
        state.selected_object_id = entity->id();

      ImGui::SameLine(240.0f);
      ImGui::TextDisabled("%s", type_name);
    }

    ImGui::End();
  }
}
