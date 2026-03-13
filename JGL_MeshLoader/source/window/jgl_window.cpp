#include "pch.h"

#include "jgl_window.h"
#include "elems/input.h"

namespace nwindow
{
  bool GLWindow::init(int width, int height, const std::string& title)
  {
    Width = width;
    Height = height;
    Title = title;

    mRenderCtx->init(this);

    mUICtx->init(this);

    nengine::RenderEngine::CreateInfo engine_info;
    engine_info.render_target_size = { width, height };

    mEngine = std::make_shared<nengine::RenderEngine>(engine_info);
    mSceneView = std::make_unique<nui::SceneView>(mEngine);
    mPropertyPanel = std::make_unique<nui::Property_Panel>();

    return mIsRunning;
  }

  GLWindow::~GLWindow()
  {
    mUICtx->end();

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
    if (action == GLFW_PRESS)
    {
    }
  }

  void GLWindow::on_close()
  {
    mIsRunning = false;
  }

  void GLWindow::render()
  {
    // Clear the view
    mRenderCtx->pre_render();

    // Initialize UI components
    mUICtx->pre_render();

    if (mSceneView)
      mSceneView->render();
    if (mPropertyPanel)
      mPropertyPanel->render(mEngine.get());

    // Render the UI 
    mUICtx->post_render();

    // Render end, swap buffers
    mRenderCtx->post_render();

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
}
