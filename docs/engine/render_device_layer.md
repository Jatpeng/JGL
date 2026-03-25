# 渲染设备层模块说明

## 1. 模块目标

`render/device/` 目录用于集中管理渲染设备层相关代码，职责是把“选择哪种图形后端”和“如何创建对应的上下文/缓冲对象”从上层引擎逻辑中抽离出来。

这层抽象的目标有三个：

- 让 `Engine`、`GLWindow`、`RenderEngine`、`Mesh` 和编辑器面板不再直接依赖某个具体图形 API 的实现类。
- 为 OpenGL 之外的后端保留统一入口，当前已经接入了 DirectX 11 的窗口与 UI 运行时骨架。
- 把设备对象创建逻辑集中到一处，避免 FrameBuffer、VertexBuffer、UIContext 在多个模块里各自硬编码后端实现。

## 2. 目录结构

```text
JGL_Engine/source/render/
├─ render_base.h
├─ deferred_gbuffer.*
├─ renderdoc_capture.*
├─ post_process/
├─ ibl/
└─ device/
   ├─ render_device.*
   ├─ opengl_context.*
   ├─ opengl_buffer_manager.*
   ├─ ui_context.*
   ├─ directx11_context.*
   ├─ directx11_ui_context.*
   └─ directx11_runtime.*
```

其中：

- `render_base.h` 定义跨后端共用的抽象接口，例如 `RenderContext`、`VertexIndexBuffer`、`FrameBuffer`。
- `device/render_device.*` 提供后端枚举、环境变量解析、设备创建和全局设备管理。
- `device/opengl_*` 和 `device/ui_context.*` 是当前完整可运行的 OpenGL 设备实现。
- `device/directx11_*` 是 Windows 下的 DirectX 11 运行时骨架，用于承接窗口、交换链和 ImGui。

## 3. 核心对象

### 3.1 `GraphicsBackend`

用于描述当前图形后端，当前枚举值包括：

- `OpenGL`
- `DirectX11`
- `DirectX12`

目前真正接通的后端是 `OpenGL` 和部分 `DirectX11` 路径，`DirectX12` 仍为保留占位。

### 3.2 `IRenderDevice`

`IRenderDevice` 是设备层对上暴露的统一接口，负责创建：

- 渲染上下文 `RenderContext`
- UI 上下文 `RenderContext`
- 顶点/索引缓冲对象 `VertexIndexBuffer`
- 帧缓冲对象 `FrameBuffer`

上层模块只依赖这些抽象，不直接 `new OpenGL_*` 或 `new DirectX11_*`。

### 3.3 `RenderDeviceManager`

`RenderDeviceManager` 是设备层的入口，负责：

- 初始化当前后端
- 缓存活动设备实例
- 对外分发上下文、缓冲和帧缓冲创建请求
- 提供后端名称、支持情况和环境变量解析能力

当前调用点包括：

- `Engine::init()`：初始化活动图形后端
- `GLWindow::init()`：创建主渲染上下文
- `EditorOverlay::on_attach()`：创建 UI 上下文
- `Mesh`：创建顶点/索引缓冲
- `RenderEngine` / `PostProcessStack`：创建帧缓冲

## 4. 初始化流程

设备层的典型启动顺序如下：

1. `Engine::CreateInfo.render_backend` 提供默认后端。
2. `resolve_graphics_backend_from_env()` 再读取环境变量 `JGL_RENDER_BACKEND`，允许在不改代码的情况下切换后端。
3. `RenderDeviceManager::initialize()` 创建对应的 `IRenderDevice` 实例。
4. `GLWindow` 通过设备层创建渲染上下文。
5. 编辑器通过设备层创建 UI 上下文。
6. 场景渲染、后处理和网格缓冲都从同一设备层请求底层对象。

这使得后端切换不再分散在多个子系统中。

## 5. 当前实现状态

### 5.1 OpenGL

OpenGL 仍然是当前最完整的运行路径，已经覆盖：

- 窗口上下文创建
- ImGui 编辑器上下文
- Mesh 顶点/索引缓冲
- FrameBuffer
- Forward / Deferred 场景渲染
- IBL、阴影和后处理链路

### 5.2 DirectX11

DirectX11 当前主要完成了设备层基础设施：

- GLFW 无图形 API 窗口创建
- D3D11 Device / DeviceContext / SwapChain 初始化
- 主渲染目标创建与窗口尺寸同步
- ImGui DX11 后端接入

但运行时场景渲染仍然是 OpenGL-only，因此当前行为是：

- 可以切到 `DirectX11` 设备路径。
- 编辑器窗口和 UI 上下文可以创建。
- `RenderEngine` 会明确提示当前场景渲染尚未实现。
- `ResourceManager` 在非 OpenGL 路径下不会继续创建 shader、texture、cubemap 等运行时资源。

这让设备层抽象先落地，再逐步补齐真正的跨后端渲染器。

## 6. 扩展建议

如果后续要继续补全 DirectX11 或引入新后端，建议按下面顺序推进：

1. 先实现 `IRenderDevice` 需要的上下文、缓冲和帧缓冲工厂。
2. 再补齐 shader、texture、material、frame graph 等运行时资源路径。
3. 最后把 `RenderEngine` 中仍然直接依赖 OpenGL 的部分逐步下沉到后端实现。

遵循这个顺序，可以保证设备层继续作为统一入口，而不是重新回到“上层逻辑直接依赖具体 API”的状态。
