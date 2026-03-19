#pragma once

#include "pch.h"
#include "shader/shader_util.h"

namespace nrender
{
  class IBLPipeline
  {
  public:
    IBLPipeline();
    ~IBLPipeline();

    bool init();
    bool build_from_cubemap(uint32_t cubemap_texture);
    bool load_environment_map(const std::string& filepath);

    uint32_t get_environment_cubemap() const { return mEnvironmentCubemap; }
    uint32_t get_irradiance_map() const { return mIrradianceMap; }
    uint32_t get_prefilter_map() const { return mPrefilterMap; }
    uint32_t get_brdf_lut() const { return mBrdfLUTTexture; }

  private:
    void destroy_generated_textures();
    bool generate_ibl_maps();
    void setup_cube();
    void setup_quad();
    void render_cube();
    void render_quad();

    uint32_t mEnvironmentCubemap = 0;
    uint32_t mIrradianceMap = 0;
    uint32_t mPrefilterMap = 0;
    uint32_t mBrdfLUTTexture = 0;
    bool mOwnsEnvironmentCubemap = false;

    uint32_t mCaptureFBO = 0;
    uint32_t mCaptureRBO = 0;

    uint32_t mCubeVAO = 0;
    uint32_t mCubeVBO = 0;
    uint32_t mQuadVAO = 0;
    uint32_t mQuadVBO = 0;

    std::unique_ptr<nshaders::Shader> mEquirectangularToCubemapShader;
    std::unique_ptr<nshaders::Shader> mIrradianceShader;
    std::unique_ptr<nshaders::Shader> mPrefilterShader;
    std::unique_ptr<nshaders::Shader> mBrdfShader;
  };
}
