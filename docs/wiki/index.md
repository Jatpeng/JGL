# JGL Wiki

> 本页由 `tools/generate_wiki_docs.py` 自动生成，用于把 `docs/` 下的 Markdown 文档组织成可浏览的知识库入口。

当前已收录 21 篇文档，覆盖 总览2篇、引擎与渲染6篇、编辑器与功能专题9篇、API 文档3篇、其他1篇。

## 快速入口

- [文档索引](../index.md)
- [Wiki 使用与维护](./contributing.md)
- [API Docs 说明](../README.md)

## 分类导航

### 总览

- [JGL 文档索引](../index.md) - 本文档目录用于集中存放当前仓库的功能说明、设计说明和 API 文档，并尽量与 source/ 中的实际实现保持一致
- [深入解析 JGL Engine 中的基于物理渲染 (PBR) 系统](../PBR_Implementation.md) - 在现代游戏开发和图形学领域，**基于物理的渲染 (Physically Based Rendering, PBR)** 已经成为了不可或缺的基石。相比于传统的 Phong 或 Blinn-Phong 模型，PBR 通过严谨的数学物理方程来模拟光线与物体表面的相互作用，从而使得在不…

### 引擎与渲染

- [JGL 引擎架构重构总结：迈向商业级 ECS 框架](../engine/architecture_summary.md) - 在现代游戏引擎开发中，基础架构的选择决定了引擎的扩展性、性能以及未来的跨平台和渲染管线升级能力。本文记录了将 JGL 引擎从一个基础渲染项目重构为商业级引擎架构的过程
- [JGL 渲染管线：原理与实现（教学向）](../engine/rendering_pipeline_tutorial.md) - 本文面向希望**理解“为什么这样画一帧”**的读者，结合仓库里 nengine::RenderEngine 的真实实现，把 Forward / Deferred、阴影、IBL、后处理串成一条可对照代码的阅读路线。若只需条目式说明，可参见同目录下的 rendering_pipeli…
- [功能说明](../engine/feature_guide.md) - 本文档集中描述当前仓库已经接入、并能在运行时或编辑器中直接控制的主要功能
- [地形系统实现原理](../engine/terrain_system.md) - 本文基于当前仓库里已经落地的程序化地形实现来讲，不讨论“理想中的通用地形引擎”，而是专注于这套代码现在是怎么工作的、为什么这样设计、后面还能往哪里扩展
- [渲染管线（当前实现）](../engine/rendering_pipeline.md) - 本文档以当前仓库代码为准，描述 nengine::RenderEngine 中已经接入运行时的渲染流程
- [渲染队列（当前实现）](../engine/render_queue.md) - 本文档以当前仓库代码为准，描述 nengine::RenderEngine 中已经接入运行时的渲染队列实现，以及它与剔除、Forward / Deferred pass 的关系

### 编辑器与功能专题

- [PBR 材质](../sections/PBR材质.md) - 项目默认材质为 PBR：
- [Python 接口（当前状态）](../sections/Python接口设计.md) - 当前工程已接入 pybind11 绑定入口，相关代码位于：
- [天气效果（按当前实现）](../sections/Weather.md) - 当前仓库里与天气相关的实现分成两部分：
- [延迟渲染管线（当前实现）](../sections/延迟渲染管线设计.md) - 当前延迟渲染在 RenderEngine 中已接入：
- [星空材质（Night Sky）](../sections/SkyNight.md) - 星空效果使用独立材质配置与片元着色器：
- [毛发材质（Fur）](../sections/Fur.md) - 当前毛发方案基于经典壳层（shell）多 Pass 渲染：
- [特效（Bloom）](../sections/bloom.md) - 项目已提供 Bloom 材质配置与 Shader 文件：
- [编辑器面板与工作流](../sections/JGLEditor.md) - 本文档描述当前编辑器外壳的结构，以及 panel 拆分后的职责边界
- [骨骼动画加载](../sections/骨骼动画加载.md) - 骨骼动画在模型加载阶段自动识别并启用：

### API 文档

- [JGL API Docs](../api/index.md) - Generated documentation entry points
- [JGL C++ Runtime API](../api/cpp_runtime_api.md) - Top-level runtime entry point that owns the window, renderer, resource manager, and active scene.
- [JGL Python API](../api/python_api.md) - Typical script bootstrap lives in Game/script/init.py

### 其他

- [API Docs](../README.md) - This repository ships a source-driven markdown API generator.

## 最近更新

- 2026-03-23 [JGL 文档索引](../index.md) - 本文档目录用于集中存放当前仓库的功能说明、设计说明和 API 文档，并尽量与 source/ 中的实际实现保持一致
- 2026-03-23 [渲染队列（当前实现）](../engine/render_queue.md) - 本文档以当前仓库代码为准，描述 nengine::RenderEngine 中已经接入运行时的渲染队列实现，以及它与剔除、Forward / Deferred pass 的关系
- 2026-03-23 [JGL 渲染管线：原理与实现（教学向）](../engine/rendering_pipeline_tutorial.md) - 本文面向希望**理解“为什么这样画一帧”**的读者，结合仓库里 nengine::RenderEngine 的真实实现，把 Forward / Deferred、阴影、IBL、后处理串成一条可对照代码的阅读路线。若只需条目式说明，可参见同目录下的 rendering_pipeli…
- 2026-03-23 [骨骼动画加载](../sections/骨骼动画加载.md) - 骨骼动画在模型加载阶段自动识别并启用：
- 2026-03-23 [延迟渲染管线（当前实现）](../sections/延迟渲染管线设计.md) - 当前延迟渲染在 RenderEngine 中已接入：
- 2026-03-23 [特效（Bloom）](../sections/bloom.md) - 项目已提供 Bloom 材质配置与 Shader 文件：
- 2026-03-23 [天气效果（按当前实现）](../sections/Weather.md) - 当前仓库里与天气相关的实现分成两部分：
- 2026-03-23 [星空材质（Night Sky）](../sections/SkyNight.md) - 星空效果使用独立材质配置与片元着色器：

## 全量索引

| 页面 | 分类 | 路径 |
| --- | --- | --- |
| [JGL 文档索引](../index.md) | 总览 | `index.md` |
| [深入解析 JGL Engine 中的基于物理渲染 (PBR) 系统](../PBR_Implementation.md) | 总览 | `PBR_Implementation.md` |
| [JGL 引擎架构重构总结：迈向商业级 ECS 框架](../engine/architecture_summary.md) | 引擎与渲染 | `engine/architecture_summary.md` |
| [JGL 渲染管线：原理与实现（教学向）](../engine/rendering_pipeline_tutorial.md) | 引擎与渲染 | `engine/rendering_pipeline_tutorial.md` |
| [功能说明](../engine/feature_guide.md) | 引擎与渲染 | `engine/feature_guide.md` |
| [地形系统实现原理](../engine/terrain_system.md) | 引擎与渲染 | `engine/terrain_system.md` |
| [渲染管线（当前实现）](../engine/rendering_pipeline.md) | 引擎与渲染 | `engine/rendering_pipeline.md` |
| [渲染队列（当前实现）](../engine/render_queue.md) | 引擎与渲染 | `engine/render_queue.md` |
| [PBR 材质](../sections/PBR材质.md) | 编辑器与功能专题 | `sections/PBR材质.md` |
| [Python 接口（当前状态）](../sections/Python接口设计.md) | 编辑器与功能专题 | `sections/Python接口设计.md` |
| [天气效果（按当前实现）](../sections/Weather.md) | 编辑器与功能专题 | `sections/Weather.md` |
| [延迟渲染管线（当前实现）](../sections/延迟渲染管线设计.md) | 编辑器与功能专题 | `sections/延迟渲染管线设计.md` |
| [星空材质（Night Sky）](../sections/SkyNight.md) | 编辑器与功能专题 | `sections/SkyNight.md` |
| [毛发材质（Fur）](../sections/Fur.md) | 编辑器与功能专题 | `sections/Fur.md` |
| [特效（Bloom）](../sections/bloom.md) | 编辑器与功能专题 | `sections/bloom.md` |
| [编辑器面板与工作流](../sections/JGLEditor.md) | 编辑器与功能专题 | `sections/JGLEditor.md` |
| [骨骼动画加载](../sections/骨骼动画加载.md) | 编辑器与功能专题 | `sections/骨骼动画加载.md` |
| [JGL API Docs](../api/index.md) | API 文档 | `api/index.md` |
| [JGL C++ Runtime API](../api/cpp_runtime_api.md) | API 文档 | `api/cpp_runtime_api.md` |
| [JGL Python API](../api/python_api.md) | API 文档 | `api/python_api.md` |
| [API Docs](../README.md) | 其他 | `README.md` |
