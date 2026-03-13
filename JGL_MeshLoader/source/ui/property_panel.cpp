#include "pch.h"
#include "property_panel.h"

#include <commdlg.h>
#include <array>
#include <filesystem>
#include <optional>

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
      // Start from the repository root so imported assets line up with the engine's default paths.
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

    void draw_light_controls(
      const std::string& id_suffix,
      glm::vec3& position,
      glm::vec3& color,
      float& strength,
      bool* enabled = nullptr)
    {
      if (enabled)
      {
        const std::string enabled_label = "Enabled##" + id_suffix;
        ImGui::Checkbox(enabled_label.c_str(), enabled);
      }

      const std::string position_label = "Position##" + id_suffix;
      const std::string strength_label = "Strength##" + id_suffix;
      const std::string color_label = "Color##" + id_suffix;

      ImGui::SliderFloat3(position_label.c_str(), (float*)&position, -20.0f, 20.0f);
      ImGui::SliderFloat(strength_label.c_str(), &strength, 0.0f, 200.0f);
      ImGui::ColorEdit3(color_label.c_str(), (float*)&color);
    }
  }

  void Property_Panel::render(nengine::RenderEngine* engine)
  {
    if (!engine)
      return;

    auto model = engine->get_model();
    auto material = engine->get_material();
    const bool deferred_requested = engine->get_render_mode() == nengine::RenderEngine::RenderMode::Deferred;
    const bool deferred_available = engine->is_deferred_available();

    auto file_label = [](const std::string& path)
    {
      if (path.empty())
        return std::string("<empty>");

      return std::filesystem::path(path).filename().string();
    };

    if (!engine->get_shader_asset_path().empty())
      mCurrentShaderFile = file_label(engine->get_shader_asset_path());
    if (!engine->get_material_asset_path().empty())
      mCurrentMaterialFile = file_label(engine->get_material_asset_path());

    ImGui::SetNextWindowSize(ImVec2(360.0f, 620.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Properties");
    if (ImGui::Button("Reset Camera"))
      engine->reset_view();

    ImGui::SameLine();
    bool show_plane = engine->is_plane_show();
    if (ImGui::Checkbox("Plane##PropsQuick", &show_plane))
      engine->set_plane_show(show_plane);

    ImGui::SameLine();
    ImGui::TextDisabled("RMB Orbit | MMB Pan | W/S Zoom | F Reset");

    ImGui::Separator();
    ImGui::TextDisabled("Mesh");
    ImGui::SameLine();
    ImGui::TextUnformatted(mCurrentMeshFile.c_str());
    ImGui::TextDisabled("Shader");
    ImGui::SameLine();
    ImGui::TextUnformatted(mCurrentShaderFile.c_str());
    ImGui::TextDisabled("Material");
    ImGui::SameLine();
    ImGui::TextUnformatted(mCurrentMaterialFile.c_str());

    if (ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen))
    {
      if (ImGui::Button("Load Mesh..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Mesh",
          L"Mesh Files (*.fbx;*.obj;*.dae)\0*.fbx;*.obj;*.dae\0All Files (*.*)\0*.*\0");
        if (file_path)
        {
          engine->load_mesh(*file_path);
          mCurrentMeshFile = file_path->substr(file_path->find_last_of("/\\") + 1);
        }
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentMeshFile.c_str());

      if (ImGui::Button("Load Shader..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Shader",
          L"Shader Files (*_vs.shader;*_fs.shader)\0*_vs.shader;*_fs.shader\0All Files (*.*)\0*.*\0");
        if (file_path)
        {
          const std::string previous_shader_path = engine->get_shader_asset_path();
          engine->load_shader(*file_path);
          if (engine->get_shader_asset_path() != previous_shader_path && !engine->get_shader_asset_path().empty())
            mCurrentShaderFile = file_label(engine->get_shader_asset_path());
        }
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentShaderFile.c_str());

      if (ImGui::Button("Load Material..."))
      {
        const auto file_path = open_native_file_dialog(
          L"Open Material",
          L"Material Files (*.mtl;*.xml)\0*.mtl;*.xml\0All Files (*.*)\0*.*\0");
        if (file_path)
        {
          const std::string previous_material_path = engine->get_material_asset_path();
          const std::string previous_shader_path = engine->get_shader_asset_path();
          engine->load_material(*file_path);
          if (engine->get_material_asset_path() != previous_material_path && !engine->get_material_asset_path().empty())
            mCurrentMaterialFile = file_label(engine->get_material_asset_path());
          if (engine->get_shader_asset_path() != previous_shader_path && !engine->get_shader_asset_path().empty())
            mCurrentShaderFile = file_label(engine->get_shader_asset_path());
        }
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::TextUnformatted(mCurrentMaterialFile.c_str());
    }

    if (ImGui::CollapsingHeader("Material Parameters", ImGuiTreeNodeFlags_DefaultOpen) && model && material)
    {
      if (ImGui::TreeNodeEx("Scalars", ImGuiTreeNodeFlags_DefaultOpen))
      {
        for (auto& it: material->getFloatMap())
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

    if (ImGui::CollapsingHeader("Transparency"))
    {
        bool transparent = engine->is_model_transparent();
        if (ImGui::Checkbox("Transparent Model", &transparent))
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
          ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "Current material falls back to forward rendering.");
    }

    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& primary_light = engine->get_primary_light();
        if (ImGui::TreeNodeEx("Light 0", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextDisabled("Forward and deferred rendering share the same light array.");
            draw_light_controls(
              "primary",
              primary_light.position,
              primary_light.color,
              primary_light.strength,
              &primary_light.enabled);
            ImGui::TreePop();
        }

        int extra_count = static_cast<int>(engine->get_additional_deferred_light_count());
        if (ImGui::SliderInt("Additional Lights", &extra_count, 0, 7))
            engine->set_additional_deferred_light_count(static_cast<size_t>(extra_count));

        ImGui::TextDisabled("Shared Light Array");
        ImGui::Separator();

        for (size_t i = 0; i < engine->get_additional_deferred_light_count(); ++i)
        {
            auto& light = engine->get_additional_deferred_light(i);
            const std::string header = "Light " + std::to_string(i + 1);
            if (ImGui::TreeNode(header.c_str()))
            {
                draw_light_controls(
                  std::to_string(i),
                  light.position,
                  light.color,
                  light.strength,
                  &light.enabled);
                ImGui::TreePop();
            }
        }
    }

    ImGui::End();
  }
}
