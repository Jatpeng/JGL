#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>

#include "component.h"

namespace nengine
{
  class Entity;

  /**
   * @brief 管理实体的位置、旋转和缩放，以及父子层级关系的组件。
   *
   * 所有实体在创建时都默认带有此组件。提供世界矩阵和局部矩阵计算。
   */
  class TransformComponent : public IComponent
  {
  public:
    glm::vec3 position { 0.0f, 0.0f, 0.0f }; // 局部位置
    glm::vec3 rotation { 0.0f, 0.0f, 0.0f }; // 局部欧拉角旋转（度）
    glm::vec3 scale { 1.0f, 1.0f, 1.0f };    // 局部缩放比例

    TransformComponent() = default;

    /**
     * @brief 计算并返回相较于父节点的局部变换矩阵。
     */
    glm::mat4 local_matrix() const
    {
      glm::mat4 result { 1.0f };
      result = glm::translate(result, position);
      result = glm::rotate(result, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
      result = glm::rotate(result, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
      result = glm::rotate(result, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
      result = glm::scale(result, scale);
      return result;
    }

    /**
     * @brief 计算并返回绝对的世界变换矩阵。
     *        通过递归乘上所有祖先节点的局部矩阵来实现。
     */
    glm::mat4 world_matrix() const
    {
      if (mParent)
      {
        return mParent->world_matrix() * local_matrix();
      }
      return local_matrix();
    }

    /**
     * @brief 设置当前变换组件的父节点，自动维护双向指针。
     * @param parent 目标父节点的变换组件指针
     */
    void set_parent(TransformComponent* parent)
    {
      if (mParent == parent)
        return;

      if (mParent)
        mParent->remove_child(this);

      mParent = parent;
      if (mParent)
        mParent->add_child(this);
    }

    TransformComponent* parent() const { return mParent; }

    const std::vector<TransformComponent*>& children() const { return mChildren; }

    const char* component_type() const override { return "Transform"; }

  private:
    void add_child(TransformComponent* child)
    {
      mChildren.push_back(child);
    }

    void remove_child(TransformComponent* child)
    {
      mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), child), mChildren.end());
    }

    TransformComponent* mParent = nullptr;
    std::vector<TransformComponent*> mChildren;
  };
}