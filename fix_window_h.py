with open("JGL_Engine/source/window/jgl_window.h", "r") as f:
    text = f.read()

search = """<<<<<<< HEAD
      const nengine::RenderEngine::CreateInfo& engine_info = nengine::RenderEngine::CreateInfo());
=======

      const nengine::RenderEngine::CreateInfo& engine_info = nengine::RenderEngine::CreateInfo{});

>>>>>>> origin/master"""

replace = """      const nengine::RenderEngine::CreateInfo& engine_info = nengine::RenderEngine::CreateInfo());"""

text = text.replace(search, replace)

with open("JGL_Engine/source/window/jgl_window.h", "w") as f:
    f.write(text)
