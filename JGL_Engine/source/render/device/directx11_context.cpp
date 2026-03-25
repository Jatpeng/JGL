#include "pch.h"

#ifdef _WIN32

#include "render/device/directx11_context.h"

#include "render/device/directx11_runtime.h"

namespace nrender
{
  namespace
  {
    void on_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
      auto* pWindow = static_cast<nwindow::IWindow*>(glfwGetWindowUserPointer(window));
      pWindow->on_key(key, scancode, action, mods);
    }

    void on_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
      (void)xoffset;
      auto* pWindow = static_cast<nwindow::IWindow*>(glfwGetWindowUserPointer(window));
      pWindow->on_scroll(yoffset);
    }

    void on_window_size_callback(GLFWwindow* window, int width, int height)
    {
      DirectX11Runtime::instance().resize(width, height);
      auto* pWindow = static_cast<nwindow::IWindow*>(glfwGetWindowUserPointer(window));
      pWindow->on_resize(width, height);
    }

    void on_window_close_callback(GLFWwindow* window)
    {
      auto* pWindow = static_cast<nwindow::IWindow*>(glfwGetWindowUserPointer(window));
      pWindow->on_close();
    }
  }

  bool DirectX11_Context::init(nwindow::IWindow* window)
  {
    RenderContext::init(window);

    if (!glfwInit())
    {
      fprintf(stderr, "Error: GLFW Window couldn't be initialized\n");
      return false;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto* dxWindow = glfwCreateWindow(window->Width, window->Height, window->Title.c_str(), nullptr, nullptr);
    window->set_native_window(dxWindow);

    if (!dxWindow)
    {
      fprintf(stderr, "Error: GLFW Window couldn't be created\n");
      glfwTerminate();
      return false;
    }

    glfwSetWindowUserPointer(dxWindow, window);
    glfwSetKeyCallback(dxWindow, on_key_callback);
    glfwSetScrollCallback(dxWindow, on_scroll_callback);
    glfwSetWindowSizeCallback(dxWindow, on_window_size_callback);
    glfwSetWindowCloseCallback(dxWindow, on_window_close_callback);

    if (!DirectX11Runtime::instance().initialize(dxWindow, window->Width, window->Height))
    {
      fprintf(stderr, "Error: DirectX11 runtime couldn't be initialized\n");
      glfwDestroyWindow(dxWindow);
      window->set_native_window(nullptr);
      glfwTerminate();
      return false;
    }

    return true;
  }

  void DirectX11_Context::pre_render()
  {
    DirectX11Runtime::instance().begin_frame(0.2f, 0.2f, 0.2f, 1.0f);
  }

  void DirectX11_Context::post_render()
  {
    DirectX11Runtime::instance().end_frame();
  }

  void DirectX11_Context::end()
  {
    DirectX11Runtime::instance().shutdown();
    glfwDestroyWindow(static_cast<GLFWwindow*>(mWindow->get_native_window()));
    glfwTerminate();
  }
}

#endif
