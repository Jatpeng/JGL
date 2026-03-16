#include "pch.h"

#include "deferred_gbuffer.h"

namespace nrender
{
  namespace
  {
    void setup_color_texture(uint32_t texture_id, GLenum internal_format, GLenum format, int32_t width, int32_t height)
    {
      glBindTexture(GL_TEXTURE_2D, texture_id);
      glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void setup_float_texture(uint32_t texture_id, GLenum internal_format, GLenum format, int32_t width, int32_t height)
    {
      glBindTexture(GL_TEXTURE_2D, texture_id);
      glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_FLOAT, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  }

  DeferredGBuffer::~DeferredGBuffer()
  {
    destroy();
  }

  bool DeferredGBuffer::init(int32_t width, int32_t height)
  {
    mWidth = width;
    mHeight = height;

    destroy();

    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

    const bool created = create_textures();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return created;
  }

  void DeferredGBuffer::resize(int32_t width, int32_t height)
  {
    if (width <= 0 || height <= 0)
      return;

    if (width == mWidth && height == mHeight && mFBO != 0)
      return;

    init(width, height);
  }

  void DeferredGBuffer::destroy()
  {
    if (mDepthTexture != 0)
      glDeleteTextures(1, &mDepthTexture);
    if (mAlbedoMetallicTexture != 0)
      glDeleteTextures(1, &mAlbedoMetallicTexture);
    if (mNormalRoughnessTexture != 0)
      glDeleteTextures(1, &mNormalRoughnessTexture);
    if (mPositionTexture != 0)
      glDeleteTextures(1, &mPositionTexture);
    if (mFBO != 0)
      glDeleteFramebuffers(1, &mFBO);

    mDepthTexture = 0;
    mAlbedoMetallicTexture = 0;
    mNormalRoughnessTexture = 0;
    mPositionTexture = 0;
    mFBO = 0;
  }

  bool DeferredGBuffer::create_textures()
  {
    // Position and normal attachments keep float precision; albedo/metallic stays compact in RGBA8.
    glGenTextures(1, &mPositionTexture);
    setup_float_texture(mPositionTexture, GL_RGBA16F, GL_RGBA, mWidth, mHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPositionTexture, 0);

    glGenTextures(1, &mNormalRoughnessTexture);
    setup_float_texture(mNormalRoughnessTexture, GL_RGBA16F, GL_RGBA, mWidth, mHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mNormalRoughnessTexture, 0);

    glGenTextures(1, &mAlbedoMetallicTexture);
    setup_color_texture(mAlbedoMetallicTexture, GL_RGBA8, GL_RGBA, mWidth, mHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mAlbedoMetallicTexture, 0);

    glGenTextures(1, &mDepthTexture);
    glBindTexture(GL_TEXTURE_2D, mDepthTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, mWidth, mHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mDepthTexture, 0);

    const GLenum attachments[3] = {
      GL_COLOR_ATTACHMENT0,
      GL_COLOR_ATTACHMENT1,
      GL_COLOR_ATTACHMENT2
    };
    glDrawBuffers(3, attachments);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cerr << "Deferred GBuffer incomplete: 0x" << std::hex << status << std::dec << std::endl;
      return false;
    }

    return true;
  }

  void DeferredGBuffer::bind_for_geometry_pass()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glViewport(0, 0, mWidth, mHeight);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void DeferredGBuffer::bind_textures(GLenum position_unit) const
  {
    glActiveTexture(position_unit + 0);
    glBindTexture(GL_TEXTURE_2D, mPositionTexture);

    glActiveTexture(position_unit + 1);
    glBindTexture(GL_TEXTURE_2D, mNormalRoughnessTexture);

    glActiveTexture(position_unit + 2);
    glBindTexture(GL_TEXTURE_2D, mAlbedoMetallicTexture);
  }

  void DeferredGBuffer::copy_depth_to(uint32_t target_fbo) const
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target_fbo);
    glBlitFramebuffer(0, 0, mWidth, mHeight,
                      0, 0, mWidth, mHeight,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, target_fbo);
    glViewport(0, 0, mWidth, mHeight);
  }

  void DeferredGBuffer::unbind() const
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}
