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
    uint32_t get_environment_preview_texture() const { return mEnvironmentPreviewTexture; }
    uint32_t get_irradiance_preview_texture() const { return mIrradiancePreviewTexture; }
    uint32_t get_prefilter_preview_texture() const { return mPrefilterPreviewTexture; }
    uint32_t get_brdf_lut_preview_texture() const { return mBrdfLUTTexture; }

  private:
    void destroy_generated_textures();
    void destroy_preview_textures();
    bool generate_ibl_maps();
    bool generate_preview_textures();
    bool create_preview_texture(uint32_t* out_texture, uint32_t width, uint32_t height);
    void render_cubemap_preview(uint32_t cubemap_texture, uint32_t target_texture, uint32_t width, uint32_t height, float lod);
    void setup_cube();
    void setup_quad();
    void render_cube();
    void render_quad();

    uint32_t mEnvironmentCubemap = 0;
    uint32_t mIrradianceMap = 0;
    uint32_t mPrefilterMap = 0;
    uint32_t mBrdfLUTTexture = 0;
    uint32_t mEnvironmentPreviewTexture = 0;
    uint32_t mIrradiancePreviewTexture = 0;
    uint32_t mPrefilterPreviewTexture = 0;
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
    std::unique_ptr<nshaders::Shader> mCubemapPreviewShader;
  };
}
