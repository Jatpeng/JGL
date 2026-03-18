#pragma once

#include "pch.h"
#include "shader/shader_util.h"

namespace nrender
{
  class PostProcessStack
  {
  public:
    PostProcessStack();
    ~PostProcessStack();

    bool init();
    void render(uint32_t hdr_texture_id, int32_t width, int32_t height);

  private:
    std::unique_ptr<nshaders::Shader> mShader;
    uint32_t mQuadVAO = 0;
    uint32_t mQuadVBO = 0;
  };
}
