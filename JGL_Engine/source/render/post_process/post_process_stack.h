#pragma once

#include "pch.h"
#include "engine/resource_manager.h"
#include "shader/shader_util.h"

namespace nrender
{
  class OpenGL_FrameBuffer;

  class PostProcessStack
  {
  public:
    PostProcessStack();
    ~PostProcessStack();

    bool init(std::shared_ptr<nengine::IResourceManager> resources, int32_t width, int32_t height);
    void resize(int32_t width, int32_t height);
    bool set_effect_material(const std::string& material_path);
    void clear_effect_material();
    bool has_effect() const;
    std::shared_ptr<Material> get_effect_material() const;
    const std::string& get_effect_material_path() const;
    void render(uint32_t hdr_texture_id, int32_t width, int32_t height, float time_seconds);
    uint32_t get_output_texture();
    bool is_ready() const;

  private:
    void ensure_output_buffer(int32_t width, int32_t height);

    std::shared_ptr<nengine::IResourceManager> mResources;
    std::unique_ptr<nshaders::Shader> mEffectShader;
    std::shared_ptr<Material> mEffectMaterial;
    std::string mEffectMaterialPath;
    std::unique_ptr<OpenGL_FrameBuffer> mOutputBuffer;
    uint32_t mQuadVAO = 0;
    uint32_t mQuadVBO = 0;
    int32_t mOutputWidth = 0;
    int32_t mOutputHeight = 0;
  };
}
