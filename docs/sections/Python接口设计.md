# Python 接口（当前状态）

## 已有实现

当前工程已接入 `pybind11` 绑定入口，相关代码位于：

- `JGL_Engine/source/python/python_bindings.h`
- `JGL_Engine/source/python/python_bindings.cpp`

这表示 Python 绑定基础设施已存在，但对外 API 仍在演进中。

## 运行时基础

当前运行时已具备场景/实体组件基础：

- `Scene`
- `Entity`
- `MeshComponent`
- `LightComponent`
- `TransformComponent`
- `RenderEngine`

因此本文不再沿用“SceneView 单对象预览器”的旧描述。

## 文档定位

本页只记录“当前代码已存在”的 Python 接入状态。  
若需要完整的 Python API 规格（类、方法、生命周期、线程约束），建议另起 `docs/sections/Python接口规划.md` 作为纯规划文档，并显式标注“未实现项”。

## 实现原理（概述）

工程通过 `pybind11` 将 C++ 引擎类型与函数导出为 Python 扩展模块（`.pyd` / `.so`）。整体链路可概括为：

- **模块初始化**：在 `python_bindings.cpp` 中通过 `PYBIND11_MODULE(...)`（或等效入口）创建 Python 模块，并注册类/函数。
- **类型绑定**：把引擎的核心类型（如 `Scene`、`Entity`、各类 `*Component`、`RenderEngine` 等）用 `py::class_` 暴露给 Python。
  - **对象所有权/生命周期**：Python 侧对象通常持有 C++ 对象指针或智能指针包装；需明确“由引擎持有”还是“由 Python 创建并持有”。（若绑定使用 `std::shared_ptr`，Python 与 C++ 可共享引用计数；若是裸指针/引用，必须保证引擎侧对象在 Python 访问期间始终有效。）
  - **句柄语义**：`Entity` 常见实现是轻量句柄（ID + scene 指针/引用），Python 调用会转发到 scene/registry 做真实读写。
- **调用转发**：Python 方法调用进入 C++ 绑定层，再调用引擎真实实现（组件增删查改、渲染参数设置等）。
- **数据类型映射**：基础类型（`int/float/bool/string`）直接映射；数学类型（如 `vec3/mat4`）一般需要：
  - 直接绑定自定义数学类型，或
  - 用 Python `tuple/list` 在绑定层做转换，或
  -（可选）与 `numpy` 互操作以降低拷贝成本。
- **错误处理**：C++ 异常可由 `pybind11` 自动转换为 Python 异常；建议在关键 API 上把引擎错误转成可诊断的异常/返回值，而不是静默失败。
- **线程边界（重要）**：渲染/窗口/资源加载可能有主线程约束。Python 侧若允许多线程调用，需要在绑定层或引擎层统一约束：
  - 强制只在主线程调用引擎 API，或
  - 在 C++ 层做任务投递（command queue）并在主线程执行，或
  - 明确哪些调用可跨线程，哪些必须主线程。

> 以上为实现原理的“当前工程形态”抽象总结；具体以 `JGL_Engine/source/python/python_bindings.*` 中实际注册内容为准。

## 需要哪些环境

### 构建环境（编译 Python 扩展）

- **C++ 工具链**：Windows 推荐 Visual Studio 2022（MSVC），并确保与你的 CMake 生成器一致。
- **CMake**：用于生成工程与组织第三方依赖（含 `pybind11`）。
- **Python（开发用）**：
  - 需要安装与你要生成的 `.pyd` **ABI 兼容**的 Python 版本（例如 CPython 3.13 x64）。
  - 需要对应架构（x64/x86）与编译配置匹配；常见要求是 **x64 + Release/Debug 对齐**。
- **pybind11**：工程已接入；来源可能是子模块/FetchContent/包管理器，按工程实际配置即可。
- **可选：vcpkg / conan**：若工程用其管理依赖（以项目配置为准）。

### 运行环境（执行 Python 脚本驱动引擎）

- **Python（运行用）**：与构建产物匹配的 CPython 版本与架构。
- **模块可发现性**：确保生成的 `.pyd` 位于 Python 的 `sys.path` 可搜索路径中，或在运行时设置：
  - `PYTHONPATH` 指向包含 `.pyd` 的目录，或
  - 将 `.pyd` 放到脚本同目录/虚拟环境 site-packages 中。
- **原生依赖 DLL**：若扩展或引擎依赖其他 DLL（渲染后端、第三方库等），需要在运行时可被加载：
  - 将 DLL 放到可执行文件同目录，或
  - 配置系统 `PATH`，或
  - 使用 Windows 的“应用本地部署”策略。

### 可选调试/分析工具

- **Visual Studio 调试**：可用于调试 C++ 扩展与引擎；若要从 Python 启动并调试，可用“附加到进程”或配置启动参数。
- **RenderDoc**：用于渲染帧捕获与分析（若工程已集成相关入口/开关）。
- **Python 虚拟环境**：建议使用 venv/conda 隔离不同 Python 版本与依赖，降低 ABI/路径冲突概率。

