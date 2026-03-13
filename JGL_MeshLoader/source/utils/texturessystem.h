#ifndef TEXTURESSYSTEM_H
#define TEXTURESSYSTEM_H

#include <string>
#include <vector>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

class TextureSystem
{
public:
    static std::string getPathRelativeToExecutable(const std::string& path);

    static unsigned int getTextureId(const char* path, GLint tex_wrapping = GL_REPEAT);

    static unsigned int loadCubemap(const std::vector<std::string>& faces, bool isflip);
};

#endif

