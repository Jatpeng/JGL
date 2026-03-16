#include "python/python_bindings.h"

PYBIND11_MODULE(pyjgl, m)
{
  m.doc() = "Python bindings for the JGL runtime scene API.";

  bind_transform(m);
  bind_scene_object(m);
  bind_mesh_object(m);
  bind_light_object(m);
  bind_scene(m);
  bind_engine(m);
}
