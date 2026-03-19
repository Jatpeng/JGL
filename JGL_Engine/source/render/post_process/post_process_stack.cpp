#include "pch.h"
#include "post_process_stack.h"

#include "render/opengl_buffer_manager.h"

namespace nrender
{
  PostProcessStack::PostProcessStack()
  {
  }

  PostProcessStack::~PostProcessStack()
  {
    if (mEffectShader && mEffectShader->get_program_id() != 0)
      mEffectShader->unload();
    if (mOutputBuffer)
      mOutputBuffer->delete_buffers();
    if (mQuadVAO != 0)
    {
      glDeleteVertexArrays(1, &mQuadVAO);
      glDeleteBuffers(1, &mQuadVBO);
    }
  }

  bool PostProcessStack::init(std::shared_ptr<nengine::IResourceManager> resources, int32_t width, int32_t height)
  {
    mResources = std::move(resources);
    ensure_output_buffer(width, height);

    const float quadVertices[] = {
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
  }

  void PostProcessStack::resize(int32_t width, int32_t height)
  {
    ensure_output_buffer(width, height);
  }

  bool PostProcessStack::set_effect_material(const std::string& material_path)
  {
    if (!mResources)
      return false;

    if (material_path.empty())
    {
      clear_effect_material();
      return true;
    }

    auto effect_material = mResources->load_material(material_path);
    if (!effect_material || effect_material->getshaderPath().empty())
      return false;

    auto effect_shader = mResources->load_shader_program(
      "JGL_Engine/shaders/post_process_vs.shader",
      effect_material->getshaderPath());
    if (!effect_shader || effect_shader->get_program_id() == 0)
      return false;

    if (mEffectShader && mEffectShader->get_program_id() != 0)
      mEffectShader->unload();

    mEffectShader = std::move(effect_shader);
    mEffectMaterial = std::move(effect_material);
    mEffectMaterialPath = mResources->resolve_path(material_path);
    return true;
  }

  void PostProcessStack::clear_effect_material()
  {
    if (mEffectShader && mEffectShader->get_program_id() != 0)
      mEffectShader->unload();

    mEffectShader.reset();
    mEffectMaterial.reset();
    mEffectMaterialPath.clear();
  }

  bool PostProcessStack::has_effect() const
  {
    return mEffectShader && mEffectShader->get_program_id() != 0 && mEffectMaterial;
  }

  std::shared_ptr<Material> PostProcessStack::get_effect_material() const
  {
    return mEffectMaterial;
  }

  const std::string& PostProcessStack::get_effect_material_path() const
  {
    return mEffectMaterialPath;
  }

  void PostProcessStack::render(
    uint32_t hdr_texture_id,
    int32_t width,
    int32_t height,
    float time_seconds)
  {
    if (!has_effect() || hdr_texture_id == 0 || width <= 0 || height <= 0)
      return;

    ensure_output_buffer(width, height);
    if (!mOutputBuffer)
      return;

    mOutputBuffer->bind();
    glViewport(0, 0, width, height);
    mEffectShader->use();
    mEffectShader->set_i1(0, "hdrBuffer");
    mEffectShader->set_f1(time_seconds, "time");
    mEffectShader->set_f2(static_cast<float>(width), static_cast<float>(height), "screenResolution");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdr_texture_id);
    mEffectMaterial->update_shader_params(mEffectShader.get(), 1);

    glBindVertexArray(mQuadVAO);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    mOutputBuffer->unbind();
  }

  uint32_t PostProcessStack::get_output_texture()
  {
    if (!has_effect())
      return 0;
    return mOutputBuffer ? mOutputBuffer->get_texture() : 0;
  }

  bool PostProcessStack::is_ready() const
  {
    return mOutputBuffer && mQuadVAO != 0;
  }

  void PostProcessStack::ensure_output_buffer(int32_t width, int32_t height)
  {
    if (width <= 0 || height <= 0)
      return;

    if (!mOutputBuffer)
      mOutputBuffer = std::make_unique<OpenGL_FrameBuffer>();

    if (mOutputWidth == width && mOutputHeight == height && mOutputBuffer->get_texture() != 0)
      return;

    mOutputBuffer->create_buffers(width, height);
    mOutputWidth = width;
    mOutputHeight = height;
  }
}
