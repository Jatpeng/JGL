#pragma once

#include "elems/animation.h"
#include "elems/animator.h"
#include "elems/camera.h"
#include "elems/input.h"
#include "elems/material.h"
#include "elems/mesh.h"
#include "elems/model.h"
#include "engine/resource_manager.h"
#include "render/deferred_gbuffer.h"
#include "render/opengl_buffer_manager.h"
#include "shader/shader_util.h"

namespace nengine
{
  class RenderEngine
  {
  public:
    struct Light
    {
      glm::vec3 position = glm::vec3(0.0f, 3.0f, 0.0f);
      glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
      float strength = 40.0f;
      bool enabled = true;
    };

    enum class RenderMode
    {
      Forward = 0,
      Deferred = 1
    };

    enum class DebugView
    {
      Final = 0,
      Position = 1,
      Normal = 2,
      Albedo = 3,
      Roughness = 4,
      Metallic = 5
    };

    struct CreateInfo
    {
      glm::ivec2 render_target_size = { 800, 600 };
      std::string default_mesh_path;
      std::string default_material_path;
      bool load_default_skybox = true;
      bool load_default_plane = true;
      std::shared_ptr<IResourceManager> resource_manager;
    };

    explicit RenderEngine(const CreateInfo& create_info = {});
    ~RenderEngine();

    void resize(int32_t width, int32_t height);
    void render();

    void load_mesh(const std::string& filepath);
    void load_material(const std::string& filepath);
    void load_shader(const std::string& filepath);

    void set_model(std::shared_ptr<nelems::Model> model) { mModel = std::move(model); }
    std::shared_ptr<nelems::Model> get_model() const { return mModel; }
    std::shared_ptr<Material> get_material() const { return mMaterial; }
    Light& get_primary_light() { return mLights.front(); }
    const Light& get_primary_light() const { return mLights.front(); }
    const std::string& get_material_asset_path() const { return mMaterialAssetPath; }
    const std::string& get_shader_asset_path() const { return mShaderAssetPath; }

    void on_mouse_move(double x, double y, nelems::EInputButton button);
    void on_mouse_wheel(double delta);
    void reset_view();

    void set_render_mode(RenderMode mode) { mRenderMode = mode; }
    RenderMode get_render_mode() const { return mRenderMode; }

    void set_debug_view(DebugView view) { mDebugView = view; }
    DebugView get_debug_view() const { return mDebugView; }

    void set_model_transparent(bool is_transparent) { mModelTransparent = is_transparent; }
    bool is_model_transparent() const { return mModelTransparent; }

    void set_model_opacity(float opacity) { mModelOpacity = opacity; }
    float get_model_opacity() const { return mModelOpacity; }

    void set_plane_show(bool is_show) { mShowPlane = is_show; }
    bool is_plane_show() const { return mShowPlane; }

    void set_additional_deferred_light_count(size_t count);
    size_t get_additional_deferred_light_count() const { return mLights.empty() ? 0 : mLights.size() - 1; }
    Light& get_additional_deferred_light(size_t index) { return mLights.at(index + 1); }
    const Light& get_additional_deferred_light(size_t index) const { return mLights.at(index + 1); }

    bool is_deferred_available() const;
    glm::ivec2 get_render_target_size() const { return mRenderTargetSize; }

    uint32_t get_output_texture();
    uint32_t get_gbuffer_position_texture() const;
    uint32_t get_gbuffer_normal_roughness_texture() const;
    uint32_t get_gbuffer_albedo_metallic_texture() const;

  private:
    void init_deferred_pipeline();
    void create_fullscreen_quad();
    void create_fallback_textures();
    void init_default_additional_lights();

    void load_skybox();
    void load_plane();

    void update_frame_state();
    void apply_skinning(nshaders::Shader* shader);

    void render_forward_to_framebuffer();
    void render_deferred_to_framebuffer();
    void geometry_pass();
    void lighting_pass();
    void forward_overlay_pass();

    void render_active_model(nshaders::Shader* shader, bool update_lighting, bool allow_multipass);
    void render_plane();
    void render_plane_deferred();
    void render_skybox();
    void render_transparent_model_overlay();
    void render_fullscreen_quad();

    void upload_lights(nshaders::Shader* shader);

  private:
    std::unique_ptr<nelems::Camera> mCamera;
    std::unique_ptr<nrender::OpenGL_FrameBuffer> mFrameBuffer;
    std::unique_ptr<nshaders::Shader> mShader;
    std::shared_ptr<nelems::Model> mModel;
    std::shared_ptr<Material> mMaterial;

    bool mIsSkin = false;
    glm::ivec2 mRenderTargetSize = { 800, 600 };
    std::string mShaderName;
    std::string mShaderAssetPath;
    std::string mMaterialAssetPath;
    std::string mDefaultMeshPath;

    Animation mAnimation;
    Animator mAnimator;

    std::unique_ptr<nshaders::Shader> mSkyShader;
    std::shared_ptr<nelems::Model> mSkyBox;
    std::unique_ptr<nshaders::Shader> mPlaneShader;
    std::shared_ptr<nelems::Model> mPlane;

    bool mShowPlane = false;
    RenderMode mRenderMode = RenderMode::Forward;
    DebugView mDebugView = DebugView::Final;
    bool mModelTransparent = false;
    float mModelOpacity = 1.0f;

    float mDeltaTime = 0.0f;
    float mLastFrame = 0.0f;
    unsigned int mCubemapTexture = 0;
    unsigned int mPlaneTexture = 0;
    std::shared_ptr<IResourceManager> mResources;

    std::unique_ptr<nrender::DeferredGBuffer> mGBuffer;
    std::unique_ptr<nshaders::Shader> mDeferredGeometryShader;
    std::unique_ptr<nshaders::Shader> mDeferredLightingShader;
    unsigned int mQuadVAO = 0;
    unsigned int mQuadVBO = 0;
    unsigned int mFallbackWhiteTexture = 0;
    unsigned int mFallbackBlackTexture = 0;
    unsigned int mFallbackNormalTexture = 0;
    std::vector<Light> mLights;
  };
}
