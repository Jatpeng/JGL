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

namespace nengine
{
  class IResourceManager;

  struct Transform
  {
    glm::vec3 position { 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale { 1.0f, 1.0f, 1.0f };

    glm::mat4 local_matrix() const;
  };

  class SceneObject
  {
  public:
    virtual ~SceneObject() = default;

    uint64_t id() const { return mId; }
    const std::string& name() const { return mName; }
    void set_name(const std::string& name) { mName = name; }

    Transform& transform() { return mTransform; }
    const Transform& transform() const { return mTransform; }

    virtual const char* object_type() const = 0;

  protected:
    SceneObject(uint64_t id, std::string name);

  private:
    uint64_t mId = 0;
    std::string mName;
    Transform mTransform;
  };

  class MeshObject final : public SceneObject
  {
  public:
    MeshObject(uint64_t id, std::string name, std::shared_ptr<IResourceManager> resources);
    ~MeshObject() override;

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

    const char* object_type() const override { return "Mesh"; }

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

  class LightObject final : public SceneObject
  {
  public:
    LightObject(uint64_t id, std::string name);

    void set_color(const glm::vec3& color) { mColor = color; }
    const glm::vec3& color() const { return mColor; }

    void set_strength(float strength) { mStrength = strength; }
    float strength() const { return mStrength; }

    void set_enabled(bool enabled) { mEnabled = enabled; }
    bool enabled() const { return mEnabled; }

    const char* object_type() const override { return "Light"; }

  private:
    glm::vec3 mColor { 1.0f, 1.0f, 1.0f };
    float mStrength = 100.0f;
    bool mEnabled = true;
  };

  class Scene
  {
  public:
    explicit Scene(std::string name, std::shared_ptr<IResourceManager> resources);

    const std::string& name() const { return mName; }
    void set_name(const std::string& name) { mName = name; }

    std::shared_ptr<MeshObject> create_mesh(const std::string& name);
    std::shared_ptr<LightObject> create_light(const std::string& name);
    void remove_object(uint64_t id);

    std::shared_ptr<SceneObject> find_object(uint64_t id) const;
    std::shared_ptr<SceneObject> find_object(const std::string& name) const;
    const std::vector<std::shared_ptr<SceneObject>>& objects() const { return mObjects; }

    void set_skybox_enabled(bool enabled) { mSkyboxEnabled = enabled; }
    bool skybox_enabled() const { return mSkyboxEnabled; }

  private:
    std::string mName;
    std::weak_ptr<IResourceManager> mResources;
    std::vector<std::shared_ptr<SceneObject>> mObjects;
    bool mSkyboxEnabled = true;
  };
}
