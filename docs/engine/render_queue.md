# 渲染队列（当前实现）

本文档以当前仓库代码为准，描述 `nengine::RenderEngine` 中已经接入运行时的渲染队列实现，以及它与剔除、Forward / Deferred pass 的关系。

## 目标

当前渲染队列的目标不是引入一套全新的渲染框架，而是在保留现有 Forward / Deferred 主流程的前提下，把下面三件事前移到“每帧构建队列”阶段：

- 收集可渲染 mesh
- 按不同 pass 分桶
- 在进入 pass 之前完成排序与相机剔除

这样每个 pass 不再重复遍历整个场景，而是直接消费自己对应的队列。

## 每帧流程

当前主入口仍然是 `RenderEngine::render()`，但内部顺序已经调整为：

1. `update_frame_state()`
2. `build_render_queue()`
3. `shadow_pass()`
4. 根据模式进入：
   - `render_forward_to_framebuffer()`
   - `render_deferred_to_framebuffer()`
5. `PostProcessStack::render()`

也就是说：**渲染队列是在阴影、Forward、Deferred 几何阶段之前统一生成的。**

## 队列分桶

当前实现中的 `RenderQueue` 包含 4 组 item：

- `shadow_items`
  - 阴影 pass 使用
  - 只要求实体有 `MeshComponent`、`TransformComponent` 和可用模型
- `forward_opaque_items`
  - Forward 主路径中的不透明 mesh 使用
- `deferred_opaque_items`
  - Deferred Geometry Pass 使用
  - 仅包含满足延迟材质要求的可见 mesh
- `transparent_items`
  - 透明叠加路径使用

当前 `RenderItem` 会缓存：

- `Entity*`
- `MeshComponent*`
- `Shader*`
- 世界矩阵
- 相机距离平方
- Shader Program ID
- Material 指针

这样后续 pass 可以直接使用队列数据，而不需要重新查询场景与组件。

## 排序策略

当前实现采用的排序策略如下：

- `forward_opaque_items`
  - 先按 `Shader Program ID`
  - 再按 `Material` 指针
  - 再按距离做近到远
- `deferred_opaque_items`
  - 按距离做近到远
- `transparent_items`
  - 按距离做远到近
- `shadow_items`
  - 维持稳定的实体顺序

其中：

- 不透明排序优先考虑减少状态切换，并兼顾一定的前向近到远绘制
- 透明排序优先保证混合结果正确

## 在渲染队列里做剔除

可以，而且这是当前实现里最合适的位置之一。

当前做法是：

1. 在 `build_render_queue()` 中从相机的 `viewProjection` 提取 6 个视锥平面
2. 为每个 mesh 计算世界空间 AABB
3. 用 AABB 与视锥做相交测试
4. 只有通过测试的对象，才进入可见队列

当前世界 AABB 的来源：

- 优先使用模型导入后保存的本地包围盒
- 若模型没有本地 bounds，则退回到一个单位盒近似

## 为什么阴影队列不直接复用相机剔除结果

当前 `shadow_items` 是**保守收集**，不会直接使用相机视锥结果裁掉。

原因是：

- 屏幕外的物体仍然可能把阴影投到屏幕内
- 如果把 `shadow_pass()` 和主相机可见性完全绑定，容易出现“物体本身不在画面里，但阴影也突然消失”的错误

因此当前策略是：

- 主相机相关的 Forward / Deferred 几何队列做相机剔除
- 阴影队列先不过度裁剪，优先保证结果正确

如果后续需要继续优化，可再单独为阴影 pass 增加 light frustum / shadow caster culling。

## 与现有渲染路径的关系

当前渲染队列是对现有路径的收敛，不是替换：

- Forward 路径仍负责：
  - 不透明物体
  - 地面
  - skybox
  - 透明叠加
- Deferred 路径仍负责：
  - Geometry Pass
  - Lighting Pass
  - Forward Overlay

队列只是把“哪些物体应该进哪个 pass、以什么顺序进入”提前算好。

## 当前限制

当前实现仍然保留这些限制：

- 透明仍然受 `mModelTransparent` 全局开关控制
- 还没有每材质独立的 `blend mode / render queue id`
- 动画模型的包围盒仍以导入模型的本地 bounds 为主，极端 pose 下可能不够精确
- Deferred 可用性仍然沿用当前项目的场景级判定
  - 只要场景里有不满足延迟材质要求的 mesh，就会回退到 Forward

## 关键代码位置

- `JGL_Engine/source/engine/render_engine.h`
- `JGL_Engine/source/engine/render_engine.cpp`
- `JGL_Engine/source/elems/camera.h`
- `JGL_Engine/source/elems/model.h`

## 后续建议

如果后续要继续扩展渲染队列，优先级建议如下：

1. 给材质增加 `opaque / transparent / masked` 与显式 `render queue`
2. 给透明对象加入更稳定的排序键
3. 为阴影 pass 增加独立剔除
4. 为队列补充统计信息与调试视图
