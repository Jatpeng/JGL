#include "pch.h"

#include "ui/render_settings_panel.h"

#include <filesystem>

#include "engine/render_engine.h"

namespace nui
{
  void RenderSettingsPanel::render(const EditorPanelContext& context, EditorPanelState& state)
  {
    if (!context.engine)
      return;

    const bool scene_renderer_available = context.engine->is_scene_renderer_available();
    const char* backend_name = context.engine->graphics_backend_name();
    ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Render Settings");

    if (!scene_renderer_available)
    {
      ImGui::TextColored(
        ImVec4(0.95f, 0.75f, 0.25f, 1.0f),
        "%s device path is active. Environment, render and screen-effect controls stay disabled until the runtime renderer is implemented.",
        backend_name);
      ImGui::Separator();
    }

    bool transparent = context.engine->is_model_transparent();
    if (ImGui::Checkbox("Transparent Meshes", &transparent))
      context.engine->set_model_transparent(transparent);

    ImGui::SameLine();
    bool show_plane = context.engine->is_plane_show();
    if (ImGui::Checkbox("Ground Plane", &show_plane))
      context.engine->set_plane_show(show_plane);

    if (ImGui::CollapsingHeader("Transparency", ImGuiTreeNodeFlags_DefaultOpen))
    {
      float opacity = context.engine->get_model_opacity();
      begin_disabled(!transparent);
      if (ImGui::SliderFloat("Opacity", &opacity, 0.05f, 1.0f))
        context.engine->set_model_opacity(opacity);
      end_disabled(!transparent);
    }

    if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen))
    {
      begin_disabled(!scene_renderer_available);

      const bool has_custom_environment = context.engine->has_custom_environment_map();
      const std::string environment_source = has_custom_environment
        ? display_project_path(context.engine->get_environment_map_path())
        : std::string("Built-in skybox cubemap");

      ImGui::TextDisabled("Source");
      ImGui::SameLine();
      ImGui::TextWrapped("%s", environment_source.c_str());

      ImGui::TextDisabled("IBL");
      ImGui::SameLine();
      if (context.engine->is_ibl_available())
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Ready");
      else
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "Unavailable");

      begin_disabled(!context.engine->is_ibl_available());
      ImGui::Checkbox("Show IBL Previews in Scene", &state.show_ibl_previews);
      end_disabled(!context.engine->is_ibl_available());
      if (state.show_ibl_previews)
        ImGui::TextDisabled("Scene panel shows environment / irradiance / prefilter / BRDF previews.");

      if (!context.environment_maps.empty())
      {
        draw_path_combo(
          "HDR Preset",
          context.environment_maps,
          has_custom_environment ? context.engine->get_environment_map_path() : std::string(),
          "Built-in skybox",
          [&](const std::string& path)
          {
            const bool loaded = context.engine->load_environment_map(path);
            state.environment_status = loaded
              ? "HDR environment loaded."
              : "Failed to load HDR environment map. See console for details.";
          });
      }
      else
      {
        ImGui::TextDisabled("No .hdr presets found under Assets.");
      }

      if (ImGui::Button("Load HDR Environment..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open HDR Environment Map",
          L"HDR Files (*.hdr)\0*.hdr\0All Files (*.*)\0*.*\0",
          "Assets");
        if (file_path)
        {
          const bool loaded = context.engine->load_environment_map(*file_path);
          state.environment_status = loaded
            ? "HDR environment loaded."
            : "Failed to load HDR environment map. See console for details.";
        }
      }

      if (has_custom_environment)
      {
        ImGui::SameLine();
        if (ImGui::Button("Use Built-in Skybox"))
        {
          context.engine->reset_environment_map();
          state.environment_status = "Reverted to built-in skybox environment.";
        }
      }

      if (!state.environment_status.empty())
        ImGui::TextWrapped("%s", state.environment_status.c_str());

      ImGui::TextDisabled("Skybox visibility is controlled from Scene Hierarchy.");
      end_disabled(!scene_renderer_available);
    }

    if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen))
    {
      begin_disabled(!scene_renderer_available);

      int render_mode = context.engine->get_render_mode() == nengine::RenderEngine::RenderMode::Forward ? 0 : 1;
      if (ImGui::Combo("Mode", &render_mode, "Forward\0Deferred\0"))
      {
        context.engine->set_render_mode(render_mode == 0
          ? nengine::RenderEngine::RenderMode::Forward
          : nengine::RenderEngine::RenderMode::Deferred);
      }

      const bool disable_debug_view = context.engine->get_render_mode() != nengine::RenderEngine::RenderMode::Deferred;
      begin_disabled(disable_debug_view);
      int debug_view = static_cast<int>(context.engine->get_debug_view());
      if (ImGui::Combo("Debug View", &debug_view,
                       "Final\0Position\0Normal\0Albedo\0Roughness\0Metallic\0"))
      {
        context.engine->set_debug_view(static_cast<nengine::RenderEngine::DebugView>(debug_view));
      }
      end_disabled(disable_debug_view);

      if (context.deferred_requested && !context.deferred_available)
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "At least one mesh falls back to forward rendering.");

      if (ImGui::Button("Reload All Shaders (F5)"))
      {
        const bool reload_ok = context.engine->reload_runtime_shaders();
        state.shader_reload_status = reload_ok
          ? "Runtime shaders reloaded."
          : "Shader reload failed or no shader was active. See console for details.";
      }

      if (!state.shader_reload_status.empty())
        ImGui::TextWrapped("%s", state.shader_reload_status.c_str());

      end_disabled(!scene_renderer_available);
    }

    if (ImGui::CollapsingHeader("Screen Effects", ImGuiTreeNodeFlags_DefaultOpen))
    {
      begin_disabled(!scene_renderer_available);

      const std::string current_effect_path = context.engine->get_screen_effect_material_path();
      const std::string current_effect_label = context.engine->has_screen_effect_material()
        ? file_label(current_effect_path)
        : std::string("Off");

      if (ImGui::BeginCombo("Effect Material", current_effect_label.c_str()))
      {
        const bool off_selected = !context.engine->has_screen_effect_material();
        if (ImGui::Selectable("Off", off_selected))
          context.engine->clear_screen_effect_material();
        if (off_selected)
          ImGui::SetItemDefaultFocus();

        for (const auto& effect_path : context.screen_effect_materials)
        {
          const bool is_selected = current_effect_path == effect_path;
          const std::string effect_name = std::filesystem::path(effect_path).stem().string();
          if (ImGui::Selectable(effect_name.c_str(), is_selected))
            context.engine->set_screen_effect_material(effect_path);
          if (is_selected)
            ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
      }

      if (ImGui::Button("Load Effect Material..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Screen Effect Material",
          L"Material Files (*.xml;*.mtl)\0*.xml;*.mtl\0All Files (*.*)\0*.*\0",
          "Assets/screen_effects");
        if (file_path)
          context.engine->set_screen_effect_material(*file_path);
      }

      if (context.engine->has_screen_effect_material())
      {
        ImGui::SameLine();
        if (ImGui::Button("Clear Effect"))
          context.engine->clear_screen_effect_material();

        ImGui::TextDisabled("Current");
        ImGui::SameLine();
        ImGui::TextUnformatted(file_label(current_effect_path).c_str());

        auto effect_material = context.engine->get_screen_effect_material();
        if (effect_material &&
            ImGui::CollapsingHeader("Effect Parameters", ImGuiTreeNodeFlags_DefaultOpen))
        {
          draw_material_parameter_editor(effect_material, context.texture_presets);
        }
      }

      end_disabled(!scene_renderer_available);
    }

    if (ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const bool capture_available = context.engine->is_frame_capture_available();
      const bool capture_pending = context.engine->is_frame_capture_pending();
      const bool capture_in_progress = context.engine->is_frame_capture_in_progress();

      begin_disabled(!capture_available);
      if (ImGui::Button("Capture Frame (F12)"))
        context.engine->request_frame_capture();
      end_disabled(!capture_available);

      if (!capture_available)
      {
        ImGui::TextColored(
          ImVec4(0.95f, 0.75f, 0.25f, 1.0f),
          "RenderDoc not detected. Launch from RenderDoc or make renderdoc.dll available.");
      }
      else if (capture_in_progress)
      {
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Capturing current frame...");
      }
      else if (capture_pending)
      {
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Capture queued for the next frame.");
      }
      else
      {
        ImGui::TextDisabled("Capture output: build/captures");
      }

      if (capture_available && !context.engine->get_last_frame_capture_path().empty())
        ImGui::TextWrapped("Last Capture: %s", context.engine->get_last_frame_capture_path().c_str());
    }

    ImGui::End();
  }
}
