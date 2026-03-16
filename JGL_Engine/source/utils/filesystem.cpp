#include "filesystem.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#ifdef _WIN32
#   include <Windows.h>
#else
#   include <dlfcn.h>
#endif

namespace
{
    std::string normalizeProjectPath(std::string path)
    {
        std::replace(path.begin(), path.end(), '\\', '/');

        constexpr std::string_view kLegacyResourceRoot = "JGL_MeshLoader/resource";
        constexpr std::string_view kLegacyResourcePrefix = "JGL_MeshLoader/resource/";
        constexpr std::string_view kRenamedResourceRoot = "JGL_Engine/resource";
        constexpr std::string_view kRenamedResourcePrefix = "JGL_Engine/resource/";
        constexpr std::string_view kLegacyShaderRoot = "JGL_MeshLoader/shaders";
        constexpr std::string_view kLegacyShaderPrefix = "JGL_MeshLoader/shaders/";
        constexpr std::string_view kRenamedShaderRoot = "JGL_Engine/shaders";
        constexpr std::string_view kRenamedShaderPrefix = "JGL_Engine/shaders/";

        if (path == kLegacyResourceRoot || path == kRenamedResourceRoot)
            return "Assets";

        if (path.rfind(kLegacyResourcePrefix, 0) == 0)
            return "Assets/" + path.substr(kLegacyResourcePrefix.size());

        if (path.rfind(kRenamedResourcePrefix, 0) == 0)
            return "Assets/" + path.substr(kRenamedResourcePrefix.size());

        if (path == kLegacyShaderRoot || path == kRenamedShaderRoot)
            return "Shaders";

        if (path.rfind(kLegacyShaderPrefix, 0) == 0)
            return "Shaders/" + path.substr(kLegacyShaderPrefix.size());

        if (path.rfind(kRenamedShaderPrefix, 0) == 0)
            return "Shaders/" + path.substr(kRenamedShaderPrefix.size());

        return path;
    }

    std::optional<std::filesystem::path> getModuleDirectory()
    {
#ifdef _WIN32
        HMODULE module = nullptr;
        if (!GetModuleHandleExA(
              GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
              reinterpret_cast<LPCSTR>(&getModuleDirectory),
              &module))
        {
            return std::nullopt;
        }

        char buf[1024];
        DWORD n = GetModuleFileNameA(module, buf, sizeof(buf) - 1);
        if (n == 0 || n >= sizeof(buf) - 1)
            return std::nullopt;
        buf[n] = '\0';
        return std::filesystem::path(buf).parent_path();
#else
        Dl_info info{};
        if (dladdr(reinterpret_cast<const void*>(&getModuleDirectory), &info) == 0 || info.dli_fname == nullptr)
            return std::nullopt;

        return std::filesystem::path(info.dli_fname).parent_path();
#endif
    }

    void appendCandidateRoot(std::vector<std::filesystem::path>& roots, const std::filesystem::path& root)
    {
        if (root.empty())
            return;

        const auto normalized = root.lexically_normal();
        if (std::find(roots.begin(), roots.end(), normalized) == roots.end())
            roots.push_back(normalized);
    }

    std::vector<std::filesystem::path> buildCandidateRoots()
    {
        std::vector<std::filesystem::path> roots;

        auto appendPathAndParents = [&roots](std::filesystem::path path)
        {
            for (int i = 0; i < 6 && !path.empty(); ++i)
            {
                appendCandidateRoot(roots, path);

                const auto parent = path.parent_path();
                if (parent == path)
                    break;

                path = parent;
            }
        };

        appendPathAndParents(std::filesystem::current_path());

        if (const auto module_dir = getModuleDirectory())
            appendPathAndParents(*module_dir);

        return roots;
    }

    std::optional<std::filesystem::path> resolveFromWorkspaceRoots(const std::filesystem::path& relative_path)
    {
        const std::string generic_relative = relative_path.generic_string();

        for (const auto& root : buildCandidateRoots())
        {
            const auto candidate = (root / relative_path).lexically_normal();
            if (std::filesystem::exists(candidate))
                return candidate;

            if (generic_relative == "Assets" || generic_relative.rfind("Assets/", 0) == 0)
            {
                const auto legacy_suffix = generic_relative == "Assets"
                    ? std::filesystem::path()
                    : std::filesystem::path(generic_relative.substr(std::string("Assets/").size()));
                const auto renamed_candidate = (root / "JGL_Engine" / "resource" / legacy_suffix).lexically_normal();
                if (std::filesystem::exists(renamed_candidate))
                    return renamed_candidate;

                const auto legacy_candidate = (root / "JGL_MeshLoader" / "resource" / legacy_suffix).lexically_normal();
                if (std::filesystem::exists(legacy_candidate))
                    return legacy_candidate;
            }

            if (generic_relative == "Shaders" || generic_relative.rfind("Shaders/", 0) == 0)
            {
                const auto legacy_suffix = generic_relative == "Shaders"
                    ? std::filesystem::path()
                    : std::filesystem::path(generic_relative.substr(std::string("Shaders/").size()));
                const auto renamed_candidate = (root / "JGL_Engine" / "shaders" / legacy_suffix).lexically_normal();
                if (std::filesystem::exists(renamed_candidate))
                    return renamed_candidate;

                const auto legacy_candidate = (root / "JGL_MeshLoader" / "shaders" / legacy_suffix).lexically_normal();
                if (std::filesystem::exists(legacy_candidate))
                    return legacy_candidate;
            }
        }

        return std::nullopt;
    }
}

std::string FileSystem::getPath(const std::string& path)
{
    return getPathRelativeBinary(path);
}

std::string const& FileSystem::getRoot()
{
    static std::string empty_root = "";
    return empty_root;
}

FileSystem::Builder FileSystem::getPathBuilder()
{
    return &FileSystem::getPathRelativeBinary;
}

std::string FileSystem::getPathRelativeRoot(const std::string& path)
{
    return getPathRelativeBinary(path);
}

std::string FileSystem::getPathRelativeBinary(const std::string& path)
{
    const std::string normalized_path = normalizeProjectPath(path);
    const std::filesystem::path relative_path(normalized_path);

    if (const auto workspace_path = resolveFromWorkspaceRoots(relative_path))
        return workspace_path->string();

    if (const auto module_dir = getModuleDirectory())
        return ((*module_dir / "../../../" / relative_path).lexically_normal()).string();

    return ("../../../" + normalized_path);
}
