import os
import re

with open("JGL_Engine/source/engine/render_engine.h", "r") as f:
    content = f.read()

content = content.replace("explicit RenderEngine(const CreateInfo& create_info = {});", "explicit RenderEngine(const CreateInfo& create_info = CreateInfo());")

with open("JGL_Engine/source/engine/render_engine.h", "w") as f:
    f.write(content)

with open("JGL_Engine/source/engine/engine.h", "r") as f:
    content = f.read()

content = content.replace("explicit Engine(const CreateInfo& create_info = {});", "explicit Engine(const CreateInfo& create_info = CreateInfo());")

with open("JGL_Engine/source/engine/engine.h", "w") as f:
    f.write(content)

with open("JGL_Engine/source/window/jgl_window.h", "r") as f:
    content = f.read()

content = content.replace("const nengine::RenderEngine::CreateInfo& engine_info = {});", "const nengine::RenderEngine::CreateInfo& engine_info = nengine::RenderEngine::CreateInfo());")

with open("JGL_Engine/source/window/jgl_window.h", "w") as f:
    f.write(content)
