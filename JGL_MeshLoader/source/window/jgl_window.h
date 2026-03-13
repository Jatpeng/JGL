#pragma once

#include "engine/render_engine.h"
#include "render/ui_context.h"
#include "render/opengl_context.h"
#include "window/window.h"
#include "ui/property_panel.h"
#include "ui/scene_view.h"

namespace nwindow
{
  class GLWindow : public IWindow
  {
  public:

    GLWindow() :
      mIsRunning(true), mWindow(nullptr)
    {
      mUICtx = std::make_unique<nrender::UIContext>();
      mRenderCtx = std::make_unique<nrender::OpenGL_Context>();
    }

    ~GLWindow();

    bool init(int width, int height, const std::string& title);

    void render();

    void handle_input();

    void* get_native_window() override { return mWindow; }

    void set_native_window(void* window)
    {
      mWindow = (GLFWwindow*)window;
    }

    void on_scroll(double delta) override;

    void on_key(int key, int scancode, int action, int mods) override;

    void on_resize(int width, int height) override;

    void on_close() override;

    bool is_running() { return mIsRunning; }

    nengine::RenderEngine* get_engine() const { return mEngine.get(); }

  private:
    GLFWwindow* mWindow;

    std::unique_ptr<nrender::UIContext> mUICtx;
    std::unique_ptr<nrender::OpenGL_Context> mRenderCtx;
    std::shared_ptr<nengine::RenderEngine> mEngine;
    std::unique_ptr<nui::Property_Panel> mPropertyPanel;
    std::unique_ptr<nui::SceneView> mSceneView;

    bool mIsRunning;
  };
}


