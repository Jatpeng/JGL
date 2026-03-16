#include "pch.h"

#include "engine/engine.h"

#include "engine/render_engine.h"
#include "engine/resource_manager.h"
#include "window/jgl_window.h"

namespace nengine
{
  Engine::Engine(const CreateInfo& create_info)
    : mCreateInfo(create_info), mResources(std::make_shared<ResourceManager>())
  {
  }

  Engine::~Engine()
  {
    if (mWindow)
      mWindow->set_scene(nullptr);

    mActiveScene.reset();
    mScenes.clear();
    mWindow.reset();
  }

  bool Engine::init()
  {
    if (mInitialized)
      return true;

    mWindow = std::make_unique<nwindow::GLWindow>();

    RenderEngine::CreateInfo render_info;
    render_info.render_target_size = { mCreateInfo.width, mCreateInfo.height };
    render_info.resource_manager = mResources;

    if (!mWindow->init(mCreateInfo.width, mCreateInfo.height, mCreateInfo.title, render_info))
    {
      mWindow.reset();
      return false;
    }

    if (auto* renderer = render_engine())
      renderer->set_plane_show(mCreateInfo.show_plane);

    if (!mActiveScene && mCreateInfo.create_default_scene)
      create_default_scene_if_needed();
    else if (mActiveScene)
      mWindow->set_scene(mActiveScene);

    mInitialized = true;
    return true;
  }

  void Engine::run()
  {
    if (!init())
      return;

    while (mWindow && mWindow->is_running())
      mWindow->render();
  }

  void Engine::tick()
  {
    if (!init())
      return;

    if (mWindow && mWindow->is_running())
      mWindow->render();
  }

  std::shared_ptr<Scene> Engine::create_scene(const std::string& name)
  {
    auto scene = std::make_shared<Scene>(name, mResources);
    mScenes.push_back(scene);
    return scene;
  }

  void Engine::set_active_scene(std::shared_ptr<Scene> scene)
  {
    mActiveScene = std::move(scene);
    if (mWindow)
      mWindow->set_scene(mActiveScene);
  }

  RenderEngine* Engine::render_engine() const
  {
    return mWindow ? mWindow->get_engine() : nullptr;
  }

  void Engine::create_default_scene_if_needed()
  {
    auto scene = create_scene("default");

    auto mesh = scene->create_mesh("cube");
    mesh->set_model("Assets/cube.fbx");
    mesh->set_material("Assets/PBR.xml");

    auto light = scene->create_light("main_light");
    light->transform().position = glm::vec3(1.5f, 3.5f, 3.0f);
    light->set_color(glm::vec3(1.0f, 1.0f, 1.0f));
    light->set_strength(100.0f);

    set_active_scene(scene);
  }
}
