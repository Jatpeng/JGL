#include "python/python_bindings.h"

#include "engine/scene.h"

void bind_transform(py::module_& m)
{
  py::class_<nengine::Transform>(m, "Transform")
    .def(py::init<>())
    .def_property(
      "position",
      [](const nengine::Transform& self)
      {
        return vec3_to_python(self.position);
      },
      [](nengine::Transform& self, const py::object& value)
      {
        self.position = vec3_from_python(value);
      })
    .def_property(
      "rotation",
      [](const nengine::Transform& self)
      {
        return vec3_to_python(self.rotation);
      },
      [](nengine::Transform& self, const py::object& value)
      {
        self.rotation = vec3_from_python(value);
      })
    .def_property(
      "scale",
      [](const nengine::Transform& self)
      {
        return vec3_to_python(self.scale);
      },
      [](nengine::Transform& self, const py::object& value)
      {
        self.scale = vec3_from_python(value);
      });
}

void bind_scene_object(py::module_& m)
{
  py::class_<nengine::SceneObject, std::shared_ptr<nengine::SceneObject>>(m, "SceneObject")
    .def_property(
      "name",
      [](const nengine::SceneObject& self)
      {
        return self.name();
      },
      [](nengine::SceneObject& self, const std::string& value)
      {
        self.set_name(value);
      })
    .def_property_readonly("id", &nengine::SceneObject::id)
    .def_property_readonly(
      "transform",
      [](nengine::SceneObject& self) -> nengine::Transform&
      {
        return self.transform();
      },
      py::return_value_policy::reference_internal)
    .def_property_readonly(
      "object_type",
      [](const nengine::SceneObject& self)
      {
        return std::string(self.object_type());
      });
}

void bind_mesh_object(py::module_& m)
{
  py::class_<nengine::MeshObject, nengine::SceneObject, std::shared_ptr<nengine::MeshObject>>(m, "MeshObject")
    .def("set_model", &nengine::MeshObject::set_model)
    .def("set_material", &nengine::MeshObject::set_material);
}

void bind_light_object(py::module_& m)
{
  py::class_<nengine::LightObject, nengine::SceneObject, std::shared_ptr<nengine::LightObject>>(m, "LightObject")
    .def_property(
      "color",
      [](const nengine::LightObject& self)
      {
        return vec3_to_python(self.color());
      },
      [](nengine::LightObject& self, const py::object& value)
      {
        self.set_color(vec3_from_python(value));
      })
    .def_property("strength", &nengine::LightObject::strength, &nengine::LightObject::set_strength)
    .def_property("enabled", &nengine::LightObject::enabled, &nengine::LightObject::set_enabled);
}
