#pragma once

#include <stdexcept>

#include <glm/glm.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

inline glm::vec3 vec3_from_python(const py::handle& value)
{
  py::sequence seq = py::reinterpret_borrow<py::sequence>(value);
  if (py::len(seq) != 3)
    throw std::runtime_error("Expected a 3-element sequence.");

  return glm::vec3(
    py::cast<float>(seq[0]),
    py::cast<float>(seq[1]),
    py::cast<float>(seq[2]));
}

inline py::tuple vec3_to_python(const glm::vec3& value)
{
  return py::make_tuple(value.x, value.y, value.z);
}

void bind_transform(py::module_& m);
void bind_scene_object(py::module_& m);
void bind_mesh_object(py::module_& m);
void bind_light_object(py::module_& m);
void bind_scene(py::module_& m);
void bind_engine(py::module_& m);
