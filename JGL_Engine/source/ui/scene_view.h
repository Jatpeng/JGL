#pragma once

#include "engine/render_engine.h"
#include "engine/scene.h"

namespace nui
{
  class SceneView
  {
  public:
    SceneView() = default;
    explicit SceneView(std::shared_ptr<nengine::RenderEngine> engine);
    explicit SceneView(nengine::RenderEngine* engine);

    void render();
    void set_engine(std::shared_ptr<nengine::RenderEngine> engine);
    void set_engine(nengine::RenderEngine* engine);
    void set_scene(std::shared_ptr<nengine::Scene> scene);
    std::shared_ptr<nengine::Scene> get_scene() const;

  private:
    nengine::RenderEngine* mEngine = nullptr;
    bool mShowGBufferPreviews = true;
  };
}
