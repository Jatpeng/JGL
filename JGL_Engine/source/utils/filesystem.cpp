#include "filesystem.h"

#include <algorithm>
#include <cctype>
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
    std::string toLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    bool extractSimpleAssetFilename(
        const std::filesystem::path& relative_path,
        std::filesystem::path* out_filename)
    {
        const std::string generic_relative = relative_path.generic_string();

        if (generic_relative.rfind("Assets/", 0) == 0)
        {
            const std::filesystem::path asset_suffix(generic_relative.substr(std::string("Assets/").size()));
            if (asset_suffix.empty() || asset_suffix.has_parent_path())
                return false;

            if (out_filename)
                *out_filename = asset_suffix.filename();
            return true;
        }

        if (!relative_path.has_parent_path())
        {
            if (out_filename)
                *out_filename = relative_path.filename();
            return true;
        }

        return false;
    }

    std::vector<std::filesystem::path> categorizedAssetSearchRoots(const std::filesystem::path& asset_filename)
    {
        std::vector<std::filesystem::path> roots;
        auto append_unique = [&roots](const std::filesystem::path& root)
        {
            if (std::find(roots.begin(), roots.end(), root) == roots.end())
                roots.push_back(root);
        };

        const std::string ext = toLowerCopy(asset_filename.extension().string());
        if (ext == ".xml" || ext == ".mtl")
        {
            append_unique("Assets/materials");
            append_unique("Assets/screen_effects");
            append_unique("Assets/scenes");
        }
        else if (ext == ".fbx" ||
                 ext == ".obj" ||
                 ext == ".dae" ||
                 ext == ".blend" ||
                 ext == ".gltf" ||
                 ext == ".glb")
        {
            append_unique("Assets/models");
            append_unique("Assets/built_in");
            append_unique("Assets");
        }
        else if (ext == ".png" ||
                 ext == ".jpg" ||
                 ext == ".jpeg" ||
                 ext == ".tga" ||
                 ext == ".bmp" ||
                 ext == ".dds" ||
                 ext == ".hdr" ||
                 ext == ".tif" ||
                 ext == ".tiff")
        {
            append_unique("Assets/textures");
            append_unique("Assets/built_in/textures");
            append_unique("Assets/environments");
            append_unique("Assets/skybox");
            append_unique("Assets/models");
            append_unique("Assets");
        }
        else if (ext == ".janim")
        {
            append_unique("Assets/animations");
        }
        else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")
        {
            append_unique("Assets/audio");
        }
        else if (ext == ".lvl")
        {
            append_unique("Assets/levels");
        }

        append_unique("Assets");
        return roots;
    }

    std::optional<std::filesystem::path> findFilenameInSubtree(
        const std::filesystem::path& search_root,
        const std::filesystem::path& asset_filename)
    {
        const std::string expected_filename = toLowerCopy(asset_filename.filename().string());
        std::error_code ec;
        if (!std::filesystem::exists(search_root, ec) || ec)
            return std::nullopt;

        std::vector<std::filesystem::path> matches;
        for (std::filesystem::recursive_directory_iterator it(search_root, ec), end;
             !ec && it != end;
             it.increment(ec))
        {
            if (ec)
                break;

            const bool is_regular_file = it->is_regular_file(ec);
            if (ec)
            {
                ec.clear();
                continue;
            }

            if (!is_regular_file)
                continue;

            if (toLowerCopy(it->path().filename().string()) != expected_filename)
                continue;

            matches.push_back(it->path().lexically_normal());
        }

        if (matches.empty())
            return std::nullopt;

        std::sort(matches.begin(), matches.end(), [](const auto& lhs, const auto& rhs)
        {
            return lhs.generic_string() < rhs.generic_string();
        });

        return matches.front();
    }

    std::optional<std::filesystem::path> resolveCategorizedAssetPath(
        const std::filesystem::path& workspace_root,
        const std::filesystem::path& relative_path)
    {
        std::filesystem::path asset_filename;
        if (!extractSimpleAssetFilename(relative_path, &asset_filename))
            return std::nullopt;

        const auto search_roots = categorizedAssetSearchRoots(asset_filename);
        for (const auto& search_root_relative : search_roots)
        {
            const auto direct_candidate = (workspace_root / search_root_relative / asset_filename).lexically_normal();
            if (std::filesystem::exists(direct_candidate))
                return direct_candidate;
        }

        for (const auto& search_root_relative : search_roots)
        {
            const auto search_root = (workspace_root / search_root_relative).lexically_normal();
            if (const auto found = findFilenameInSubtree(search_root, asset_filename))
                return found;
        }

        return std::nullopt;
    }

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

            if (const auto categorized_candidate = resolveCategorizedAssetPath(root, relative_path))
                return categorized_candidate->lexically_normal();

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
