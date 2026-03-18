#include "pch.h"
#include "ibl_pipeline.h"
#include <stb_image.h>

namespace nrender
{
  IBLPipeline::IBLPipeline()
  {}

  IBLPipeline::~IBLPipeline()
  {
    if (mCaptureFBO) glDeleteFramebuffers(1, &mCaptureFBO);
    if (mCaptureRBO) glDeleteRenderbuffers(1, &mCaptureRBO);
    if (mCubeVAO) { glDeleteVertexArrays(1, &mCubeVAO); glDeleteBuffers(1, &mCubeVBO); }
    if (mQuadVAO) { glDeleteVertexArrays(1, &mQuadVAO); glDeleteBuffers(1, &mQuadVBO); }
    if (mEnvironmentCubemap) glDeleteTextures(1, &mEnvironmentCubemap);
    if (mIrradianceMap) glDeleteTextures(1, &mIrradianceMap);
    if (mPrefilterMap) glDeleteTextures(1, &mPrefilterMap);
    if (mBrdfLUTTexture) glDeleteTextures(1, &mBrdfLUTTexture);
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
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
    setup_cube();
    setup_quad();

    mEquirectangularToCubemapShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/cubemap.vs", "JGL_Engine/shaders/equirectangular_to_cubemap.fs");
    if (mEquirectangularToCubemapShader->get_program_id() == 0) return false;

    mIrradianceShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/cubemap.vs", "JGL_Engine/shaders/irradiance_convolution.fs");
    if (mIrradianceShader->get_program_id() == 0) return false;

    mPrefilterShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/cubemap.vs", "JGL_Engine/shaders/prefilter.fs");
    if (mPrefilterShader->get_program_id() == 0) return false;

    mBrdfShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/post_process_vs.shader", "JGL_Engine/shaders/brdf.fs");
    if (mBrdfShader->get_program_id() == 0) return false;

    glGenFramebuffers(1, &mCaptureFBO);
    glGenRenderbuffers(1, &mCaptureRBO);

    return true;
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

    uint32_t hdrTexture;
    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    glGenTextures(1, &mEnvironmentCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvironmentCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mCaptureRBO);

    mEquirectangularToCubemapShader->use();
    mEquirectangularToCubemapShader->set_i1(0, "equirectangularMap");
    mEquirectangularToCubemapShader->set_mat4(captureProjection, "projection");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512);
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

    // Irradiance map
    glGenTextures(1, &mIrradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mCaptureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    mIrradianceShader->use();
    mIrradianceShader->set_i1(0, "environmentMap");
    mIrradianceShader->set_mat4(captureProjection, "projection");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvironmentCubemap);

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
      mIrradianceShader->set_mat4(captureViews[i], "view");
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mIrradianceMap, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      render_cube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Prefilter map
    glGenTextures(1, &mPrefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
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

    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
      unsigned int mipWidth = 128 * std::pow(0.5, mip);
      unsigned int mipHeight = 128 * std::pow(0.5, mip);
      glBindRenderbuffer(GL_RENDERBUFFER, mCaptureRBO);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
      glViewport(0, 0, mipWidth, mipHeight);

      float roughness = (float)mip / (float)(maxMipLevels - 1);
      mPrefilterShader->set_f1(roughness, "roughness");
      for (unsigned int i = 0; i < 6; ++i)
      {
        mPrefilterShader->set_mat4(captureViews[i], "view");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mPrefilterMap, mip);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_cube();
      }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // BRDF LUT
    glGenTextures(1, &mBrdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, mBrdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, mCaptureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mCaptureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBrdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    mBrdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_quad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
  }
}
