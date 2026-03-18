#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <typeindex>

#include "component.h"

namespace nengine
{
  /**
   * @brief 游戏引擎的核心实体类。
   *
   * 采用实体-组件-系统(ECS)架构，实体(Entity)本身只作为一个唯一标识符(ID)的容器，
   * 具体的行为和数据由挂载在实体上的组件(Component)来定义。
   * 这种设计提高了内存管理的灵活性，降低了类之间的耦合度。
   */
  class Entity
  {
  public:
    Entity(uint64_t id, std::string name) : mId(id), mName(std::move(name)) {}
    ~Entity() = default;

    uint64_t id() const { return mId; }
    const std::string& name() const { return mName; }
    void set_name(const std::string& name) { mName = name; }

    /**
     * @brief 为实体添加一个新组件。
     * @tparam T 组件类型，必须继承自 IComponent
     * @tparam Args 传递给组件构造函数的参数类型
     * @return 返回指向新创建组件的指针
     */
    template <typename T, typename... Args>
    T* add_component(Args&&... args)
    {
      auto comp = std::make_shared<T>(std::forward<Args>(args)...);
      mComponents[typeid(T)] = std::static_pointer_cast<IComponent>(comp);
      return comp.get();
    }

    /**
     * @brief 获取实体上的指定类型组件。
     * @tparam T 需要获取的组件类型
     * @return 如果存在则返回组件指针，否则返回 nullptr
     */
    template <typename T>
    T* get_component() const
    {
      auto it = mComponents.find(typeid(T));
      if (it != mComponents.end())
        return static_cast<T*>(it->second.get());
      return nullptr;
    }

    /**
     * @brief 从实体中移除指定类型的组件。
     * @tparam T 需要移除的组件类型
     */
    template <typename T>
    void remove_component()
    {
      mComponents.erase(typeid(T));
    }

  private:
    uint64_t mId; // 唯一标识符
    std::string mName; // 实体名称（方便调试和UI显示）
    // 存储所有组件的哈希表，通过 type_index 实现快速查找
    std::unordered_map<std::type_index, std::shared_ptr<IComponent>> mComponents;
  };
}
