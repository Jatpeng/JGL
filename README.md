# JGL Demos

![项目预览](./Images/README/image-20230521164413353.png)

用于日常学习 OpenGL/Shader 的实验项目，包含编辑器框架、材质系统与多个效果样例。

## Sections

- [编辑器架构](sections/JGLEditor.md)  
  说明窗口循环、SceneView、PropertyPanel、材质加载与模型加载流程。
- [模拟 2D 天气效果](sections/Weather.md)  
  记录天气与水面扰动相关资源、Shader 逻辑和接入方式。
- [特效（Bloom）](sections/bloom.md)  
  说明 Bloom 示例当前状态、配置入口与后续扩展建议。
- [骨骼动画加载](sections/骨骼动画加载.md)  
  说明 Assimp 骨骼提取、Animation/Animator 更新与 Shader 绑定流程。
- [毛发材质](sections/Fur.md)  
  说明多 Pass 毛发壳层实现与参数控制。
- [PBR 材质](sections/PBR材质.md)  
  说明 PBR 贴图参数、光照计算与编辑器参数联动。
- [星空材质](sections/SkyNight.md)  
  说明夜空体积感效果的核心思路与使用方式。

## 技术栈

- GLEW: <http://glew.sourceforge.net/>
- GLFW: <https://www.glfw.org/>
- GLM: <https://glm.g-truc.net/0.9.9/index.html>
- Assimp: <https://github.com/assimp/assimp>
- ImGui: <https://github.com/ocornut/imgui>
