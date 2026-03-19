# 特效（Bloom）

## 当前状态

项目已提供 Bloom 材质配置与 Shader 文件：

- 配置：`Assets/Bloom.xml`
- Shader：`shaders/bloom_vs.shader` + `shaders/bloom_fs.shader`

目前 `bloom_fs.shader` 仍是占位实现（输出固定红色），尚未完成完整 Bloom 后处理链路。
当前主离屏帧缓冲为 LDR 颜色格式，因此文档中的 HDR Bloom 流程仍属于规划项。

## 已接入能力

- 可通过 Property 面板加载 `Bloom.xml`。
- 材质参数（贴图/float/float3）可沿用现有 `Material` 系统下发。

## 规划中的 Bloom 路径（未实现）

1. 场景渲染到 HDR FBO（浮点纹理）。
2. 提取亮部（threshold pass）。
3. 对亮部做高斯模糊（ping-pong FBO）。
4. 与原图做加权合成（tone mapping + gamma）。

## 备注

当前文档不再将 Bloom 视为已接入能力；后续实现应接入 `RenderEngine` 的现有后处理链路（`PostProcessStack`）。

