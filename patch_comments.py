import re

with open("JGL_Engine/source/engine/render_engine.cpp", "r") as f:
    content = f.read()

# Add Chinese comments
content = content.replace("    // Sort meshes by shader ID to minimize state changes", "    // 根据着色器ID对网格进行排序，以最小化状态切换 (State Sorting)")
content = content.replace("    // Shadow Map Init", "    // 阴影贴图初始化 (Shadow Map Init)")
content = content.replace("      // TODO: Support skinning in shadow pass", "      // TODO: 在阴影通道中支持蒙皮动画 (Support skinning in shadow pass)")

with open("JGL_Engine/source/engine/render_engine.cpp", "w") as f:
    f.write(content)

with open("JGL_Engine/source/elems/camera.h", "r") as f:
    content = f.read()

content = content.replace("    std::vector<FrustumPlane> get_frustum_planes() const", "    // 获取视锥体的6个平面\n    std::vector<FrustumPlane> get_frustum_planes() const")
content = content.replace("    bool is_aabb_in_frustum(const glm::vec3& minExtents, const glm::vec3& maxExtents) const", "    // 检查AABB（轴对齐包围盒）是否在视锥体内\n    bool is_aabb_in_frustum(const glm::vec3& minExtents, const glm::vec3& maxExtents) const")

with open("JGL_Engine/source/elems/camera.h", "w") as f:
    f.write(content)

with open("JGL_Engine/source/engine/scene.h", "r") as f:
    content = f.read()

content = content.replace("    void set_is_directional(bool directional) { mIsDirectional = directional; }", "    // 设置是否为平行光 (Directional Light)\n    void set_is_directional(bool directional) { mIsDirectional = directional; }")
content = content.replace("    void set_cast_shadows(bool cast) { mCastShadows = cast; }", "    // 设置是否投射阴影 (Cast Shadows)\n    void set_cast_shadows(bool cast) { mCastShadows = cast; }")

with open("JGL_Engine/source/engine/scene.h", "w") as f:
    f.write(content)
