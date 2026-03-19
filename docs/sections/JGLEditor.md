# 编辑器架构

## 概览

当前编辑器运行时以 `GLWindow + RenderEngine + Property_Panel` 为核心，职责如下：

- `main.cpp` 负责创建窗口与运行循环。
- `GLWindow` 负责窗口生命周期、渲染调用顺序与输入转发。
- `RenderEngine` 负责 Forward/Deferred 渲染、G-Buffer、屏幕特效和输出纹理。
- `Property_Panel` 负责 ImGui 参数面板、对象管理与资源加载。

## 渲染与 UI 流程

每帧调用顺序在 `GLWindow::render()` 中：

1. `OpenGL_Context::pre_render()` 清屏。
2. `UIContext::pre_render()` 创建 DockSpace。
3. `RenderEngine::render()` 渲染到离屏 FBO（必要时叠加后处理）。
4. `Property_Panel::render()` 绘制属性面板并处理文件加载。
5. `UIContext::post_render()` 提交 ImGui。
6. `OpenGL_Context::post_render()` 交换缓冲并轮询事件。

## 材质加载

材质由 `Material::load()` 从 XML 读取并建立参数映射：

- `Type=Texture` -> 加载纹理并缓存到 `mTexture_map`。
- `Type=float` -> 写入 `mFloat_map`。
- `Type=float3` -> 写入 `mFloat3_map`。

运行时由 `Material::update_shader_params()` 统一把参数推送到 Shader，实现“配置即渲染”。

## 模型加载

模型在 `Property_Panel` 里通过 `Open...` 选择，支持 `fbx/obj/dae`。  
运行时通过 `MeshComponent::set_model()` 完成模型加载；若检测到骨骼数据，会进入骨骼动画更新与蒙皮流程。

## 交互控制

- 鼠标拖拽：旋转视角（由窗口输入转发到 `RenderEngine::on_mouse_move`）。
- 滚轮 / `W/S`：缩放视距（`on_mouse_wheel`）。
- `F`：重置相机视角（`RenderEngine::reset_view`）。

![编辑器界面](../../sections/Images/JGLEditor/image-20230521165936149.png)
![材质参数面板](../../sections/Images/JGLEditor/image-20230521164933037.png)

