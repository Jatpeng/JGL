# 渲染管线（以当前实现为准）

> 本文档以当前代码实现为唯一准绳；设计里写了但未接入/未实现的部分会删除或明确标注为“未接入”。

当前渲染链路由 `nengine::RenderEngine` 驱动，包含：

- **Forward / Deferred 两条渲染路径**：输出到引擎的离屏 `OpenGL_FrameBuffer`
- **可选 Screen Effect（后处理）**：通过 `nrender::PostProcessStack` 对离屏颜色纹理做一次全屏 Pass

## 主离屏帧缓冲（当前为 LDR）

`nrender::OpenGL_FrameBuffer` 当前创建的颜色附件为 **`GL_RGBA` + `GL_UNSIGNED_BYTE`**（LDR 8-bit），深度为 **`GL_DEPTH24_STENCIL8`**。

这意味着：

- 当前管线**不以 HDR 工作流为基础**（没有 `GL_RGBA16F` 主颜色缓冲）。
- Screen Effect 的输入 uniform 名仍叫 `hdrBuffer`，但它实际拿到的是 **LDR 颜色纹理**（命名不等于 HDR）。

## RenderEngine 渲染流程（简化版）

- **Forward 模式**：
  - 渲染场景 mesh（不透明）
  -（可选）渲染地面
  -（可选）渲染天空盒
  -（可选）透明 mesh overlay（alpha blend）
- **Deferred 模式**：
  - Geometry Pass：写入 `DeferredGBuffer`
  - Lighting Pass：全屏合成到 `OpenGL_FrameBuffer`
  - Forward overlay：天空盒、透明 mesh overlay
  - Debug View：在 `deferred_lighting_fs.shader` 里通过 `debugView` 切换输出（Final / Position / Normal / Albedo / Roughness / Metallic）
- **Screen Effect（可选）**：
  - 若 `PostProcessStack` 配置了 effect material，则对 `OpenGL_FrameBuffer` 的颜色纹理做一次全屏 Pass，输出到 `PostProcessStack` 的内部 `OpenGL_FrameBuffer`
  - 若未配置 effect material，则最终输出仍为 `OpenGL_FrameBuffer` 的颜色纹理

## Screen Effect（PostProcessStack）机制

### 资源组成

- 全屏 VS：`JGL_Engine/shaders/post_process_vs.shader`
- effect FS：由 effect material 的 `shader` 字段指定（例如雨雪）
- effect material：XML / MTL（通常放在 `Assets/screen_effects/`）

### 固定输入 uniform（由引擎设置）

`PostProcessStack::render()` 每帧固定设置：

- `hdrBuffer`（`sampler2D`）：绑定到 `GL_TEXTURE0`
- `time`（`float`）：秒
- `screenResolution`（`vec2`）：(width, height)

### material 参数下发与纹理槽约定

在绑定 `hdrBuffer`（texture unit 0）后，引擎会调用：

- `mEffectMaterial->update_shader_params(shader, 1)`

含义是：

- effect material 自己的贴图参数从 **texture unit 1** 开始绑定
- 例如 `Rain.xml` 的 `noiseMap` 会占用 `GL_TEXTURE1`

## IBL（未接入到主渲染）

代码中存在 `nrender::IBLPipeline`（可生成 environment cubemap / irradiance / prefilter / BRDF LUT），但当前 `RenderEngine` 渲染流程 **未调用/未接入 IBLPipeline**，因此本文档不把 IBL 视为已集成功能。


