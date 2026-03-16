#pragma once

#include "engine/engine.h"

class Application
{
public:
  Application(const std::string& app_name);

  static Application& Instance() { return *sInstance; }

  void loop();
  nengine::Engine* get_engine() const;

private:
  static Application* sInstance;

  std::unique_ptr<nengine::Engine> mEngine;
};
