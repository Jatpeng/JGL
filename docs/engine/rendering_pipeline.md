# 渲染管线（当前实现）

本文档以当前仓库代码为准，描述 `nengine::RenderEngine` 中已经接入运行时的渲染流程。

## 总览

当前渲染器包含这些核心链路：

- Forward / Deferred 双路径
- Directional Light Shadow Pass
- PBR 光照与 IBL
- Screen Effect 后处理
- G-Buffer 调试视图

默认启动时，编辑器会加载 `Assets/scenes/default_scene.xml`，该场景会同时启用方向光阴影、HDR 环境图和地面平面，用来直接验证这套链路。

## 每帧流程

### Forward

1. 更新场景与相机状态
2. 执行方向光阴影 pass
3. 渲染不透明 mesh
4. 可选渲染地面
5. 可选渲染 skybox
6. 叠加透明物体
7. 如配置了 screen effect，再执行一次全屏后处理

### Deferred

1. 更新场景与相机状态
2. 执行方向光阴影 pass
3. Geometry Pass 写入 `DeferredGBuffer`
4. Lighting Pass 合成到主离屏 framebuffer
5. Forward Overlay 叠加 skybox 和透明物体
6. 如配置了 screen effect，再执行一次全屏后处理

## 阴影映射

当前阴影系统已经接入主渲染流程。

- 仅使用第一个满足条件的 Directional Light 作为阴影光源
- 该光源必须 `enabled == true`
- 该光源必须 `type == Directional`
- 该光源必须 `casts_shadows == true`

阴影 pass 的实现特点：

- 根据场景包围盒动态计算 light-space 矩阵
- 使用独立的深度 framebuffer 和深度纹理
- Forward 和 Deferred 光照阶段都会采样同一张 shadow map
- 支持 PCF 过滤半径
- 支持最小 / 最大 bias 调节

当前限制：

- 只有一个方向光阴影源
- Point Light 仍然不生成 shadow map

## IBL 与环境光

当前 IBL 已经接入主渲染流程。

运行时支持两类环境输入：

- 内置 skybox cubemap
- 外部 `.hdr` 环境贴图

无论来源是哪一种，最终都会生成并用于着色的资源：

- Environment Cubemap
- Irradiance Map
- Prefilter Map
- BRDF LUT

当前默认展示场景会加载 `Assets/environments/newport_loft/Newport_Loft_Env.hdr` 作为环境源；如果未加载外部环境图，则回退到内置 skybox。

## 材质与延迟可用性

Deferred 不是无条件启用的。当前 mesh 只有在材质输入完整时，才会进入 deferred geometry pass。

必须具备的纹理槽位：

- `baseMap`
- `metallicMap`
- `roughnessMap`
- `normalMap`
- `aoMap`

如果任意 mesh 不满足这一条件，渲染器会自动退回 Forward 路径。

## Screen Effect 后处理

后处理由 `nrender::PostProcessStack` 驱动。

固定资源约定：

- 全屏顶点着色器：`JGL_Engine/shaders/post_process_vs.shader`
- effect 片元着色器：由 effect material 决定
- effect material 常放在 `Assets/screen_effects/`

每帧固定下发的 uniform：

- `hdrBuffer`
- `time`
- `screenResolution`

## Debug View

Deferred 模式下支持以下调试输出：

- Final
- Position
- Normal
- Albedo
- Roughness
- Metallic

SceneView 中还提供了 G-Buffer 缩略图预览，用于快速切换这些视图。

## 编辑器入口

这些渲染能力在编辑器中的控制位置如下：

- 阴影参数：`Inspector -> Light`
- HDR / IBL：`Render Settings -> Environment`
- Forward / Deferred 与 Debug View：`Render Settings -> Render`
- Shader 热重载：`Render Settings -> Render` 或 `Scene` 工具栏
- 后处理：`Render Settings -> Screen Effects`
