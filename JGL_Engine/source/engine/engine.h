#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/scene.h"

namespace nwindow
{
  class GLWindow;
}

namespace nengine
{
  class RenderEngine;
  class ResourceManager;

  class Engine
  {
  public:
    struct CreateInfo
    {
      int width = 1024;
      int height = 720;
      std::string title = "JGL_Engine";
      bool create_default_scene = true;
      bool show_plane = false;
    };

    explicit Engine(const CreateInfo& create_info = CreateInfo());
    ~Engine();

    bool init();
    void run();
    void tick();

    std::shared_ptr<Scene> create_scene(const std::string& name);
    void set_active_scene(std::shared_ptr<Scene> scene);
    std::shared_ptr<Scene> active_scene() const { return mActiveScene; }

    RenderEngine* render_engine() const;
    bool is_initialized() const { return mInitialized; }

  private:
    void create_default_scene_if_needed();

  private:
    CreateInfo mCreateInfo;
    bool mInitialized = false;
    std::shared_ptr<ResourceManager> mResources;
    std::unique_ptr<nwindow::GLWindow> mWindow;
    std::vector<std::shared_ptr<Scene>> mScenes;
    std::shared_ptr<Scene> mActiveScene;
  };
}
