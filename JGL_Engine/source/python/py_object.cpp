#include "python/python_bindings.h"

#include "engine/scene.h"

void bind_transform(py::module_& m)
{
  py::class_<nengine::TransformComponent>(m, "TransformComponent")
    .def(py::init<>())
    .def_property(
      "position",
      [](const nengine::TransformComponent& self)
      {
        return vec3_to_python(self.position);
      },
      [](nengine::TransformComponent& self, const py::object& value)
      {
        self.position = vec3_from_python(value);
      })
    .def_property(
      "rotation",
      [](const nengine::TransformComponent& self)
      {
        return vec3_to_python(self.rotation);
      },
      [](nengine::TransformComponent& self, const py::object& value)
      {
        self.rotation = vec3_from_python(value);
      })
    .def_property(
      "scale",
      [](const nengine::TransformComponent& self)
      {
        return vec3_to_python(self.scale);
      },
      [](nengine::TransformComponent& self, const py::object& value)
      {
        self.scale = vec3_from_python(value);
      });
}

void bind_scene_object(py::module_& m)
{
  py::class_<nengine::Entity, std::shared_ptr<nengine::Entity>>(m, "Entity")
    .def_property(
      "name",
      [](const nengine::Entity& self)
      {
        return self.name();
      },
      [](nengine::Entity& self, const std::string& value)
      {
        self.set_name(value);
      })
    .def_property_readonly("id", &nengine::Entity::id)
    .def_property_readonly(
      "transform",
      [](nengine::Entity& self) -> nengine::TransformComponent*
      {
        return self.get_component<nengine::TransformComponent>();
      },
      py::return_value_policy::reference_internal)
    .def_property_readonly(
      "mesh",
      [](nengine::Entity& self) -> nengine::MeshComponent*
      {
        return self.get_component<nengine::MeshComponent>();
      },
      py::return_value_policy::reference_internal)
    .def_property_readonly(
      "light",
      [](nengine::Entity& self) -> nengine::LightComponent*
      {
        return self.get_component<nengine::LightComponent>();
      },
      py::return_value_policy::reference_internal);
}

void bind_mesh_object(py::module_& m)
{
  py::class_<nengine::MeshComponent>(m, "MeshComponent")
    .def("set_model", &nengine::MeshComponent::set_model)
    .def("set_material", &nengine::MeshComponent::set_material);
}

void bind_light_object(py::module_& m)
{
  py::class_<nengine::LightComponent>(m, "LightComponent")
    .def_property(
      "color",
      [](const nengine::LightComponent& self)
      {
        return vec3_to_python(self.color());
      },
      [](nengine::LightComponent& self, const py::object& value)
      {
        self.set_color(vec3_from_python(value));
      })
    .def_property("strength", &nengine::LightComponent::strength, &nengine::LightComponent::set_strength)
    .def_property("enabled", &nengine::LightComponent::enabled, &nengine::LightComponent::set_enabled);
}
