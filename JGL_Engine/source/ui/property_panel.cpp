#include "pch.h"

#include "ui/property_panel.h"

#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include <array>
#include <cctype>
#include <filesystem>
#include <optional>

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

    std::optional<std::string> open_native_file_dialog(
      const wchar_t* title,
      const wchar_t* filter,
      const std::string& initial_relative_dir = "")
    {
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
    }
#else
    std::optional<std::string> open_native_file_dialog(
      const wchar_t* title,
      const wchar_t* filter,
      const std::string& initial_relative_dir = "")
    {
      (void)title;
      (void)filter;
      (void)initial_relative_dir;
      return {};
    }
#endif

    std::string file_label(const std::string& path)
    {
      if (path.empty())
        return std::string("<empty>");

      return std::filesystem::path(path).filename().string();
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

    std::vector<std::string> list_screen_effect_materials()
    {
      std::vector<std::string> material_paths;
      const std::filesystem::path effects_root(FileSystem::getPath("Assets/screen_effects"));
      if (!std::filesystem::exists(effects_root))
        return material_paths;

      for (const auto& entry : std::filesystem::directory_iterator(effects_root))
      {
        if (!entry.is_regular_file())
          continue;

        const std::string ext = entry.path().extension().string();
        if (ext == ".xml" || ext == ".mtl")
          material_paths.push_back(entry.path().string());
      }

      std::sort(material_paths.begin(), material_paths.end());
      return material_paths;
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

  Property_Panel::Property_Panel()
  {
    mCurrentShaderFile = "pbr_fs.shader";
    mCurrentMaterialFile = "PBR.xml";
    mCurrentMeshFile = "cube.fbx";
  }

  void Property_Panel::render(nengine::RenderEngine* engine)
  {
    if (!engine)
      return;

    auto scene = engine->get_scene();
    auto selected_entity = resolve_selected_entity(scene, &mSelectedObjectId);
    nengine::MeshComponent* selected_mesh = nullptr;
    nengine::LightComponent* selected_light = nullptr;

    if (selected_entity)
    {
      selected_mesh = selected_entity->get_component<nengine::MeshComponent>();
      selected_light = selected_entity->get_component<nengine::LightComponent>();
    }

    if (selected_mesh)
    {
      if (!selected_mesh->model_path().empty())
        mCurrentMeshFile = file_label(selected_mesh->model_path());
      if (!selected_mesh->shader_path().empty())
        mCurrentShaderFile = file_label(selected_mesh->shader_path());
      if (!selected_mesh->material_path().empty())
        mCurrentMaterialFile = file_label(selected_mesh->material_path());
    }

    const bool deferred_requested = engine->get_render_mode() == nengine::RenderEngine::RenderMode::Deferred;
    const bool deferred_available = engine->is_deferred_available();

    ImGui::SetNextWindowSize(ImVec2(380.0f, 680.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Properties");

    if (ImGui::Button("Reset Camera"))
      engine->reset_view();

    ImGui::SameLine();
    bool show_plane = engine->is_plane_show();
    if (ImGui::Checkbox("Plane##PropsQuick", &show_plane))
      engine->set_plane_show(show_plane);

    ImGui::SameLine();
    ImGui::TextDisabled("RMB Orbit | MMB Pan | W/S Zoom | F Reset");

    if (!scene)
    {
      ImGui::Separator();
      ImGui::TextDisabled("No active scene.");
    }

    if (scene && ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool skybox_enabled = scene->skybox_enabled();
      if (ImGui::Checkbox("Skybox", &skybox_enabled))
        scene->set_skybox_enabled(skybox_enabled);

      if (ImGui::Button("Add Mesh"))
      {
        auto mesh = scene->create_mesh("mesh_" + std::to_string(scene->entities().size() + 1));
        mesh->get_component<nengine::MeshComponent>()->set_model("Assets/models/cube.fbx");
        mesh->get_component<nengine::MeshComponent>()->set_material("Assets/materials/PBR.xml");
        mSelectedObjectId = mesh->id();
      }

      ImGui::SameLine();
      if (ImGui::Button("Add Light"))
      {
        auto light = scene->create_light("light_" + std::to_string(scene->entities().size() + 1));
        light->get_component<nengine::TransformComponent>()->position = glm::vec3(1.5f, 3.5f, 3.0f);
        light->get_component<nengine::LightComponent>()->set_strength(100.0f);
        mSelectedObjectId = light->id();
      }

      ImGui::SameLine();
      begin_disabled(!selected_entity);
      if (ImGui::Button("Remove Selected") && selected_entity)
      {
        scene->remove_entity(selected_entity->id());
        mSelectedObjectId = 0;
        selected_entity.reset();
        selected_mesh = nullptr;
        selected_light = nullptr;
      }
      end_disabled(!selected_entity);

      ImGui::Separator();
      ImGui::TextDisabled("Objects");
      for (const auto& entity : scene->entities())
      {
        const bool is_selected = selected_entity && entity->id() == selected_entity->id();
        const char* type_name = "Entity";
        if (entity->get_component<nengine::MeshComponent>()) type_name = "Mesh";
        else if (entity->get_component<nengine::LightComponent>()) type_name = "Light";

        std::string label = std::string(type_name) + "##" + std::to_string(entity->id());
        if (ImGui::Selectable((entity->name() + "###" + label).c_str(), is_selected))
          mSelectedObjectId = entity->id();

        ImGui::SameLine(260.0f);
        ImGui::TextDisabled("%s", type_name);
      }
    }

    if (selected_entity && ImGui::CollapsingHeader("Selected Object", ImGuiTreeNodeFlags_DefaultOpen))
    {
      std::string name = selected_entity->name();
      if (ImGui::InputText("Name", &name))
        selected_entity->set_name(name);

      const char* type_name = "Entity";
      if (selected_mesh) type_name = "Mesh";
      else if (selected_light) type_name = "Light";

      ImGui::TextDisabled("Id %llu | Type %s",
                          static_cast<unsigned long long>(selected_entity->id()),
                          type_name);

      auto transform = selected_entity->get_component<nengine::TransformComponent>();
      if (transform && ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
      {
        if (ImGui::SliderFloat3("Position", (float*)&transform->position, -20.0f, 20.0f))
          transform->position = transform->position;
        if (ImGui::SliderFloat3("Rotation", (float*)&transform->rotation, -180.0f, 180.0f))
          transform->rotation = transform->rotation;
        if (ImGui::SliderFloat3("Scale", (float*)&transform->scale, 0.01f, 20.0f))
          transform->scale = transform->scale;
        ImGui::TreePop();
      }
    }

    if (selected_mesh && ImGui::CollapsingHeader("Mesh Assets", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TextDisabled("Mesh");
      ImGui::SameLine();
      ImGui::TextUnformatted(mCurrentMeshFile.c_str());
      ImGui::TextDisabled("Shader");
      ImGui::SameLine();
      ImGui::TextUnformatted(mCurrentShaderFile.c_str());
      ImGui::TextDisabled("Material");
      ImGui::SameLine();
      ImGui::TextUnformatted(mCurrentMaterialFile.c_str());

      if (ImGui::Button("Load Mesh..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Mesh",
          L"Mesh Files (*.fbx;*.obj;*.dae)\0*.fbx;*.obj;*.dae\0All Files (*.*)\0*.*\0",
          "Assets/models");
        if (file_path && selected_mesh->set_model(*file_path))
          mCurrentMeshFile = file_label(*file_path);
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentMeshFile.c_str());

      if (ImGui::Button("Load Shader..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Shader",
          L"Shader Files (*_vs.shader;*_fs.shader)\0*_vs.shader;*_fs.shader\0All Files (*.*)\0*.*\0",
          "Shaders");
        if (file_path && selected_mesh->set_shader(*file_path))
          mCurrentShaderFile = file_label(selected_mesh->shader_path());
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentShaderFile.c_str());

      if (ImGui::Button("Load Material..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Material",
          L"Material Files (*.mtl;*.xml)\0*.mtl;*.xml\0All Files (*.*)\0*.*\0",
          "Assets/materials");
        if (file_path && selected_mesh->set_material(*file_path))
        {
          mCurrentMaterialFile = file_label(selected_mesh->material_path());
          mCurrentShaderFile = file_label(selected_mesh->shader_path());
        }
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentMaterialFile.c_str());
    }

    if (selected_mesh &&
        selected_mesh->model() &&
        selected_mesh->material() &&
        ImGui::CollapsingHeader("Material Parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
      draw_material_parameter_editor(selected_mesh->material());
    }

    if (selected_light && ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool enabled = selected_light->enabled();
      if (ImGui::Checkbox("Enabled", &enabled))
        selected_light->set_enabled(enabled);

      glm::vec3 color = selected_light->color();
      if (ImGui::ColorEdit3("Color", (float*)&color))
        selected_light->set_color(color);

      float strength = selected_light->strength();
      if (ImGui::SliderFloat("Strength", &strength, 0.0f, 200.0f))
        selected_light->set_strength(strength);
    }

    if (ImGui::CollapsingHeader("Transparency"))
    {
      bool transparent = engine->is_model_transparent();
      if (ImGui::Checkbox("Transparent Meshes", &transparent))
        engine->set_model_transparent(transparent);

      float opacity = engine->get_model_opacity();
      begin_disabled(!transparent);
      if (ImGui::SliderFloat("Opacity", &opacity, 0.05f, 1.0f))
        engine->set_model_opacity(opacity);
      end_disabled(!transparent);
    }

    if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen))
    {
      int render_mode = engine->get_render_mode() == nengine::RenderEngine::RenderMode::Forward ? 0 : 1;
      if (ImGui::Combo("Mode", &render_mode, "Forward\0Deferred\0"))
      {
        engine->set_render_mode(render_mode == 0 ? nengine::RenderEngine::RenderMode::Forward
                                                 : nengine::RenderEngine::RenderMode::Deferred);
      }

      const bool disable_debug_view = engine->get_render_mode() != nengine::RenderEngine::RenderMode::Deferred;
      begin_disabled(disable_debug_view);
      int debug_view = static_cast<int>(engine->get_debug_view());
      if (ImGui::Combo("Debug View", &debug_view,
                       "Final\0Position\0Normal\0Albedo\0Roughness\0Metallic\0"))
      {
        engine->set_debug_view(static_cast<nengine::RenderEngine::DebugView>(debug_view));
      }
      end_disabled(disable_debug_view);

      if (deferred_requested && !deferred_available)
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "At least one mesh falls back to forward rendering.");
    }

    if (ImGui::CollapsingHeader("Screen Effects", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const auto effect_paths = list_screen_effect_materials();
      const std::string current_effect_path = engine->get_screen_effect_material_path();
      const std::string current_effect_label = engine->has_screen_effect_material()
        ? file_label(current_effect_path)
        : std::string("Off");

      if (ImGui::BeginCombo("Effect Material", current_effect_label.c_str()))
      {
        const bool off_selected = !engine->has_screen_effect_material();
        if (ImGui::Selectable("Off", off_selected))
          engine->clear_screen_effect_material();
        if (off_selected)
          ImGui::SetItemDefaultFocus();

        for (const auto& effect_path : effect_paths)
        {
          const bool is_selected = current_effect_path == effect_path;
          const std::string effect_name = std::filesystem::path(effect_path).stem().string();
          if (ImGui::Selectable(effect_name.c_str(), is_selected))
            engine->set_screen_effect_material(effect_path);
          if (is_selected)
            ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
      }

      if (ImGui::Button("Load Effect Material..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Screen Effect Material",
          L"Material Files (*.xml;*.mtl)\0*.xml;*.mtl\0All Files (*.*)\0*.*\0",
          "Assets/screen_effects");
        if (file_path)
          engine->set_screen_effect_material(*file_path);
      }

      if (engine->has_screen_effect_material())
      {
        ImGui::SameLine();
        if (ImGui::Button("Clear Effect"))
          engine->clear_screen_effect_material();

        ImGui::TextDisabled("Current");
        ImGui::SameLine();
        ImGui::TextUnformatted(file_label(current_effect_path).c_str());

        auto effect_material = engine->get_screen_effect_material();
        if (effect_material &&
            ImGui::CollapsingHeader("Effect Parameters", ImGuiTreeNodeFlags_DefaultOpen))
        {
          draw_material_parameter_editor(effect_material);
        }
      }
    }

    if (ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const bool capture_available = engine->is_frame_capture_available();
      const bool capture_pending = engine->is_frame_capture_pending();
      const bool capture_in_progress = engine->is_frame_capture_in_progress();

      begin_disabled(!capture_available);
      if (ImGui::Button("Capture Frame (F12)"))
        engine->request_frame_capture();
      end_disabled(!capture_available);

      if (!capture_available)
      {
        ImGui::TextColored(
          ImVec4(0.95f, 0.75f, 0.25f, 1.0f),
          "RenderDoc not detected. Launch from RenderDoc or make renderdoc.dll available.");
      }
      else if (capture_in_progress)
      {
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Capturing current frame...");
      }
      else if (capture_pending)
      {
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Capture queued for the next frame.");
      }
      else
      {
        ImGui::TextDisabled("Capture output: build/captures");
      }

      if (capture_available && !engine->get_last_frame_capture_path().empty())
      {
        ImGui::TextWrapped("Last Capture: %s", engine->get_last_frame_capture_path().c_str());
      }
    }

    ImGui::End();
  }
}
