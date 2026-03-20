#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/scene.h"

namespace nwindow
{
  class GLWindow;
  class IWindowOverlay;
}

namespace nengine
{
  class RenderEngine;
  class ResourceManager;

  /**
   * @brief 游戏引擎的核心管理类。
   *
   * 负责引擎的基础架构和主循环。管理所有子系统（Subsystems）的生命周期，
   * 比如 RenderEngine, ResourceManager，并持有应用的主窗口（Window）。
   * 同时也管理场景（Scene）的切换与更新。
   */
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
      CreateInfo() {}
    };


    explicit Engine(const CreateInfo& create_info = CreateInfo{});

    ~Engine();

    bool init();
    void run();
    void tick();
    bool reload_shaders();

    std::shared_ptr<Scene> create_scene(const std::string& name);
    void set_active_scene(std::shared_ptr<Scene> scene);
    std::shared_ptr<Scene> active_scene() const { return mActiveScene; }
    void set_window_overlay(std::shared_ptr<nwindow::IWindowOverlay> overlay);

    RenderEngine* render_engine() const;
    bool is_initialized() const { return mInitialized; }

  private:
    void create_default_scene_if_needed();

  private:
    CreateInfo mCreateInfo;
    bool mInitialized = false;
    std::shared_ptr<ResourceManager> mResources;
    std::unique_ptr<nwindow::GLWindow> mWindow;
    std::shared_ptr<nwindow::IWindowOverlay> mWindowOverlay;
    std::vector<std::shared_ptr<Scene>> mScenes;
    std::shared_ptr<Scene> mActiveScene;
  };
}
