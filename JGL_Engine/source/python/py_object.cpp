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
      "terrain",
      [](nengine::Entity& self) -> nengine::TerrainComponent*
      {
        return self.get_component<nengine::TerrainComponent>();
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
    .def("set_animation", &nengine::MeshComponent::set_animation, py::arg("path"), py::arg("clip_name") = "")
    .def("set_animation_asset", &nengine::MeshComponent::set_animation_asset)
    .def("clear_animation", &nengine::MeshComponent::clear_animation)
    .def("stop_animation", &nengine::MeshComponent::stop_animation)
    .def("set_material", &nengine::MeshComponent::set_material)
    .def("set_animation_playing", &nengine::MeshComponent::set_animation_playing)
    .def("set_animation_looping", &nengine::MeshComponent::set_animation_looping)
    .def("set_animation_speed", &nengine::MeshComponent::set_animation_speed)
    .def("reload_shader", &nengine::MeshComponent::reload_shader);
}

void bind_light_object(py::module_& m)
{
  py::enum_<nengine::LightComponent::LightType>(m, "LightType")
    .value("POINT", nengine::LightComponent::LightType::Point)
    .value("DIRECTIONAL", nengine::LightComponent::LightType::Directional)
    .export_values();

  py::class_<nengine::LightComponent>(m, "LightComponent")
    .def_property("type", &nengine::LightComponent::type, &nengine::LightComponent::set_type)
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
    .def_property(
      "direction",
      [](const nengine::LightComponent& self)
      {
        return vec3_to_python(self.direction());
      },
      [](nengine::LightComponent& self, const py::object& value)
      {
        self.set_direction(vec3_from_python(value));
      })
    .def_property("strength", &nengine::LightComponent::strength, &nengine::LightComponent::set_strength)
    .def_property("enabled", &nengine::LightComponent::enabled, &nengine::LightComponent::set_enabled)
    .def_property("casts_shadows", &nengine::LightComponent::casts_shadows, &nengine::LightComponent::set_casts_shadows)
    .def_property("shadow_bias_min", &nengine::LightComponent::shadow_bias_min, &nengine::LightComponent::set_shadow_bias_min)
    .def_property("shadow_bias_max", &nengine::LightComponent::shadow_bias_max, &nengine::LightComponent::set_shadow_bias_max)
    .def_property(
      "shadow_filter_radius",
      &nengine::LightComponent::shadow_filter_radius,
      &nengine::LightComponent::set_shadow_filter_radius);
}

void bind_terrain_object(py::module_& m)
{
  py::class_<nengine::TerrainComponent>(m, "TerrainComponent")
    .def("rebuild", &nengine::TerrainComponent::rebuild)
    .def("sample_height", &nengine::TerrainComponent::sample_height)
    .def_property("width", &nengine::TerrainComponent::width, &nengine::TerrainComponent::set_width)
    .def_property("depth", &nengine::TerrainComponent::depth, &nengine::TerrainComponent::set_depth)
    .def_property("resolution_x", &nengine::TerrainComponent::resolution_x, &nengine::TerrainComponent::set_resolution_x)
    .def_property("resolution_z", &nengine::TerrainComponent::resolution_z, &nengine::TerrainComponent::set_resolution_z)
    .def_property("height_scale", &nengine::TerrainComponent::height_scale, &nengine::TerrainComponent::set_height_scale)
    .def_property("height_offset", &nengine::TerrainComponent::height_offset, &nengine::TerrainComponent::set_height_offset)
    .def_property("uv_scale", &nengine::TerrainComponent::uv_scale, &nengine::TerrainComponent::set_uv_scale)
    .def_property("noise_frequency", &nengine::TerrainComponent::noise_frequency, &nengine::TerrainComponent::set_noise_frequency)
    .def_property("noise_octaves", &nengine::TerrainComponent::noise_octaves, &nengine::TerrainComponent::set_noise_octaves)
    .def_property("noise_persistence", &nengine::TerrainComponent::noise_persistence, &nengine::TerrainComponent::set_noise_persistence)
    .def_property("noise_lacunarity", &nengine::TerrainComponent::noise_lacunarity, &nengine::TerrainComponent::set_noise_lacunarity)
    .def_property("seed", &nengine::TerrainComponent::seed, &nengine::TerrainComponent::set_seed);
}
