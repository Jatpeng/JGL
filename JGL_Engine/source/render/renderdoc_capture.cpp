#include "pch.h"

#include "renderdoc_capture.h"

#include "utils/filesystem.h"

#ifdef _WIN32
#  include <cstdint>
#  include <cstdlib>
#  include <filesystem>
#  include <optional>
#  include <set>
#  include <vector>
#endif

namespace nrender
{
#ifdef _WIN32
  namespace
  {
    constexpr int kRenderDocApiVersion = 10600;
    constexpr uint32_t kCaptureKeysDisabled = 0;
    constexpr uint32_t kDefaultOutput = 0;

    using RenderDocGetApi = int(__cdecl*)(int version, void** out_api);

    typedef void(__cdecl* pRENDERDOC_GetAPIVersion)(int* major, int* minor, int* patch);
    typedef void(__cdecl* pRENDERDOC_SetCaptureOptionU32)(uint32_t opt, uint32_t val);
    typedef void(__cdecl* pRENDERDOC_SetCaptureOptionF32)(uint32_t opt, float val);
    typedef uint32_t(__cdecl* pRENDERDOC_GetCaptureOptionU32)(uint32_t opt);
    typedef float(__cdecl* pRENDERDOC_GetCaptureOptionF32)(uint32_t opt);
    typedef void(__cdecl* pRENDERDOC_SetFocusToggleKeys)(const void* keys, int num);
    typedef void(__cdecl* pRENDERDOC_SetCaptureKeys)(const void* keys, int num);
    typedef uint32_t(__cdecl* pRENDERDOC_GetOverlayBits)();
    typedef void(__cdecl* pRENDERDOC_MaskOverlayBits)(uint32_t and_mask, uint32_t or_mask);
    typedef void(__cdecl* pRENDERDOC_Shutdown)();
    typedef void(__cdecl* pRENDERDOC_UnloadCrashHandler)();
    typedef void(__cdecl* pRENDERDOC_SetCaptureFilePathTemplate)(const char* path_template);
    typedef const char*(__cdecl* pRENDERDOC_GetCaptureFilePathTemplate)();
    typedef uint32_t(__cdecl* pRENDERDOC_GetNumCaptures)();
    typedef uint32_t(__cdecl* pRENDERDOC_GetCapture)(
      uint32_t idx,
      char* filename,
      uint32_t* path_length,
      uint64_t* timestamp);
    typedef void(__cdecl* pRENDERDOC_TriggerCapture)();
    typedef uint32_t(__cdecl* pRENDERDOC_IsTargetControlConnected)();
    typedef uint32_t(__cdecl* pRENDERDOC_LaunchReplayUI)(uint32_t connect_target_control, const char* cmdline);
    typedef void(__cdecl* pRENDERDOC_SetActiveWindow)(void* device, void* wnd_handle);
    typedef void(__cdecl* pRENDERDOC_StartFrameCapture)(void* device, void* wnd_handle);
    typedef uint32_t(__cdecl* pRENDERDOC_IsFrameCapturing)();
    typedef uint32_t(__cdecl* pRENDERDOC_EndFrameCapture)(void* device, void* wnd_handle);
    typedef void(__cdecl* pRENDERDOC_TriggerMultiFrameCapture)(uint32_t num_frames);
    typedef void(__cdecl* pRENDERDOC_SetCaptureFileComments)(const char* file_path, const char* comments);
    typedef uint32_t(__cdecl* pRENDERDOC_DiscardFrameCapture)(void* device, void* wnd_handle);
    typedef uint32_t(__cdecl* pRENDERDOC_ShowReplayUI)();
    typedef void(__cdecl* pRENDERDOC_SetCaptureTitle)(const char* title);

    struct RenderDocApi
    {
      pRENDERDOC_GetAPIVersion GetAPIVersion;
      pRENDERDOC_SetCaptureOptionU32 SetCaptureOptionU32;
      pRENDERDOC_SetCaptureOptionF32 SetCaptureOptionF32;
      pRENDERDOC_GetCaptureOptionU32 GetCaptureOptionU32;
      pRENDERDOC_GetCaptureOptionF32 GetCaptureOptionF32;

      pRENDERDOC_SetFocusToggleKeys SetFocusToggleKeys;
      pRENDERDOC_SetCaptureKeys SetCaptureKeys;

      pRENDERDOC_GetOverlayBits GetOverlayBits;
      pRENDERDOC_MaskOverlayBits MaskOverlayBits;

      pRENDERDOC_Shutdown RemoveHooks;
      pRENDERDOC_UnloadCrashHandler UnloadCrashHandler;

      pRENDERDOC_SetCaptureFilePathTemplate SetCaptureFilePathTemplate;
      pRENDERDOC_GetCaptureFilePathTemplate GetCaptureFilePathTemplate;

      pRENDERDOC_GetNumCaptures GetNumCaptures;
      pRENDERDOC_GetCapture GetCapture;

      pRENDERDOC_TriggerCapture TriggerCapture;

      pRENDERDOC_IsTargetControlConnected IsTargetControlConnected;
      pRENDERDOC_LaunchReplayUI LaunchReplayUI;
      pRENDERDOC_SetActiveWindow SetActiveWindow;

      pRENDERDOC_StartFrameCapture StartFrameCapture;
      pRENDERDOC_IsFrameCapturing IsFrameCapturing;
      pRENDERDOC_EndFrameCapture EndFrameCapture;

      pRENDERDOC_TriggerMultiFrameCapture TriggerMultiFrameCapture;

      pRENDERDOC_SetCaptureFileComments SetCaptureFileComments;
      pRENDERDOC_DiscardFrameCapture DiscardFrameCapture;

      pRENDERDOC_ShowReplayUI ShowReplayUI;

      pRENDERDOC_SetCaptureTitle SetCaptureTitle;
    };

    HMODULE try_load_renderdoc_module(bool* owns_module)
    {
      if (owns_module)
        *owns_module = false;

      HMODULE module = GetModuleHandleA("renderdoc.dll");
      if (module != nullptr)
        return module;

      module = LoadLibraryA("renderdoc.dll");
      if (module != nullptr && owns_module)
        *owns_module = true;

      if (module != nullptr)
        return module;

      auto query_registry_string = [](HKEY root, const wchar_t* subkey, const wchar_t* value_name) -> std::wstring
      {
        DWORD byte_count = 0;
        LONG result = RegGetValueW(root, subkey, value_name, RRF_RT_REG_SZ, nullptr, nullptr, &byte_count);
        if (result != ERROR_SUCCESS || byte_count <= sizeof(wchar_t))
          return {};

        std::wstring value(byte_count / sizeof(wchar_t), L'\0');
        result = RegGetValueW(root, subkey, value_name, RRF_RT_REG_SZ, nullptr, value.data(), &byte_count);
        if (result != ERROR_SUCCESS)
          return {};

        value.resize((byte_count / sizeof(wchar_t)) - 1);
        return value;
      };

      auto sanitize_registry_path = [](std::wstring value) -> std::filesystem::path
      {
        if (value.empty())
          return {};

        const size_t comma_pos = value.find(L',');
        if (comma_pos != std::wstring::npos)
          value = value.substr(0, comma_pos);

        if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"')
          value = value.substr(1, value.size() - 2);

        std::filesystem::path path(value);
        if (path.has_filename() && path.filename() != L"renderdoc.dll")
          path = path.parent_path() / L"renderdoc.dll";
        else if (!path.has_extension())
          path /= L"renderdoc.dll";

        return path.lexically_normal();
      };

      auto add_candidate = [](std::vector<std::filesystem::path>* candidates,
                              std::set<std::wstring>* dedupe,
                              const std::filesystem::path& candidate)
      {
        if (!candidates || !dedupe || candidate.empty())
          return;

        const std::filesystem::path normalized = candidate.lexically_normal();
        const std::wstring key = normalized.native();
        if (dedupe->insert(key).second)
          candidates->push_back(normalized);
      };

      auto add_directory_candidate = [&](std::vector<std::filesystem::path>* candidates,
                                         std::set<std::wstring>* dedupe,
                                         const std::filesystem::path& directory)
      {
        if (directory.empty())
          return;

        add_candidate(candidates, dedupe, directory / L"RenderDoc" / L"renderdoc.dll");
        add_candidate(candidates, dedupe, directory / L"renderdoc.dll");
      };

      auto parse_env_path = [](const char* env_name) -> std::optional<std::filesystem::path>
      {
        const char* value = std::getenv(env_name);
        if (!value || value[0] == '\0')
          return std::nullopt;

        std::filesystem::path path(value);
        if (!path.has_filename() || !path.has_extension())
          path /= "renderdoc.dll";
        return path.lexically_normal();
      };

      auto parse_env_directory = [](const char* env_name) -> std::optional<std::filesystem::path>
      {
        const char* value = std::getenv(env_name);
        if (!value || value[0] == '\0')
          return std::nullopt;

        return std::filesystem::path(value).lexically_normal();
      };

      std::vector<std::filesystem::path> candidates;
      std::set<std::wstring> dedupe;

      if (const auto env_dll = parse_env_path("RENDERDOC_DLL"))
        add_candidate(&candidates, &dedupe, *env_dll);
      if (const auto env_path = parse_env_path("RENDERDOC_PATH"))
        add_candidate(&candidates, &dedupe, *env_path);

      if (const auto program_files = parse_env_directory("ProgramFiles"))
        add_directory_candidate(&candidates, &dedupe, *program_files);
      if (const auto program_files_x86 = parse_env_directory("ProgramFiles(x86)"))
        add_directory_candidate(&candidates, &dedupe, *program_files_x86);

      constexpr const wchar_t* registry_keys[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\RenderDoc",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\RenderDoc"
      };

      for (const auto* key : registry_keys)
      {
        add_candidate(
          &candidates,
          &dedupe,
          sanitize_registry_path(query_registry_string(HKEY_LOCAL_MACHINE, key, L"InstallLocation")));
        add_candidate(
          &candidates,
          &dedupe,
          sanitize_registry_path(query_registry_string(HKEY_LOCAL_MACHINE, key, L"DisplayIcon")));
        add_candidate(
          &candidates,
          &dedupe,
          sanitize_registry_path(query_registry_string(HKEY_CURRENT_USER, key, L"InstallLocation")));
        add_candidate(
          &candidates,
          &dedupe,
          sanitize_registry_path(query_registry_string(HKEY_CURRENT_USER, key, L"DisplayIcon")));
      }

      for (const auto& candidate : candidates)
      {
        std::error_code error;
        if (!std::filesystem::exists(candidate, error) || error)
          continue;

        module = LoadLibraryW(candidate.c_str());
        if (module != nullptr)
        {
          if (owns_module)
            *owns_module = true;
          std::cout << "[RenderDoc] Loaded from: " << candidate.string() << std::endl;
          return module;
        }
      }

      return module;
    }

    std::string query_last_capture_path(RenderDocApi* api)
    {
      if (!api || !api->GetNumCaptures || !api->GetCapture)
        return {};

      const uint32_t capture_count = api->GetNumCaptures();
      if (capture_count == 0)
        return {};

      uint32_t path_length = 4096;
      uint64_t timestamp = 0;
      std::vector<char> filename(path_length, '\0');
      uint32_t result = api->GetCapture(capture_count - 1, filename.data(), &path_length, &timestamp);
      if (result == 0 && path_length > filename.size())
      {
        filename.assign(path_length, '\0');
        result = api->GetCapture(capture_count - 1, filename.data(), &path_length, &timestamp);
      }

      if (result == 0 || filename.empty())
        return {};

      return std::string(filename.data());
    }
  }

  struct RenderDocCapture::Api : RenderDocApi
  {
  };

  RenderDocCapture::RenderDocCapture()
  {
    initialize();
  }

  RenderDocCapture::~RenderDocCapture()
  {
    if (mOwnsModule && mModule != nullptr)
      FreeLibrary(static_cast<HMODULE>(mModule));
  }

  bool RenderDocCapture::is_available() const
  {
    return mApi != nullptr;
  }

  bool RenderDocCapture::is_capture_pending() const
  {
    return mCapturePending;
  }

  bool RenderDocCapture::is_capture_in_progress() const
  {
    return mCaptureInProgress;
  }

  const std::string& RenderDocCapture::last_capture_path() const
  {
    return mLastCapturePath;
  }

  void RenderDocCapture::request_capture()
  {
    if (!is_available())
      return;

    if (mCaptureInProgress)
      return;

    mCapturePending = true;
    std::cout << "[RenderDoc] Capture queued for the next frame." << std::endl;
  }

  bool RenderDocCapture::begin_capture(void* native_window, const std::string& capture_title)
  {
    if (!is_available() || !mCapturePending || mCaptureInProgress)
      return false;

    mCapturePending = false;

    if (mApi->IsFrameCapturing && mApi->IsFrameCapturing() != 0)
    {
      std::cout << "[RenderDoc] A capture is already in progress." << std::endl;
      return false;
    }

    if (mApi->SetActiveWindow && native_window != nullptr)
      mApi->SetActiveWindow(nullptr, native_window);

    if (mApi->SetCaptureTitle && !capture_title.empty())
      mApi->SetCaptureTitle(capture_title.c_str());
    mApi->StartFrameCapture(nullptr, native_window);

    mCaptureInProgress = true;
    std::cout << "[RenderDoc] Capturing frame: " << capture_title << std::endl;
    return true;
  }

  void RenderDocCapture::end_capture(void* native_window)
  {
    if (!is_available() || !mCaptureInProgress)
      return;

    const uint32_t result = mApi->EndFrameCapture(nullptr, native_window);
    mCaptureInProgress = false;

    if (result == kDefaultOutput)
    {
      std::cout << "[RenderDoc] Frame capture failed." << std::endl;
      return;
    }

    mLastCapturePath = query_last_capture_path(mApi);
    if (mLastCapturePath.empty())
      std::cout << "[RenderDoc] Frame capture saved." << std::endl;
    else
      std::cout << "[RenderDoc] Frame capture saved: " << mLastCapturePath << std::endl;
  }

  void RenderDocCapture::initialize()
  {
    bool owns_module = false;
    HMODULE module = try_load_renderdoc_module(&owns_module);
    if (module == nullptr)
      return;

    const auto get_api =
      reinterpret_cast<RenderDocGetApi>(GetProcAddress(module, "RENDERDOC_GetAPI"));
    if (!get_api)
    {
      if (owns_module)
        FreeLibrary(module);
      return;
    }

    Api* api = nullptr;
    if (get_api(kRenderDocApiVersion, reinterpret_cast<void**>(&api)) != 1 || api == nullptr)
    {
      if (owns_module)
        FreeLibrary(module);
      return;
    }

    mModule = module;
    mOwnsModule = owns_module;
    mApi = api;

    if (mApi->SetCaptureKeys)
      mApi->SetCaptureKeys(nullptr, static_cast<int>(kCaptureKeysDisabled));

    const std::filesystem::path capture_template(FileSystem::getPath("build/captures/jgl_capture"));
    std::error_code error;
    std::filesystem::create_directories(capture_template.parent_path(), error);

    if (mApi->SetCaptureFilePathTemplate)
      mApi->SetCaptureFilePathTemplate(capture_template.string().c_str());

    std::cout << "[RenderDoc] Integration ready." << std::endl;
  }
#else
  struct RenderDocCapture::Api
  {
  };

  RenderDocCapture::RenderDocCapture()
  {
    initialize();
  }

  RenderDocCapture::~RenderDocCapture() = default;

  bool RenderDocCapture::is_available() const
  {
    return false;
  }

  bool RenderDocCapture::is_capture_pending() const
  {
    return false;
  }

  bool RenderDocCapture::is_capture_in_progress() const
  {
    return false;
  }

  const std::string& RenderDocCapture::last_capture_path() const
  {
    return mLastCapturePath;
  }

  void RenderDocCapture::request_capture()
  {
  }

  bool RenderDocCapture::begin_capture(void* native_window, const std::string& capture_title)
  {
    (void)native_window;
    (void)capture_title;
    return false;
  }

  void RenderDocCapture::end_capture(void* native_window)
  {
    (void)native_window;
  }

  void RenderDocCapture::initialize()
  {
  }
#endif
}
