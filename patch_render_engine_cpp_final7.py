import re

with open("JGL_Engine/source/engine/render_engine.cpp", "r") as f:
    content = f.read()

content = content.replace("    // Sort meshes by shader ID to minimize state changes", "    // 根据着色器ID对网格进行排序，以最小化状态切换 (State Sorting)")
content = content.replace("    // Shadow Map Init", "    // 阴影贴图初始化 (Shadow Map Init)")
content = content.replace("      // TODO: Support skinning in shadow pass", "      // TODO: 在阴影通道中支持蒙皮动画 (Support skinning in shadow pass)")

with open("JGL_Engine/source/engine/render_engine.cpp", "w") as f:
    f.write(content)
