#pragma once

#include "window/jgl_window.h"

namespace nengine
{
  class RenderEngine;
}

class Application
{
public:
  Application(const std::string& app_name);

  static Application& Instance() { return *sInstance; }

  void loop();
  nengine::RenderEngine* get_engine() const;

private:
  static Application* sInstance;

  std::unique_ptr<nwindow::GLWindow> mWindow;
};
