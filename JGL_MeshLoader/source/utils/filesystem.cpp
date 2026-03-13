#include "FileSystem.h"
#include <filesystem>

#ifdef _WIN32
#   include <Windows.h>
#endif

// 基于可执行文件所在目录解析路径，不依赖当前工作目录（VS 启动时 CWD 可能是项目根）
static std::string getExecutableDirectory()
{
#ifdef _WIN32
    char buf[1024];
    DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf) - 1);
    if (n == 0 || n >= sizeof(buf) - 1)
        return "";
    buf[n] = '\0';
    std::string exePath(buf);
    std::string::size_type lastSlash = exePath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        return exePath.substr(0, lastSlash);
    return exePath;
#else
#   include <unistd.h>
    char buf[1024];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0)
        return "";
    buf[n] = '\0';
    std::string exePath(buf);
    std::string::size_type lastSlash = exePath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        return exePath.substr(0, lastSlash);
    return exePath;
#endif
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

// 可执行文件在 build/bin/(Debug|Release)，上三级为仓库根 D:/JGL，再拼 path
std::string FileSystem::getPathRelativeBinary(const std::string& path)
{
    std::string exeDir = getExecutableDirectory();
    if (exeDir.empty())
        return "../../../" + path; // 回退到相对 CWD

    std::string combined = exeDir + "/../../../" + path;
    return std::filesystem::path(combined).lexically_normal().string();
}