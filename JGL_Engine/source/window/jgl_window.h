#pragma once

#include "engine/render_engine.h"
#include "engine/scene.h"
#include "render/opengl_context.h"
#include "window/window.h"
#include "window/window_overlay.h"

namespace nrender
{
  class RenderDocCapture;
}

namespace nwindow
{
  class GLWindow : public IWindow
  {
  public:

    GLWindow() :
      mIsRunning(true), mWindow(nullptr)
    {
      mRenderCtx = std::make_unique<nrender::OpenGL_Context>();
    }

    ~GLWindow();

    bool init(
      int width,
      int height,
      const std::string& title,
      const nengine::RenderEngine::CreateInfo& engine_info = nengine::RenderEngine::CreateInfo());

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
    void set_overlay(std::shared_ptr<IWindowOverlay> overlay);
    void set_scene(std::shared_ptr<nengine::Scene> scene);
    std::shared_ptr<nengine::Scene> get_scene() const;

  private:
    GLFWwindow* mWindow;

    std::unique_ptr<nrender::OpenGL_Context> mRenderCtx;
    std::shared_ptr<nrender::RenderDocCapture> mRenderDocCapture;
    std::shared_ptr<nengine::RenderEngine> mEngine;
    std::shared_ptr<IWindowOverlay> mOverlay;

    bool mIsRunning;
  };
}


