# 深入解析 JGL Engine 中的基于物理渲染 (PBR) 系统

在现代游戏开发和图形学领域，**基于物理的渲染 (Physically Based Rendering, PBR)** 已经成为了不可或缺的基石。相比于传统的 Phong 或 Blinn-Phong 模型，PBR 通过严谨的数学物理方程来模拟光线与物体表面的相互作用，从而使得在不同的光照环境下材质能够保持一致性与真实感。

本文将作为一份详细的教学指南，带领大家深入了解 JGL Engine 中是如何从零构建和实现一套商业级 PBR 材质与 Shader 渲染管线的。

---

## 1. PBR 核心概念解析

JGL Engine 采用的是业界最流行的 **Metallic-Roughness (金属度-粗糙度)** 工作流。这种工作流直观、易懂，且能够有效避免美术在制作材质时产生物理上不合理的参数。

一个标准的 PBR 材质通常需要提供以下几张核心纹理贴图：
*   **Albedo (反照率/基础色贴图)**：对于非金属（绝缘体），它表示漫反射的颜色；对于金属（导体），它表示镜面反射的初始颜色（F0）。
*   **Metallic (金属度贴图)**：通常是一张灰度图。黑色（0）代表非金属，白色（1）代表金属。
*   **Roughness (粗糙度贴图)**：灰度图。控制微表面的粗糙程度。值越小表面越光滑，反射越清晰锐利；值越大表面越粗糙，反射越模糊。
*   **Normal (法线贴图)**：在切线空间下扰动法线方向，用以在不增加多边形模型复杂度的前提下模拟出丰富的表面凹凸细节。
*   **AO (Ambient Occlusion, 环境光遮蔽贴图)**：用来描述表面被自投影或周围几何体遮挡的程度，用来削弱间接光（环境光）对暗部的影响。

---

## 2. PBR 数学核心：Cook-Torrance BRDF

我们如何计算光线打在物体表面后进入摄像机的颜色？答案是求解渲染方程。在实时渲染中，我们使用的是 **Cook-Torrance 微面元 BRDF (双向反射分布函数)** 模型。

Cook-Torrance BRDF 由漫反射（Diffuse）和镜面反射（Specular）两部分组成：
$$ f_r = k_d f_{lambert} + k_s f_{cook-torrance} $$

### 2.1 镜面反射项 (Specular)

镜面反射项的公式是 PBR 的核心，它由三个重要函数构成，被统称为 DGF：
$$ f_{cook-torrance} = \frac{D \cdot G \cdot F}{4 (\omega_o \cdot n)(\omega_i \cdot n)} $$

在 JGL Engine 的片段着色器 (`pbr_fs.shader`) 中，这三部分的具体实现如下：

**1. 法向分布函数 (Normal Distribution Function, NDF - 常用 GGX/Trowbridge-Reitz)**
估算微表面上有多少比例的微面元恰好朝向半程向量 $H$（光线方向 $L$ 和视线方向 $V$ 的中间向量）。表面越粗糙，朝向越分散。
```glsl
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    // 防止除以0
    return nom / max(denom, 0.0000001);
}
```

**2. 几何函数 (Geometry Function - 常用 Schlick-GGX)**
描述了微表面相互遮挡（Shadowing）和自身遮挡（Masking）的概率。粗糙度越高，被遮挡丢失的光线越多。
```glsl
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
```

**3. 菲涅尔方程 (Fresnel Equation - 常用 Fresnel-Schlick 近似)**
描述了光线被反射的比例随着观察角度变化的关系。视线越与表面平行（掠角），反射的光越强。在这里需要引入基础反射率 `F0`。绝缘体的 `F0` 一般固定在 0.04 左右，而金属的 `F0` 则是它原本的颜色。
```glsl
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
```

---

## 3. JGL Engine 的材质管线架构

了解了底层 Shader 的数学原理后，我们来看看在 C++ 引擎端是如何把模型、纹理和 Shader 串联起来的。

### 3.1 `Material` 类的定义
在 `JGL_Engine/source/elems/material.h` 中，`Material` 类充当了数据桥梁。它负责：
1. **数据解析**：引擎并不把 Shader 参数硬编码在 C++ 中，而是通过解析 XML 文件实现数据驱动。
2. **状态绑定**：提供 `update_shader_params(nshaders::Shader* shader)` 方法，在每次 Draw Call 之前，自动遍历映射表，把标量（如 `ao` 强度）和各类纹理（`Albedo`, `Normal`, `Metallic`, `Roughness`, `AO`）绑定到 OpenGL 的对应纹理单元和 Uniform 变量上。

### 3.2 数据驱动的 XML 资产配置
以我们的测试材质 `Assets/PBR.xml` 为例：
```xml
<?xml version="1.0" encoding="utf-8"?>
<Material Name="PBR" multipass="false" shader="JGL_MeshLoader/shaders/pbr_fs.shader">
  <Param Name="baseMap" Type="Texture" Default="Assets/textures/PBR/albedo.png" />
  <Param Name="metallicMap" Type="Texture" Default="Assets/textures/PBR/metallic.png" />
  <Param Name="roughnessMap" Type="Texture" Default="Assets/textures/PBR/roughness.png" />
  <Param Name="normalMap" Type="Texture" Default="Assets/textures/PBR/normal.png" />
  <Param Name="aoMap" Type="Texture" Default="Assets/textures/PBR/ao.png" />
  <Param Name="color" Type="float3" Default="1.0,1.0,1.0" />
</Material>
```
当关卡或场景加载时，`SceneObject` (如 `MeshObject`) 会解析这个 XML。它告诉引擎：“嗨，我需要使用 `pbr_fs.shader` 这个 Shader，并且请把我的这五张 PBR 贴图加载进来，绑定到 Shader 里名字对应的 Uniform 变量上”。

### 3.3 渲染引擎 (`RenderEngine`) 的智能分发
JGL Engine 支持**前向渲染 (Forward Rendering)**和**延迟渲染 (Deferred Rendering)**。在 `render_engine.cpp` 中，引擎有一个非常智能的特性：
```cpp
bool RenderEngine::is_mesh_deferred_available(const MeshObject& mesh_object) const {
    auto& texture_map = material->getTextureMap();
    return texture_map.find("baseMap") != texture_map.end() &&
           texture_map.find("metallicMap") != texture_map.end() &&
           texture_map.find("roughnessMap") != texture_map.end() &&
           texture_map.find("normalMap") != texture_map.end() &&
           texture_map.find("aoMap") != texture_map.end();
}
```
引擎会在绘制前检查一个网格模型 (Mesh) 的材质是否拥有全部必备的 PBR 参数贴图。如果不满足，它可以将其降级或放入前向渲染管线中单独处理，这大大增强了引擎的鲁棒性。

---

## 4. 光照计算与颜色空间的最终输出

回到 `pbr_fs.shader`，最后在 `main()` 函数中，我们完成了光线的累加。

1. **颜色空间处理 (sRGB to Linear)**：美术制作的 Albedo 贴图通常位于 sRGB 空间。在做物理运算前，必须用 `pow(texture, 2.2)` 将其转换到线性空间 (Linear Space)。
2. **计算基础反射率 (F0)**：通过 Metallic 属性混合非金属的默认 0.04 反射率和金属原本颜色。
3. **累加光照 (Lo)**：循环遍历场景中的光照（例如点光源），分别累加它们的贡献值。
4. **施加环境光遮蔽 (AO)**：
   ```glsl
   float ao = texture(aoMap, TexCoords).r;
   vec3 ambient = vec3(0.03) * albedo * ao; // 基础环境光受 AO 贴图影响变暗
   vec3 color = ambient + Lo;
   ```
5. **色调映射 (Tonemapping) 和 Gamma 校正**：
   累加后的光照很容易超出 `[0, 1]` 的范围。我们通过 Reinhard 色调映射 (`color / (color + vec3(1.0))`) 将其压缩，最后通过 `pow(color, 1.0/2.2)` 重新进行 Gamma 校正输出给显示器。

## 5. 总结

通过引入 `Material` 类，利用 XML 将 Shader 参数外部化，并搭配完善的 Cook-Torrance GLSL 着色器实现，JGL Engine 成功地从早期的硬编码 Phong 渲染管线平滑过渡到了现代化的 PBR 体系。
这套架构不仅提高了代码解耦程度，还让技术美术能极其方便地制作具有高保真物理真实感的游戏场景。希望这篇文档能对你在学习引擎架构和渲染原理的道路上有所帮助！