#pragma once

#include "engine/render_engine.h"

namespace nui
{
  class SceneView
  {
  public:
    explicit SceneView(std::shared_ptr<nengine::RenderEngine> engine);

    void render();

  private:
    std::shared_ptr<nengine::RenderEngine> mEngine;
    bool mShowGBufferPreviews = true;
  };
}
