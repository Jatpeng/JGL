#pragma once

#include <cstdint>

#include "engine/render_engine.h"

#include "imgui.h"

namespace nui
{
  class Property_Panel
  {
  public:
    Property_Panel();

    void render(nengine::RenderEngine* engine);

  private:
    uint64_t mSelectedObjectId = 0;
    std::string mCurrentMeshFile;
    std::string mCurrentShaderFile;
    std::string mCurrentMaterialFile;
    std::string mShaderReloadStatus;
  };
}
