#include "pch.h"

#include "render/device/render_device.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#ifdef _WIN32
#include "render/device/directx11_context.h"
#include "render/device/directx11_ui_context.h"
#endif
#include "render/device/opengl_buffer_manager.h"
#include "render/device/opengl_context.h"
#include "render/device/ui_context.h"

namespace nrender
{
  namespace
  {
    constexpr const char* kRenderBackendEnvVar = "JGL_RENDER_BACKEND";

    std::string read_environment_variable(const char* name)
    {
#ifdef _MSC_VER
      char* value = nullptr;
      size_t value_size = 0;
      if (_dupenv_s(&value, &value_size, name) != 0 || !value)
        return {};

      std::string result(value);
      free(value);
      return result;
#else
      const char* value = std::getenv(name);
      return value ? value : "";
#endif
    }

    class OpenGLRenderDevice final : public IRenderDevice
    {
    public:
      GraphicsBackend backend() const override
      {
        return GraphicsBackend::OpenGL;
      }

      std::unique_ptr<RenderContext> create_render_context() const override
      {
        return std::make_unique<OpenGL_Context>();
      }

      std::unique_ptr<RenderContext> create_ui_context() const override
      {
        return std::make_unique<UIContext>();
      }

      std::unique_ptr<VertexIndexBuffer> create_vertex_index_buffer() const override
      {
        return std::make_unique<OpenGL_VertexIndexBuffer>();
      }

      std::unique_ptr<FrameBuffer> create_frame_buffer() const override
      {
        return std::make_unique<OpenGL_FrameBuffer>();
      }
    };

#ifdef _WIN32
    class DirectX11RenderDevice final : public IRenderDevice
    {
    public:
      GraphicsBackend backend() const override
      {
        return GraphicsBackend::DirectX11;
      }

      std::unique_ptr<RenderContext> create_render_context() const override
      {
        return std::make_unique<DirectX11_Context>();
      }

      std::unique_ptr<RenderContext> create_ui_context() const override
      {
        return std::make_unique<DirectX11_UIContext>();
      }

      std::unique_ptr<VertexIndexBuffer> create_vertex_index_buffer() const override
      {
        return nullptr;
      }

      std::unique_ptr<FrameBuffer> create_frame_buffer() const override
      {
        return nullptr;
      }
    };
#endif
  }

  const char* graphics_backend_name(GraphicsBackend backend)
  {
    switch (backend)
    {
    case GraphicsBackend::OpenGL:
      return "OpenGL";
    case GraphicsBackend::DirectX11:
      return "DirectX11";
    case GraphicsBackend::DirectX12:
      return "DirectX12";
    default:
      return "Unknown";
    }
  }

  bool try_parse_graphics_backend(const std::string& text, GraphicsBackend* out_backend)
  {
    if (!out_backend)
      return false;

    std::string normalized = text;
    std::transform(
      normalized.begin(),
      normalized.end(),
      normalized.begin(),
      [](unsigned char ch)
      {
        return static_cast<char>(std::tolower(ch));
      });

    if (normalized == "opengl" || normalized == "gl")
    {
      *out_backend = GraphicsBackend::OpenGL;
      return true;
    }

    if (normalized == "directx11" || normalized == "dx11" || normalized == "d3d11")
    {
      *out_backend = GraphicsBackend::DirectX11;
      return true;
    }

    if (normalized == "directx12" || normalized == "dx12" || normalized == "d3d12")
    {
      *out_backend = GraphicsBackend::DirectX12;
      return true;
    }

    return false;
  }

  GraphicsBackend resolve_graphics_backend_from_env(GraphicsBackend fallback_backend)
  {
    const std::string backend_text = read_environment_variable(kRenderBackendEnvVar);
    if (backend_text.empty())
      return fallback_backend;

    GraphicsBackend backend = fallback_backend;
    if (try_parse_graphics_backend(backend_text, &backend))
      return backend;

    std::cout
      << "[RenderDevice] Ignoring invalid " << kRenderBackendEnvVar
      << " value '" << backend_text
      << "'. Falling back to " << graphics_backend_name(fallback_backend) << "."
      << std::endl;
    return fallback_backend;
  }

  std::string supported_graphics_backends()
  {
#ifdef _WIN32
    return "OpenGL, DirectX11";
#else
    return "OpenGL";
#endif
  }

  RenderDeviceManager& RenderDeviceManager::instance()
  {
    static RenderDeviceManager instance;
    return instance;
  }

  bool RenderDeviceManager::initialize(GraphicsBackend backend)
  {
    if (mDevice && mBackend == backend)
      return true;

    std::shared_ptr<IRenderDevice> next_device = create_device(backend);
    if (!next_device)
    {
      std::cout
        << "[RenderDevice] Backend " << graphics_backend_name(backend)
        << " is not available. Supported backends: " << supported_graphics_backends() << "."
        << std::endl;
      return false;
    }

    mBackend = backend;
    mDevice = std::move(next_device);
    std::cout << "[RenderDevice] Active backend: " << graphics_backend_name(mBackend) << std::endl;
    return true;
  }

  std::shared_ptr<IRenderDevice> RenderDeviceManager::device()
  {
    return ensure_device();
  }

  bool RenderDeviceManager::is_backend_supported(GraphicsBackend backend) const
  {
    return static_cast<bool>(create_device(backend));
  }

  std::unique_ptr<RenderContext> RenderDeviceManager::create_render_context()
  {
    auto current_device = ensure_device();
    return current_device ? current_device->create_render_context() : nullptr;
  }

  std::unique_ptr<RenderContext> RenderDeviceManager::create_ui_context()
  {
    auto current_device = ensure_device();
    return current_device ? current_device->create_ui_context() : nullptr;
  }

  std::unique_ptr<VertexIndexBuffer> RenderDeviceManager::create_vertex_index_buffer()
  {
    auto current_device = ensure_device();
    return current_device ? current_device->create_vertex_index_buffer() : nullptr;
  }

  std::unique_ptr<FrameBuffer> RenderDeviceManager::create_frame_buffer()
  {
    auto current_device = ensure_device();
    return current_device ? current_device->create_frame_buffer() : nullptr;
  }

  std::shared_ptr<IRenderDevice> RenderDeviceManager::ensure_device()
  {
    if (!mDevice)
    {
      const GraphicsBackend resolved_backend = resolve_graphics_backend_from_env(mBackend);
      if (!initialize(resolved_backend))
        return nullptr;
    }

    return mDevice;
  }

  std::shared_ptr<IRenderDevice> RenderDeviceManager::create_device(GraphicsBackend backend) const
  {
    switch (backend)
    {
    case GraphicsBackend::OpenGL:
      return std::make_shared<OpenGLRenderDevice>();
    case GraphicsBackend::DirectX11:
#ifdef _WIN32
      return std::make_shared<DirectX11RenderDevice>();
#else
      return nullptr;
#endif
    case GraphicsBackend::DirectX12:
      return nullptr;
    default:
      return nullptr;
    }
  }
}
