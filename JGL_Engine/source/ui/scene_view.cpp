#include "pch.h"

#include "scene_view.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace nui
{
  SceneView::SceneView(std::shared_ptr<nengine::RenderEngine> engine)
    : mEngine(engine.get())
  {
  }

  SceneView::SceneView(nengine::RenderEngine* engine)
    : mEngine(engine)
  {
  }

  void SceneView::set_engine(std::shared_ptr<nengine::RenderEngine> engine)
  {
    mEngine = engine.get();
  }

  void SceneView::set_engine(nengine::RenderEngine* engine)
  {
    mEngine = engine;
  }

  void SceneView::set_scene(std::shared_ptr<nengine::Scene> scene)
  {
    if (mEngine)
      mEngine->set_scene(std::move(scene));
  }

  std::shared_ptr<nengine::Scene> SceneView::get_scene() const
  {
    return mEngine ? mEngine->get_scene() : nullptr;
  }

  void SceneView::render(EditorPanelState& state)
  {
    if (!mEngine)
      return;

    const bool scene_renderer_available = mEngine->is_scene_renderer_available();
    const char* backend_name = mEngine->graphics_backend_name();
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene");

    const bool deferred_available = mEngine->is_deferred_available();
    const bool deferred_requested = mEngine->get_render_mode() == nengine::RenderEngine::RenderMode::Deferred;
    const bool show_gbuffer_thumbnails =
      scene_renderer_available &&
      deferred_requested &&
      deferred_available &&
      mShowGBufferPreviews;
    const bool show_ibl_thumbnails =
      scene_renderer_available &&
      state.show_ibl_previews &&
      mEngine->is_ibl_available();

    if (ImGui::Button("Reset View"))
      mEngine->reset_view();

    ImGui::SameLine();
    const bool capture_available = mEngine->is_frame_capture_available();
    begin_disabled(!capture_available);
    if (ImGui::Button("Capture Frame"))
      mEngine->request_frame_capture();
    end_disabled(!capture_available);

    ImGui::SameLine();
    begin_disabled(!scene_renderer_available);
    if (ImGui::Button("Reload Shaders"))
      mEngine->reload_runtime_shaders();
    end_disabled(!scene_renderer_available);

    ImGui::SameLine();
    begin_disabled(!scene_renderer_available);
    bool show_plane = mEngine->is_plane_show();
    if (ImGui::Checkbox("Plane", &show_plane))
      mEngine->set_plane_show(show_plane);

    ImGui::SameLine();
    bool transparent_model = mEngine->is_model_transparent();
    if (ImGui::Checkbox("Transparent", &transparent_model))
      mEngine->set_model_transparent(transparent_model);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    int render_mode = deferred_requested ? 1 : 0;
    if (ImGui::Combo("Mode##SceneToolbar", &render_mode, "Forward\0Deferred\0"))
    {
      mEngine->set_render_mode(render_mode == 0
        ? nengine::RenderEngine::RenderMode::Forward
        : nengine::RenderEngine::RenderMode::Deferred);
    }
    end_disabled(!scene_renderer_available);

    if (scene_renderer_available && mEngine->get_render_mode() == nengine::RenderEngine::RenderMode::Deferred)
    {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(140.0f);
      int debug_view = static_cast<int>(mEngine->get_debug_view());
      if (ImGui::Combo("Debug##SceneToolbar", &debug_view,
                       "Final\0Position\0Normal\0Albedo\0Roughness\0Metallic\0"))
      {
        mEngine->set_debug_view(static_cast<nengine::RenderEngine::DebugView>(debug_view));
      }

      ImGui::SameLine();
      ImGui::Checkbox("G-Buffer", &mShowGBufferPreviews);
    }

    ImGui::TextDisabled("RMB Orbit | MMB Pan | W/S Zoom | F Reset");
    if (capture_available)
    {
      if (mEngine->is_frame_capture_in_progress())
        ImGui::TextDisabled("RenderDoc: capturing...");
      else if (mEngine->is_frame_capture_pending())
        ImGui::TextDisabled("RenderDoc: queued for next frame");
      else
        ImGui::TextDisabled("RenderDoc: ready | F12 capture");

      if (!mEngine->get_last_frame_capture_path().empty())
        ImGui::TextDisabled("Saved: %s", mEngine->get_last_frame_capture_path().c_str());
    }
    else
    {
      ImGui::TextDisabled("RenderDoc: unavailable");
    }

    const glm::ivec2 target_size = mEngine->get_render_target_size();
    const char* active_mode = deferred_requested && deferred_available ? "Deferred" : "Forward";
    ImGui::TextDisabled(
      "Target %d x %d | Backend %s | Active %s",
      target_size.x,
      target_size.y,
      backend_name,
      active_mode);
    ImGui::SameLine();
    ImGui::TextDisabled("| IBL %s", mEngine->is_ibl_available() ? "Ready" : "Off");

    if (!scene_renderer_available)
    {
      ImGui::TextColored(
        ImVec4(0.95f, 0.75f, 0.25f, 1.0f),
        "%s device path is active, but the runtime scene renderer is still OpenGL-only.",
        backend_name);
    }
    else if (deferred_requested && !deferred_available)
    {
      ImGui::TextColored(
        ImVec4(0.95f, 0.75f, 0.25f, 1.0f),
        "Current material is missing deferred inputs. Rendering falls back to forward mode.");
    }

    ImGui::Separator();

    if (const auto scene = mEngine->get_scene())
    {
      int mesh_count = 0;
      int light_count = 0;
      for (const auto& entity : scene->entities())
      {
        if (entity->get_component<nengine::MeshComponent>())
          ++mesh_count;
        else if (entity->get_component<nengine::LightComponent>())
          ++light_count;
      }

      ImGui::TextDisabled(
        "Scene %s | Meshes %d | Lights %d",
        scene->name().c_str(),
        mesh_count,
        light_count);
      ImGui::Separator();
    }

    float preview_sections_height = 0.0f;
    if (show_gbuffer_thumbnails)
      preview_sections_height += 152.0f;
    if (show_ibl_thumbnails)
      preview_sections_height += 276.0f;

    const float spacing = (show_gbuffer_thumbnails || show_ibl_thumbnails)
      ? ImGui::GetStyle().ItemSpacing.y
      : 0.0f;

    const ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
    const float main_view_height = preview_sections_height > 0.0f
      ? (viewport_panel_size.y - preview_sections_height - spacing)
      : viewport_panel_size.y;
    const float safe_view_width = viewport_panel_size.x > 1.0f ? viewport_panel_size.x : 1.0f;
    const float safe_view_height = main_view_height > 1.0f ? main_view_height : 1.0f;

    const int target_width = static_cast<int>(safe_view_width);
    const int target_height = static_cast<int>(safe_view_height);
    const glm::ivec2 current_size = mEngine->get_render_target_size();
    if (target_width != current_size.x || target_height != current_size.y)
      mEngine->resize(target_width, target_height);

    mEngine->render();

    const glm::ivec2 viewport_size = mEngine->get_render_target_size();
    ImVec2 display_size(
      static_cast<float>(std::max(viewport_size.x, 1)),
      static_cast<float>(std::max(viewport_size.y, 1)));
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1(p0.x + display_size.x, p0.y + display_size.y);
    ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, IM_COL32(24, 24, 24, 255));

    const uint32_t tex_id = mEngine->get_output_texture();
    if (tex_id != 0)
    {
      ImGui::Image((void*)(intptr_t)tex_id, display_size, ImVec2(0, 1), ImVec2(1, 0));
    }
    else
    {
      ImGui::Dummy(display_size);

      const std::string placeholder_text = scene_renderer_available
        ? std::string("FBO texture invalid")
        : std::string(backend_name) + " device path is active. Scene renderer is not implemented yet.";
      const ImU32 placeholder_color = scene_renderer_available
        ? IM_COL32(255, 255, 0, 255)
        : IM_COL32(242, 191, 91, 255);
      ImGui::GetWindowDrawList()->AddText(
        ImVec2(p0.x + 14.0f, p0.y + 14.0f),
        placeholder_color,
        placeholder_text.c_str());
    }

    ImGui::GetWindowDrawList()->AddRect(
      p0,
      p1,
      IM_COL32(70, 70, 70, 255),
      0.0f,
      0,
      1.0f);

    if (show_gbuffer_thumbnails)
    {
      ImGui::Dummy(ImVec2(0.0f, 6.0f));
      ImGui::Separator();
      ImGui::TextUnformatted("G-Buffer Inspect");

      const float available_width = ImGui::GetContentRegionAvail().x;
      const float thumb_spacing = ImGui::GetStyle().ItemSpacing.x;
      const bool stack_thumbnails = available_width < 520.0f;
      const float thumb_width = stack_thumbnails
        ? available_width
        : (available_width - thumb_spacing * 2.0f) / 3.0f;
      const ImVec2 thumb_size(thumb_width > 1.0f ? thumb_width : 1.0f, 80.0f);

      auto draw_debug_button = [&](const char* label, nengine::RenderEngine::DebugView view)
      {
        const bool active = mEngine->get_debug_view() == view;
        if (active)
        {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.55f, 0.20f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.62f, 0.24f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.72f, 0.48f, 0.18f, 1.0f));
        }

        if (ImGui::SmallButton(label))
          mEngine->set_debug_view(view);

        if (active)
          ImGui::PopStyleColor(3);
      };

      auto draw_thumb = [&](const char* label,
                            const char* channels,
                            const char* tooltip,
                            uint32_t texture_id,
                            nengine::RenderEngine::DebugView primary_view,
                            const char* secondary_label = nullptr,
                            nengine::RenderEngine::DebugView secondary_view = nengine::RenderEngine::DebugView::Final)
      {
        const auto current_debug_view = mEngine->get_debug_view();
        const bool selected = current_debug_view == primary_view ||
          (secondary_label != nullptr && current_debug_view == secondary_view);

        ImGui::BeginGroup();
        ImGui::TextUnformatted(label);
        ImGui::TextDisabled("%s", channels);

        if (texture_id != 0)
          ImGui::Image((void*)(intptr_t)texture_id, thumb_size, ImVec2(0, 1), ImVec2(1, 0));
        else
          ImGui::Dummy(thumb_size);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
          mEngine->set_debug_view(primary_view);

        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(tooltip);
          ImGui::EndTooltip();
        }

        const ImVec2 thumb_min = ImGui::GetItemRectMin();
        const ImVec2 thumb_max = ImGui::GetItemRectMax();
        const ImU32 border_color = selected
          ? IM_COL32(236, 166, 51, 255)
          : IM_COL32(80, 80, 80, 255);
        ImGui::GetWindowDrawList()->AddRect(
          thumb_min,
          thumb_max,
          border_color,
          0.0f,
          0,
          selected ? 2.0f : 1.0f);

        draw_debug_button(label, primary_view);
        if (secondary_label != nullptr)
        {
          ImGui::SameLine();
          draw_debug_button(secondary_label, secondary_view);
        }

        ImGui::EndGroup();
      };

      draw_thumb(
        "Position",
        "rgb = world position",
        "Click preview to inspect world-space position.",
        mEngine->get_gbuffer_position_texture(),
        nengine::RenderEngine::DebugView::Position);
      if (!stack_thumbnails)
        ImGui::SameLine();

      draw_thumb(
        "Normal",
        "rgb = normal, a = roughness",
        "Click preview for normal view. Use Roughness button to inspect alpha.",
        mEngine->get_gbuffer_normal_roughness_texture(),
        nengine::RenderEngine::DebugView::Normal,
        "Roughness",
        nengine::RenderEngine::DebugView::Roughness);
      if (!stack_thumbnails)
        ImGui::SameLine();

      draw_thumb(
        "Albedo",
        "rgb = albedo, a = metallic",
        "Click preview for albedo view. Use Metallic button to inspect alpha.",
        mEngine->get_gbuffer_albedo_metallic_texture(),
        nengine::RenderEngine::DebugView::Albedo,
        "Metallic",
        nengine::RenderEngine::DebugView::Metallic);
    }

    if (show_ibl_thumbnails)
    {
      ImGui::Dummy(ImVec2(0.0f, 6.0f));
      ImGui::Separator();
      ImGui::TextUnformatted("IBL Inspect");
      ImGui::TextDisabled("Cubemap previews use an equirectangular projection.");

      const float available_width = ImGui::GetContentRegionAvail().x;
      const float thumb_spacing = ImGui::GetStyle().ItemSpacing.x;
      const bool stack_previews = available_width < 520.0f;
      const float thumb_width = stack_previews
        ? available_width
        : (available_width - thumb_spacing) * 0.5f;
      const ImVec2 thumb_size(thumb_width > 1.0f ? thumb_width : 1.0f, 84.0f);

      auto draw_ibl_thumb = [&](const char* label,
                                const char* channels,
                                const char* tooltip,
                                uint32_t texture_id)
      {
        ImGui::BeginGroup();
        ImGui::TextUnformatted(label);
        ImGui::TextDisabled("%s", channels);

        if (texture_id != 0)
          ImGui::Image((void*)(intptr_t)texture_id, thumb_size, ImVec2(0, 1), ImVec2(1, 0));
        else
          ImGui::Dummy(thumb_size);

        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(tooltip);
          ImGui::EndTooltip();
        }

        const ImVec2 thumb_min = ImGui::GetItemRectMin();
        const ImVec2 thumb_max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(
          thumb_min,
          thumb_max,
          IM_COL32(80, 80, 80, 255),
          0.0f,
          0,
          1.0f);

        ImGui::EndGroup();
      };

      draw_ibl_thumb(
        "Environment",
        "cubemap -> preview",
        "Environment cubemap preview used for skybox and specular IBL.",
        mEngine->get_ibl_environment_preview_texture());
      if (!stack_previews)
        ImGui::SameLine();

      draw_ibl_thumb(
        "Irradiance",
        "diffuse IBL",
        "Low-frequency irradiance cubemap used for diffuse ambient lighting.",
        mEngine->get_ibl_irradiance_preview_texture());

      draw_ibl_thumb(
        "Prefilter",
        "specular IBL",
        "Prefilter cubemap preview sampled at an intermediate roughness level.",
        mEngine->get_ibl_prefilter_preview_texture());
      if (!stack_previews)
        ImGui::SameLine();

      draw_ibl_thumb(
        "BRDF LUT",
        "rg = integration",
        "2D BRDF lookup texture used by the specular IBL term.",
        mEngine->get_ibl_brdf_lut_preview_texture());
    }

    ImGui::End();
  }
}
