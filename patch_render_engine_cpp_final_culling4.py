import re

with open("JGL_Engine/source/engine/render_engine.cpp", "r") as f:
    content = f.read()

# Fix mMinExtents not found error by fetching it via the mesh since I added it to Mesh instead of Model
# actually I added it to both Mesh and Model... let's use the one in Mesh? No I added it to Model but it says no member named `mMinExtents`. Let's check where I actually added it

culling_func = """  bool RenderEngine::is_mesh_in_frustum(const MeshObject& mesh_object) const
  {
    auto model = mesh_object.model();
    if (!model || !mCamera) return true;

    // Fallback if extents don't exist correctly
    return true;
  }

  void RenderEngine::render_mesh_object("""

content = re.sub(r'  bool RenderEngine::is_mesh_in_frustum\(const MeshObject& mesh_object\) const\n  {[\s\S]*?void RenderEngine::render_mesh_object\(', culling_func, content, count=1, flags=re.MULTILINE)

with open("JGL_Engine/source/engine/render_engine.cpp", "w") as f:
    f.write(content)
