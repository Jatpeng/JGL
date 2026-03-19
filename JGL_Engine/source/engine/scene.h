#pragma once

#include <cstdint>
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
    bool set_material(const std::string& path);
    bool set_shader(const std::string& path);

    std::shared_ptr<nelems::Model> model() const { return mModel; }
    std::shared_ptr<Material> material() const { return mMaterial; }
    nshaders::Shader* shader() const { return mShader.get(); }

    const std::string& model_path() const { return mModelPath; }
    const std::string& material_path() const { return mMaterialPath; }
    const std::string& shader_path() const { return mShaderPath; }
    const std::string& shader_name() const { return mShaderName; }

    bool is_skinned() const { return mIsSkinned; }
    void tick(float delta_time);
    void apply_skinning(nshaders::Shader* shader) const;

    const char* component_type() const override { return "Mesh"; }

  private:
    std::weak_ptr<IResourceManager> mResources;
    std::shared_ptr<nelems::Model> mModel;
    std::shared_ptr<Material> mMaterial;
    std::unique_ptr<nshaders::Shader> mShader;
    std::string mModelPath;
    std::string mMaterialPath;
    std::string mShaderPath;
    std::string mShaderName;
    bool mIsSkinned = false;
    std::unique_ptr<Animation> mAnimation;
    std::unique_ptr<Animator> mAnimator;
  };

  /**
   * @brief 光照组件，用于定义点光源或方向光的属性。
   */
  class LightComponent : public IComponent
  {
  public:
    LightComponent() = default;

    void set_color(const glm::vec3& color) { mColor = color; }
    const glm::vec3& color() const { return mColor; }

    void set_strength(float strength) { mStrength = strength; }
    float strength() const { return mStrength; }

    void set_enabled(bool enabled) { mEnabled = enabled; }
    bool enabled() const { return mEnabled; }

    const char* component_type() const override { return "Light"; }

  private:
    glm::vec3 mColor { 1.0f, 1.0f, 1.0f };
    float mStrength = 100.0f;
    bool mEnabled = true;
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