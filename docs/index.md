# JGL 文档索引

本文档目录用于集中存放当前仓库的功能说明、设计文档和 API 文档，并尽量与 `source/` 中的实际实现保持一致。

## 推荐先读

- [JGL Wiki](./wiki/index.md)
- [渲染管线（当前实现）](./engine/rendering_pipeline.md)
- [渲染设备层模块说明](./engine/render_device_layer.md)
- [功能说明](./engine/feature_guide.md)
- [编辑器面板与工作流](./sections/JGLEditor.md)

## 引擎与渲染

- [渲染管线（当前实现）](./engine/rendering_pipeline.md)
- [渲染管线（教学向）](./engine/rendering_pipeline_tutorial.md)
- [渲染队列（当前实现）](./engine/render_queue.md)
- [渲染设备层模块说明](./engine/render_device_layer.md)
- [功能说明](./engine/feature_guide.md)
- [架构重构总结](./engine/architecture_summary.md)
- [地形系统实现原理](./engine/terrain_system.md)

## 编辑器与功能专题

- [编辑器面板与工作流](./sections/JGLEditor.md)
- [延迟渲染管线设计](./sections/延迟渲染管线设计.md)
- [Python 接口设计](./sections/Python接口设计.md)
- [骨骼动画加载](./sections/骨骼动画加载.md)
- [PBR 材质](./sections/PBR材质.md)
- [Bloom](./sections/bloom.md)
- [Fur](./sections/Fur.md)
- [SkyNight](./sections/SkyNight.md)
- [Weather](./sections/Weather.md)

## Wiki

- [JGL Wiki 首页](./wiki/index.md)
- [Wiki 使用与维护](./wiki/contributing.md)

## API 文档

- [API Docs 说明](./README.md)
- [API Docs 入口](./api/index.md)
- [C++ Runtime API](./api/cpp_runtime_api.md)
- [Python API](./api/python_api.md)

## 默认展示资源

当前仓库默认会加载一套资源化展示场景，相关入口如下：

- 场景资源：`Assets/scenes/default_scene.xml`
- 展示模型：`Assets/models/showcase/bunny.obj`、`Assets/models/showcase/teapot.obj`
- 环境贴图：`Assets/environments/newport_loft/Newport_Loft_Env.hdr`
- 脚本入口：`Game/script/showcase_scene.py`
