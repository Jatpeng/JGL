#include "pch.h"

#include "engine/render_engine.h"

#include <cctype>
#include <filesystem>

namespace nengine
{
  namespace
  {
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

    std::string resolve_path_or_default(const std::string& value, const char* fallback)
    {
      if (!value.empty())
        return value;

      return fallback;
    }

    std::string to_lower_copy(std::string value)
    {
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
      {
        return static_cast<char>(std::tolower(c));
      });
      return value;
    }

    bool is_material_definition_path(const std::string& path)
    {
      const std::string ext = to_lower_copy(std::filesystem::path(path).extension().string());
      return ext == ".mtl" || ext == ".xml";
    }

    bool is_shader_source_path(const std::string& path)
    {
      const std::string ext = to_lower_copy(std::filesystem::path(path).extension().string());
      return ext == ".shader";
    }

    bool ends_with(const std::string& value, const std::string& suffix)
    {
      return value.size() >= suffix.size() &&
             value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool split_shader_program_base(const std::string& shader_path, std::string* out_base_path)
    {
      if (!out_base_path)
        return false;

      if (ends_with(shader_path, "_vs.shader"))
      {
        *out_base_path = shader_path.substr(0, shader_path.size() - std::string("_vs.shader").size());
        return true;
      }
      if (ends_with(shader_path, "_fs.shader"))
      {
        *out_base_path = shader_path.substr(0, shader_path.size() - std::string("_fs.shader").size());
        return true;
      }
      return false;
    }
  }

  RenderEngine::RenderEngine(const CreateInfo& create_info)
  {
    mRenderTargetSize = create_info.render_target_size;
    if (mRenderTargetSize.x <= 0)
      mRenderTargetSize.x = 800;
    if (mRenderTargetSize.y <= 0)
      mRenderTargetSize.y = 600;

    mFrameBuffer = std::make_unique<nrender::OpenGL_FrameBuffer>();
    mFrameBuffer->create_buffers(mRenderTargetSize.x, mRenderTargetSize.y);

    mCamera = std::make_unique<nelems::Camera>(
      glm::vec3(0, 0, 3),
      45.0f,
      static_cast<float>(mRenderTargetSize.x) / static_cast<float>(mRenderTargetSize.y),
      0.1f,
      1000.0f);
    // Light 0 is the shared key light. Both forward and deferred passes read from mLights.
    mLights.push_back(Light{
      glm::vec3(1.5f, 3.5f, 3.0f),
      glm::vec3(1.0f, 1.0f, 1.0f),
      100.0f,
      true
    });
    mMaterial = std::make_shared<Material>();
    mResources = create_info.resource_manager;
    if (!mResources)
      mResources = std::make_shared<ResourceManager>();

    mDefaultMeshPath = mResources->resolve_path(
      resolve_path_or_default(create_info.default_mesh_path, "JGL_MeshLoader/resource/cube.fbx"));
    const std::string default_material_path = mResources->resolve_path(resolve_path_or_default(
      create_info.default_material_path,
      "JGL_MeshLoader/resource/PBR.mtl"));

    load_material(default_material_path);
    load_mesh(mDefaultMeshPath);

    if (create_info.load_default_skybox)
      load_skybox();
    if (create_info.load_default_plane)
      load_plane();

    init_deferred_pipeline();
  }

  RenderEngine::~RenderEngine()
  {
    if (mShader && mShader->get_program_id() != 0)
      mShader->unload();
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
      "JGL_MeshLoader/shaders/deferred_gbuffer_vs.shader",
      "JGL_MeshLoader/shaders/deferred_gbuffer_fs.shader");

    mDeferredLightingShader = mResources->load_shader_program(
      "JGL_MeshLoader/shaders/deferred_lighting_vs.shader",
      "JGL_MeshLoader/shaders/deferred_lighting_fs.shader");

    create_fullscreen_quad();
    create_fallback_textures();
    init_default_additional_lights();
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

  void RenderEngine::init_default_additional_lights()
  {
    if (mLights.size() > 1)
      return;

    set_additional_deferred_light_count(2);
    mLights[1].position = glm::vec3(3.5f, 1.5f, 2.0f);
    mLights[1].color = glm::vec3(1.0f, 0.45f, 0.25f);
    mLights[1].strength = 20.0f;
    mLights[1].enabled = true;

    mLights[2].position = glm::vec3(-3.0f, 2.0f, 1.0f);
    mLights[2].color = glm::vec3(0.2f, 0.45f, 1.0f);
    mLights[2].strength = 16.0f;
    mLights[2].enabled = true;
  }

  void RenderEngine::set_additional_deferred_light_count(size_t count)
  {
    const size_t max_extra_lights = 7;
    if (count > max_extra_lights)
      count = max_extra_lights;

    if (mLights.empty())
      mLights.push_back(Light{});

    const size_t old_size = mLights.size();
    const size_t target_size = count + 1;
    mLights.resize(target_size);
    for (size_t i = old_size; i < mLights.size(); ++i)
    {
      auto& light = mLights[i];
      const float offset = static_cast<float>(i);
      light.position = glm::vec3(offset, 2.0f, 1.5f);
      light.color = glm::vec3(1.0f, 1.0f, 1.0f);
      light.strength = 20.0f;
      light.enabled = true;
    }
  }

  void RenderEngine::resize(int32_t width, int32_t height)
  {
    if (width <= 0 || height <= 0)
      return;

    mRenderTargetSize = { width, height };
    mFrameBuffer->create_buffers(width, height);
    if (mGBuffer)
      mGBuffer->resize(width, height);
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

  void RenderEngine::update_frame_state()
  {
    const float current_frame = static_cast<float>(glfwGetTime());
    mDeltaTime = current_frame - mLastFrame;
    mLastFrame = current_frame;
    mAnimator.UpdateAnimation(mDeltaTime);
  }

  void RenderEngine::apply_skinning(nshaders::Shader* shader)
  {
    if (!shader || shader->get_program_id() == 0)
      return;

    shader->set_i1(mIsSkin ? 1 : 0, "useSkinning");

    if (!mIsSkin)
      return;

    const auto transforms = mAnimator.GetFinalBoneMatrices();
    for (size_t i = 0; i < transforms.size(); ++i)
      shader->set_mat4(transforms[i], "finalBonesMatrices[" + std::to_string(i) + "]");
  }

  void RenderEngine::load_skybox()
  {
    mSkyShader = mResources->load_shader_program(
      "JGL_MeshLoader/shaders/buit_in/skybox_vs.shader",
      "JGL_MeshLoader/shaders/buit_in/skybox_fs.shader");
    if (mSkyShader->get_program_id() == 0)
      std::cout << "[RenderEngine] Skybox shader failed." << std::endl;

    mSkyBox = mResources->load_model("JGL_MeshLoader/resource/defaultcube.fbx");

    vector<std::string> faces
    {
      "JGL_MeshLoader/resource/built_in/textures/skybox/right.jpg",
      "JGL_MeshLoader/resource/built_in/textures/skybox/left.jpg",
      "JGL_MeshLoader/resource/built_in/textures/skybox/top.jpg",
      "JGL_MeshLoader/resource/built_in/textures/skybox/bottom.jpg",
      "JGL_MeshLoader/resource/built_in/textures/skybox/front.jpg",
      "JGL_MeshLoader/resource/built_in/textures/skybox/back.jpg"
    };
    mCubemapTexture = mResources->load_cubemap(faces, false);
  }

  void RenderEngine::load_plane()
  {
    mPlaneShader = mResources->load_shader_program(
      "JGL_MeshLoader/shaders/buit_in/base_vs.shader",
      "JGL_MeshLoader/shaders/buit_in/base_fs.shader");
    mPlane = mResources->load_model("JGL_MeshLoader/resource/built_in/plane.fbx");
    mPlaneTexture = mResources->load_texture_2d("JGL_MeshLoader/resource/built_in/textures/wood.png");
  }

  void RenderEngine::load_mesh(const std::string& filepath)
  {
    const std::string resolved_path = mResources->resolve_path(filepath);
    mModel = mResources->load_model(resolved_path);
    if (!mModel)
      return;

    mIsSkin = mModel->GetIsSkinInModel();

    if (mIsSkin)
    {
      load_material(mResources->resolve_path("JGL_MeshLoader/resource/Anim.mtl"));
      mAnimation = Animation(resolved_path, mModel.get());
      mAnimator = Animator(&mAnimation);
      if (mMaterial)
        mMaterial->set_textures(mModel->GetTexturesMap());
    }
  }

  void RenderEngine::load_material(const std::string& filepath)
  {
    const std::string resolved_path = mResources->resolve_path(filepath);
    if (resolved_path.empty())
      return;
    if (is_shader_source_path(resolved_path))
    {
      load_shader(resolved_path);
      return;
    }
    if (!is_material_definition_path(resolved_path))
    {
      std::cout << "[RenderEngine] Unsupported material file: " << resolved_path << std::endl;
      return;
    }

    auto material = mResources->load_material(resolved_path);
    if (!material)
      return;
    mMaterial = material;
    mMaterialAssetPath = resolved_path;

    if (mMaterial->getshaderPath().empty())
    {
      std::cout << "[RenderEngine] Material has no shader binding: " << resolved_path << std::endl;
      return;
    }

    load_shader(mMaterial->getshaderPath());
  }

  void RenderEngine::load_shader(const std::string& filepath)
  {
    const std::string resolved_path = mResources->resolve_path(filepath);
    if (resolved_path.empty())
      return;
    if (is_material_definition_path(resolved_path))
    {
      load_material(resolved_path);
      return;
    }
    if (!is_shader_source_path(resolved_path))
    {
      std::cout << "[RenderEngine] Unsupported shader file: " << resolved_path << std::endl;
      return;
    }

    std::string shader_base_path;
    if (!split_shader_program_base(resolved_path, &shader_base_path))
    {
      std::cout << "[RenderEngine] Shader file must end with _vs.shader or _fs.shader: "
                << resolved_path << std::endl;
      return;
    }

    const std::string vsname = shader_base_path + "_vs.shader";
    const std::string fsname = shader_base_path + "_fs.shader";
    std::filesystem::path shader_path(shader_base_path);
    mShaderAssetPath = resolved_path;
    mShaderName = shader_path.stem().string();
    mShader = mResources->load_shader_program(vsname, fsname);
    if (mShader->get_program_id() == 0)
      std::cout << "[RenderEngine] Main shader failed: " << vsname << " / " << fsname << std::endl;
  }

  void RenderEngine::render_skybox()
  {
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
    mPlaneShader->use();
    mCamera->update(mPlaneShader.get());
    mPlaneShader->set_i1(0, "baseMap");
    mPlaneShader->set_texture(GL_TEXTURE0, GL_TEXTURE_2D, mPlaneTexture);
    mPlane->Draw();
  }

  void RenderEngine::render_active_model(nshaders::Shader* shader, bool update_lighting, bool allow_multipass)
  {
    if (!mModel || !shader || shader->get_program_id() == 0)
      return;

    shader->use();
    mCamera->update(shader);
    if (update_lighting)
    {
      upload_lights(shader);
    }

    apply_skinning(shader);
    shader->set_f1(static_cast<float>(glfwGetTime()), "time");
    shader->set_f1(mModelOpacity, "opacity");

    if (allow_multipass && mShaderName == "fur" && mMaterial && mMaterial->isMultyPass())
    {
      const int passcount = static_cast<int>(mMaterial->getFloatMap()["Pass"]);
      for (int i = 0; i < passcount; ++i)
      {
        shader->set_f1(static_cast<float>(i), "PassIndex");
        mMaterial->update_shader_params(shader);
        mModel->Draw();
      }
      return;
    }

    if (mMaterial)
      mMaterial->update_shader_params(shader);
    mModel->Draw();
  }

  bool RenderEngine::is_deferred_available() const
  {
    if (!mGBuffer || !mDeferredGeometryShader || !mDeferredLightingShader)
      return false;
    if (mDeferredGeometryShader->get_program_id() == 0 || mDeferredLightingShader->get_program_id() == 0)
      return false;
    if (!mModel || !mMaterial)
      return false;

    auto& texture_map = mMaterial->getTextureMap();
    return texture_map.find("baseMap") != texture_map.end() &&
           texture_map.find("metallicMap") != texture_map.end() &&
           texture_map.find("roughnessMap") != texture_map.end() &&
           texture_map.find("normalMap") != texture_map.end();
  }

  void RenderEngine::geometry_pass()
  {
    mGBuffer->bind_for_geometry_pass();
    if (!mModelTransparent)
      render_active_model(mDeferredGeometryShader.get(), false, false);
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

    // Reconstruct the shaded scene from the G-Buffer into the same output texture the UI already displays.
    mDeferredLightingShader->use();
    mDeferredLightingShader->set_i1(0, "gPosition");
    mDeferredLightingShader->set_i1(1, "gNormalRoughness");
    mDeferredLightingShader->set_i1(2, "gAlbedoMetallic");
    mDeferredLightingShader->set_i1(static_cast<int>(mDebugView), "debugView");
    mDeferredLightingShader->set_vec3(mCamera->get_position(), "camPos");

    upload_lights(mDeferredLightingShader.get());

    mGBuffer->bind_textures(GL_TEXTURE0);
    render_fullscreen_quad();

    glEnable(GL_DEPTH_TEST);
    mGBuffer->copy_depth_to(mFrameBuffer->get_fbo());
  }

  void RenderEngine::forward_overlay_pass()
  {
    if (mSkyShader && mSkyShader->get_program_id() != 0 && mSkyBox)
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

    mDeferredGeometryShader->set_texture(GL_TEXTURE0, GL_TEXTURE_2D, mPlaneTexture);
    mDeferredGeometryShader->set_texture(GL_TEXTURE1, GL_TEXTURE_2D, mFallbackBlackTexture);
    mDeferredGeometryShader->set_texture(GL_TEXTURE2, GL_TEXTURE_2D, mFallbackWhiteTexture);
    mDeferredGeometryShader->set_texture(GL_TEXTURE3, GL_TEXTURE_2D, mFallbackNormalTexture);

    mPlane->Draw();
  }

  void RenderEngine::render_transparent_model_overlay()
  {
    if (!mModelTransparent || !mShader || mShader->get_program_id() == 0 || !mModel)
      return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    render_active_model(mShader.get(), true, false);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
  }

  void RenderEngine::upload_lights(nshaders::Shader* shader)
  {
    if (!shader || shader->get_program_id() == 0)
      return;

    constexpr size_t kMaxDeferredLights = 8;
    size_t light_index = 0;

    // Always upload a dense prefix of lights[] so forward and deferred shaders consume the same contract.
    for (const auto& light : mLights)
    {
      if (light_index >= kMaxDeferredLights)
        break;

      shader->set_vec3(light.position, "lights[" + std::to_string(light_index) + "].position");
      shader->set_vec3(light.color * light.strength, "lights[" + std::to_string(light_index) + "].color");
      shader->set_i1(light.enabled ? 1 : 0, "lights[" + std::to_string(light_index) + "].enabled");
      ++light_index;
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

    if (!mModel && !mDefaultMeshPath.empty())
      load_mesh(mDefaultMeshPath);

    update_frame_state();

    if (mShader && mShader->get_program_id() != 0 && !mModelTransparent)
      render_active_model(mShader.get(), true, true);

    if (mShowPlane && mPlaneShader && mPlaneShader->get_program_id() != 0 && mPlane)
      render_plane();
    if (mSkyShader && mSkyShader->get_program_id() != 0 && mSkyBox)
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
  }

  uint32_t RenderEngine::get_output_texture()
  {
    return mFrameBuffer ? mFrameBuffer->get_texture() : 0;
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
