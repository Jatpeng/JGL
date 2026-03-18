# Physically Based Rendering (PBR) Implementation in JGL Engine

This document explains the architecture, principles, and implementation details of the PBR (Physically Based Rendering) Material and Shader pipeline in the JGL Engine. This guide is designed to help developers and technical artists understand how materials are handled, from asset definition to final pixel shading.

## 1. Core Principles of PBR

Physically Based Rendering aims to simulate the interaction of light with surfaces in a physically accurate manner. The JGL Engine implements a metallic-roughness workflow based on the Cook-Torrance microfacet BRDF model.

The essential inputs for a standard PBR material are:
*   **Albedo (Base Color):** The diffuse color of non-metals, or the specular reflectance color of metals.
*   **Metallic:** A mask defining whether a surface behaves like a metal (conductor) or a dielectric (insulator).
*   **Roughness:** Controls the microsurface smoothness. A low value results in sharp, mirror-like reflections, while a high value causes blurred, diffuse-looking reflections.
*   **Normal:** A tangent-space vector map used to simulate geometric detail without adding polygons.
*   **Ambient Occlusion (AO):** An approximation of self-shadowing, simulating how exposed a point is to ambient lighting.

## 2. Architecture of the Material System

The material pipeline translates high-level asset definitions into low-level shader parameters.

### `Material` Class (`JGL_Engine/source/elems/material.h`)
The `Material` class acts as the bridge between disk files (`.xml` definitions) and the GPU shaders.

Key responsibilities:
1.  **Parsing:** Reads XML-based material definitions (e.g., `Assets/PBR.xml`) to identify the target shader program and collect material parameters (`float`, `float3`, `Texture`).
2.  **Storage:** Maintains maps of parameter names to their current values or loaded texture IDs (`mTexture_map`, `mFloat_map`, etc.).
3.  **Binding (`update_shader_params`):** When rendering an object, this function iterates over the stored parameters and explicitly binds them to the active `nshaders::Shader` object. Textures are bound to sequential texture units (`GL_TEXTURE0`, `GL_TEXTURE1`, etc.), and their corresponding sampler uniform names are set.

### XML Material Definitions (e.g., `Assets/PBR.xml`)
Materials are authored via XML, allowing data-driven material configuration without recompiling the engine.

Example structure:
```xml
<Material Name="PBR" multipass="false" shader="JGL_MeshLoader/shaders/pbr_fs.shader">
  <Param Name="baseMap" Type="Texture" Default="Assets/textures/PBR/albedo.png" />
  <Param Name="metallicMap" Type="Texture" Default="Assets/textures/PBR/metallic.png" />
  <Param Name="roughnessMap" Type="Texture" Default="Assets/textures/PBR/roughness.png" />
  <Param Name="normalMap" Type="Texture" Default="Assets/textures/PBR/normal.png" />
  <Param Name="aoMap" Type="Texture" Default="Assets/textures/PBR/ao.png" />
</Material>
```
When this file is loaded, the Engine parses the tags and loads the specified textures via `TextureSystem::getTextureId`.

## 3. Rendering Infrastructure

The engine supports multiple rendering paradigms, and the `RenderEngine` is responsible for evaluating which materials can be rendered via forward or deferred shading.

### `RenderEngine` (`JGL_Engine/source/engine/render_engine.cpp`)
The `RenderEngine` orchestrates the drawing of meshes. When iterating over the scene's objects:
1.  **Material Inspection:** Functions like `is_mesh_deferred_available` analyze the `Material` to see if it provides all necessary maps (`baseMap`, `metallicMap`, `roughnessMap`, `normalMap`, `aoMap`). If a map is missing, the engine might fall back to forward rendering or use default maps.
2.  **Shader Preparation:** `RenderEngine::render_mesh_object` calls `shader->use()`, sets up camera and transform uniforms (`model`, `view`, `projection`, `camPos`), and then invokes `material->update_shader_params(shader)`.
3.  **Lighting:** For forward rendering, `upload_lights` provides point light data arrays directly to the PBR shader.

## 4. PBR Shader Implementation Details

The core implementation of the Cook-Torrance BRDF resides in the forward shader, specifically `pbr_fs.shader`.

### The Reflectance Equation
The shader solves the rendering equation by accumulating lighting from multiple light sources. For each light, it calculates:
`Lo = (kD * albedo / PI + specular) * radiance * NdotL`

#### The Specular BRDF (Cook-Torrance)
The specular term calculates the microfacet reflection:
`Specular = (D * G * F) / (4 * max(N.V, 0.0) * max(N.L, 0.0))`

*   **D (Normal Distribution Function - Trowbridge-Reitz GGX):** Estimates the relative surface area of microfacets exactly aligned to the half-vector `H`.
*   **G (Geometry Function - Schlick-GGX):** Describes the self-shadowing property of the microfacets. Rougher surfaces shadow each other more.
*   **F (Fresnel Equation - Fresnel-Schlick Approximation):** Computes the ratio of light that gets reflected versus refracted at different viewing angles.

### Shader Execution Flow
1.  **Input Sampling:** The fragment shader samples all textures. The base map (`baseMap`) is expected to be in sRGB color space and is converted to linear space (`pow(texture(...), 2.2)`) before lighting calculations. Normal mapping is decoded from `[0,1]` to `[-1,1]` and transformed into world space.
2.  **F0 Calculation:** The base reflectivity at zero incidence (`F0`) is assumed to be `0.04` for dielectrics (plastics, wood, etc.). If the surface is metallic, `F0` becomes the albedo color.
3.  **Light Accumulation:** A loop runs over all active point lights. For each light, distance attenuation and the BRDF components (D, G, F) are calculated and accumulated into `Lo`.
4.  **Ambient Addition:** The `aoMap` modulates a basic ambient term `vec3(0.03) * albedo * ao`. *(Note: In a fully fleshed-out PBR pipeline, this would be replaced by Image Based Lighting (IBL) sampled from Irradiance and Prefiltered Environment maps).*
5.  **Tonemapping & Gamma Correction:** The HDR linear color is tonemapped (Reinhard or ACES) and converted back to sRGB space (`pow(color, 1.0/2.2)`) for monitor display.

By centralizing parameters into a robust `Material` class and strictly adhering to the Cook-Torrance math in the GLSL shaders, the JGL Engine can consistently render predictable, highly realistic surfaces.