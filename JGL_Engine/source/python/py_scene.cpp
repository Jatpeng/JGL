#include "python/python_bindings.h"

#include "engine/scene.h"

void bind_scene(py::module_& m)
{
  py::class_<nengine::Scene, std::shared_ptr<nengine::Scene>>(m, "Scene")
    .def_property(
      "name",
      [](const nengine::Scene& self)
      {
        return self.name();
      },
      [](nengine::Scene& self, const std::string& value)
      {
        self.set_name(value);
      })
    .def("create_entity", &nengine::Scene::create_entity)
    .def("create_mesh", &nengine::Scene::create_mesh)
    .def("create_light", &nengine::Scene::create_light)
    .def("remove_entity", &nengine::Scene::remove_entity)
    .def("find_entity", py::overload_cast<const std::string&>(&nengine::Scene::find_entity, py::const_))
    .def(
      "entities",
      [](const nengine::Scene& self)
      {
        return self.entities();
      })
    .def("set_skybox_enabled", &nengine::Scene::set_skybox_enabled)
    .def_property("skybox_enabled", &nengine::Scene::skybox_enabled, &nengine::Scene::set_skybox_enabled);
}
