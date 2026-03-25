#include "pch.h"

#ifdef _WIN32

#include "render/device/directx11_runtime.h"

#ifndef GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

#include <d3d11.h>
#include <dxgi.h>

namespace nrender
{
  namespace
  {
    void release_com_object(IUnknown** object)
    {
      if (!object || !*object)
        return;

      (*object)->Release();
      *object = nullptr;
    }
  }

  DirectX11Runtime& DirectX11Runtime::instance()
  {
    static DirectX11Runtime instance;
    return instance;
  }

  bool DirectX11Runtime::initialize(GLFWwindow* window, int width, int height)
  {
    if (is_initialized() && mWindow == window)
      return true;

    shutdown();
    return create_device_and_swap_chain(window, width, height) && create_render_target();
  }

  void DirectX11Runtime::shutdown()
  {
    release_render_target();
    release_com_object(reinterpret_cast<IUnknown**>(&mSwapChain));
    release_com_object(reinterpret_cast<IUnknown**>(&mDeviceContext));
    release_com_object(reinterpret_cast<IUnknown**>(&mDevice));
    mWindow = nullptr;
  }

  void DirectX11Runtime::resize(int width, int height)
  {
    if (!is_initialized() || width <= 0 || height <= 0)
      return;

    release_render_target();
    mSwapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0);
    create_render_target();
  }

  void DirectX11Runtime::begin_frame(float r, float g, float b, float a)
  {
    if (!is_initialized())
      return;

    const float clear_color[4] = { r, g, b, a };
    set_render_target();
    mDeviceContext->ClearRenderTargetView(mRenderTargetView, clear_color);
  }

  void DirectX11Runtime::set_render_target()
  {
    if (!is_initialized())
      return;

    mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);
  }

  void DirectX11Runtime::end_frame()
  {
    if (!is_initialized())
      return;

    mSwapChain->Present(1, 0);
  }

  bool DirectX11Runtime::create_device_and_swap_chain(GLFWwindow* window, int width, int height)
  {
    if (!window)
      return false;

    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd)
      return false;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc {};
    swap_chain_desc.BufferDesc.Width = static_cast<UINT>(std::max(width, 1));
    swap_chain_desc.BufferDesc.Height = static_cast<UINT>(std::max(height, 1));
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.OutputWindow = hwnd;
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT create_flags = 0;
#if defined(_DEBUG)
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] = {
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL created_feature_level = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      create_flags,
      feature_levels,
      static_cast<UINT>(std::size(feature_levels)),
      D3D11_SDK_VERSION,
      &swap_chain_desc,
      &mSwapChain,
      &mDevice,
      &created_feature_level,
      &mDeviceContext);

    if (FAILED(hr))
    {
      hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_WARP,
        nullptr,
        0,
        feature_levels,
        static_cast<UINT>(std::size(feature_levels)),
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &mSwapChain,
        &mDevice,
        &created_feature_level,
        &mDeviceContext);
    }

    if (FAILED(hr))
      return false;

    mWindow = window;
    return true;
  }

  bool DirectX11Runtime::create_render_target()
  {
    if (!mSwapChain || !mDevice)
      return false;

    ID3D11Texture2D* back_buffer = nullptr;
    if (FAILED(mSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))) || !back_buffer)
      return false;

    const HRESULT hr = mDevice->CreateRenderTargetView(back_buffer, nullptr, &mRenderTargetView);
    back_buffer->Release();
    return SUCCEEDED(hr) && mRenderTargetView != nullptr;
  }

  void DirectX11Runtime::release_render_target()
  {
    release_com_object(reinterpret_cast<IUnknown**>(&mRenderTargetView));
  }
}

#endif
