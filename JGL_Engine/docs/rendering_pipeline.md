# 高级渲染管线：基于图像的照明 (IBL) 与 后处理 (Post-Processing)

本文档介绍了 JGL 引擎中新集成的基于图像的照明 (IBL) 和后处理栈的架构设计与实现原理。

## 架构概述

渲染引擎现在支持高动态范围 (HDR) 工作流，从渲染通道（前向或延迟渲染）开始，直到最终图像输出到屏幕。
为了实现这一目标，引入了两个主要组件：
1. **IBL 管线 (`nrender::IBLPipeline`)**：负责加载 HDR 环境贴图，并预计算基于物理的渲染 (PBR) 所需的必要纹理（辐照度贴图 Irradiance Map、预过滤环境贴图 Prefiltered Environment Map 和 BRDF 查找表 LUT）。
2. **后处理栈 (`nrender::PostProcessStack`)**：在渲染通道结束时应用的一个框架，负责在渲染到默认帧缓冲（屏幕）之前进行 HDR 色调映射 (Tone Mapping) 和伽马校正 (Gamma Correction)。

## 基于图像的照明 (IBL)

IBL 通过将周围环境视为复杂的光源，提供高度逼真的环境光照。`IBLPipeline` 组件自动生成三个基本贴图：

1. **等距柱状投影到立方体贴图的转换 (Equirectangular to Cubemap)**：管线使用 `stbi_loadf` 加载 HDR 等距柱状投影贴图（例如 `.hdr` 文件）。然后，它从中心视角将该贴图渲染到单位立方体上，生成环境立方体贴图 (`GL_RGB16F`)。
2. **辐照度贴图 (漫反射环境光 Irradiance Map)**：通过对环境立方体贴图进行卷积计算，我们为每个法线方向预先计算了漫反射辐照度。此卷积过程使用专用着色器 (`irradiance_convolution.fs`) 在每个法线上方的半球进行采样。
3. **预过滤环境贴图 (镜面反射环境光 Prefiltered Environment Map)**：为了近似不同材质粗糙度值的镜面反射，环境贴图在不同的 Mipmap 级别进行了预过滤。较高的 Mip 级别对应较粗糙的表面（反射更加散射），利用基于 GGX 法线分布函数的重点采样 (Importance Sampling) (`prefilter.fs`) 实现。
4. **BRDF 查找表 (BRDF LUT)**：预先计算的 2D 查找纹理 (`brdf.fs`)，用于存储给定表面法线与视线方向的点积 ($N \cdot V$) 以及表面粗糙度时的 BRDF 响应（缩放和偏移至 $F_0$）。它独立于环境贴图，只需要生成一次。

这些贴图由 `IBLPipeline` 在内部存储，并在渲染场景网格或执行延迟光照通道之前绑定到特定的纹理单元（前向渲染为 `GL_TEXTURE5`, `GL_TEXTURE6`, `GL_TEXTURE7`；延迟光照为 `GL_TEXTURE3`, `GL_TEXTURE4`, `GL_TEXTURE5`）。然后，PBR 着色器（`pbr_fs.shader`, `deferred_lighting_fs.shader`）利用这些纹理根据分离和近似 (split-sum approximation) 计算环境漫反射和镜面反射。

## 高动态范围 (HDR) 与 后处理

### HDR 帧缓冲
为了准确累积光照强度而不被截断到标准的 $0.0 - 1.0$ LDR（低动态范围）范围，主帧缓冲 (`nrender::OpenGL_FrameBuffer`) 现在使用 `GL_RGBA16F` 作为其内部颜色格式，并使用 `GL_FLOAT` 数据类型。所有的几何和光照通道都渲染到此 HDR 目标。

### 后处理栈
`PostProcessStack` 管理一个全屏四边形和一个后处理着色器 (`post_process_fs.shader`)，作为渲染管线的最后阶段。

1. **色调映射 (Tone Mapping)**：引擎采用 ACES (Academy Color Encoding System) 电影级色调映射曲线。这条曲线将无界的 HDR 颜色值平滑地映射回显示器可显示的 $0.0 - 1.0$ 范围，同时保留亮部区域的对比度，避免生硬的过曝。
2. **伽马校正 (Gamma Correction)**：在色调映射之后，线性颜色空间的值通过应用 $1 / 2.2$ 的伽马校正转换到 sRGB 空间。这确保了颜色在标准显示器上能够被正确感知。

通过在专用的后处理通道中隔离色调映射和伽马校正，渲染管线的其余部分和着色器可以严格保持在线性空间中，这极大地简化了光照计算并确保了物理上的正确性。
