#include "pch.h"

#include "ui/property_panel.h"

#include "imgui_internal.h"

#include <array>
#include <filesystem>
#include <optional>

#include <commdlg.h>

namespace nui
{
  namespace
  {
    std::wstring wide_from_narrow(const std::string& narrow)
    {
      if (narrow.empty())
        return {};

      const int size = MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, nullptr, 0);
      if (size <= 1)
        return {};

      std::vector<wchar_t> buffer(static_cast<size_t>(size), L'\0');
      MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, buffer.data(), size);
      return std::wstring(buffer.data());
    }

    std::string narrow_from_wide(const std::wstring& wide)
    {
      if (wide.empty())
        return {};

      const int size = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
      if (size <= 1)
        return {};

      std::vector<char> buffer(static_cast<size_t>(size), '\0');
      WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, buffer.data(), size, nullptr, nullptr);
      return std::string(buffer.data());
    }

    std::optional<std::string> open_native_file_dialog(const wchar_t* title, const wchar_t* filter)
    {
      std::array<wchar_t, 4096> filename = {};
      const std::wstring initial_dir = wide_from_narrow(FileSystem::getPath(""));
      OPENFILENAMEW dialog = {};
      dialog.lStructSize = sizeof(dialog);
      dialog.hwndOwner = GetActiveWindow();
      dialog.lpstrFile = filename.data();
      dialog.nMaxFile = static_cast<DWORD>(filename.size());
      dialog.lpstrInitialDir = initial_dir.empty() ? nullptr : initial_dir.c_str();
      dialog.lpstrFilter = filter;
      dialog.nFilterIndex = 1;
      dialog.lpstrTitle = title;
      dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

      if (!GetOpenFileNameW(&dialog))
        return std::nullopt;

      return narrow_from_wide(filename.data());
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

    std::string file_label(const std::string& path)
    {
      if (path.empty())
        return std::string("<empty>");

      return std::filesystem::path(path).filename().string();
    }

    std::shared_ptr<nengine::SceneObject> resolve_selected_object(
      const std::shared_ptr<nengine::Scene>& scene,
      uint64_t* selected_id)
    {
      if (!scene || !selected_id)
        return nullptr;

      auto selected = scene->find_object(*selected_id);
      if (selected)
        return selected;

      if (!scene->objects().empty())
      {
        *selected_id = scene->objects().front()->id();
        return scene->objects().front();
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
    auto selected_object = resolve_selected_object(scene, &mSelectedObjectId);
    auto selected_mesh = std::dynamic_pointer_cast<nengine::MeshObject>(selected_object);
    auto selected_light = std::dynamic_pointer_cast<nengine::LightObject>(selected_object);

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
        auto mesh = scene->create_mesh("mesh_" + std::to_string(scene->objects().size() + 1));
        mesh->set_model("Assets/cube.fbx");
        mesh->set_material("Assets/PBR.xml");
        mSelectedObjectId = mesh->id();
      }

      ImGui::SameLine();
      if (ImGui::Button("Add Light"))
      {
        auto light = scene->create_light("light_" + std::to_string(scene->objects().size() + 1));
        light->transform().position = glm::vec3(1.5f, 3.5f, 3.0f);
        light->set_strength(100.0f);
        mSelectedObjectId = light->id();
      }

      ImGui::SameLine();
      begin_disabled(!selected_object);
      if (ImGui::Button("Remove Selected") && selected_object)
      {
        scene->remove_object(selected_object->id());
        mSelectedObjectId = 0;
        selected_object.reset();
        selected_mesh.reset();
        selected_light.reset();
      }
      end_disabled(!selected_object);

      ImGui::Separator();
      ImGui::TextDisabled("Objects");
      for (const auto& object : scene->objects())
      {
        const bool is_selected = selected_object && object->id() == selected_object->id();
        std::string label = std::string(object->object_type()) + "##" + std::to_string(object->id());
        if (ImGui::Selectable((object->name() + "###" + label).c_str(), is_selected))
          mSelectedObjectId = object->id();

        ImGui::SameLine(260.0f);
        ImGui::TextDisabled("%s", object->object_type());
      }
    }

    if (selected_object && ImGui::CollapsingHeader("Selected Object", ImGuiTreeNodeFlags_DefaultOpen))
    {
      std::array<char, 256> name_buffer = {};
      strncpy_s(name_buffer.data(), name_buffer.size(), selected_object->name().c_str(), _TRUNCATE);
      if (ImGui::InputText("Name", name_buffer.data(), name_buffer.size()))
        selected_object->set_name(name_buffer.data());

      ImGui::TextDisabled("Id %llu | Type %s",
                          static_cast<unsigned long long>(selected_object->id()),
                          selected_object->object_type());

      auto transform = selected_object->transform();
      if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
      {
        if (ImGui::SliderFloat3("Position", (float*)&transform.position, -20.0f, 20.0f))
          selected_object->transform().position = transform.position;
        if (ImGui::SliderFloat3("Rotation", (float*)&transform.rotation, -180.0f, 180.0f))
          selected_object->transform().rotation = transform.rotation;
        if (ImGui::SliderFloat3("Scale", (float*)&transform.scale, 0.01f, 20.0f))
          selected_object->transform().scale = transform.scale;
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
          L"Mesh Files (*.fbx;*.obj;*.dae)\0*.fbx;*.obj;*.dae\0All Files (*.*)\0*.*\0");
        if (file_path && selected_mesh->set_model(*file_path))
          mCurrentMeshFile = file_label(*file_path);
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentMeshFile.c_str());

      if (ImGui::Button("Load Shader..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Shader",
          L"Shader Files (*_vs.shader;*_fs.shader)\0*_vs.shader;*_fs.shader\0All Files (*.*)\0*.*\0");
        if (file_path && selected_mesh->set_shader(*file_path))
          mCurrentShaderFile = file_label(selected_mesh->shader_path());
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentShaderFile.c_str());

      if (ImGui::Button("Load Material..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Material",
          L"Material Files (*.mtl;*.xml)\0*.mtl;*.xml\0All Files (*.*)\0*.*\0");
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
      auto material = selected_mesh->material();
      if (ImGui::TreeNodeEx("Scalars", ImGuiTreeNodeFlags_DefaultOpen))
      {
        for (auto& it : material->getFloatMap())
        {
          float tmp = it.second;
          if (ImGui::SliderFloat(it.first.c_str(), &tmp, -100.0f, 100.0f))
            it.second = tmp;
        }
        if (material->getFloatMap().empty())
          ImGui::TextDisabled("No editable scalar material parameters.");
        ImGui::TreePop();
      }

      if (ImGui::TreeNodeEx("Vectors", ImGuiTreeNodeFlags_DefaultOpen))
      {
        for (auto& it : material->getFloat3Map())
        {
          glm::vec3 tmp = it.second;
          if (ImGui::SliderFloat3(it.first.c_str(), (float*)&tmp, -100.0f, 100.0f))
            it.second = tmp;
        }
        if (material->getFloat3Map().empty())
          ImGui::TextDisabled("No editable vector material parameters.");
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
        ImGui::TreePop();
      }
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
