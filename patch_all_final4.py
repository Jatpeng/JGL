import os
import re

with open("JGL_Engine/source/engine/render_engine.h", "r") as f:
    content = f.read()

content = content.replace("    explicit RenderEngine(const CreateInfo& create_info = {});", "    explicit RenderEngine(const CreateInfo& create_info = CreateInfo());")

# Re-apply the shadow state variables in render_engine.h since git restore removed them
content = content.replace("    unsigned int mFallbackNormalTexture = 0;", """    unsigned int mFallbackNormalTexture = 0;

    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    unsigned int SHADOW_WIDTH = 2048;
    unsigned int SHADOW_HEIGHT = 2048;""")

content = content.replace("    void forward_overlay_pass();", """    void shadow_pass();
    void forward_overlay_pass();""")

with open("JGL_Engine/source/engine/render_engine.h", "w") as f:
    f.write(content)


with open("JGL_Engine/source/engine/engine.h", "r") as f:
    content = f.read()

content = content.replace("    explicit Engine(const CreateInfo& create_info = {});", "    explicit Engine(const CreateInfo& create_info = CreateInfo());")

with open("JGL_Engine/source/engine/engine.h", "w") as f:
    f.write(content)


with open("JGL_Engine/source/window/jgl_window.h", "r") as f:
    content = f.read()

content = content.replace("      const nengine::RenderEngine::CreateInfo& engine_info = {});", "      const nengine::RenderEngine::CreateInfo& engine_info = nengine::RenderEngine::CreateInfo());")

with open("JGL_Engine/source/window/jgl_window.h", "w") as f:
    f.write(content)
