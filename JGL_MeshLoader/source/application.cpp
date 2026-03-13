#include "pch.h"

#include "application.h"

#include "window/jgl_window.h"

Application* Application::sInstance = nullptr;

Application::Application(const std::string& app_name)
{
  sInstance = this;
  mWindow = std::make_unique<nwindow::GLWindow>();
  mWindow->init(1024, 720, app_name);
}

void Application::loop()
{
  while (mWindow->is_running())
  {
    mWindow->render();
  }
}

nengine::RenderEngine* Application::get_engine() const
{
  return mWindow ? mWindow->get_engine() : nullptr;
}
