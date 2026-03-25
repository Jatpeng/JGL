#include "pch.h"

#include "engine/engine.h"

#include "engine/render_engine.h"
#include "engine/resource_manager.h"
#include "engine/scene_loader.h"
#include "render/device/render_device.h"
#include "window/jgl_window.h"
#include "window/window_overlay.h"

namespace nengine
{
  Engine::Engine(const CreateInfo& create_info)
    : mCreateInfo(create_info), mResources(std::make_shared<ResourceManager>())
  {
  }

  Engine::~Engine()
  {
    if (mWindow)
      mWindow->set_overlay(nullptr);

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

    const nrender::GraphicsBackend resolved_backend =
      nrender::resolve_graphics_backend_from_env(mCreateInfo.render_backend);
    if (!nrender::RenderDeviceManager::instance().initialize(resolved_backend))
      return false;

    mWindow = std::make_unique<nwindow::GLWindow>();

    RenderEngine::CreateInfo render_info;
    render_info.render_target_size = { mCreateInfo.width, mCreateInfo.height };
    render_info.resource_manager = mResources;

    if (!mWindow->init(mCreateInfo.width, mCreateInfo.height, mCreateInfo.title, render_info))
    {
      mWindow.reset();
      return false;
    }

    mWindow->set_overlay(mWindowOverlay);

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

  bool Engine::reload_shaders()
  {
    if (!init())
      return false;

    auto* renderer = render_engine();
    return renderer ? renderer->reload_runtime_shaders() : false;
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

  void Engine::set_window_overlay(std::shared_ptr<nwindow::IWindowOverlay> overlay)
  {
    mWindowOverlay = std::move(overlay);
    if (mWindow)
      mWindow->set_overlay(mWindowOverlay);
  }

  RenderEngine* Engine::render_engine() const
  {
    return mWindow ? mWindow->get_engine() : nullptr;
  }

  void Engine::create_default_scene_if_needed()
  {
    if (auto* renderer = render_engine(); renderer && !renderer->is_scene_renderer_available())
    {
      std::cout
        << "[Engine] Backend " << renderer->graphics_backend_name()
        << " uses an empty default scene until the runtime renderer is implemented."
        << std::endl;
      renderer->set_plane_show(false);
      set_active_scene(create_scene("default"));
      return;
    }

    SceneResourceDefinition scene_definition;
    std::string scene_error;
    if (load_scene_resource("Assets/scenes/default_scene.xml", mResources, &scene_definition, &scene_error) &&
        scene_definition.scene)
    {
      mScenes.push_back(scene_definition.scene);

      if (auto* renderer = render_engine())
      {
        renderer->set_plane_show(scene_definition.show_plane);
        if (!scene_definition.environment_map_path.empty())
          renderer->load_environment_map(scene_definition.environment_map_path);
      }

      set_active_scene(scene_definition.scene);
      return;
    }

    if (!scene_error.empty())
      std::cout << "[Engine] Failed to load default scene resource: " << scene_error << std::endl;

    auto scene = create_scene("default");

    auto mesh = scene->create_mesh("cube");
    mesh->get_component<MeshComponent>()->set_model("Assets/models/cube.fbx");
    mesh->get_component<MeshComponent>()->set_material("Assets/materials/PBR.xml");

    auto light = scene->create_light("main_light");
    light->get_component<TransformComponent>()->position = glm::vec3(1.5f, 3.5f, 3.0f);
    light->get_component<LightComponent>()->set_type(LightComponent::LightType::Directional);
    light->get_component<LightComponent>()->set_color(glm::vec3(1.0f, 1.0f, 1.0f));
    light->get_component<LightComponent>()->set_strength(3.5f);
    light->get_component<LightComponent>()->set_direction(glm::vec3(-0.35f, -1.0f, -0.25f));
    light->get_component<LightComponent>()->set_casts_shadows(true);

    if (auto* renderer = render_engine())
      renderer->set_plane_show(true);

    set_active_scene(scene);
  }
}
