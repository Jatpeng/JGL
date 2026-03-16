#pragma once

#include "engine/render_engine.h"
#include "engine/scene.h"

namespace nui
{
  class SceneView
  {
  public:
    explicit SceneView(std::shared_ptr<nengine::RenderEngine> engine);

    void render();
    void set_scene(std::shared_ptr<nengine::Scene> scene);
    std::shared_ptr<nengine::Scene> get_scene() const;

  private:
    std::shared_ptr<nengine::RenderEngine> mEngine;
    bool mShowGBufferPreviews = true;
  };
}
