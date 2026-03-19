#include "pch.h"

#include "editor/editor_overlay.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "window/window.h"

namespace neditor
{
  void EditorOverlay::on_attach(nwindow::IWindow& window)
  {
    on_detach();

    mUiContext = std::make_unique<nrender::UIContext>();
    if (!mUiContext->init(&window))
      mUiContext.reset();

    mLayoutInitialized = false;
  }

  void EditorOverlay::on_detach()
  {
    mSceneView.set_engine(nullptr);
    mLayoutInitialized = false;

    if (!mUiContext)
      return;

    mUiContext->end();
    mUiContext.reset();
  }

  void EditorOverlay::begin_frame()
  {
    if (mUiContext)
      mUiContext->pre_render();
  }

  void EditorOverlay::render(nengine::RenderEngine& engine)
  {
    if (!mUiContext)
      return;

    mSceneView.set_engine(&engine);
    render_dockspace();
    mSceneView.render();
    mPropertyPanel.render(&engine);
  }

  void EditorOverlay::end_frame()
  {
    if (mUiContext)
      mUiContext->post_render();
  }

  void EditorOverlay::render_dockspace()
  {
    ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_NoBackground;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EditorDockspaceHost", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    const ImGuiID dock_space_id = ImGui::GetID("EditorDockspace");
    ImGui::DockSpace(dock_space_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (!mLayoutInitialized)
    {
      mLayoutInitialized = true;
      ImGui::DockBuilderRemoveNode(dock_space_id);
      const ImGuiID main_id = ImGui::DockBuilderAddNode(dock_space_id, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(main_id, viewport->Size);

      ImGuiID left_id = 0;
      ImGuiID right_id = 0;
      ImGui::DockBuilderSplitNode(main_id, ImGuiDir_Left, 0.22f, &left_id, &right_id);
      ImGui::DockBuilderDockWindow("Properties", left_id);
      ImGui::DockBuilderDockWindow("Scene", right_id);
      ImGui::DockBuilderFinish(dock_space_id);
    }

    ImGui::End();
  }
}
