#pragma once

#include <memory>

#include "render/ui_context.h"
#include "ui/property_panel.h"
#include "ui/scene_view.h"
#include "window/window_overlay.h"

namespace neditor
{
  class EditorOverlay : public nwindow::IWindowOverlay
  {
  public:
    void on_attach(nwindow::IWindow& window) override;
    void on_detach() override;
    void begin_frame() override;
    void render(nengine::RenderEngine& engine) override;
    void end_frame() override;

  private:
    void render_dockspace();

  private:
    std::unique_ptr<nrender::UIContext> mUiContext;
    nui::Property_Panel mPropertyPanel;
    nui::SceneView mSceneView;
    bool mLayoutInitialized = false;
  };
}
