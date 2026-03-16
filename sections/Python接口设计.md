# Python 调用接口设计

## 1. 背景

当前项目的运行链路是：

`Application -> GLWindow -> SceneView`

渲染能力已经具备，但“场景”和“对象”目前还没有独立的数据模型，主要表现为：

- `SceneView` 直接持有一个主模型 `mModel`
- `SceneView` 直接持有一个材质 `mMaterial`
- `SceneView` 直接持有一个灯光 `mLight`
- 天空盒、地面、动画都直接绑在 `SceneView` 内

这意味着当前代码更像“单模型预览器”，而不是“可被 Python 驱动的场景系统”。  
如果要支持 Python 创建场景、创建对象、切换材质、控制相机，必须先把运行时数据结构从 `SceneView` 中拆出来。

## 2. 目标

本设计希望实现以下能力：

- Python 可以启动引擎
- Python 可以创建和切换场景
- Python 可以创建对象并设置名称、变换、模型、材质
- Python 可以创建灯光和相机
- Python 可以驱动对象更新
- 编辑器 UI 与 Python 接口共享同一套场景数据

## 3. 非目标

第一阶段不包含以下内容：

- 不做完整 ECS
- 不做物理系统绑定
- 不做多线程脚本热重载
- 不做复杂材质图节点编辑
- 不把所有底层 OpenGL 细节直接暴露给 Python

## 4. 设计原则

- Python 只操作“引擎对象”和“场景对象”，不直接操作 OpenGL 资源
- GPU 资源创建仍然留在 C++ 渲染层
- 编辑器、可执行程序、Python 模块复用同一套 core 代码
- 先支持最小闭环：`Scene -> Object -> Transform -> Model/Material -> Render`

## 5. 总体方案

推荐采用 `pybind11` 暴露 C++ 接口给 Python，不建议把 Python 解释器直接硬嵌入当前 `main.cpp`。

原因：

- `pybind11` 更适合“从 Python 调用 C++ 引擎”
- API 形式更自然，便于后续脚本化场景搭建
- 可以把当前可执行程序拆为“核心库 + 编辑器程序 + Python 模块”
- 后续也可以保留编辑器 UI，不影响 Python 接入

## 6. 模块拆分

建议将当前项目拆成三个层次：

### 6.1 `jgl_core`

职责：

- 场景数据结构
- 对象生命周期管理
- 资源引用
- Python 绑定要访问的公共 API

建议包含：

- `Scene`
- `SceneObject`
- `Transform`
- `MeshComponent`
- `LightComponent`
- `CameraComponent`
- `AssetManager`
- `Engine`

### 6.2 `jgl_render`

职责：

- OpenGL 上下文
- Shader / Texture / FBO
- 将 `Scene` 渲染到 `SceneView`

现有代码主要落在这里：

- `render/*`
- `shader/*`
- `utils/texturessystem.*`
- `ui/scene_view.*`

### 6.3 `pyjgl`

职责：

- 用 `pybind11` 把 `jgl_core` 暴露为 Python 模块
- 暴露最小但稳定的 Python API

## 7. 新的运行时对象模型

### 7.1 `Transform`

```cpp
struct Transform {
    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation {0.0f, 0.0f, 0.0f};
    glm::vec3 scale    {1.0f, 1.0f, 1.0f};

    glm::mat4 local_matrix() const;
};
```

### 7.2 `SceneObject`

```cpp
class SceneObject {
public:
    uint64_t id() const;
    const std::string& name() const;
    void set_name(const std::string& name);

    Transform& transform();
    const Transform& transform() const;

private:
    uint64_t mId;
    std::string mName;
    Transform mTransform;
};
```

### 7.3 `MeshObject`

```cpp
class MeshObject : public SceneObject {
public:
    bool set_model(const std::string& path);
    bool set_material(const std::string& path);

    nelems::Model* model();
    Material* material();

private:
    std::shared_ptr<nelems::Model> mModel;
    std::shared_ptr<Material> mMaterial;
};
```

### 7.4 `LightObject`

```cpp
class LightObject : public SceneObject {
public:
    void set_color(const glm::vec3& color);
    void set_strength(float strength);
};
```

### 7.5 `Scene`

```cpp
class Scene {
public:
    std::shared_ptr<MeshObject> create_mesh(const std::string& name);
    std::shared_ptr<LightObject> create_light(const std::string& name);
    void remove_object(uint64_t id);

    const std::vector<std::shared_ptr<SceneObject>>& objects() const;

    void set_skybox_enabled(bool enabled);
    bool skybox_enabled() const;
};
```

## 8. 对现有代码的改造点

### 8.1 `SceneView`

当前 `SceneView` 自己保存 `mModel / mMaterial / mLight`，需要改成：

- `SceneView` 只负责渲染
- `SceneView` 持有 `Scene*` 或 `std::shared_ptr<Scene>`
- 渲染时遍历 `Scene` 中的对象

建议新增接口：

```cpp
void set_scene(std::shared_ptr<Scene> scene);
std::shared_ptr<Scene> get_scene() const;
```

### 8.2 `GLWindow`

当前 `GLWindow` 内部创建 `SceneView`，后续需要支持：

- 由 `Engine` 或 Python 设置当前场景
- 编辑器和 Python 共享同一个 `Scene`

建议新增：

```cpp
void set_scene(std::shared_ptr<Scene> scene);
std::shared_ptr<Scene> get_scene() const;
```

### 8.3 `Application`

建议把 `Application` 演化为：

```cpp
class Engine {
public:
    bool init();
    void run();
    void tick();

    std::shared_ptr<Scene> create_scene(const std::string& name);
    void set_active_scene(std::shared_ptr<Scene> scene);
    std::shared_ptr<Scene> active_scene() const;
};
```

这样 `main.cpp` 和 Python 模块都能复用同一个入口。

## 9. Python API 设计

### 9.1 目标用法

```python
import pyjgl as jgl

engine = jgl.Engine()
engine.init()

scene = engine.create_scene("demo")
engine.set_active_scene(scene)

cube = scene.create_mesh("cube")
cube.set_model("Assets/cube.fbx")
cube.set_material("Assets/PBR.xml")
cube.transform.position = (0.0, 0.0, 0.0)
cube.transform.scale = (1.0, 1.0, 1.0)

light = scene.create_light("main_light")
light.transform.position = (1.5, 3.5, 3.0)
light.color = (1.0, 1.0, 1.0)
light.strength = 100.0

scene.set_skybox_enabled(True)

engine.run()
```

### 9.2 推荐暴露的 Python 类

- `Engine`
- `Scene`
- `SceneObject`
- `MeshObject`
- `LightObject`
- `Transform`

### 9.3 Python 侧接口建议

#### `Engine`

- `init() -> bool`
- `run() -> None`
- `tick() -> None`
- `create_scene(name: str) -> Scene`
- `set_active_scene(scene: Scene) -> None`
- `active_scene() -> Scene`

#### `Scene`

- `create_mesh(name: str) -> MeshObject`
- `create_light(name: str) -> LightObject`
- `remove_object(object_id: int) -> None`
- `find_object(name: str) -> SceneObject | None`
- `objects() -> list[SceneObject]`
- `set_skybox_enabled(enabled: bool) -> None`

#### `MeshObject`

- `set_model(path: str) -> bool`
- `set_material(path: str) -> bool`
- `name`
- `id`
- `transform`

#### `LightObject`

- `color`
- `strength`
- `transform`

## 10. 线程与上下文约束

这是本设计里最关键的一条。

当前项目使用 GLFW + OpenGL，上下文和渲染线程不能被 Python 任意跨线程操作。  
因此建议采用以下规则：

- Python 可以创建和修改场景数据
- 真正的 OpenGL 资源创建与销毁在渲染线程执行
- `Scene` 修改通过“主线程直接执行”或“命令队列”进入渲染层

第一阶段建议采用最简单方案：

- Python 与渲染循环运行在同一主线程
- 不开放多线程 Python 调用
- `engine.run()` 进入主循环前完成场景创建

第二阶段再补：

- `enqueue_command()`
- 热更新脚本
- 帧回调

## 11. 资源管理设计

为了避免 Python 侧重复创建相同模型和材质，建议引入 `AssetManager`。

```cpp
class AssetManager {
public:
    std::shared_ptr<nelems::Model> load_model(const std::string& path);
    std::shared_ptr<Material> load_material(const std::string& path);
};
```

要求：

- 相同路径的模型只加载一次
- 相同路径的材质只解析一次
- Python 对象只持有资源引用，不直接持有裸指针

## 12. 场景渲染流程调整

调整后的流程：

1. `Engine` 持有当前 `Scene`
2. `GLWindow` 从 `Engine` 获取当前 `Scene`
3. `SceneView` 遍历 `Scene` 中对象
4. 对 `MeshObject`：
   - 更新模型矩阵
   - 更新材质参数
   - 执行绘制
5. 对 `LightObject`：
   - 汇总灯光参数到 shader
6. 最后绘制地面、天空盒、调试 UI

## 13. `pybind11` 绑定建议

建议新增目录：

- `JGL_Engine/source/python/`

建议文件：

- `python_bindings.cpp`
- `py_engine.cpp`
- `py_scene.cpp`
- `py_object.cpp`

示例结构：

```cpp
PYBIND11_MODULE(pyjgl, m) {
    bind_transform(m);
    bind_scene_object(m);
    bind_mesh_object(m);
    bind_light_object(m);
    bind_scene(m);
    bind_engine(m);
}
```

## 14. CMake 改造建议

当前工程是单一可执行程序。  
建议改为：

- `jgl_core` 静态库
- `JGL_Engine` 可执行程序
- `pyjgl` Python 模块

建议新增配置：

```cmake
option(JGL_ENABLE_PYTHON "Build Python bindings" ON)
```

建议依赖：

- `Python3::Interpreter`
- `Python3::Development`
- `pybind11`

建议结构：

```cmake
add_library(jgl_core ...)
add_executable(JGL_Engine ...)
target_link_libraries(JGL_Engine PRIVATE jgl_core ...)

if(JGL_ENABLE_PYTHON)
    pybind11_add_module(pyjgl ...)
    target_link_libraries(pyjgl PRIVATE jgl_core ...)
endif()
```

## 15. 推荐实施阶段

### 阶段一：运行时抽象

目标：

- 引入 `Scene / SceneObject / Transform`
- `SceneView` 从“单对象渲染”改成“场景渲染”

产出：

- C++ 侧可创建多个对象
- 编辑器仍可正常运行

### 阶段二：引擎核心化

目标：

- 把 `Application + GLWindow + SceneView` 组合成 `Engine`
- 将可执行程序和核心逻辑解耦

产出：

- `main.cpp` 只保留启动逻辑
- 核心逻辑可被 Python 模块复用

### 阶段三：Python 绑定

目标：

- 引入 `pybind11`
- 暴露 `Engine / Scene / MeshObject / LightObject / Transform`

产出：

- Python 可创建场景和对象
- Python 可运行 demo 脚本

### 阶段四：脚本驱动更新

目标：

- 支持 Python 每帧更新
- 支持对象动画脚本

产出：

- `engine.tick()`
- `on_update(callback)`

## 16. 风险与注意点

### 16.1 当前 `SceneView` 耦合过高

问题：

- 模型、材质、灯光、天空盒都堆在一个类里

影响：

- Python 接口没法稳定复用

解决：

- 先抽场景层，再做绑定

### 16.2 资源生命周期

问题：

- Python 可能提前释放对象引用

解决：

- C++ 侧统一使用 `shared_ptr`
- Python 只持有绑定后的共享句柄

### 16.3 OpenGL 上下文线程限制

问题：

- Python 后台线程直接改 GPU 资源容易崩

解决：

- 第一阶段只支持主线程调用
- 后续再引入命令队列

## 17. 最小可交付版本

建议第一版只支持以下能力：

- `Engine.init()`
- `Engine.create_scene()`
- `Engine.set_active_scene()`
- `Scene.create_mesh()`
- `Scene.create_light()`
- `MeshObject.set_model()`
- `MeshObject.set_material()`
- `Transform.position / rotation / scale`
- `Engine.run()`

这套能力已经足够支撑：

- Python 创建默认场景
- Python 动态添加模型
- Python 调整灯光
- Python 控制对象位置与缩放

## 18. 结论

对于当前项目，正确的接入顺序不是“直接给 `SceneView` 加 Python 调用”，而是：

1. 先引入 `Scene / SceneObject / Transform`
2. 再把当前程序重构为 `jgl_core + editor app`
3. 最后用 `pybind11` 暴露 Python 模块

这样做的好处是：

- 编辑器和 Python 共用一套运行时
- 后续扩展动画、材质、相机都不会推倒重来
- API 会比较稳定，不会把 OpenGL 细节泄漏给 Python
