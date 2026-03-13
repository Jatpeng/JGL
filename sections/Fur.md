# 毛发材质（Fur）

## 实现思路

当前毛发方案基于经典壳层（shell）多 Pass 渲染：

- 配置文件：`resource/Fur.xml`
- Shader：`shaders/fur_vs.shader` + `shaders/fur_fs.shader`
- 关键参数：
  - `Pass`：壳层次数（层数越高越蓬松，但开销更大）
  - `FurLength`：毛发长度
  - `vGravity`：重力/方向偏移
  - `FurNoiseTex`、`FurColorTex`：噪声与颜色控制

在 `SceneView::render()` 中，当材质名为 `fur` 且开启 `multipass` 时，会按 `Pass` 循环绘制模型，并通过 `PassIndex` 驱动每层偏移。

## 代码关联

- 多 Pass 判定：`SceneView::render()`
- 材质参数加载：`Material::load()`
- 参数下发 Shader：`Material::update_shader_params()`

## 使用方式

1. 在 Property 面板加载模型。
2. 点击 `OpenMaterial...` 选择 `Fur.xml`。
3. 在 Material 参数区调节 `Pass/FurLength/vGravity`，观察毛发体积变化。

![Fur 效果预览](../Images/Fur/1692618678620.png)

参考：<http://sorumi.xyz/posts/unity-fur-shader/>