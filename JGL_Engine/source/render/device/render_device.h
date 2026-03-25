#pragma once

#include <memory>
#include <string>

#include "render/render_base.h"

namespace nrender
{
  enum class GraphicsBackend
  {
    OpenGL = 0,
    DirectX11 = 1,
    DirectX12 = 2
  };

  const char* graphics_backend_name(GraphicsBackend backend);
  bool try_parse_graphics_backend(const std::string& text, GraphicsBackend* out_backend);
  GraphicsBackend resolve_graphics_backend_from_env(GraphicsBackend fallback_backend);
  std::string supported_graphics_backends();

  class IRenderDevice
  {
  public:
    virtual ~IRenderDevice() = default;

    virtual GraphicsBackend backend() const = 0;
    virtual std::unique_ptr<RenderContext> create_render_context() const = 0;
    virtual std::unique_ptr<RenderContext> create_ui_context() const = 0;
    virtual std::unique_ptr<VertexIndexBuffer> create_vertex_index_buffer() const = 0;
    virtual std::unique_ptr<FrameBuffer> create_frame_buffer() const = 0;
  };

  class RenderDeviceManager
  {
  public:
    static RenderDeviceManager& instance();

    bool initialize(GraphicsBackend backend);
    GraphicsBackend backend() const { return mBackend; }
    std::shared_ptr<IRenderDevice> device();
    bool is_backend_supported(GraphicsBackend backend) const;

    std::unique_ptr<RenderContext> create_render_context();
    std::unique_ptr<RenderContext> create_ui_context();
    std::unique_ptr<VertexIndexBuffer> create_vertex_index_buffer();
    std::unique_ptr<FrameBuffer> create_frame_buffer();

  private:
    RenderDeviceManager() = default;

    std::shared_ptr<IRenderDevice> ensure_device();
    std::shared_ptr<IRenderDevice> create_device(GraphicsBackend backend) const;

    GraphicsBackend mBackend = GraphicsBackend::OpenGL;
    std::shared_ptr<IRenderDevice> mDevice;
  };
}
