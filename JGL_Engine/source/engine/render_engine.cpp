#include "pch.h"

#include "engine/render_engine.h"

#include <sstream>

#include "render/renderdoc_capture.h"

namespace nengine
{
  namespace
  {
    constexpr int kForwardIBLIrradianceUnit = 8;
    constexpr int kForwardIBLPrefilterUnit = 9;
    constexpr int kForwardIBLBrdfUnit = 10;
    constexpr int kDeferredIBLIrradianceUnit = 3;
    constexpr int kDeferredIBLPrefilterUnit = 4;
    constexpr int kDeferredIBLBrdfUnit = 5;
    constexpr float kPrefilterMaxLod = 4.0f;

    uint32_t create_solid_texture_rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    {
      uint32_t texture_id = 0;
      const unsigned char pixel[4] = { r, g, b, a };

      glGenTextures(1, &texture_id);
      glBindTexture(GL_TEXTURE_2D, texture_id);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glBindTexture(GL_TEXTURE_2D, 0);

      return texture_id;
    }

    const char* debug_view_name(RenderEngine::DebugView view)
    {
      switch (view)
      {
      case RenderEngine::DebugView::Final:
        return "Final";
      case RenderEngine::DebugView::Position:
        return "Position";
      case RenderEngine::DebugView::Normal:
        return "Normal";
      case RenderEngine::DebugView::Albedo:
        return "Albedo";
      case RenderEngine::DebugView::Roughness:
        return "Roughness";
      case RenderEngine::DebugView::Metallic:
        return "Metallic";
      default:
        return "Unknown";
      }
    }
  }

  RenderEngine::RenderEngine(const RenderEngine::CreateInfo& create_info)
  {
    mRenderTargetSize = create_info.render_target_size;
    if (mRenderTargetSize.x <= 0)
      mRenderTargetSize.x = 800;
    if (mRenderTargetSize.y <= 0)
      mRenderTargetSize.y = 600;

    mFrameBuffer = std::make_unique<nrender::OpenGL_FrameBuffer>();
    mFrameBuffer->create_buffers(mRenderTargetSize.x, mRenderTargetSize.y);
    mRenderDocCapture = create_info.renderdoc_capture;
    if (!mRenderDocCapture)
      mRenderDocCapture = std::make_shared<nrender::RenderDocCapture>();

    mCamera = std::make_unique<nelems::Camera>(
      glm::vec3(0, 0, 3),
      45.0f,
      static_cast<float>(mRenderTargetSize.x) / static_cast<float>(mRenderTargetSize.y),
      0.1f,
      1000.0f);

    mResources = create_info.resource_manager;
    if (!mResources)
      mResources = std::make_shared<ResourceManager>();

    if (create_info.load_default_skybox)
      load_skybox();
    if (create_info.load_default_plane)
      load_plane();

    init_deferred_pipeline();
    mPostProcessStack = std::make_unique<nrender::PostProcessStack>();
    mPostProcessStack->init(mResources, mRenderTargetSize.x, mRenderTargetSize.y);
  }

  RenderEngine::~RenderEngine()
  {
    if (mSkyShader && mSkyShader->get_program_id() != 0)
      mSkyShader->unload();
    if (mPlaneShader && mPlaneShader->get_program_id() != 0)
      mPlaneShader->unload();
    if (mDeferredGeometryShader && mDeferredGeometryShader->get_program_id() != 0)
      mDeferredGeometryShader->unload();
    if (mDeferredLightingShader && mDeferredLightingShader->get_program_id() != 0)
      mDeferredLightingShader->unload();
    if (mQuadVBO != 0)
      glDeleteBuffers(1, &mQuadVBO);
    if (mQuadVAO != 0)
      glDeleteVertexArrays(1, &mQuadVAO);
    if (mFallbackWhiteTexture != 0)
      glDeleteTextures(1, &mFallbackWhiteTexture);
    if (mFallbackBlackTexture != 0)
      glDeleteTextures(1, &mFallbackBlackTexture);
    if (mFallbackNormalTexture != 0)
      glDeleteTextures(1, &mFallbackNormalTexture);
  }

  void RenderEngine::init_deferred_pipeline()
  {
    mGBuffer = std::make_unique<nrender::DeferredGBuffer>();
    if (!mGBuffer->init(mRenderTargetSize.x, mRenderTargetSize.y))
      std::cout << "[RenderEngine] Failed to initialize deferred GBuffer." << std::endl;

    mDeferredGeometryShader = mResources->load_shader_program(
      "JGL_Engine/shaders/deferred_gbuffer_vs.shader",
      "JGL_Engine/shaders/deferred_gbuffer_fs.shader");

    mDeferredLightingShader = mResources->load_shader_program(
      "JGL_Engine/shaders/deferred_lighting_vs.shader",
      "JGL_Engine/shaders/deferred_lighting_fs.shader");

    create_fullscreen_quad();
    create_fallback_textures();
  }

  void RenderEngine::init_ibl_pipeline()
  {
    if (mCubemapTexture == 0)
    {
      mIBLPipeline.reset();
      return;
    }

    if (!mIBLPipeline)
      mIBLPipeline = std::make_unique<nrender::IBLPipeline>();

    if (!mIBLPipeline->init())
    {
      std::cout << "[RenderEngine] Failed to initialize IBL pipeline." << std::endl;
      mIBLPipeline.reset();
      return;
    }

    if (!mIBLPipeline->build_from_cubemap(mCubemapTexture))
    {
      std::cout << "[RenderEngine] Failed to build IBL textures from skybox cubemap." << std::endl;
      mIBLPipeline.reset();
    }
  }

  void RenderEngine::create_fullscreen_quad()
  {
    if (mQuadVAO != 0)
      return;

    const float quad_vertices[] = {
      -1.0f,  1.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f,
       1.0f,  1.0f, 1.0f, 1.0f,
       1.0f, -1.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &mQuadVAO);
    glGenBuffers(1, &mQuadVBO);
    glBindVertexArray(mQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  void RenderEngine::create_fallback_textures()
  {
    if (mFallbackWhiteTexture == 0)
      mFallbackWhiteTexture = create_solid_texture_rgba(255, 255, 255, 255);
    if (mFallbackBlackTexture == 0)
      mFallbackBlackTexture = create_solid_texture_rgba(0, 0, 0, 255);
    if (mFallbackNormalTexture == 0)
      mFallbackNormalTexture = create_solid_texture_rgba(128, 128, 255, 255);
  }

  void RenderEngine::resize(int32_t width, int32_t height)
  {
    if (width <= 0 || height <= 0)
      return;

    mRenderTargetSize = { width, height };
    mFrameBuffer->create_buffers(width, height);
    if (mGBuffer)
      mGBuffer->resize(width, height);
    if (mPostProcessStack)
      mPostProcessStack->resize(width, height);
    if (mCamera)
      mCamera->set_aspect(static_cast<float>(width) / static_cast<float>(height));
  }

  void RenderEngine::on_mouse_move(double x, double y, nelems::EInputButton button)
  {
    if (mCamera)
      mCamera->on_mouse_move(x, y, button);
  }

  void RenderEngine::on_mouse_wheel(double delta)
  {
    if (mCamera)
      mCamera->on_mouse_wheel(delta);
  }

  void RenderEngine::reset_view()
  {
    if (mCamera)
      mCamera->reset();
  }

  bool RenderEngine::set_screen_effect_material(const std::string& path)
  {
    return mPostProcessStack && mPostProcessStack->set_effect_material(path);
  }

  void RenderEngine::clear_screen_effect_material()
  {
    if (mPostProcessStack)
      mPostProcessStack->clear_effect_material();
  }

  bool RenderEngine::has_screen_effect_material() const
  {
    return mPostProcessStack && mPostProcessStack->has_effect();
  }

  std::shared_ptr<Material> RenderEngine::get_screen_effect_material() const
  {
    return mPostProcessStack ? mPostProcessStack->get_effect_material() : nullptr;
  }

  const std::string& RenderEngine::get_screen_effect_material_path() const
  {
    static const std::string empty_path;
    return mPostProcessStack ? mPostProcessStack->get_effect_material_path() : empty_path;
  }

  std::vector<std::shared_ptr<Entity>> RenderEngine::collect_mesh_entities() const
  {
    std::vector<std::shared_ptr<Entity>> meshes;
    if (!mScene)
      return meshes;

    for (const auto& entity : mScene->entities())
    {
      if (entity->get_component<MeshComponent>())
        meshes.push_back(entity);
    }

    return meshes;
  }

  void RenderEngine::update_frame_state()
  {
    const float current_frame = static_cast<float>(glfwGetTime());
    mDeltaTime = current_frame - mLastFrame;
    mLastFrame = current_frame;

    if (mScene)
    {
      mScene->tick(mDeltaTime);
    }
  }

  void RenderEngine::load_skybox()
  {
    mSkyShader = mResources->load_shader_program(
      "JGL_Engine/shaders/buit_in/skybox_vs.shader",
      "JGL_Engine/shaders/buit_in/skybox_fs.shader");
    if (mSkyShader->get_program_id() == 0)
      std::cout << "[RenderEngine] Skybox shader failed." << std::endl;

    mSkyBox = mResources->load_model("Assets/models/defaultcube.fbx");

    std::vector<std::string> faces
    {
      "Assets/built_in/textures/skybox/right.jpg",
      "Assets/built_in/textures/skybox/left.jpg",
      "Assets/built_in/textures/skybox/top.jpg",
      "Assets/built_in/textures/skybox/bottom.jpg",
      "Assets/built_in/textures/skybox/front.jpg",
      "Assets/built_in/textures/skybox/back.jpg"
    };
    mCubemapTexture = mResources->load_cubemap(faces, false);
    init_ibl_pipeline();
  }

  void RenderEngine::load_plane()
  {
    mPlaneShader = mResources->load_shader_program(
      "JGL_Engine/shaders/buit_in/base_vs.shader",
      "JGL_Engine/shaders/buit_in/base_fs.shader");
    mPlane = mResources->load_model("Assets/built_in/plane.fbx");
    mPlaneTexture = mResources->load_texture_2d("Assets/built_in/textures/wood.png");
  }

  void RenderEngine::render_skybox()
  {
    if (!mSkyShader || mSkyShader->get_program_id() == 0 || !mSkyBox)
      return;

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    mSkyShader->use();
    mCamera->setcam(mSkyShader.get());
    mSkyShader->set_i1(0, "skybox");
    mSkyShader->set_texture(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP, mCubemapTexture);

    mSkyBox->Draw();
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
  }

  void RenderEngine::render_plane()
  {
    if (!mPlaneShader || mPlaneShader->get_program_id() == 0 || !mPlane)
      return;

    mPlaneShader->use();
    mCamera->update(mPlaneShader.get());
    mPlaneShader->set_i1(0, "baseMap");
    mPlaneShader->set_texture(GL_TEXTURE0, GL_TEXTURE_2D, mPlaneTexture);
    mPlane->Draw();
  }

  bool RenderEngine::is_mesh_deferred_available(const MeshComponent& mesh_comp) const
  {
    const auto model = mesh_comp.model();
    const auto material = mesh_comp.material();
    if (!model || !material)
      return false;

    auto& texture_map = material->getTextureMap();
    return texture_map.find("baseMap") != texture_map.end() &&
           texture_map.find("metallicMap") != texture_map.end() &&
           texture_map.find("roughnessMap") != texture_map.end() &&
           texture_map.find("normalMap") != texture_map.end() &&
           texture_map.find("aoMap") != texture_map.end();
  }

  void RenderEngine::render_mesh_object(
    Entity& entity,
    MeshComponent& mesh_comp,
    TransformComponent& transform_comp,
    nshaders::Shader* shader,
    bool update_lighting,
    bool allow_multipass)
  {
    auto model = mesh_comp.model();
    auto material = mesh_comp.material();
    if (!model || !shader || shader->get_program_id() == 0)
      return;

    shader->use();
    mCamera->update(shader);
    shader->set_mat4(transform_comp.local_matrix(), "model");

    if (update_lighting)
      upload_lights(shader);

    mesh_comp.apply_skinning(shader);
    shader->set_f1(static_cast<float>(glfwGetTime()), "time");
    shader->set_f1(mModelOpacity, "opacity");

    if (allow_multipass && mesh_comp.shader_name() == "fur" && material && material->isMultyPass())
    {
      const int passcount = static_cast<int>(material->getFloatMap()["Pass"]);
      for (int i = 0; i < passcount; ++i)
      {
        shader->set_f1(static_cast<float>(i), "PassIndex");
        material->update_shader_params(shader);
        apply_ibl_to_shader(shader, kForwardIBLIrradianceUnit, kForwardIBLPrefilterUnit, kForwardIBLBrdfUnit);
        model->Draw();
      }
      return;
    }

    if (material)
      material->update_shader_params(shader);
    apply_ibl_to_shader(shader, kForwardIBLIrradianceUnit, kForwardIBLPrefilterUnit, kForwardIBLBrdfUnit);
    model->Draw();
  }

  void RenderEngine::render_scene_meshes_forward()
  {
    for (const auto& entity : collect_mesh_entities())
    {
      if (!entity)
        continue;

      auto mesh_comp = entity->get_component<MeshComponent>();
      auto transform_comp = entity->get_component<TransformComponent>();

      if (!mesh_comp || !transform_comp)
        continue;

      auto* shader = mesh_comp->shader();
      if (!shader || shader->get_program_id() == 0)
        continue;

      render_mesh_object(*entity, *mesh_comp, *transform_comp, shader, true, true);
    }
  }

  bool RenderEngine::is_deferred_available() const
  {
    if (!mGBuffer || !mDeferredGeometryShader || !mDeferredLightingShader)
      return false;
    if (mDeferredGeometryShader->get_program_id() == 0 || mDeferredLightingShader->get_program_id() == 0)
      return false;

    const auto entities = collect_mesh_entities();
    if (entities.empty())
      return false;

    return std::all_of(entities.begin(), entities.end(), [this](const std::shared_ptr<Entity>& entity)
    {
      if (!entity)
        return false;
      auto mesh_comp = entity->get_component<MeshComponent>();
      return mesh_comp && is_mesh_deferred_available(*mesh_comp);
    });
  }

  void RenderEngine::geometry_pass()
  {
    mGBuffer->bind_for_geometry_pass();
    if (!mModelTransparent)
    {
      for (const auto& entity : collect_mesh_entities())
      {
        if (!entity)
          continue;

        auto mesh_comp = entity->get_component<MeshComponent>();
        auto transform_comp = entity->get_component<TransformComponent>();

        if (!mesh_comp || !transform_comp || !is_mesh_deferred_available(*mesh_comp))
          continue;

        render_mesh_object(*entity, *mesh_comp, *transform_comp, mDeferredGeometryShader.get(), false, false);
      }
    }

    if (mShowPlane && mPlane)
      render_plane_deferred();
    mGBuffer->unbind();
  }

  void RenderEngine::render_fullscreen_quad()
  {
    if (mQuadVAO == 0)
      create_fullscreen_quad();

    glBindVertexArray(mQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
  }

  void RenderEngine::lighting_pass()
  {
    mFrameBuffer->bind();
    glDisable(GL_DEPTH_TEST);

    mDeferredLightingShader->use();
    mDeferredLightingShader->set_i1(0, "gPosition");
    mDeferredLightingShader->set_i1(1, "gNormalRoughness");
    mDeferredLightingShader->set_i1(2, "gAlbedoMetallic");
    mDeferredLightingShader->set_i1(static_cast<int>(mDebugView), "debugView");
    mDeferredLightingShader->set_vec3(mCamera->get_position(), "camPos");
    apply_ibl_to_shader(mDeferredLightingShader.get(), kDeferredIBLIrradianceUnit, kDeferredIBLPrefilterUnit, kDeferredIBLBrdfUnit);

    upload_lights(mDeferredLightingShader.get());

    mGBuffer->bind_textures(GL_TEXTURE0);
    render_fullscreen_quad();

    glEnable(GL_DEPTH_TEST);
    mGBuffer->copy_depth_to(mFrameBuffer->get_fbo());
  }

  void RenderEngine::forward_overlay_pass()
  {
    if (mScene && mScene->skybox_enabled())
      render_skybox();

    render_transparent_model_overlay();
    mFrameBuffer->unbind();
  }

  void RenderEngine::render_plane_deferred()
  {
    if (!mPlane || !mDeferredGeometryShader || mDeferredGeometryShader->get_program_id() == 0)
      return;

    mDeferredGeometryShader->use();
    mCamera->update(mDeferredGeometryShader.get());
    mDeferredGeometryShader->set_i1(0, "useSkinning");
    mDeferredGeometryShader->set_vec3(glm::vec3(1.0f, 1.0f, 1.0f), "color");
    mDeferredGeometryShader->set_i1(0, "baseMap");
    mDeferredGeometryShader->set_i1(1, "metallicMap");
    mDeferredGeometryShader->set_i1(2, "roughnessMap");
    mDeferredGeometryShader->set_i1(3, "normalMap");
    mDeferredGeometryShader->set_i1(4, "aoMap");

    mDeferredGeometryShader->set_texture(GL_TEXTURE0, GL_TEXTURE_2D, mPlaneTexture);
    mDeferredGeometryShader->set_texture(GL_TEXTURE1, GL_TEXTURE_2D, mFallbackBlackTexture);
    mDeferredGeometryShader->set_texture(GL_TEXTURE2, GL_TEXTURE_2D, mFallbackWhiteTexture);
    mDeferredGeometryShader->set_texture(GL_TEXTURE3, GL_TEXTURE_2D, mFallbackNormalTexture);
    mDeferredGeometryShader->set_texture(GL_TEXTURE4, GL_TEXTURE_2D, mFallbackWhiteTexture);

    mPlane->Draw();
  }

  void RenderEngine::render_transparent_model_overlay()
  {
    if (!mModelTransparent)
      return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    render_scene_meshes_forward();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
  }

  bool RenderEngine::is_ibl_available() const
  {
    return mIBLPipeline &&
           mIBLPipeline->get_irradiance_map() != 0 &&
           mIBLPipeline->get_prefilter_map() != 0 &&
           mIBLPipeline->get_brdf_lut() != 0;
  }

  void RenderEngine::apply_ibl_to_shader(nshaders::Shader* shader, int irradiance_unit, int prefilter_unit, int brdf_unit) const
  {
    if (!shader || shader->get_program_id() == 0)
      return;

    const bool ibl_available = is_ibl_available();
    shader->set_i1(ibl_available ? 1 : 0, "iblEnabled");
    shader->set_f1(kPrefilterMaxLod, "prefilterMaxLod");

    if (!ibl_available)
      return;

    shader->set_i1(irradiance_unit, "irradianceMap");
    shader->set_i1(prefilter_unit, "prefilterMap");
    shader->set_i1(brdf_unit, "brdfLUT");
    shader->set_texture(GL_TEXTURE0 + irradiance_unit, GL_TEXTURE_CUBE_MAP, mIBLPipeline->get_irradiance_map());
    shader->set_texture(GL_TEXTURE0 + prefilter_unit, GL_TEXTURE_CUBE_MAP, mIBLPipeline->get_prefilter_map());
    shader->set_texture(GL_TEXTURE0 + brdf_unit, GL_TEXTURE_2D, mIBLPipeline->get_brdf_lut());
  }

  void RenderEngine::upload_lights(nshaders::Shader* shader)
  {
    if (!shader || shader->get_program_id() == 0)
      return;

    constexpr size_t kMaxDeferredLights = 8;
    size_t light_index = 0;

    if (mScene)
    {
      for (const auto& entity : mScene->entities())
      {
        auto light = entity->get_component<LightComponent>();
        auto transform = entity->get_component<TransformComponent>();
        if (!light || !transform || light_index >= kMaxDeferredLights)
          continue;

        shader->set_vec3(transform->position, "lights[" + std::to_string(light_index) + "].position");
        shader->set_vec3(
          light->color() * light->strength(),
          "lights[" + std::to_string(light_index) + "].color");
        shader->set_i1(light->enabled() ? 1 : 0, "lights[" + std::to_string(light_index) + "].enabled");
        ++light_index;
      }
    }

    shader->set_i1(static_cast<int>(light_index), "lightCount");

    for (; light_index < kMaxDeferredLights; ++light_index)
    {
      shader->set_vec3(glm::vec3(0.0f), "lights[" + std::to_string(light_index) + "].position");
      shader->set_vec3(glm::vec3(0.0f), "lights[" + std::to_string(light_index) + "].color");
      shader->set_i1(0, "lights[" + std::to_string(light_index) + "].enabled");
    }
  }

  void RenderEngine::render_forward_to_framebuffer()
  {
    mFrameBuffer->bind();
    update_frame_state();

    if (!mModelTransparent)
      render_scene_meshes_forward();

    if (mShowPlane && mPlaneShader && mPlaneShader->get_program_id() != 0 && mPlane)
      render_plane();
    if (mScene && mScene->skybox_enabled())
      render_skybox();
    render_transparent_model_overlay();

    mFrameBuffer->unbind();
  }

  void RenderEngine::render_deferred_to_framebuffer()
  {
    if (!is_deferred_available())
    {
      render_forward_to_framebuffer();
      return;
    }

    update_frame_state();
    geometry_pass();
    lighting_pass();
    forward_overlay_pass();
  }

  void RenderEngine::render()
  {
    if (mRenderMode == RenderMode::Deferred)
      render_deferred_to_framebuffer();
    else
      render_forward_to_framebuffer();

    if (mPostProcessStack && mFrameBuffer)
    {
      mPostProcessStack->render(
        mFrameBuffer->get_texture(),
        mRenderTargetSize.x,
        mRenderTargetSize.y,
        static_cast<float>(glfwGetTime()));
    }
  }

  uint32_t RenderEngine::get_output_texture()
  {
    if (mPostProcessStack)
    {
      const uint32_t post_process_texture = mPostProcessStack->get_output_texture();
      if (post_process_texture != 0)
        return post_process_texture;
    }

    return mFrameBuffer ? mFrameBuffer->get_texture() : 0;
  }

  bool RenderEngine::is_frame_capture_available() const
  {
    return mRenderDocCapture && mRenderDocCapture->is_available();
  }

  bool RenderEngine::is_frame_capture_pending() const
  {
    return mRenderDocCapture && mRenderDocCapture->is_capture_pending();
  }

  bool RenderEngine::is_frame_capture_in_progress() const
  {
    return mRenderDocCapture && mRenderDocCapture->is_capture_in_progress();
  }

  const std::string& RenderEngine::get_last_frame_capture_path() const
  {
    static const std::string empty_path;
    return mRenderDocCapture ? mRenderDocCapture->last_capture_path() : empty_path;
  }

  void RenderEngine::request_frame_capture()
  {
    if (mRenderDocCapture)
      mRenderDocCapture->request_capture();
  }

  bool RenderEngine::begin_frame_capture(void* native_window)
  {
    if (!mRenderDocCapture)
      return false;

    std::ostringstream title;
    title << "JGL_Engine | " << (mRenderMode == RenderMode::Deferred ? "Deferred" : "Forward");
    title << " | View " << debug_view_name(mDebugView);
    if (mScene)
      title << " | Scene " << mScene->name();

    return mRenderDocCapture->begin_capture(native_window, title.str());
  }

  void RenderEngine::end_frame_capture(void* native_window)
  {
    if (mRenderDocCapture)
      mRenderDocCapture->end_capture(native_window);
  }

  uint32_t RenderEngine::get_gbuffer_position_texture() const
  {
    return mGBuffer ? mGBuffer->get_position_texture() : 0;
  }

  uint32_t RenderEngine::get_gbuffer_normal_roughness_texture() const
  {
    return mGBuffer ? mGBuffer->get_normal_roughness_texture() : 0;
  }

  uint32_t RenderEngine::get_gbuffer_albedo_metallic_texture() const
  {
    return mGBuffer ? mGBuffer->get_albedo_metallic_texture() : 0;
  }
}
