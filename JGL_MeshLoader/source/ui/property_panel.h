#pragma once

#include "engine/render_engine.h"

#include "imgui.h"
#include "utils/imgui_widgets.h"

namespace nui
{
  class Property_Panel
  {
  public:

    Property_Panel()
    {
        mCurrentShaderFile = "pbr_fs.shader";
        mCurrentMaterialFile = "PBR.mtl";
        mCurrentMeshFile = "cube.fbx";
    }

    void render(nengine::RenderEngine* engine);

  private:
    std::string mCurrentMeshFile;
    std::string mCurrentShaderFile;
    std::string mCurrentMaterialFile;
  };
}

