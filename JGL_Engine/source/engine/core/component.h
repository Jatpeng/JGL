#pragma once

namespace nengine
{
  /**
   * @brief 组件的抽象基类接口。
   *
   * 所有具体的组件（如 TransformComponent, MeshComponent 等）都必须继承自此接口。
   * 组件仅用于存储状态数据或封装特定行为，不包含跨组件的逻辑。
   */
  class IComponent
  {
  public:
    virtual ~IComponent() = default;

    /**
     * @brief 返回组件的字符串类型名称，用于反射、UI和调试。
     */
    virtual const char* component_type() const = 0;
  };
}