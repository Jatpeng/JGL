# JGL 引擎架构重构总结：迈向商业级 ECS 框架

## 1. 引言

在现代游戏引擎开发中，基础架构的选择决定了引擎的扩展性、性能以及未来的跨平台和渲染管线升级能力。本文记录了将 JGL 引擎从一个基础渲染项目重构为商业级引擎架构的过程。

核心目标：
- 拆分单体式（Monolithic）架构，引入解耦的子系统（Subsystems）机制。
- 淘汰传统的深层对象继承树（`SceneObject` 派生），引入灵活的实体-组件-系统（ECS, Entity-Component-System）模式。
- 为未来的高级渲染特性（如 PBR/NPR）和跨平台渲染硬件接口（RHI）打下坚实的内存和对象模型基础。

---

## 2. 核心架构重构设计

### 2.1 子系统生命周期管理 (Subsystem Lifecycle)

以往的引擎主循环常常将渲染、输入、逻辑更新揉杂在一起。在此次重构中，我们引入了统一的 `ISubsystem` 接口：

```cpp
class ISubsystem {
public:
    virtual ~ISubsystem() = default;
    virtual bool init() = 0;
    virtual void tick(float delta_time) = 0;
    virtual void shutdown() = 0;
};
```

**设计优势：**
1. **解耦：** `Engine` 类不再需要了解每个模块的内部工作原理，只需要在对应的时机调用 `init()`、`tick()` 和 `shutdown()`。
2. **可扩展性：** 未来引入物理系统、音频系统时，只需实现该接口并注册到 `Engine` 即可，不会破坏现有结构。

### 2.2 实体-组件-系统架构 (Entity-Component System)

以往，JGL 引擎使用面向对象的深度继承树，例如 `SceneObject -> MeshObject / LightObject`。这种设计在项目规模扩大时会导致严重的“菱形继承”问题和代码臃肿。

本次重构我们彻底移除了传统的继承结构，实现了基础的 ECS 模式：

* **Entity（实体）：** 仅仅是一个包含唯一 ID (`uint64_t`) 和名称的容器。它本身没有逻辑。
* **IComponent（组件）：** 一切数据（如 `TransformComponent`, `MeshComponent`, `LightComponent`）都被拆分为独立的组件，挂载在 Entity 上。

**组件挂载的内存模型：**
```cpp
template <typename T>
T* add_component(Args&&... args) {
    auto comp = std::make_shared<T>(std::forward<Args>(args)...);
    mComponents[typeid(T)] = std::static_pointer_cast<IComponent>(comp);
    return comp.get();
}
```
通过 `std::unordered_map<std::type_index, std::shared_ptr<IComponent>>` 实现组件的安全存储和多态类型擦除，极大地减少了内存碎片和显式 `new`/`delete` 的调用。

### 2.3 `Transform` 层级树

任何一个游戏引擎都需要处理父子层级关系，例如手持武器随角色移动。重构中，`TransformComponent` 封装了 `local_matrix` 和 `world_matrix` 的计算，并内置了父子节点 `mParent` 和 `mChildren` 指针，确保矩阵变换（平移、旋转、缩放）能正确继承。

---

## 3. UI 交互与 Python 绑定

### 3.1 属性面板重构
由于数据模型从继承变成了组件式，我们重构了 ImGui 界面（如 `property_panel.cpp`）。现在不再使用 `dynamic_cast<MeshObject>`，而是使用类型安全的组件查询机制：

```cpp
auto selected_mesh = entity->get_component<nengine::MeshComponent>();
auto selected_light = entity->get_component<nengine::LightComponent>();
```

### 3.2 脚本接口升级
Python 绑定同样进行了全面升级，通过 `pybind11` 我们暴露了 `Entity` 以及它的只读属性（如 `mesh`、`light` 和 `transform`），从而允许脚本端无缝访问新架构的组件。

---

## 4. 总结与展望

通过引入 ECS 和子系统模块化，JGL 引擎完成了核心基础设施的脱胎换骨：
1. **彻底消除深层继承**，实体功能组装更加灵活。
2. **现代内存管理**，大幅减少了生命周期引起的崩溃问题。
3. **架构清晰**，可以立刻着手进行 PBR 管线、多线程渲染和跨平台 RHI 的后续开发。

JGL 已初步具备了商业级游戏引擎的核心特征，接下来我们将向更加高性能的渲染管线进发！