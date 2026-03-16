#include "pch.h"

#include "application.h"

Application* Application::sInstance = nullptr;

Application::Application(const std::string& app_name)
{
  sInstance = this;
  nengine::Engine::CreateInfo create_info;
  create_info.title = app_name;
  mEngine = std::make_unique<nengine::Engine>(create_info);
  mEngine->init();
}

void Application::loop()
{
  if (mEngine)
    mEngine->run();
}

nengine::Engine* Application::get_engine() const
{
  return mEngine.get();
}
