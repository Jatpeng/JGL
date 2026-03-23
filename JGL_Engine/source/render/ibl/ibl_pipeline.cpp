#include "pch.h"
#include "ibl_pipeline.h"
#include <stb_image.h>

namespace nrender
{
  namespace
  {
    constexpr uint32_t kEnvironmentSize = 512;
    constexpr uint32_t kIrradianceSize = 32;
    constexpr uint32_t kPrefilterSize = 128;
    constexpr uint32_t kBrdfLutSize = 512;
    constexpr uint32_t kPrefilterMipLevels = 5;
    constexpr uint32_t kCubemapPreviewWidth = 256;
    constexpr uint32_t kCubemapPreviewHeight = 128;
  }

  IBLPipeline::IBLPipeline()
  {}

  IBLPipeline::~IBLPipeline()
  {
    if (mCaptureFBO) glDeleteFramebuffers(1, &mCaptureFBO);
    if (mCaptureRBO) glDeleteRenderbuffers(1, &mCaptureRBO);
    if (mCubeVAO) { glDeleteVertexArrays(1, &mCubeVAO); glDeleteBuffers(1, &mCubeVBO); }
    if (mQuadVAO) { glDeleteVertexArrays(1, &mQuadVAO); glDeleteBuffers(1, &mQuadVBO); }
    destroy_generated_textures();
  }

  void IBLPipeline::destroy_generated_textures()
  {
    destroy_preview_textures();

    if (mOwnsEnvironmentCubemap && mEnvironmentCubemap)
      glDeleteTextures(1, &mEnvironmentCubemap);
    if (mIrradianceMap)
      glDeleteTextures(1, &mIrradianceMap);
    if (mPrefilterMap)
      glDeleteTextures(1, &mPrefilterMap);
    if (mBrdfLUTTexture)
      glDeleteTextures(1, &mBrdfLUTTexture);

    mEnvironmentCubemap = 0;
    mIrradianceMap = 0;
    mPrefilterMap = 0;
    mBrdfLUTTexture = 0;
    mOwnsEnvironmentCubemap = false;
  }

  void IBLPipeline::destroy_preview_textures()
  {
    if (mPrefilterPreviewTexture)
      glDeleteTextures(1, &mPrefilterPreviewTexture);
    if (mIrradiancePreviewTexture)
      glDeleteTextures(1, &mIrradiancePreviewTexture);
    if (mEnvironmentPreviewTexture)
      glDeleteTextures(1, &mEnvironmentPreviewTexture);

    mEnvironmentPreviewTexture = 0;
    mIrradiancePreviewTexture = 0;
    mPrefilterPreviewTexture = 0;
  }

  bool IBLPipeline::create_preview_texture(uint32_t* out_texture, uint32_t width, uint32_t height)
  {
    if (!out_texture || width == 0 || height == 0)
      return false;

    if (*out_texture != 0)
      glDeleteTextures(1, out_texture);

    glGenTextures(1, out_texture);
    glBindTexture(GL_TEXTURE_2D, *out_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return *out_texture != 0;
  }

  void IBLPipeline::render_cubemap_preview(
    uint32_t cubemap_texture,
    uint32_t target_texture,
    uint32_t width,
    uint32_t height,
    float lod)
  {
    if (!mCubemapPreviewShader ||
        mCubemapPreviewShader->get_program_id() == 0 ||
        cubemap_texture == 0 ||
        target_texture == 0 ||
        mCaptureFBO == 0)
    {
      return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_texture, 0);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    mCubemapPreviewShader->use();
    mCubemapPreviewShader->set_i1(0, "cubemapTexture");
    mCubemapPreviewShader->set_f1(lod, "lod");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
    render_quad();
  }

  void IBLPipeline::setup_cube()
  {
    float vertices[] = {
      // back face
      -1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      // front face
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,
      // left face
      -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      // right face
       1.0f,  1.0f,  1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
      // bottom face
      -1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
      // top face
      -1.0f,  1.0f, -1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f
    };
    glGenVertexArrays(1, &mCubeVAO);
    glGenBuffers(1, &mCubeVBO);
    glBindVertexArray(mCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  void IBLPipeline::setup_quad()
  {
    float quadVertices[] = {
      // positions        // texture Coords
      -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
       1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
       1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    glGenVertexArrays(1, &mQuadVAO);
    glGenBuffers(1, &mQuadVBO);
    glBindVertexArray(mQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
  }

  void IBLPipeline::render_cube()
  {
    glBindVertexArray(mCubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
  }

  void IBLPipeline::render_quad()
  {
    glBindVertexArray(mQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
  }

  bool IBLPipeline::init()
  {
    if (mCubeVAO == 0)
      setup_cube();
    if (mQuadVAO == 0)
      setup_quad();

    if (!mEquirectangularToCubemapShader)
      mEquirectangularToCubemapShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/cubemap.vs", "JGL_Engine/shaders/equirectangular_to_cubemap.fs");
    if (mEquirectangularToCubemapShader->get_program_id() == 0) return false;

    if (!mIrradianceShader)
      mIrradianceShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/cubemap.vs", "JGL_Engine/shaders/irradiance_convolution.fs");
    if (mIrradianceShader->get_program_id() == 0) return false;

    if (!mPrefilterShader)
      mPrefilterShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/cubemap.vs", "JGL_Engine/shaders/prefilter.fs");
    if (mPrefilterShader->get_program_id() == 0) return false;

    if (!mBrdfShader)
      mBrdfShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/post_process_vs.shader", "JGL_Engine/shaders/brdf.fs");
    if (mBrdfShader->get_program_id() == 0) return false;

    if (!mCubemapPreviewShader)
      mCubemapPreviewShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/post_process_vs.shader", "JGL_Engine/shaders/ibl_preview.fs");
    if (mCubemapPreviewShader && mCubemapPreviewShader->get_program_id() == 0)
    {
      std::cout << "[IBLPipeline] Cubemap preview shader unavailable." << std::endl;
      mCubemapPreviewShader.reset();
    }

    if (mCaptureFBO == 0)
      glGenFramebuffers(1, &mCaptureFBO);
    if (mCaptureRBO == 0)
      glGenRenderbuffers(1, &mCaptureRBO);

    return true;
  }

  bool IBLPipeline::build_from_cubemap(uint32_t cubemap_texture)
  {
    if (cubemap_texture == 0 || !mCaptureFBO || !mCaptureRBO)
      return false;

    destroy_generated_textures();
    mEnvironmentCubemap = cubemap_texture;
    mOwnsEnvironmentCubemap = false;

    return generate_ibl_maps();
  }

  bool IBLPipeline::load_environment_map(const std::string& filepath)
  {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, 0);
    if (!data)
    {
      std::cerr << "Failed to load HDR image." << std::endl;
      return false;
    }

    destroy_generated_textures();

    const GLenum hdr_format = nrComponents == 4 ? GL_RGBA : GL_RGB;

    uint32_t hdrTexture = 0;
    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, hdr_format, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    glGenTextures(1, &mEnvironmentCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvironmentCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, kEnvironmentSize, kEnvironmentSize, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mOwnsEnvironmentCubemap = true;

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mCaptureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kEnvironmentSize, kEnvironmentSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mCaptureRBO);

    mEquirectangularToCubemapShader->use();
    mEquirectangularToCubemapShader->set_i1(0, "equirectangularMap");
    mEquirectangularToCubemapShader->set_mat4(captureProjection, "projection");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, kEnvironmentSize, kEnvironmentSize);
    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
      mEquirectangularToCubemapShader->set_mat4(captureViews[i], "view");
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mEnvironmentCubemap, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      render_cube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvironmentCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glDeleteTextures(1, &hdrTexture);

    return generate_ibl_maps();
  }

  bool IBLPipeline::generate_ibl_maps()
  {
    if (mEnvironmentCubemap == 0 || !mCaptureFBO || !mCaptureRBO)
      return false;

    GLint previous_framebuffer = 0;
    GLint previous_viewport[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_framebuffer);
    glGetIntegerv(GL_VIEWPORT, previous_viewport);

    const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 captureViews[] = {
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvironmentCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glGenTextures(1, &mIrradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, kIrradianceSize, kIrradianceSize, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mCaptureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kIrradianceSize, kIrradianceSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mCaptureRBO);

    mIrradianceShader->use();
    mIrradianceShader->set_i1(0, "environmentMap");
    mIrradianceShader->set_mat4(captureProjection, "projection");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvironmentCubemap);

    glViewport(0, 0, kIrradianceSize, kIrradianceSize);
    for (unsigned int i = 0; i < 6; ++i)
    {
      mIrradianceShader->set_mat4(captureViews[i], "view");
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mIrradianceMap, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      render_cube();
    }

    glGenTextures(1, &mPrefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, kPrefilterSize, kPrefilterSize, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    mPrefilterShader->use();
    mPrefilterShader->set_i1(0, "environmentMap");
    mPrefilterShader->set_mat4(captureProjection, "projection");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvironmentCubemap);

    for (unsigned int mip = 0; mip < kPrefilterMipLevels; ++mip)
    {
      const unsigned int mip_width = static_cast<unsigned int>(kPrefilterSize * std::pow(0.5f, static_cast<float>(mip)));
      const unsigned int mip_height = static_cast<unsigned int>(kPrefilterSize * std::pow(0.5f, static_cast<float>(mip)));
      glBindRenderbuffer(GL_RENDERBUFFER, mCaptureRBO);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_width, mip_height);
      glViewport(0, 0, mip_width, mip_height);

      const float roughness = static_cast<float>(mip) / static_cast<float>(kPrefilterMipLevels - 1);
      mPrefilterShader->set_f1(roughness, "roughness");
      for (unsigned int i = 0; i < 6; ++i)
      {
        mPrefilterShader->set_mat4(captureViews[i], "view");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mPrefilterMap, mip);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_cube();
      }
    }

    glGenTextures(1, &mBrdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, mBrdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, kBrdfLutSize, kBrdfLutSize, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mCaptureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kBrdfLutSize, kBrdfLutSize);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBrdfLUTTexture, 0);
    glViewport(0, 0, kBrdfLutSize, kBrdfLutSize);

    mBrdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_quad();

    if (!generate_preview_textures())
      std::cout << "[IBLPipeline] Failed to generate IBL preview textures." << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, previous_framebuffer);
    glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);

    return true;
  }

  bool IBLPipeline::generate_preview_textures()
  {
    if (!mCubemapPreviewShader ||
        mCubemapPreviewShader->get_program_id() == 0 ||
        mCaptureFBO == 0 ||
        mEnvironmentCubemap == 0 ||
        mIrradianceMap == 0 ||
        mPrefilterMap == 0)
    {
      destroy_preview_textures();
      return false;
    }

    if (!create_preview_texture(&mEnvironmentPreviewTexture, kCubemapPreviewWidth, kCubemapPreviewHeight) ||
        !create_preview_texture(&mIrradiancePreviewTexture, kCubemapPreviewWidth, kCubemapPreviewHeight) ||
        !create_preview_texture(&mPrefilterPreviewTexture, kCubemapPreviewWidth, kCubemapPreviewHeight))
    {
      destroy_preview_textures();
      return false;
    }

    GLint previous_framebuffer = 0;
    GLint previous_viewport[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_framebuffer);
    glGetIntegerv(GL_VIEWPORT, previous_viewport);
    const GLboolean depth_was_enabled = glIsEnabled(GL_DEPTH_TEST);

    render_cubemap_preview(
      mEnvironmentCubemap,
      mEnvironmentPreviewTexture,
      kCubemapPreviewWidth,
      kCubemapPreviewHeight,
      0.0f);
    render_cubemap_preview(
      mIrradianceMap,
      mIrradiancePreviewTexture,
      kCubemapPreviewWidth,
      kCubemapPreviewHeight,
      0.0f);
    render_cubemap_preview(
      mPrefilterMap,
      mPrefilterPreviewTexture,
      kCubemapPreviewWidth,
      kCubemapPreviewHeight,
      2.0f);

    if (depth_was_enabled)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, previous_framebuffer);
    glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
    return true;
  }
}
