#include "pch.h"

#include "jgl_window.h"
#include "elems/input.h"
#include "render/opengl_context.h"
#include "render/renderdoc_capture.h"

#ifdef _WIN32
#  ifndef GLFW_EXPOSE_NATIVE_WIN32
#    define GLFW_EXPOSE_NATIVE_WIN32
#  endif
#  include <GLFW/glfw3native.h>
#endif

namespace nwindow
{
  namespace
  {
    void* get_renderdoc_window_handle(GLFWwindow* window)
    {
#ifdef _WIN32
      return window != nullptr ? glfwGetWin32Window(window) : nullptr;
#else
      (void)window;
      return nullptr;
#endif
    }
  }

  bool GLWindow::init(
    int width,
    int height,
    const std::string& title,
    const nengine::RenderEngine::CreateInfo& engine_info)
  {
    Width = width;
    Height = height;
    Title = title;

    mRenderDocCapture = std::make_shared<nrender::RenderDocCapture>();
    mRenderCtx->init(this);

    auto resolved_engine_info = engine_info;
    resolved_engine_info.render_target_size = { width, height };
    resolved_engine_info.renderdoc_capture = mRenderDocCapture;

    mEngine = std::make_shared<nengine::RenderEngine>(resolved_engine_info);

    if (mOverlay)
      mOverlay->on_attach(*this);

    return mIsRunning;
  }

  GLWindow::~GLWindow()
  {
    set_overlay(nullptr);
    mEngine.reset();
    mRenderCtx->end();
  }

  void GLWindow::on_resize(int width, int height)
  {
    Width = width;
    Height = height;
    render();
  }

  void GLWindow::on_scroll(double delta)
  {
    if (mEngine)
      mEngine->on_mouse_wheel(delta);
  }

  void GLWindow::on_key(int key, int scancode, int action, int mods)
  {
    (void)scancode;
    (void)mods;

    if (action == GLFW_PRESS)
    {
      if (key == GLFW_KEY_F12 && mEngine)
        mEngine->request_frame_capture();
    }
  }

  void GLWindow::on_close()
  {
    mIsRunning = false;
  }

  void GLWindow::render()
  {
    glfwPollEvents();
    const void* renderdoc_window_handle = get_renderdoc_window_handle(mWindow);
    const bool capture_started = mEngine && mEngine->begin_frame_capture(const_cast<void*>(renderdoc_window_handle));

    // Clear the view
    mRenderCtx->pre_render();

    if (mOverlay)
    {
      mOverlay->begin_frame();
      if (mEngine)
        mOverlay->render(*mEngine);
      mOverlay->end_frame();
    }

    // Render end, swap buffers
    mRenderCtx->post_render();

    if (capture_started && mEngine)
      mEngine->end_frame_capture(const_cast<void*>(renderdoc_window_handle));

    handle_input();
  }

  void GLWindow::handle_input()
  {
    // TODO: move this and camera to scene UI component?

    if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
      if (mEngine)
        mEngine->on_mouse_wheel(-0.4f);
    }

    if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
      if (mEngine)
        mEngine->on_mouse_wheel(0.4f);
    }

    if (glfwGetKey(mWindow, GLFW_KEY_F) == GLFW_PRESS)
    {
      if (mEngine)
        mEngine->reset_view();
    }

    double x, y;
    glfwGetCursorPos(mWindow, &x, &y);

    if (mEngine)
      mEngine->on_mouse_move(x, y, nelems::Input::GetPressedButton(mWindow));
  }

  void GLWindow::set_overlay(std::shared_ptr<IWindowOverlay> overlay)
  {
    if (mOverlay == overlay)
      return;

    if (mOverlay && mWindow)
      mOverlay->on_detach();

    mOverlay = std::move(overlay);

    if (mOverlay && mWindow)
      mOverlay->on_attach(*this);
  }

  void GLWindow::set_scene(std::shared_ptr<nengine::Scene> scene)
  {
    if (mEngine)
      mEngine->set_scene(std::move(scene));
  }

  std::shared_ptr<nengine::Scene> GLWindow::get_scene() const
  {
    return mEngine ? mEngine->get_scene() : nullptr;
  }
}
