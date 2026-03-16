#pragma once

#include "pch.h"

namespace nrender
{
  class DeferredGBuffer
  {
  public:
    DeferredGBuffer() = default;
    ~DeferredGBuffer();

    bool init(int32_t width, int32_t height);

    void resize(int32_t width, int32_t height);

    void destroy();

    void bind_for_geometry_pass();

    void bind_textures(GLenum position_unit = GL_TEXTURE0) const;

    void copy_depth_to(uint32_t target_fbo) const;

    void unbind() const;

    uint32_t get_position_texture() const { return mPositionTexture; }
    uint32_t get_normal_roughness_texture() const { return mNormalRoughnessTexture; }
    uint32_t get_albedo_metallic_texture() const { return mAlbedoMetallicTexture; }
    uint32_t get_depth_texture() const { return mDepthTexture; }

  private:
    bool create_textures();

    uint32_t mFBO = 0;
    uint32_t mPositionTexture = 0;
    uint32_t mNormalRoughnessTexture = 0;
    uint32_t mAlbedoMetallicTexture = 0;
    uint32_t mDepthTexture = 0;
    int32_t mWidth = 0;
    int32_t mHeight = 0;
  };
}
