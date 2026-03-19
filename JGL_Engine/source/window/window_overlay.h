#pragma once

namespace nengine
{
  class RenderEngine;
}

namespace nwindow
{
  class IWindow;

  class IWindowOverlay
  {
  public:
    virtual ~IWindowOverlay() = default;

    virtual void on_attach(IWindow& window)
    {
      (void)window;
    }

    virtual void on_detach() {}
    virtual void begin_frame() {}
    virtual void render(nengine::RenderEngine& engine) = 0;
    virtual void end_frame() {}
  };
}
