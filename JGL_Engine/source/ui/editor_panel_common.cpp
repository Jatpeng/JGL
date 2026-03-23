#include "pch.h"

#include "ui/editor_panel_common.h"

#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <functional>

#include "elems/material.h"
#include "engine/render_engine.h"
#include "engine/scene.h"
#include "utils/filesystem.h"

#ifdef _WIN32
#include <commdlg.h>
#endif

namespace nui
{
  namespace
  {
#ifdef _WIN32
    std::wstring wide_from_narrow(const std::string& narrow)
    {
#ifdef _WIN32
      if (narrow.empty())
        return std::wstring();

      const int size = MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, nullptr, 0);
      if (size <= 1)
        return {};

      std::vector<wchar_t> buffer(static_cast<size_t>(size), L'\0');
      MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, buffer.data(), size);
      return std::wstring(buffer.data());
#else
      return std::wstring(narrow.begin(), narrow.end());
#endif
    }

    std::string narrow_from_wide(const std::wstring& wide)
    {
#ifdef _WIN32
      if (wide.empty())
        return std::string();

      const int size = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
      if (size <= 1)
        return {};

      std::vector<char> buffer(static_cast<size_t>(size), '\0');
      WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, buffer.data(), size, nullptr, nullptr);
      return std::string(buffer.data());
#else
      return std::string(wide.begin(), wide.end());
#endif
    }
#endif

    std::string to_lower_copy(std::string value)
    {
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
      {
        return static_cast<char>(std::tolower(c));
      });
      return value;
    }

    std::string normalize_ui_path(std::string path)
    {
      std::replace(path.begin(), path.end(), '\\', '/');
      return path;
    }

    bool use_color_editor_for_param(const std::string& param_name)
    {
      std::string lower_name = param_name;
      std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), [](unsigned char c)
      {
        return static_cast<char>(std::tolower(c));
      });

      return lower_name == "color" ||
             lower_name.find("color") != std::string::npos ||
             lower_name.find("colour") != std::string::npos;
    }

    std::vector<std::string> list_project_files(
      const std::vector<std::string>& relative_roots,
      const std::vector<std::string>& extensions,
      const std::function<bool(const std::filesystem::directory_entry&)>& filter = {})
    {
      std::vector<std::string> paths;
      std::vector<std::string> lower_extensions;
      lower_extensions.reserve(extensions.size());
      for (const auto& ext : extensions)
        lower_extensions.push_back(to_lower_copy(ext));

      for (const auto& relative_root : relative_roots)
      {
        const std::filesystem::path root(FileSystem::getPath(relative_root));
        if (!std::filesystem::exists(root))
          continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
        {
          if (!entry.is_regular_file())
            continue;

          const std::string entry_extension = to_lower_copy(entry.path().extension().string());
          if (!lower_extensions.empty() &&
              std::find(lower_extensions.begin(), lower_extensions.end(), entry_extension) == lower_extensions.end())
          {
            continue;
          }

          if (filter && !filter(entry))
            continue;

          paths.push_back(normalize_ui_path(entry.path().string()));
        }
      }

      std::sort(paths.begin(), paths.end());
      paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
      return paths;
    }

    std::shared_ptr<nengine::Entity> resolve_selected_entity(
      const std::shared_ptr<nengine::Scene>& scene,
      uint64_t* selected_id)
    {
      if (!scene || !selected_id)
        return nullptr;

      auto selected = scene->find_entity(*selected_id);
      if (selected)
        return selected;

      if (!scene->entities().empty())
      {
        *selected_id = scene->entities().front()->id();
        return scene->entities().front();
      }

      *selected_id = 0;
      return nullptr;
    }
  }

  std::optional<std::string> open_native_file_dialog(
    const wchar_t* title,
    const wchar_t* filter,
    const std::string& initial_relative_dir)
  {
#ifdef _WIN32
    OPENFILENAMEW dialog = {};
    std::array<wchar_t, 4096> filename = {};
    const std::wstring initial_dir = wide_from_narrow(FileSystem::getPath(initial_relative_dir));

    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFile = filename.data();
    dialog.nMaxFile = static_cast<DWORD>(filename.size());
    dialog.lpstrFilter = filter;
    dialog.lpstrTitle = title;
    dialog.lpstrInitialDir = initial_dir.empty() ? nullptr : initial_dir.c_str();
    dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&dialog) == 0)
      return {};

    return narrow_from_wide(filename.data());
#else
    (void)title;
    (void)filter;
    (void)initial_relative_dir;
    return {};
#endif
  }

  std::optional<std::string> save_native_file_dialog(
    const wchar_t* title,
    const wchar_t* filter,
    const wchar_t* default_extension,
    const std::string& initial_relative_dir,
    const std::string& default_filename)
  {
#ifdef _WIN32
    OPENFILENAMEW dialog = {};
    std::array<wchar_t, 4096> filename = {};
    const std::wstring initial_dir = wide_from_narrow(FileSystem::getPath(initial_relative_dir));
    const std::wstring initial_name = wide_from_narrow(default_filename);

    if (!initial_name.empty())
    {
      const size_t copy_count = std::min(initial_name.size(), filename.size() - 1);
      std::copy_n(initial_name.begin(), copy_count, filename.begin());
      filename[copy_count] = L'\0';
    }

    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFile = filename.data();
    dialog.nMaxFile = static_cast<DWORD>(filename.size());
    dialog.lpstrFilter = filter;
    dialog.lpstrTitle = title;
    dialog.lpstrInitialDir = initial_dir.empty() ? nullptr : initial_dir.c_str();
    dialog.lpstrDefExt = default_extension;
    dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&dialog) == 0)
      return {};

    return narrow_from_wide(filename.data());
#else
    (void)title;
    (void)filter;
    (void)default_extension;
    (void)initial_relative_dir;
    (void)default_filename;
    return {};
#endif
  }

  std::string file_label(const std::string& path)
  {
    if (path.empty())
      return std::string("<empty>");

    return std::filesystem::path(path).filename().string();
  }

  std::string display_project_path(const std::string& path)
  {
    if (path.empty())
      return std::string("<empty>");

    namespace fs = std::filesystem;

    std::error_code ec;
    fs::path absolute_path = fs::path(path);
    if (!absolute_path.is_absolute())
      absolute_path = fs::path(FileSystem::getPath(path));

    const fs::path project_root = fs::path(FileSystem::getPath("Assets")).parent_path();
    const fs::path relative_path = fs::relative(absolute_path, project_root, ec);
    if (!ec && !relative_path.empty())
    {
      const std::string relative = normalize_ui_path(relative_path.string());
      if (relative != "." && relative.rfind("../", 0) != 0 && relative.rfind("..\\", 0) != 0)
        return relative;
    }

    return normalize_ui_path(absolute_path.string());
  }

  std::vector<std::string> list_mesh_presets()
  {
    return list_project_files(
      { "Assets/models", "Assets/built_in" },
      { ".fbx", ".obj", ".dae", ".blend", ".gltf", ".glb" });
  }

  std::vector<std::string> list_material_presets()
  {
    return list_project_files({ "Assets/materials" }, { ".xml", ".mtl" });
  }

  std::vector<std::string> list_animation_presets()
  {
    return list_project_files({ "Assets/animations" }, { ".janim" });
  }

  std::vector<std::string> list_environment_maps()
  {
    return list_project_files({ "Assets" }, { ".hdr" });
  }

  std::vector<std::string> list_mesh_shader_programs()
  {
    return list_project_files(
      { "JGL_Engine/shaders" },
      { ".shader" },
      [](const std::filesystem::directory_entry& entry)
      {
        const std::string filename = to_lower_copy(entry.path().filename().string());
        const std::string generic_path = to_lower_copy(normalize_ui_path(entry.path().generic_string()));
        const std::string suffix = "_fs.shader";

        if (filename.size() <= suffix.size() ||
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) != 0)
        {
          return false;
        }

        if (filename.rfind("deferred_", 0) == 0 ||
            filename.rfind("shadow_depth_", 0) == 0 ||
            filename.rfind("screen_effect_", 0) == 0 ||
            filename == "bloom_fs.shader")
        {
          return false;
        }

        if (generic_path.find("/buit_in/") != std::string::npos)
          return false;

        const std::string base_name = entry.path().filename().string().substr(
          0,
          entry.path().filename().string().size() - suffix.size());
        const std::filesystem::path vertex_path = entry.path().parent_path() / (base_name + "_vs.shader");
        return std::filesystem::exists(vertex_path);
      });
  }

  std::vector<std::string> list_screen_effect_materials()
  {
    return list_project_files({ "Assets/screen_effects" }, { ".xml", ".mtl" });
  }

  glm::vec3 direction_to_rotation(const glm::vec3& direction)
  {
    if (glm::length(direction) <= 0.0001f)
      return glm::vec3(0.0f);

    const glm::vec3 normalized_direction = glm::normalize(direction);
    const float pitch = glm::degrees(std::asin(glm::clamp(normalized_direction.y, -1.0f, 1.0f)));
    const float yaw = glm::degrees(std::atan2(-normalized_direction.x, -normalized_direction.z));
    return glm::vec3(pitch, yaw, 0.0f);
  }

  glm::vec3 rotation_to_direction(const glm::vec3& rotation)
  {
    glm::mat4 rotation_matrix(1.0f);
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::vec3 direction = glm::vec3(rotation_matrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    if (glm::length(direction) <= 0.0001f)
      return glm::vec3(0.0f, -1.0f, 0.0f);

    return glm::normalize(direction);
  }

  void sync_directional_light_transform_from_direction(
    nengine::TransformComponent* transform,
    const nengine::LightComponent* light)
  {
    if (!transform || !light || light->type() != nengine::LightComponent::LightType::Directional)
      return;

    transform->rotation = direction_to_rotation(light->direction());
  }

  void sync_directional_light_direction_from_transform(
    const nengine::TransformComponent* transform,
    nengine::LightComponent* light)
  {
    if (!transform || !light || light->type() != nengine::LightComponent::LightType::Directional)
      return;

    light->set_direction(rotation_to_direction(transform->rotation));
  }

  void draw_material_parameter_editor(const std::shared_ptr<Material>& material)
  {
    if (!material)
      return;

    if (ImGui::TreeNodeEx("Scalars", ImGuiTreeNodeFlags_DefaultOpen))
    {
      for (auto& it : material->getFloatMap())
      {
        float tmp = it.second;
        if (ImGui::SliderFloat(it.first.c_str(), &tmp, -100.0f, 100.0f))
          it.second = tmp;
      }
      if (material->getFloatMap().empty())
        ImGui::TextDisabled("No editable scalar parameters.");
      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Vector2", ImGuiTreeNodeFlags_DefaultOpen))
    {
      for (auto& it : material->getFloat2Map())
      {
        glm::vec2 tmp = it.second;
        if (ImGui::SliderFloat2(it.first.c_str(), (float*)&tmp, -100.0f, 100.0f))
          it.second = tmp;
      }
      if (material->getFloat2Map().empty())
        ImGui::TextDisabled("No editable float2 parameters.");
      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Vector3", ImGuiTreeNodeFlags_DefaultOpen))
    {
      for (auto& it : material->getFloat3Map())
      {
        glm::vec3 tmp = it.second;
        const bool is_color_param = use_color_editor_for_param(it.first);
        const bool changed = is_color_param
          ? ImGui::ColorEdit3(it.first.c_str(), (float*)&tmp)
          : ImGui::SliderFloat3(it.first.c_str(), (float*)&tmp, -100.0f, 100.0f);

        if (changed)
          it.second = tmp;
      }
      if (material->getFloat3Map().empty())
        ImGui::TextDisabled("No editable float3 parameters.");
      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Texture Slots", ImGuiTreeNodeFlags_DefaultOpen))
    {
      for (const auto& it : material->getTextureMap())
      {
        const std::string resolved_name = file_label(it.second.second);
        ImGui::BulletText("%s", it.first.c_str());
        ImGui::SameLine(140.0f);
        ImGui::TextUnformatted(resolved_name.c_str());

        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::TextWrapped("%s", it.second.second.c_str());
          ImGui::EndTooltip();
        }
      }
      if (material->getTextureMap().empty())
        ImGui::TextDisabled("No texture slots.");
      ImGui::TreePop();
    }
  }

  void begin_disabled(bool disabled)
  {
    if (!disabled)
      return;

    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  void end_disabled(bool disabled)
  {
    if (!disabled)
      return;

    ImGui::PopStyleVar();
    ImGui::PopItemFlag();
  }

  EditorPanelContext build_editor_panel_context(
    nengine::RenderEngine* engine,
    EditorPanelState* state)
  {
    EditorPanelContext context;
    context.engine = engine;
    if (!engine || !state)
      return context;

    context.scene = engine->get_scene();
    context.selected_entity = resolve_selected_entity(context.scene, &state->selected_object_id);
    if (context.selected_entity)
    {
      context.selected_mesh = context.selected_entity->get_component<nengine::MeshComponent>();
      context.selected_terrain = context.selected_entity->get_component<nengine::TerrainComponent>();
      context.selected_light = context.selected_entity->get_component<nengine::LightComponent>();
    }

    context.deferred_requested = engine->get_render_mode() == nengine::RenderEngine::RenderMode::Deferred;
    context.deferred_available = engine->is_deferred_available();
    context.model_presets = list_mesh_presets();
    context.material_presets = list_material_presets();
    context.animation_presets = list_animation_presets();
    context.shader_presets = list_mesh_shader_programs();
    context.environment_maps = list_environment_maps();
    context.screen_effect_materials = list_screen_effect_materials();

    if (state->new_mesh_model_path.empty() && !context.model_presets.empty())
      state->new_mesh_model_path = context.model_presets.front();
    if (state->new_mesh_material_path.empty() && !context.material_presets.empty())
      state->new_mesh_material_path = context.material_presets.front();

    return context;
  }
}
