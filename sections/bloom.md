# 特效（Bloom）

## 当前状态

项目已提供 Bloom 材质配置与 Shader 文件：

- 配置：`resource/Bloom.xml`
- Shader：`shaders/bloom_vs.shader` + `shaders/bloom_fs.shader`

目前 `bloom_fs.shader` 仍是占位实现（输出固定红色），尚未完成完整 Bloom 后处理链路。

## 已接入能力

- 可通过 Property 面板加载 `Bloom.xml`。
- 材质参数（贴图/float/float3）可沿用现有 `Material` 系统下发。

## 建议的完整 Bloom 实现路径

1. 场景渲染到 HDR FBO（浮点纹理）。
2. 提取亮部（threshold pass）。
3. 对亮部做高斯模糊（ping-pong FBO）。
4. 与原图做加权合成（tone mapping + gamma）。

## 备注

如需快速演示“粒子特效”，建议先在当前架构中增加独立的粒子系统模块，并通过 `SceneView` 接入到同一渲染循环。
