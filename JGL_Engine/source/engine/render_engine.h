#pragma once

#include "elems/camera.h"
#include "engine/resource_manager.h"
#include "engine/scene.h"
#include "render/deferred_gbuffer.h"
#include "render/opengl_buffer_manager.h"
#include "shader/shader_util.h"

namespace nrender
{
  class RenderDocCapture;
}

namespace nengine
{
  class RenderEngine
  {
  public:
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
      bool load_default_skybox = true;
      bool load_default_plane = true;
      std::shared_ptr<IResourceManager> resource_manager;
      std::shared_ptr<nrender::RenderDocCapture> renderdoc_capture;
    };

    explicit RenderEngine(const CreateInfo& create_info = CreateInfo());
    ~RenderEngine();

    void resize(int32_t width, int32_t height);
    void render();

    void set_scene(std::shared_ptr<Scene> scene) { mScene = std::move(scene); }
    std::shared_ptr<Scene> get_scene() const { return mScene; }
    std::shared_ptr<IResourceManager> get_resource_manager() const { return mResources; }

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

    bool is_deferred_available() const;
    glm::ivec2 get_render_target_size() const { return mRenderTargetSize; }

    bool is_frame_capture_available() const;
    bool is_frame_capture_pending() const;
    bool is_frame_capture_in_progress() const;
    const std::string& get_last_frame_capture_path() const;
    void request_frame_capture();
    bool begin_frame_capture(void* native_window);
    void end_frame_capture(void* native_window);

    uint32_t get_output_texture();
    uint32_t get_gbuffer_position_texture() const;
    uint32_t get_gbuffer_normal_roughness_texture() const;
    uint32_t get_gbuffer_albedo_metallic_texture() const;

  private:
    void init_deferred_pipeline();
    void create_fullscreen_quad();
    void create_fallback_textures();

    void load_skybox();
    void load_plane();

    void update_frame_state();

    void render_forward_to_framebuffer();
    void render_deferred_to_framebuffer();
    void geometry_pass();
    void lighting_pass();
    void shadow_pass();
    void shadow_pass();
    void shadow_pass();
    void shadow_pass();
    void shadow_pass();
    void forward_overlay_pass();

    bool is_mesh_deferred_available(const MeshObject& mesh_object) const;
    void render_mesh_object(MeshObject& mesh_object, nshaders::Shader* shader, bool update_lighting, bool allow_multipass);
    void render_scene_meshes_forward();
    void render_plane();
    void render_plane_deferred();
    void render_skybox();
    void render_transparent_model_overlay();
    void render_fullscreen_quad();

    void upload_lights(nshaders::Shader* shader);
    std::vector<std::shared_ptr<MeshObject>> collect_mesh_objects() const;

  private:
    std::unique_ptr<nelems::Camera> mCamera;
    std::unique_ptr<nrender::OpenGL_FrameBuffer> mFrameBuffer;
    std::shared_ptr<Scene> mScene;

    glm::ivec2 mRenderTargetSize = { 800, 600 };

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
    std::shared_ptr<nrender::RenderDocCapture> mRenderDocCapture;
    std::unique_ptr<nshaders::Shader> mDeferredGeometryShader;
    std::unique_ptr<nshaders::Shader> mDeferredLightingShader;
    unsigned int mQuadVAO = 0;
    unsigned int mQuadVBO = 0;
    unsigned int mFallbackWhiteTexture = 0;
    unsigned int mFallbackBlackTexture = 0;
    unsigned int mFallbackNormalTexture = 0;

    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;

    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;

    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    unsigned int SHADOW_WIDTH = 2048;
    unsigned int SHADOW_HEIGHT = 2048;

    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;

    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;
  };
}
