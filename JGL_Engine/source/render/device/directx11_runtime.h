#pragma once

#ifdef _WIN32

#include <cstdint>

struct GLFWwindow;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct IDXGISwapChain;

namespace nrender
{
  class DirectX11Runtime
  {
  public:
    static DirectX11Runtime& instance();

    bool initialize(GLFWwindow* window, int width, int height);
    void shutdown();
    void resize(int width, int height);

    void begin_frame(float r, float g, float b, float a);
    void set_render_target();
    void end_frame();

    bool is_initialized() const { return mDevice != nullptr && mDeviceContext != nullptr && mSwapChain != nullptr; }
    ID3D11Device* device() const { return mDevice; }
    ID3D11DeviceContext* device_context() const { return mDeviceContext; }
    ID3D11RenderTargetView* render_target_view() const { return mRenderTargetView; }
    IDXGISwapChain* swap_chain() const { return mSwapChain; }

  private:
    DirectX11Runtime() = default;
    ~DirectX11Runtime() = default;

    bool create_device_and_swap_chain(GLFWwindow* window, int width, int height);
    bool create_render_target();
    void release_render_target();

    GLFWwindow* mWindow = nullptr;
    ID3D11Device* mDevice = nullptr;
    ID3D11DeviceContext* mDeviceContext = nullptr;
    IDXGISwapChain* mSwapChain = nullptr;
    ID3D11RenderTargetView* mRenderTargetView = nullptr;
  };
}

#endif
