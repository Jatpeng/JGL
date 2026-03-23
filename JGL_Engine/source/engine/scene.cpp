#include "pch.h"

#include "engine/scene.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <limits>

#include "elems/animation.h"
#include "elems/animator.h"
#include "elems/material.h"
#include "elems/model.h"
#include "engine/animation_asset.h"
#include "engine/resource_manager.h"
#include "engine/core/entity.h"
#include "engine/core/transform.h"
#include "shader/shader_util.h"

namespace nengine
{
  namespace
  {
    std::atomic<uint64_t> gNextSceneObjectId { 1 };
    constexpr int kMaxRuntimeBones = 100;
    constexpr int kMinTerrainResolution = 2;
    constexpr int kMaxTerrainResolution = 512;
    constexpr int kMinTerrainOctaves = 1;
    constexpr int kMaxTerrainOctaves = 8;

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

    bool is_animation_asset_path(const std::string& path)
    {
      const std::string ext = to_lower_copy(std::filesystem::path(path).extension().string());
      return ext == ".janim";
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

    float clamp_positive(float value, float minimum)
    {
      return std::max(value, minimum);
    }

    int clamp_terrain_resolution(int value)
    {
      return std::clamp(value, kMinTerrainResolution, kMaxTerrainResolution);
    }

    int clamp_terrain_octaves(int value)
    {
      return std::clamp(value, kMinTerrainOctaves, kMaxTerrainOctaves);
    }

    float smooth_noise_weight(float value)
    {
      return value * value * (3.0f - 2.0f * value);
    }

    float lerp(float a, float b, float t)
    {
      return a + (b - a) * t;
    }

    uint32_t hash_grid(int x, int z, int seed)
    {
      uint32_t h = static_cast<uint32_t>(seed) ^ 0x9E3779B9u;
      h ^= static_cast<uint32_t>(x) * 0x85EBCA6Bu;
      h ^= static_cast<uint32_t>(z) * 0xC2B2AE35u;
      h ^= h >> 16;
      h *= 0x7FEB352Du;
      h ^= h >> 15;
      h *= 0x846CA68Bu;
      h ^= h >> 16;
      return h;
    }

    float random_grid_value(int x, int z, int seed)
    {
      constexpr float kRangeScale = 1.0f / static_cast<float>(0x00FFFFFFu);
      const uint32_t hashed = hash_grid(x, z, seed) & 0x00FFFFFFu;
      return static_cast<float>(hashed) * kRangeScale * 2.0f - 1.0f;
    }

    float value_noise(float x, float z, int seed)
    {
      const int x0 = static_cast<int>(std::floor(x));
      const int z0 = static_cast<int>(std::floor(z));
      const int x1 = x0 + 1;
      const int z1 = z0 + 1;

      const float tx = smooth_noise_weight(x - static_cast<float>(x0));
      const float tz = smooth_noise_weight(z - static_cast<float>(z0));

      const float v00 = random_grid_value(x0, z0, seed);
      const float v10 = random_grid_value(x1, z0, seed);
      const float v01 = random_grid_value(x0, z1, seed);
      const float v11 = random_grid_value(x1, z1, seed);

      const float v0 = lerp(v00, v10, tx);
      const float v1 = lerp(v01, v11, tx);
      return lerp(v0, v1, tz);
    }

    void initialize_static_vertex(nelems::VertexHolder* vertex)
    {
      if (!vertex)
        return;

      vertex->mTextureCoords = glm::vec2(0.0f, 0.0f);
      vertex->mTangent = glm::vec3(1.0f, 0.0f, 0.0f);
      vertex->mBitangent = glm::vec3(0.0f, 0.0f, 1.0f);
      for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
      {
        vertex->mBoneIDs[i] = -1;
        vertex->mWeights[i] = 0.0f;
      }
    }
  }

  MeshComponent::MeshComponent(std::shared_ptr<IResourceManager> resources)
    : mResources(std::move(resources))
  {
  }

  MeshComponent::~MeshComponent()
  {
    if (mShader && mShader->get_program_id() != 0)
      mShader->unload();
  }

  void MeshComponent::clear_animation_runtime()
  {
    mAnimation.reset();
    mAnimator.reset();
  }

  bool MeshComponent::bind_animation_source(const std::string& resolved_path, const std::string& clip_name, bool auto_embedded)
  {
    if (!mModel || !mIsSkinned || resolved_path.empty())
      return false;

    if (mModel->GetBoneCount() > kMaxRuntimeBones)
    {
      std::cout
        << "[MeshComponent] Skinned model exceeds shader bone limit ("
        << mModel->GetBoneCount()
        << " > "
        << kMaxRuntimeBones
        << "): "
        << mModelPath
        << std::endl;
      clear_animation_runtime();
      return false;
    }

    auto animation = std::make_unique<Animation>();
    if (!animation->load(resolved_path, mModel.get(), clip_name) || !animation->IsValid())
    {
      clear_animation_runtime();
      return false;
    }

    mAnimation = std::move(animation);
    mAnimator = std::make_unique<Animator>(mAnimation.get());
    mAnimator->StopAnimation();
    mAnimationPath = resolved_path;
    mAnimationClipName = mAnimation->GetClipName();
    if (mAnimationClipName.empty())
      mAnimationClipName = clip_name;
    mAnimationAutoEmbedded = auto_embedded;

    if (mMaterialPath.empty())
      set_material("Assets/materials/Anim.xml");
    else if (mMaterial && mModel)
      mMaterial->set_textures(mModel->GetTexturesMap());

    return true;
  }

  bool MeshComponent::set_model(const std::string& path)
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
    mAnimationPath.clear();
    mAnimationAssetPath.clear();
    mAnimationClipName.clear();
    mAnimationAutoEmbedded = false;
    mAnimationPlaying = true;
    mAnimationLoop = true;
    mAnimationSpeed = 1.0f;
    clear_animation_runtime();
    mIsSkinned = mModel->GetIsSkinInModel();

    if (mIsSkinned)
    {
      if (mMaterialPath.empty())
        set_material("Assets/materials/Anim.xml");
      else if (mMaterial)
        mMaterial->set_textures(mModel->GetTexturesMap());

      bind_animation_source(resolved_path, "", true);
    }

    return true;
  }

  bool MeshComponent::set_runtime_model(std::shared_ptr<nelems::Model> model, const std::string& debug_label)
  {
    if (!model)
      return false;

    mModel = std::move(model);
    mModelPath = debug_label.empty() ? "Generated" : debug_label;
    mAnimationPath.clear();
    mAnimationAssetPath.clear();
    mAnimationClipName.clear();
    mAnimationAutoEmbedded = false;
    mAnimationPlaying = false;
    mAnimationLoop = true;
    mAnimationSpeed = 1.0f;
    clear_animation_runtime();
    mIsSkinned = false;
    return true;
  }

  bool MeshComponent::set_animation(const std::string& path, const std::string& clip_name)
  {
    auto resources = mResources.lock();
    if (!resources)
      return false;

    const std::string resolved_path = resources->resolve_path(path);
    if (resolved_path.empty())
      return false;

    if (is_animation_asset_path(resolved_path) && clip_name.empty())
      return set_animation_asset(resolved_path);

    if (!mModel || !mIsSkinned)
      return false;

    const bool was_looping = mAnimationLoop;
    const float previous_speed = mAnimationSpeed;
    if (!bind_animation_source(resolved_path, clip_name, false))
      return false;

    mAnimationAssetPath.clear();
    mAnimationPlaying = true;
    mAnimationLoop = was_looping;
    mAnimationSpeed = previous_speed;
    return true;
  }

  bool MeshComponent::set_animation_asset(const std::string& path)
  {
    auto resources = mResources.lock();
    if (!resources)
      return false;

    AnimationAssetDefinition definition;
    std::string error;
    if (!load_animation_asset_resource(path, resources, &definition, &error))
    {
      std::cout << "[MeshComponent] " << error << std::endl;
      return false;
    }

    if (!definition.skeleton_path.empty())
    {
      if (!set_model(definition.skeleton_path))
        return false;
    }

    if (!mModel || !mIsSkinned)
      return false;

    if (!bind_animation_source(definition.source_path, definition.clip_name, false))
      return false;

    mAnimationAssetPath = resources->resolve_path(path);
    mAnimationLoop = definition.loop;
    mAnimationSpeed = definition.speed;
    mAnimationPlaying = definition.auto_play;
    if (!mAnimationPlaying)
      stop_animation();
    return true;
  }

  void MeshComponent::clear_animation()
  {
    mAnimationPath.clear();
    mAnimationAssetPath.clear();
    mAnimationClipName.clear();
    mAnimationAutoEmbedded = false;
    mAnimationPlaying = false;
    mAnimationLoop = true;
    mAnimationSpeed = 1.0f;
    clear_animation_runtime();
  }

  void MeshComponent::stop_animation()
  {
    mAnimationPlaying = false;
    if (mAnimator)
      mAnimator->StopAnimation();
  }

  bool MeshComponent::set_material(const std::string& path)
  {
    auto resources = mResources.lock();
    if (!resources)
      return false;

    const std::string resolved_path = resources->resolve_path(path);
    if (resolved_path.empty())
      return false;

    if (is_animation_asset_path(resolved_path))
      return set_animation_asset(resolved_path);

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

  bool MeshComponent::set_shader(const std::string& path)
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

  bool MeshComponent::reload_shader()
  {
    if (!mShader)
    {
      if (!mShaderPath.empty())
        return set_shader(mShaderPath);
      return false;
    }

    return mShader->reload();
  }

  void MeshComponent::tick(float delta_time)
  {
    if (mAnimator && mAnimationPlaying && mAnimationSpeed > 0.0f)
      mAnimator->UpdateAnimation(delta_time * mAnimationSpeed, mAnimationLoop);
  }

  void MeshComponent::apply_skinning(nshaders::Shader* shader) const
  {
    if (!shader || shader->get_program_id() == 0)
      return;

    const bool skinning_enabled = mIsSkinned && mAnimator;
    shader->set_i1(skinning_enabled ? 1 : 0, "useSkinning");
    if (!skinning_enabled)
      return;

    const auto transforms = mAnimator->GetFinalBoneMatrices();
    for (size_t i = 0; i < transforms.size(); ++i)
      shader->set_mat4(transforms[i], "finalBonesMatrices[" + std::to_string(i) + "]");
  }

  TerrainComponent::TerrainComponent(MeshComponent* mesh, std::shared_ptr<IResourceManager> resources)
    : mResources(std::move(resources)), mMesh(mesh)
  {
    if (mMesh && mMesh->material_path().empty())
      mMesh->set_material("Assets/materials/PBR.xml");

    rebuild();
  }

  void TerrainComponent::apply_settings(const TerrainComponent::Settings& settings, bool rebuild_immediately)
  {
    Settings sanitized = settings;
    sanitized.width = clamp_positive(sanitized.width, 1.0f);
    sanitized.depth = clamp_positive(sanitized.depth, 1.0f);
    sanitized.resolution_x = clamp_terrain_resolution(sanitized.resolution_x);
    sanitized.resolution_z = clamp_terrain_resolution(sanitized.resolution_z);
    sanitized.height_scale = std::max(0.0f, sanitized.height_scale);
    sanitized.uv_scale = clamp_positive(sanitized.uv_scale, 0.01f);
    sanitized.noise_frequency = clamp_positive(sanitized.noise_frequency, 0.0001f);
    sanitized.noise_octaves = clamp_terrain_octaves(sanitized.noise_octaves);
    sanitized.noise_persistence = std::clamp(sanitized.noise_persistence, 0.0f, 1.0f);
    sanitized.noise_lacunarity = clamp_positive(sanitized.noise_lacunarity, 1.0f);
    mSettings = sanitized;

    if (rebuild_immediately)
      rebuild();
  }

  size_t TerrainComponent::height_index(int x, int z) const
  {
    const int clamped_x = std::clamp(x, 0, mSettings.resolution_x);
    const int clamped_z = std::clamp(z, 0, mSettings.resolution_z);
    const size_t column_count = static_cast<size_t>(mSettings.resolution_x + 1);
    return static_cast<size_t>(clamped_z) * column_count + static_cast<size_t>(clamped_x);
  }

  float TerrainComponent::sample_height_grid(int x, int z) const
  {
    if (mHeights.empty())
      return mSettings.height_offset;

    return mHeights[height_index(x, z)];
  }

  float TerrainComponent::evaluate_height(float local_x, float local_z) const
  {
    float amplitude = 1.0f;
    float amplitude_sum = 0.0f;
    float frequency = mSettings.noise_frequency;
    float noise_sum = 0.0f;

    for (int octave = 0; octave < mSettings.noise_octaves; ++octave)
    {
      noise_sum += value_noise(local_x * frequency, local_z * frequency, mSettings.seed + octave * 1013) * amplitude;
      amplitude_sum += amplitude;
      amplitude *= mSettings.noise_persistence;
      frequency *= mSettings.noise_lacunarity;
    }

    const float normalized_noise = amplitude_sum > 0.0f ? noise_sum / amplitude_sum : 0.0f;
    return mSettings.height_offset + normalized_noise * mSettings.height_scale;
  }

  bool TerrainComponent::rebuild()
  {
    if (!mMesh)
      return false;

    const int columns = mSettings.resolution_x + 1;
    const int rows = mSettings.resolution_z + 1;
    const float x_step = mSettings.width / static_cast<float>(mSettings.resolution_x);
    const float z_step = mSettings.depth / static_cast<float>(mSettings.resolution_z);
    const float half_width = mSettings.width * 0.5f;
    const float half_depth = mSettings.depth * 0.5f;

    mHeights.assign(static_cast<size_t>(columns * rows), 0.0f);

    float min_height = std::numeric_limits<float>::max();
    float max_height = std::numeric_limits<float>::lowest();

    for (int z = 0; z < rows; ++z)
    {
      for (int x = 0; x < columns; ++x)
      {
        const float u = static_cast<float>(x) / static_cast<float>(mSettings.resolution_x);
        const float v = static_cast<float>(z) / static_cast<float>(mSettings.resolution_z);
        const float local_x = -half_width + u * mSettings.width;
        const float local_z = -half_depth + v * mSettings.depth;
        const float height = evaluate_height(local_x, local_z);

        mHeights[height_index(x, z)] = height;
        min_height = std::min(min_height, height);
        max_height = std::max(max_height, height);
      }
    }

    std::vector<nelems::VertexHolder> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(static_cast<size_t>(columns * rows));
    indices.reserve(static_cast<size_t>(mSettings.resolution_x * mSettings.resolution_z * 6));

    for (int z = 0; z < rows; ++z)
    {
      for (int x = 0; x < columns; ++x)
      {
        const float u = static_cast<float>(x) / static_cast<float>(mSettings.resolution_x);
        const float v = static_cast<float>(z) / static_cast<float>(mSettings.resolution_z);
        const float local_x = -half_width + u * mSettings.width;
        const float local_z = -half_depth + v * mSettings.depth;

        nelems::VertexHolder vertex;
        initialize_static_vertex(&vertex);
        vertex.mPos = glm::vec3(local_x, sample_height_grid(x, z), local_z);
        vertex.mTextureCoords = glm::vec2(u, v) * mSettings.uv_scale;

        const float left = sample_height_grid(x - 1, z);
        const float right = sample_height_grid(x + 1, z);
        const float down = sample_height_grid(x, z - 1);
        const float up = sample_height_grid(x, z + 1);

        const float tangent_span = x_step * static_cast<float>((x > 0 ? 1 : 0) + (x < mSettings.resolution_x ? 1 : 0));
        const float bitangent_span = z_step * static_cast<float>((z > 0 ? 1 : 0) + (z < mSettings.resolution_z ? 1 : 0));

        glm::vec3 tangent(
          tangent_span > 0.0f ? tangent_span : x_step,
          right - left,
          0.0f);
        glm::vec3 bitangent(
          0.0f,
          up - down,
          bitangent_span > 0.0f ? bitangent_span : z_step);

        if (glm::length(tangent) <= 0.0001f)
          tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        if (glm::length(bitangent) <= 0.0001f)
          bitangent = glm::vec3(0.0f, 0.0f, 1.0f);

        vertex.mTangent = glm::normalize(tangent);

        glm::vec3 raw_normal = glm::cross(bitangent, tangent);
        if (glm::length(raw_normal) <= 0.0001f)
          raw_normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.mNormal = glm::normalize(raw_normal);

        glm::vec3 raw_bitangent = glm::cross(vertex.mNormal, vertex.mTangent);
        if (glm::length(raw_bitangent) <= 0.0001f)
          raw_bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        vertex.mBitangent = glm::normalize(raw_bitangent);

        vertices.push_back(vertex);
      }
    }

    for (int z = 0; z < mSettings.resolution_z; ++z)
    {
      for (int x = 0; x < mSettings.resolution_x; ++x)
      {
        const unsigned int i0 = static_cast<unsigned int>(z * columns + x);
        const unsigned int i1 = i0 + 1;
        const unsigned int i2 = i0 + static_cast<unsigned int>(columns);
        const unsigned int i3 = i2 + 1;

        indices.push_back(i0);
        indices.push_back(i2);
        indices.push_back(i1);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i3);
      }
    }

    std::vector<nelems::Mesh> meshes;
    meshes.emplace_back(vertices, indices);

    auto model = std::make_shared<nelems::Model>(
      std::move(meshes),
      glm::vec3(-half_width, min_height, -half_depth),
      glm::vec3(half_width, max_height, half_depth));

    if (!mMesh->set_runtime_model(std::move(model), "Procedural Terrain"))
      return false;

    if (mMesh->material_path().empty())
      return mMesh->set_material("Assets/materials/PBR.xml");

    return true;
  }

  float TerrainComponent::sample_height(float local_x, float local_z) const
  {
    if (mHeights.empty())
      return mSettings.height_offset;

    const float normalized_x = std::clamp((local_x / mSettings.width) + 0.5f, 0.0f, 1.0f);
    const float normalized_z = std::clamp((local_z / mSettings.depth) + 0.5f, 0.0f, 1.0f);

    const float grid_x = normalized_x * static_cast<float>(mSettings.resolution_x);
    const float grid_z = normalized_z * static_cast<float>(mSettings.resolution_z);

    const int x0 = static_cast<int>(std::floor(grid_x));
    const int z0 = static_cast<int>(std::floor(grid_z));
    const int x1 = std::min(x0 + 1, mSettings.resolution_x);
    const int z1 = std::min(z0 + 1, mSettings.resolution_z);

    const float tx = grid_x - static_cast<float>(x0);
    const float tz = grid_z - static_cast<float>(z0);

    const float h00 = sample_height_grid(x0, z0);
    const float h10 = sample_height_grid(x1, z0);
    const float h01 = sample_height_grid(x0, z1);
    const float h11 = sample_height_grid(x1, z1);

    const float hx0 = lerp(h00, h10, tx);
    const float hx1 = lerp(h01, h11, tx);
    return lerp(hx0, hx1, tz);
  }

  void TerrainComponent::set_width(float value)
  {
    auto updated = mSettings;
    updated.width = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_depth(float value)
  {
    auto updated = mSettings;
    updated.depth = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_resolution_x(int value)
  {
    auto updated = mSettings;
    updated.resolution_x = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_resolution_z(int value)
  {
    auto updated = mSettings;
    updated.resolution_z = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_height_scale(float value)
  {
    auto updated = mSettings;
    updated.height_scale = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_height_offset(float value)
  {
    auto updated = mSettings;
    updated.height_offset = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_uv_scale(float value)
  {
    auto updated = mSettings;
    updated.uv_scale = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_noise_frequency(float value)
  {
    auto updated = mSettings;
    updated.noise_frequency = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_noise_octaves(int value)
  {
    auto updated = mSettings;
    updated.noise_octaves = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_noise_persistence(float value)
  {
    auto updated = mSettings;
    updated.noise_persistence = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_noise_lacunarity(float value)
  {
    auto updated = mSettings;
    updated.noise_lacunarity = value;
    apply_settings(updated);
  }

  void TerrainComponent::set_seed(int value)
  {
    auto updated = mSettings;
    updated.seed = value;
    apply_settings(updated);
  }

  Scene::Scene(std::string name, std::shared_ptr<IResourceManager> resources)
    : mName(std::move(name)), mResources(std::move(resources))
  {
  }

  std::shared_ptr<Entity> Scene::create_entity(const std::string& name)
  {
    auto entity = std::make_shared<Entity>(gNextSceneObjectId.fetch_add(1), name);
    entity->add_component<TransformComponent>();
    mEntities.push_back(entity);
    return entity;
  }

  std::shared_ptr<Entity> Scene::create_mesh(const std::string& name)
  {
    auto entity = create_entity(name);
    entity->add_component<MeshComponent>(mResources.lock());
    return entity;
  }

  std::shared_ptr<Entity> Scene::create_terrain(const std::string& name)
  {
    auto entity = create_mesh(name);
    auto* mesh = entity->get_component<MeshComponent>();
    entity->add_component<TerrainComponent>(mesh, mResources.lock());
    return entity;
  }

  std::shared_ptr<Entity> Scene::create_light(const std::string& name)
  {
    auto entity = create_entity(name);
    entity->add_component<LightComponent>();
    return entity;
  }

  void Scene::remove_entity(uint64_t id)
  {
    mEntities.erase(
      std::remove_if(mEntities.begin(), mEntities.end(), [id](const std::shared_ptr<Entity>& entity)
      {
        return entity && entity->id() == id;
      }),
      mEntities.end());
  }

  std::shared_ptr<Entity> Scene::find_entity(uint64_t id) const
  {
    const auto it = std::find_if(mEntities.begin(), mEntities.end(), [id](const std::shared_ptr<Entity>& entity)
    {
      return entity && entity->id() == id;
    });
    return it != mEntities.end() ? *it : nullptr;
  }

  std::shared_ptr<Entity> Scene::find_entity(const std::string& name) const
  {
    const auto it = std::find_if(mEntities.begin(), mEntities.end(), [&name](const std::shared_ptr<Entity>& entity)
    {
      return entity && entity->name() == name;
    });
    return it != mEntities.end() ? *it : nullptr;
  }

  void Scene::tick(float delta_time)
  {
    for (auto& entity : mEntities)
    {
      if (auto mesh_comp = entity->get_component<MeshComponent>())
      {
        mesh_comp->tick(delta_time);
      }
    }
  }
}
