# 模拟 2D 天气效果

## 当前实现状态

项目中已包含天气/水面相关资源与 Shader 雏形，重点在于“基于法线扰动的流动水面效果”：

- 贴图资源：`resource/textures/weather/`
  - `base.png`
  - `water_bump_map.png`
  - `droplet_tex.png`
  - `nosie.png`
- Shader：`shaders/river_fs.shader`
  - 通过 `_RiverParam01` 控制流速与 UV 扰动缩放。
  - 使用法线贴图与高光近似模拟水面流动。

## Shader 逻辑说明（river_fs）

核心流程：

1. 读取 `baseMap` 的 alpha 作为“河流遮罩”。
2. 对 `waterbumpMap` 做滚动采样，生成动态 UV 扰动。
3. 用扰动后的法线参与简单高光计算，叠加到底图颜色。

## 如何在编辑器中接入

目前默认加载材质为 `PBR.xml`。如需体验天气/水面效果，可新增或修改材质 XML，绑定 `river_fs.shader` 对应参数，并在 Property 面板加载该 XML。

## 后续可扩展方向

- 叠加雨滴法线与屏幕空间涟漪。
- 增加风场控制，实现分区域流向。
- 将天气参数（降雨强度、流速、扰动强度）暴露到 ImGui 面板。

![天气效果预览](../Images/Weather/image-20230521172004800.png)