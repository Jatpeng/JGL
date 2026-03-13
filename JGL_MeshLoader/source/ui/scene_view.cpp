#include "pch.h"

#include "scene_view.h"

#include "imgui.h"

namespace nui
{
  SceneView::SceneView(std::shared_ptr<nengine::RenderEngine> engine)
    : mEngine(std::move(engine))
  {
  }

  void SceneView::render()
  {
    if (!mEngine)
      return;

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene");

    const bool deferred_available = mEngine->is_deferred_available();
    const bool deferred_requested = mEngine->get_render_mode() == nengine::RenderEngine::RenderMode::Deferred;
    const bool show_gbuffer_thumbnails =
      deferred_requested &&
      deferred_available &&
      mShowGBufferPreviews;

    if (ImGui::Button("Reset View"))
      mEngine->reset_view();

    ImGui::SameLine();
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

    if (mEngine->get_render_mode() == nengine::RenderEngine::RenderMode::Deferred)
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

    const glm::ivec2 target_size = mEngine->get_render_target_size();
    const char* active_mode = deferred_requested && deferred_available ? "Deferred" : "Forward";
    ImGui::TextDisabled("Target %d x %d | Active %s", target_size.x, target_size.y, active_mode);

    if (deferred_requested && !deferred_available)
    {
      ImGui::TextColored(
        ImVec4(0.95f, 0.75f, 0.25f, 1.0f),
        "Current material is missing deferred inputs. Rendering falls back to forward mode.");
    }

    ImGui::Separator();

    const float thumbnails_height = show_gbuffer_thumbnails ? 152.0f : 0.0f;
    const float spacing = show_gbuffer_thumbnails ? ImGui::GetStyle().ItemSpacing.y : 0.0f;

    const ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
    const float main_view_height = show_gbuffer_thumbnails
      ? (viewport_panel_size.y - thumbnails_height - spacing)
      : viewport_panel_size.y;
    const float safe_view_width = viewport_panel_size.x > 1.0f ? viewport_panel_size.x : 1.0f;
    const float safe_view_height = main_view_height > 1.0f ? main_view_height : 1.0f;

    const int target_width = static_cast<int>(safe_view_width);
    const int target_height = static_cast<int>(safe_view_height);
    const glm::ivec2 current_size = mEngine->get_render_target_size();
    // Keep the off-screen render targets in lock-step with the ImGui viewport.
    if (target_width != current_size.x || target_height != current_size.y)
      mEngine->resize(target_width, target_height);

    mEngine->render();

    ImVec2 display_size(safe_view_width, safe_view_height);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1(p0.x + display_size.x, p0.y + display_size.y);
    // Use a neutral backdrop so alpha in the viewport texture does not tint the scene.
    ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, IM_COL32(24, 24, 24, 255));

    const uint32_t tex_id = mEngine->get_output_texture();
    if (tex_id != 0)
      ImGui::Image((void*)(intptr_t)tex_id, display_size, ImVec2(0, 1), ImVec2(1, 0));
    else
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "FBO texture invalid");

    ImGui::GetWindowDrawList()->AddRect(
      ImGui::GetItemRectMin(),
      ImGui::GetItemRectMax(),
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

    ImGui::End();
  }
}
