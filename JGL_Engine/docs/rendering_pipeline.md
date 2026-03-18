# Advanced Rendering Pipeline: IBL & Post-Processing

This document describes the architectural design and implementation principles of the newly integrated Image-Based Lighting (IBL) and Post-Processing stack in the JGL Engine.

## Architectural Overview

The rendering engine now supports a high dynamic range (HDR) workflow from the start of the rendering passes (forward or deferred) until the final image is output to the screen.
Two major components have been introduced to accomplish this:
1. **IBL Pipeline (`nrender::IBLPipeline`)**: Responsible for loading HDR environment maps and pre-computing the necessary textures (Irradiance Map, Prefiltered Environment Map, and BRDF LUT) required for Physically Based Rendering (PBR).
2. **Post-Processing Stack (`nrender::PostProcessStack`)**: A framework applied at the end of the rendering pass that handles HDR tone mapping and gamma correction before rendering to the default framebuffer (screen).

## Image-Based Lighting (IBL)

IBL provides highly realistic ambient lighting by treating the surrounding environment as a complex light source. The `IBLPipeline` component automates the generation of the three essential maps:

1. **Equirectangular to Cubemap Conversion**: The pipeline loads an HDR equirectangular map (e.g., `.hdr`) using `stbi_loadf`. It then renders this map onto a unit cube from the perspective of its center to generate an environment cubemap (`GL_RGB16F`).
2. **Irradiance Map (Diffuse Ambient)**: By convolving the environment cubemap, we pre-calculate the diffuse irradiance for every normal direction. This convolution uses a specialized shader (`irradiance_convolution.fs`) that samples the hemisphere above each normal.
3. **Prefiltered Environment Map (Specular Ambient)**: To approximate specular reflections for different material roughness values, the environment map is prefiltered at varying mipmap levels. Higher mip levels correspond to rougher surfaces (more scattered reflections), utilizing importance sampling based on the GGX normal distribution function (`prefilter.fs`).
4. **BRDF LUT**: A 2D Lookup Texture (`brdf.fs`) is pre-computed to store the BRDF response (scale and bias to $F_0$) given the dot product between the surface normal and view direction ($N \cdot V$) and surface roughness. This is independent of the environment map and only needs to be generated once.

These maps are stored internally by `IBLPipeline` and are bound to specific texture units (`GL_TEXTURE5`, `GL_TEXTURE6`, `GL_TEXTURE7` for forward rendering, and `GL_TEXTURE3`, `GL_TEXTURE4`, `GL_TEXTURE5` for deferred lighting) before rendering scene meshes or performing the deferred lighting pass. The PBR shaders (`pbr_fs.shader`, `deferred_lighting_fs.shader`) then utilize these textures to compute the ambient diffuse and specular reflection based on the split-sum approximation.

## High Dynamic Range (HDR) & Post-Processing

### HDR Framebuffer
To accurately accumulate light intensities without clamping to the standard $0.0 - 1.0$ LDR range, the main framebuffer (`nrender::OpenGL_FrameBuffer`) now utilizes `GL_RGBA16F` as its internal color format with a `GL_FLOAT` data type. All geometry and lighting passes render to this HDR target.

### Post-Processing Stack
The `PostProcessStack` manages a fullscreen quad and a post-processing shader (`post_process_fs.shader`) which acts as the final stage of the rendering pipeline.

1. **Tone Mapping**: The engine employs the ACES (Academy Color Encoding System) filmic tone mapping curve. This curve maps the unbounded HDR color values smoothly down to the displayable $0.0 - 1.0$ range while preserving contrast in bright areas and avoiding harsh clipping (burn-out).
2. **Gamma Correction**: After tone mapping, the linear color space values are transformed into sRGB space by applying a gamma correction of $1 / 2.2$. This ensures that colors are perceived correctly on standard monitors.

By isolating tone mapping and gamma correction in a dedicated post-processing pass, the rest of the rendering pipeline and shaders remain strictly in linear space, greatly simplifying lighting calculations and ensuring physical correctness.
