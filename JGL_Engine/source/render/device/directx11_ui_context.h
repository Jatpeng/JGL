#pragma once

#ifdef _WIN32

#include "render/render_base.h"

namespace nrender
{
  class DirectX11_UIContext : public RenderContext
  {
  public:
    bool init(nwindow::IWindow* window) override;
    void pre_render() override;
    void post_render() override;
    void end() override;
  };
}

#endif
