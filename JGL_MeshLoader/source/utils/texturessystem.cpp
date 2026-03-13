#include "texturessystem.h"

std::string TextureSystem::getPathRelativeToExecutable(const std::string& path)
{
    char buf[1024];
    int len = sizeof(buf) - 1;
#ifdef _WIN32
    int n = GetModuleFileNameA(NULL, buf, len);
#else
    int n = readlink("/proc/self/exe", buf, len);
    if (n <= 0) return "";
    if (n >= len) n = len - 1;
    buf[n] = '\0';
#endif
    while (n > 0 && buf[n] != '\\' && buf[n] != '/') --n;
    return std::string(buf, buf + n + 1) + path;
}

static bool is_absolute_path(const std::string& path)
{
    if (path.size() > 2 && ((path[1] == ':' && (path[2] == '\\' || path[2] == '/')) ||
        (path[0] == '\\' && path[1] == '\\')))
    {
        return true;
    }
    return false;
}

unsigned int TextureSystem::getTextureId(const char* path, GLint tex_wrapping)
{
    stbi_set_flip_vertically_on_load(true);

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    std::string finalPath = is_absolute_path(path)
        ? std::string(path)
        : getPathRelativeToExecutable(path);

    unsigned char* data = stbi_load(finalPath.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex_wrapping);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex_wrapping);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << finalPath << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int TextureSystem::loadCubemap(const std::vector<std::string>& faces, bool isflip)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(isflip);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        const std::string& fullPath = faces[i];
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << fullPath << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}