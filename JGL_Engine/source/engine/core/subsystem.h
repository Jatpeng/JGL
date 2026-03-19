#pragma once

namespace nengine
{
  /**
   * @brief 子系统的统一生命周期接口。
   *
   * 引擎的每个独立模块（如渲染系统、输入系统、物理系统）都应实现此接口。
   * Engine 类会在合适的时机分别调用它们以解耦各模块的控制流。
   */
  class ISubsystem
  {
  public:
    virtual ~ISubsystem() = default;

    /**
     * @brief 初始化子系统资源和状态。
     * @return 成功返回 true，失败返回 false
     */
    virtual bool init() = 0;

    /**
     * @brief 每帧更新子系统逻辑。
     * @param delta_time 自上一帧经过的时间（秒）
     */
    virtual void tick(float delta_time) = 0;

    /**
     * @brief 清理和释放子系统分配的资源。
     */
    virtual void shutdown() = 0;
  };
}