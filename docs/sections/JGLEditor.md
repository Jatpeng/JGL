# 编辑器面板与工作流

本文档描述当前编辑器外壳的结构，以及 panel 拆分后的职责边界。

## 总览

当前编辑器由这些部分组成：

- `GLWindow`：窗口生命周期、输入转发、渲染驱动
- `RenderEngine`：场景渲染、阴影、IBL、后处理、输出纹理
- `EditorOverlay`：ImGui dockspace 和 panel 编排
- `SceneView`：场景视口与工具栏
- 多个独立 editor panel：场景层级、对象检查器、渲染设置

编辑器启动时会默认加载 `Assets/scenes/default_scene.xml`，所以一进编辑器就能直接看到展示场景，而不是空场景。

## 当前 Dock 布局

默认布局为：

- 左侧：`Scene Hierarchy`
- 中央：`Scene`
- 右侧：`Inspector`
- 右下：`Render Settings`

## Panel 划分

### Scene Hierarchy

负责场景结构和对象管理：

- 编辑场景名称
- 打开或关闭 skybox 可见性
- 创建 Mesh / Light
- 删除当前选中对象
- 展示当前场景对象列表
- 维护选中对象切换

### Inspector

负责当前选中对象的属性编辑：

- Transform
- Mesh 资源切换
- Shader 切换与单对象热重载
- Material 参数
- Light 参数

当前导入的展示模型也会出现在模型预设列表中，例如：

- `Assets/models/showcase/bunny.obj`
- `Assets/models/showcase/teapot.obj`

### Render Settings

负责全局渲染和工具功能：

- 透明度与地面开关
- HDR 环境图与 IBL
- Forward / Deferred 切换
- Debug View
- 全局 shader 热重载
- Screen Effects
- RenderDoc Capture

当前导入的 `Assets/environments/newport_loft/Newport_Loft_Env.hdr` 也会出现在环境图预设列表里。

### Scene

负责主场景视图：

- 离屏渲染输出显示
- 工具栏
- G-Buffer 缩略图
- Scene 视口尺寸驱动 render target resize

## 共享状态

多 panel 之间通过共享的编辑器状态协作，而不是彼此直接持有大量逻辑。

当前共享状态主要包括：

- 当前选中实体 id
- shader reload 状态文本
- environment 切换状态文本
- 新建 mesh 的默认模型 / 材质预设
- 新建 light 的默认类型

公共资源枚举和 UI helper 位于：

- `ui/editor_panel_common.h`
- `ui/editor_panel_common.cpp`

## 常用工作流

### 查看默认展示场景

1. 直接启动编辑器
2. 默认资源场景会自动加载
3. 在 `Scene Hierarchy` 中选择对象
4. 在 `Inspector` 中查看模型、材质和灯光参数

### 调整一个阴影灯光

1. 在 `Scene Hierarchy` 中选择 `sun`
2. 在 `Inspector` 中切换或确认 `Directional`
3. 启用 `Cast Shadows`
4. 调整 `Shadow Filter Radius` 和 `Shadow Bias`
5. 在 `Scene` 中观察阴影结果

### 切换环境图

1. 打开 `Render Settings -> Environment`
2. 从预设列表中选择仓库内 HDR
3. 或通过文件对话框加载外部 `.hdr`
4. 如需回退，切回内置 skybox

## 快捷键与交互

- `RMB`：Orbit
- `MMB`：Pan
- `W / S`：Zoom
- `F`：Reset View
- `F5`：Reload All Shaders
- `F12`：Capture Frame

## 后续扩展建议

如果继续增加编辑器能力，推荐沿用现在的模式：

1. 新增一个独立 panel 类，而不是继续堆进某一个大窗口
2. 通过 `EditorPanelState` 共享跨窗口状态
3. 通过 `build_editor_panel_context()` 提供只读上下文
4. 在 `EditorOverlay` 中注册该 panel 并分配 dock 区域
