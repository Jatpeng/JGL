#include "pch.h"

#include "ui/inspector_panel.h"

#include "engine/scene.h"
#include "misc/cpp/imgui_stdlib.h"

namespace nui
{
  void InspectorPanel::render(const EditorPanelContext& context, EditorPanelState& state)
  {
    if (!context.engine)
      return;

    ImGui::SetNextWindowSize(ImVec2(420.0f, 680.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    if (!context.selected_entity)
    {
      ImGui::TextDisabled("Select an entity from Scene Hierarchy.");
      ImGui::End();
      return;
    }

    std::string name = context.selected_entity->name();
    if (ImGui::InputText("Name", &name))
      context.selected_entity->set_name(name);

    const bool is_terrain = context.selected_terrain != nullptr;
    const char* type_name = "Entity";
    if (is_terrain) type_name = "Terrain";
    else if (context.selected_mesh) type_name = "Mesh";
    else if (context.selected_light) type_name = "Light";

    ImGui::TextDisabled(
      "Id %llu | Type %s",
      static_cast<unsigned long long>(context.selected_entity->id()),
      type_name);

    if (auto* transform = context.selected_entity->get_component<nengine::TransformComponent>())
    {
      if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::SliderFloat3("Position", (float*)&transform->position, -20.0f, 20.0f);
        const bool rotation_changed = ImGui::SliderFloat3("Rotation", (float*)&transform->rotation, -180.0f, 180.0f);
        ImGui::SliderFloat3("Scale", (float*)&transform->scale, 0.01f, 20.0f);
        if (rotation_changed && context.selected_light)
          sync_directional_light_direction_from_transform(transform, context.selected_light);
        ImGui::TreePop();
      }
    }

    if (context.selected_mesh && ImGui::CollapsingHeader("Mesh Assets", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const std::string current_mesh_file = is_terrain
        ? std::string("<procedural terrain>")
        : file_label(context.selected_mesh->model_path());
      const std::string current_shader_file = file_label(context.selected_mesh->shader_path());
      const std::string current_material_file = file_label(context.selected_mesh->material_path());
      const std::string current_animation_file = file_label(
        context.selected_mesh->animation_asset_path().empty()
          ? context.selected_mesh->animation_path()
          : context.selected_mesh->animation_asset_path());

      ImGui::TextDisabled("Mesh");
      ImGui::SameLine();
      ImGui::TextUnformatted(current_mesh_file.c_str());
      ImGui::TextDisabled("Shader");
      ImGui::SameLine();
      ImGui::TextUnformatted(current_shader_file.c_str());
      ImGui::TextDisabled("Material");
      ImGui::SameLine();
      ImGui::TextUnformatted(current_material_file.c_str());
      ImGui::TextDisabled("Animation");
      ImGui::SameLine();
      ImGui::TextUnformatted(current_animation_file.c_str());
      ImGui::TextDisabled("Skinned");
      ImGui::SameLine();
      ImGui::TextUnformatted(context.selected_mesh->is_skinned() ? "Yes" : "No");

      if (!is_terrain)
      {
        draw_path_combo(
          "Mesh Preset##Selected",
          context.model_presets,
          context.selected_mesh->model_path(),
          "Select mesh preset...",
          [&](const std::string& path)
          {
            context.selected_mesh->set_model(path);
          });

        if (ImGui::Button("Load Mesh..."))
        {
          const auto file_path = open_native_file_dialog(
            L"Open Mesh",
            L"Mesh Files (*.fbx;*.obj;*.dae)\0*.fbx;*.obj;*.dae\0All Files (*.*)\0*.*\0",
            "Assets");
          if (file_path)
            context.selected_mesh->set_model(*file_path);
        }
      }
      else
      {
        ImGui::TextDisabled("Terrain geometry is generated from Terrain settings.");
      }

      if (context.selected_mesh->is_skinned())
      {
        draw_path_combo(
          "Animation Preset##Selected",
          context.animation_presets,
          context.selected_mesh->animation_asset_path(),
          "Select animation preset...",
          [&](const std::string& path)
          {
            const bool load_ok = context.selected_mesh->set_animation_asset(path);
            state.animation_status = load_ok
              ? "Animation asset loaded."
              : "Animation asset load failed. See console for details.";
          });

        if (ImGui::Button("Load Animation..."))
        {
          const auto file_path = open_native_file_dialog(
            L"Open Animation Asset",
            L"JAnim Files (*.janim)\0*.janim\0All Files (*.*)\0*.*\0",
            "Assets/animations");
          if (file_path)
          {
            const bool load_ok = context.selected_mesh->set_animation_asset(*file_path);
            state.animation_status = load_ok
              ? "Animation asset loaded."
              : "Animation asset load failed. See console for details.";
          }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Animation"))
        {
          context.selected_mesh->clear_animation();
          state.animation_status = "Animation cleared.";
        }
      }
      else
      {
        ImGui::TextDisabled("The selected mesh has no skinning data. Animation assets require a skinned model.");
      }

      draw_path_combo(
        "Shader Preset##Selected",
        context.shader_presets,
        context.selected_mesh->shader_path(),
        "Select shader preset...",
        [&](const std::string& path)
        {
          context.selected_mesh->set_shader(path);
        });

      if (ImGui::Button("Load Shader..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Shader",
          L"Shader Files (*_vs.shader;*_fs.shader)\0*_vs.shader;*_fs.shader\0All Files (*.*)\0*.*\0",
          "JGL_Engine/shaders");
        if (file_path)
          context.selected_mesh->set_shader(*file_path);
      }
      ImGui::SameLine();
      if (ImGui::Button("Reload Shader"))
      {
        const bool reload_ok = context.selected_mesh->reload_shader();
        state.shader_reload_status = reload_ok
          ? "Selected mesh shader reloaded."
          : "Selected mesh shader reload failed. See console for compile errors.";
      }

      draw_path_combo(
        "Material Preset##Selected",
        context.material_presets,
        context.selected_mesh->material_path(),
        "Select material preset...",
        [&](const std::string& path)
        {
          context.selected_mesh->set_material(path);
        });

      if (ImGui::Button("Load Material..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Material",
          L"Material Files (*.mtl;*.xml)\0*.mtl;*.xml\0All Files (*.*)\0*.*\0",
          "Assets/materials");
        if (file_path)
          context.selected_mesh->set_material(*file_path);
      }

      if (!state.shader_reload_status.empty())
        ImGui::TextWrapped("%s", state.shader_reload_status.c_str());

      if (!state.animation_status.empty())
        ImGui::TextWrapped("%s", state.animation_status.c_str());
    }

    if (context.selected_terrain &&
        ImGui::CollapsingHeader("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
    {
      float width = context.selected_terrain->width();
      if (ImGui::SliderFloat("Width", &width, 1.0f, 256.0f, "%.1f"))
        context.selected_terrain->set_width(width);

      float depth = context.selected_terrain->depth();
      if (ImGui::SliderFloat("Depth", &depth, 1.0f, 256.0f, "%.1f"))
        context.selected_terrain->set_depth(depth);

      int resolution_x = context.selected_terrain->resolution_x();
      if (ImGui::SliderInt("Resolution X", &resolution_x, 2, 256))
        context.selected_terrain->set_resolution_x(resolution_x);

      int resolution_z = context.selected_terrain->resolution_z();
      if (ImGui::SliderInt("Resolution Z", &resolution_z, 2, 256))
        context.selected_terrain->set_resolution_z(resolution_z);

      float height_scale = context.selected_terrain->height_scale();
      if (ImGui::SliderFloat("Height Scale", &height_scale, 0.0f, 32.0f, "%.2f"))
        context.selected_terrain->set_height_scale(height_scale);

      float height_offset = context.selected_terrain->height_offset();
      if (ImGui::SliderFloat("Height Offset", &height_offset, -16.0f, 16.0f, "%.2f"))
        context.selected_terrain->set_height_offset(height_offset);

      float noise_frequency = context.selected_terrain->noise_frequency();
      if (ImGui::SliderFloat("Noise Frequency", &noise_frequency, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
        context.selected_terrain->set_noise_frequency(noise_frequency);

      int noise_octaves = context.selected_terrain->noise_octaves();
      if (ImGui::SliderInt("Noise Octaves", &noise_octaves, 1, 8))
        context.selected_terrain->set_noise_octaves(noise_octaves);

      float noise_persistence = context.selected_terrain->noise_persistence();
      if (ImGui::SliderFloat("Noise Persistence", &noise_persistence, 0.0f, 1.0f, "%.2f"))
        context.selected_terrain->set_noise_persistence(noise_persistence);

      float noise_lacunarity = context.selected_terrain->noise_lacunarity();
      if (ImGui::SliderFloat("Noise Lacunarity", &noise_lacunarity, 1.0f, 4.0f, "%.2f"))
        context.selected_terrain->set_noise_lacunarity(noise_lacunarity);

      float uv_scale = context.selected_terrain->uv_scale();
      if (ImGui::SliderFloat("UV Scale", &uv_scale, 0.1f, 32.0f, "%.2f"))
        context.selected_terrain->set_uv_scale(uv_scale);

      int seed = context.selected_terrain->seed();
      if (ImGui::InputInt("Seed", &seed))
        context.selected_terrain->set_seed(seed);

      if (ImGui::Button("Rebuild Terrain"))
        context.selected_terrain->rebuild();

      ImGui::SameLine();
      ImGui::TextDisabled("Height@Origin %.3f", context.selected_terrain->sample_height(0.0f, 0.0f));
    }

    if (context.selected_mesh &&
        context.selected_mesh->is_skinned() &&
        ImGui::CollapsingHeader("Animation Playback", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const bool has_animation = context.selected_mesh->has_animation();
      ImGui::TextDisabled("Clip");
      ImGui::SameLine();
      ImGui::TextUnformatted(
        context.selected_mesh->animation_clip_name().empty()
          ? "<default>"
          : context.selected_mesh->animation_clip_name().c_str());

      begin_disabled(!has_animation);
      if (ImGui::Button(context.selected_mesh->is_animation_playing() ? "Pause" : "Play"))
        context.selected_mesh->set_animation_playing(!context.selected_mesh->is_animation_playing());
      ImGui::SameLine();
      if (ImGui::Button("Stop"))
        context.selected_mesh->stop_animation();

      bool loop = context.selected_mesh->is_animation_looping();
      if (ImGui::Checkbox("Loop", &loop))
        context.selected_mesh->set_animation_looping(loop);

      float speed = context.selected_mesh->animation_speed();
      if (ImGui::SliderFloat("Speed", &speed, 0.0f, 2.0f, "%.2f"))
        context.selected_mesh->set_animation_speed(speed);
      end_disabled(!has_animation);

      if (!has_animation)
        ImGui::TextDisabled("Load a .janim asset to preview skeletal animation.");
    }

    if (context.selected_mesh &&
        context.selected_mesh->model() &&
        context.selected_mesh->material() &&
        ImGui::CollapsingHeader("Material Parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
      draw_material_parameter_editor(context.selected_mesh->material());
    }

    if (context.selected_light && ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool enabled = context.selected_light->enabled();
      if (ImGui::Checkbox("Enabled", &enabled))
        context.selected_light->set_enabled(enabled);

      int light_type = static_cast<int>(context.selected_light->type());
      if (ImGui::Combo("Type", &light_type, "Point\0Directional\0"))
      {
        context.selected_light->set_type(static_cast<nengine::LightComponent::LightType>(light_type));
        if (auto* transform = context.selected_entity->get_component<nengine::TransformComponent>())
        {
          if (context.selected_light->type() == nengine::LightComponent::LightType::Directional)
            sync_directional_light_direction_from_transform(transform, context.selected_light);
        }
      }

      glm::vec3 color = context.selected_light->color();
      if (ImGui::ColorEdit3("Color", (float*)&color))
        context.selected_light->set_color(color);

      float strength = context.selected_light->strength();
      const float max_strength =
        context.selected_light->type() == nengine::LightComponent::LightType::Directional ? 20.0f : 200.0f;
      if (ImGui::SliderFloat("Strength", &strength, 0.0f, max_strength))
        context.selected_light->set_strength(strength);

      if (context.selected_light->type() == nengine::LightComponent::LightType::Directional)
      {
        glm::vec3 direction = context.selected_light->direction();
        if (ImGui::SliderFloat3("Direction", (float*)&direction, -1.0f, 1.0f))
        {
          context.selected_light->set_direction(direction);
          if (auto* transform = context.selected_entity->get_component<nengine::TransformComponent>())
            sync_directional_light_transform_from_direction(transform, context.selected_light);
        }

        bool casts_shadows = context.selected_light->casts_shadows();
        if (ImGui::Checkbox("Cast Shadows", &casts_shadows))
          context.selected_light->set_casts_shadows(casts_shadows);

        begin_disabled(!casts_shadows);
        int filter_radius = context.selected_light->shadow_filter_radius();
        if (ImGui::SliderInt("Shadow PCF Radius", &filter_radius, 0, 4))
          context.selected_light->set_shadow_filter_radius(filter_radius);

        float shadow_bias_min = context.selected_light->shadow_bias_min();
        if (ImGui::SliderFloat("Shadow Bias Min", &shadow_bias_min, 0.00001f, 0.01f, "%.5f", ImGuiSliderFlags_Logarithmic))
          context.selected_light->set_shadow_bias_min(shadow_bias_min);

        float shadow_bias_max = context.selected_light->shadow_bias_max();
        if (ImGui::SliderFloat("Shadow Bias Max", &shadow_bias_max, 0.0001f, 0.05f, "%.5f", ImGuiSliderFlags_Logarithmic))
          context.selected_light->set_shadow_bias_max(shadow_bias_max);
        end_disabled(!casts_shadows);

        ImGui::TextDisabled("Directional shadows currently drive the imported Graphics2 shadow pass.");
      }
      else
      {
        ImGui::TextDisabled("Point lights use inverse-square falloff. Shadow maps are enabled for directional lights.");
      }
    }

    ImGui::End();
  }
}
