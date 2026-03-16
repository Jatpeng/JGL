#include "python/python_bindings.h"

#include "engine/engine.h"

void bind_engine(py::module_& m)
{
  py::class_<nengine::Engine>(m, "Engine")
    .def(py::init<>())
    .def("init", &nengine::Engine::init)
    .def("run", &nengine::Engine::run)
    .def("tick", &nengine::Engine::tick)
    .def("create_scene", &nengine::Engine::create_scene)
    .def("set_active_scene", &nengine::Engine::set_active_scene)
    .def("active_scene", &nengine::Engine::active_scene);
}
