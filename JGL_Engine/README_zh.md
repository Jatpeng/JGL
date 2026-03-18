# JGL_Engine 渲染优化与阴影实现

本分支为JGL引擎带来了三个主要的渲染优化和核心功能：

## 1. 视锥体剔除 (Geometry Culling)
- 在 `camera.h` 中实现了基于提取观察投影矩阵平面的视锥体剔除。
- 当通过Assimp加载网格时，我们在 `model.h` 中计算并存储边界框 (AABB)。
- 在 `render_engine.cpp` 的正向渲染和延迟渲染的几何阶段，会检查对象的AABB是否在相机的视锥体内，并在完全不可见时跳过绘制调用。

## 2. 平行光阴影 (Cascaded Shadow Maps / 单张阴影贴图)
- 我们为定向光 (例如太阳) 实施了强大的单张阴影贴图解决方案。
- 在 `scene.h` 中的 `LightObject` 添加了关于光源方向以及是否投射阴影的属性。
- 引入了 `depth_vs.shader` 和 `depth_fs.shader` 以使用单独的 `mShadowMapFBO` FBO 渲染从光源视角的深度。
- 在片段着色器 (`deferred_lighting_fs.shader` 和 `pbr_fs.shader`) 中加入了PCF (Percentage-Closer Filtering) 对阴影进行采样和柔化处理。

## 3. 渲染状态排序 (State Sorting)
- 为了减少OpenGL的状态切换 (主要针对Shader绑定) 以提升性能，我们在 `render_scene_meshes_forward` 方法中增加了排序逻辑。
- 根据 `Shader ID` 对准备渲染的对象进行排序，使得使用相同材质或着色器的对象会连续渲染。

## 编译说明
当前已验证可以在Ubuntu下使用CMake成功编译。
对于Linux环境下的UI组件依赖，如果使用 `_WIN32` 原生文件对话框功能则被预处理宏隔离，使用 `std::nullopt` 做作为占位符进行编译。

## 预期的性能提升
引入视锥体剔除后可以明显感觉到相机背对物体时的帧率上升，而基于材质排序也能部分减少CPU端渲染状态调用的开销。
