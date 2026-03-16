#include "pch.h"

#include "engine/scene.h"

#include <atomic>
#include <cctype>
#include <filesystem>

#include "elems/animation.h"
#include "elems/animator.h"
#include "elems/material.h"
#include "elems/model.h"
#include "engine/resource_manager.h"
#include "shader/shader_util.h"

namespace nengine
{
  namespace
  {
    std::atomic<uint64_t> gNextSceneObjectId { 1 };

    std::string to_lower_copy(std::string value)
    {
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
      {
        return static_cast<char>(std::tolower(c));
      });
      return value;
    }

    bool ends_with(const std::string& value, const std::string& suffix)
    {
      return value.size() >= suffix.size() &&
             value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
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

  glm::mat4 Transform::local_matrix() const
  {
    glm::mat4 result { 1.0f };
    result = glm::translate(result, position);
    result = glm::rotate(result, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    result = glm::rotate(result, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    result = glm::rotate(result, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    result = glm::scale(result, scale);
    return result;
  }

  SceneObject::SceneObject(uint64_t id, std::string name)
    : mId(id), mName(std::move(name))
  {
  }

  MeshObject::MeshObject(uint64_t id, std::string name, std::shared_ptr<IResourceManager> resources)
    : SceneObject(id, std::move(name)), mResources(std::move(resources))
  {
  }

  MeshObject::~MeshObject()
  {
    if (mShader && mShader->get_program_id() != 0)
      mShader->unload();
  }

  bool MeshObject::set_model(const std::string& path)
  {
    auto resources = mResources.lock();
    if (!resources)
      return false;

    const std::string resolved_path = resources->resolve_path(path);
    auto model = resources->load_model(resolved_path);
    if (!model)
      return false;

    mModel = std::move(model);
    mModelPath = resolved_path;
    mIsSkinned = mModel->GetIsSkinInModel();

    if (mIsSkinned)
    {
      mAnimation = std::make_unique<Animation>(resolved_path, mModel.get());
      mAnimator = std::make_unique<Animator>(mAnimation.get());

      if (mMaterialPath.empty())
        set_material("Assets/Anim.xml");
      else if (mMaterial)
        mMaterial->set_textures(mModel->GetTexturesMap());
    }
    else
    {
      mAnimation.reset();
      mAnimator.reset();
    }

    return true;
  }

  bool MeshObject::set_material(const std::string& path)
  {
    auto resources = mResources.lock();
    if (!resources)
      return false;

    const std::string resolved_path = resources->resolve_path(path);
    if (resolved_path.empty())
      return false;

    if (is_shader_source_path(resolved_path))
      return set_shader(resolved_path);

    if (!is_material_definition_path(resolved_path))
      return false;

    auto material = resources->load_material(resolved_path);
    if (!material)
      return false;

    if (mIsSkinned && mModel)
      material->set_textures(mModel->GetTexturesMap());

    mMaterial = std::move(material);
    mMaterialPath = resolved_path;

    if (mMaterial->getshaderPath().empty())
      return false;

    return set_shader(mMaterial->getshaderPath());
  }

  bool MeshObject::set_shader(const std::string& path)
  {
    auto resources = mResources.lock();
    if (!resources)
      return false;

    const std::string resolved_path = resources->resolve_path(path);
    if (resolved_path.empty())
      return false;

    if (is_material_definition_path(resolved_path))
      return set_material(resolved_path);
    if (!is_shader_source_path(resolved_path))
      return false;

    std::string shader_base_path;
    if (!split_shader_program_base(resolved_path, &shader_base_path))
      return false;

    const std::string vertex_path = shader_base_path + "_vs.shader";
    const std::string fragment_path = shader_base_path + "_fs.shader";

    auto shader = resources->load_shader_program(vertex_path, fragment_path);
    if (!shader || shader->get_program_id() == 0)
      return false;

    if (mShader && mShader->get_program_id() != 0)
      mShader->unload();

    mShader = std::move(shader);
    mShaderPath = resolved_path;
    mShaderName = std::filesystem::path(shader_base_path).stem().string();
    return true;
  }

  void MeshObject::tick(float delta_time)
  {
    if (mAnimator)
      mAnimator->UpdateAnimation(delta_time);
  }

  void MeshObject::apply_skinning(nshaders::Shader* shader) const
  {
    if (!shader || shader->get_program_id() == 0)
      return;

    shader->set_i1(mIsSkinned ? 1 : 0, "useSkinning");
    if (!mIsSkinned || !mAnimator)
      return;

    const auto transforms = mAnimator->GetFinalBoneMatrices();
    for (size_t i = 0; i < transforms.size(); ++i)
      shader->set_mat4(transforms[i], "finalBonesMatrices[" + std::to_string(i) + "]");
  }

  LightObject::LightObject(uint64_t id, std::string name)
    : SceneObject(id, std::move(name))
  {
  }

  Scene::Scene(std::string name, std::shared_ptr<IResourceManager> resources)
    : mName(std::move(name)), mResources(std::move(resources))
  {
  }

  std::shared_ptr<MeshObject> Scene::create_mesh(const std::string& name)
  {
    auto object = std::make_shared<MeshObject>(gNextSceneObjectId.fetch_add(1), name, mResources.lock());
    mObjects.push_back(object);
    return object;
  }

  std::shared_ptr<LightObject> Scene::create_light(const std::string& name)
  {
    auto object = std::make_shared<LightObject>(gNextSceneObjectId.fetch_add(1), name);
    mObjects.push_back(object);
    return object;
  }

  void Scene::remove_object(uint64_t id)
  {
    mObjects.erase(
      std::remove_if(mObjects.begin(), mObjects.end(), [id](const std::shared_ptr<SceneObject>& object)
      {
        return object && object->id() == id;
      }),
      mObjects.end());
  }

  std::shared_ptr<SceneObject> Scene::find_object(uint64_t id) const
  {
    const auto it = std::find_if(mObjects.begin(), mObjects.end(), [id](const std::shared_ptr<SceneObject>& object)
    {
      return object && object->id() == id;
    });
    return it != mObjects.end() ? *it : nullptr;
  }

  std::shared_ptr<SceneObject> Scene::find_object(const std::string& name) const
  {
    const auto it = std::find_if(mObjects.begin(), mObjects.end(), [&name](const std::shared_ptr<SceneObject>& object)
    {
      return object && object->name() == name;
    });
    return it != mObjects.end() ? *it : nullptr;
  }
}
