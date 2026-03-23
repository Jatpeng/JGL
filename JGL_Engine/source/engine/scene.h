#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

class Animation;
class Animator;
class Material;

namespace nelems
{
  class Model;
}

namespace nshaders
{
  class Shader;
}

#include "core/component.h"
#include "core/entity.h"
#include "core/transform.h"

namespace nengine
{
  class IResourceManager;

  /**
   * @brief 管理实体3D网格渲染的组件。
   *
   * 包含模型(Model)、材质(Material)、着色器(Shader)，并管理骨骼动画(Animation)。
   */
  class MeshComponent : public IComponent
  {
  public:
    MeshComponent(std::shared_ptr<IResourceManager> resources);
    ~MeshComponent() override;

    bool set_model(const std::string& path);
    bool set_runtime_model(std::shared_ptr<nelems::Model> model, const std::string& debug_label = "Generated");
    bool set_animation(const std::string& path, const std::string& clip_name = "");
    bool set_animation_asset(const std::string& path);
    void clear_animation();
    void stop_animation();
    bool set_material(const std::string& path);
    bool set_shader(const std::string& path);
    bool reload_shader();

    std::shared_ptr<nelems::Model> model() const { return mModel; }
    std::shared_ptr<Material> material() const { return mMaterial; }
    nshaders::Shader* shader() const { return mShader.get(); }

    const std::string& model_path() const { return mModelPath; }
    const std::string& material_path() const { return mMaterialPath; }
    const std::string& shader_path() const { return mShaderPath; }
    const std::string& shader_name() const { return mShaderName; }
    const std::string& animation_path() const { return mAnimationPath; }
    const std::string& animation_asset_path() const { return mAnimationAssetPath; }
    const std::string& animation_clip_name() const { return mAnimationClipName; }

    bool is_skinned() const { return mIsSkinned; }
    bool has_animation() const { return mAnimation && mAnimator; }
    bool is_animation_playing() const { return mAnimationPlaying; }
    bool is_animation_looping() const { return mAnimationLoop; }
    float animation_speed() const { return mAnimationSpeed; }
    void set_animation_playing(bool is_playing) { mAnimationPlaying = is_playing; }
    void set_animation_looping(bool is_looping) { mAnimationLoop = is_looping; }
    void set_animation_speed(float speed) { mAnimationSpeed = std::max(0.0f, speed); }
    void tick(float delta_time);
    void apply_skinning(nshaders::Shader* shader) const;

    const char* component_type() const override { return "Mesh"; }

  private:
    bool bind_animation_source(const std::string& resolved_path, const std::string& clip_name, bool auto_embedded);
    void clear_animation_runtime();

    std::weak_ptr<IResourceManager> mResources;
    std::shared_ptr<nelems::Model> mModel;
    std::shared_ptr<Material> mMaterial;
    std::unique_ptr<nshaders::Shader> mShader;
    std::string mModelPath;
    std::string mMaterialPath;
    std::string mShaderPath;
    std::string mShaderName;
    std::string mAnimationPath;
    std::string mAnimationAssetPath;
    std::string mAnimationClipName;
    bool mIsSkinned = false;
    bool mAnimationPlaying = false;
    bool mAnimationLoop = true;
    float mAnimationSpeed = 1.0f;
    bool mAnimationAutoEmbedded = false;
    std::unique_ptr<Animation> mAnimation;
    std::unique_ptr<Animator> mAnimator;
  };

  class TerrainComponent : public IComponent
  {
  public:
    struct Settings
    {
      float width = 32.0f;
      float depth = 32.0f;
      int resolution_x = 128;
      int resolution_z = 128;
      float height_scale = 4.0f;
      float height_offset = 0.0f;
      float uv_scale = 8.0f;
      float noise_frequency = 0.08f;
      int noise_octaves = 5;
      float noise_persistence = 0.5f;
      float noise_lacunarity = 2.0f;
      int seed = 1337;
    };

    TerrainComponent(MeshComponent* mesh, std::shared_ptr<IResourceManager> resources);
    ~TerrainComponent() override = default;

    const Settings& settings() const { return mSettings; }
    void apply_settings(const Settings& settings, bool rebuild_immediately = true);
    bool rebuild();

    float sample_height(float local_x, float local_z) const;

    float width() const { return mSettings.width; }
    void set_width(float value);

    float depth() const { return mSettings.depth; }
    void set_depth(float value);

    int resolution_x() const { return mSettings.resolution_x; }
    void set_resolution_x(int value);

    int resolution_z() const { return mSettings.resolution_z; }
    void set_resolution_z(int value);

    float height_scale() const { return mSettings.height_scale; }
    void set_height_scale(float value);

    float height_offset() const { return mSettings.height_offset; }
    void set_height_offset(float value);

    float uv_scale() const { return mSettings.uv_scale; }
    void set_uv_scale(float value);

    float noise_frequency() const { return mSettings.noise_frequency; }
    void set_noise_frequency(float value);

    int noise_octaves() const { return mSettings.noise_octaves; }
    void set_noise_octaves(int value);

    float noise_persistence() const { return mSettings.noise_persistence; }
    void set_noise_persistence(float value);

    float noise_lacunarity() const { return mSettings.noise_lacunarity; }
    void set_noise_lacunarity(float value);

    int seed() const { return mSettings.seed; }
    void set_seed(int value);

    const char* component_type() const override { return "Terrain"; }

  private:
    size_t height_index(int x, int z) const;
    float sample_height_grid(int x, int z) const;
    float evaluate_height(float local_x, float local_z) const;

    std::weak_ptr<IResourceManager> mResources;
    MeshComponent* mMesh = nullptr;
    Settings mSettings;
    std::vector<float> mHeights;
  };

  /**
   * @brief 光照组件，用于定义点光源或方向光的属性。
   */
  class LightComponent : public IComponent
  {
  public:
    enum class LightType
    {
      Point = 0,
      Directional = 1
    };

    LightComponent()
    {
      set_direction(glm::vec3(-0.35f, -1.0f, -0.25f));
    }

    void set_type(LightType type) { mType = type; }
    LightType type() const { return mType; }

    void set_color(const glm::vec3& color) { mColor = color; }
    const glm::vec3& color() const { return mColor; }

    void set_strength(float strength) { mStrength = strength; }
    float strength() const { return mStrength; }

    void set_direction(const glm::vec3& direction)
    {
      if (glm::length(direction) <= 0.0001f)
        return;
      mDirection = glm::normalize(direction);
    }
    const glm::vec3& direction() const { return mDirection; }

    void set_enabled(bool enabled) { mEnabled = enabled; }
    bool enabled() const { return mEnabled; }

    void set_casts_shadows(bool casts_shadows) { mCastsShadows = casts_shadows; }
    bool casts_shadows() const { return mCastsShadows; }

    void set_shadow_bias_min(float bias)
    {
      mShadowBiasMin = std::max(0.0f, bias);
      if (mShadowBiasMax < mShadowBiasMin)
        mShadowBiasMax = mShadowBiasMin;
    }
    float shadow_bias_min() const { return mShadowBiasMin; }

    void set_shadow_bias_max(float bias)
    {
      mShadowBiasMax = std::max(mShadowBiasMin, bias);
    }
    float shadow_bias_max() const { return mShadowBiasMax; }

    void set_shadow_filter_radius(int radius)
    {
      mShadowFilterRadius = std::clamp(radius, 0, 4);
    }
    int shadow_filter_radius() const { return mShadowFilterRadius; }

    const char* component_type() const override { return "Light"; }

  private:
    LightType mType = LightType::Point;
    glm::vec3 mColor { 1.0f, 1.0f, 1.0f };
    float mStrength = 100.0f;
    glm::vec3 mDirection { 0.0f, -1.0f, 0.0f };
    bool mEnabled = true;
    bool mCastsShadows = false;
    float mShadowBiasMin = 0.0008f;
    float mShadowBiasMax = 0.02f;
    int mShadowFilterRadius = 1;
  };

  /**
   * @brief 游戏场景类。
   *
   * 作为所有 Entity 的容器，管理实体的生命周期，提供实体的创建、查询和删除功能。
   * 提供 tick 接口以统一更新场景内所有活动实体的状态。
   */
  class Scene
  {
  public:
    explicit Scene(std::string name, std::shared_ptr<IResourceManager> resources);

    const std::string& name() const { return mName; }
    void set_name(const std::string& name) { mName = name; }

    std::shared_ptr<Entity> create_entity(const std::string& name);
    std::shared_ptr<Entity> create_mesh(const std::string& name);
    std::shared_ptr<Entity> create_terrain(const std::string& name);
    std::shared_ptr<Entity> create_light(const std::string& name);

    void remove_entity(uint64_t id);
    std::shared_ptr<Entity> find_entity(uint64_t id) const;
    std::shared_ptr<Entity> find_entity(const std::string& name) const;

    const std::vector<std::shared_ptr<Entity>>& entities() const { return mEntities; }

    void tick(float delta_time);

    void set_skybox_enabled(bool enabled) { mSkyboxEnabled = enabled; }
    bool skybox_enabled() const { return mSkyboxEnabled; }

  private:
    std::string mName;
    std::weak_ptr<IResourceManager> mResources;
    std::vector<std::shared_ptr<Entity>> mEntities;
    bool mSkyboxEnabled = true;
  };
}
