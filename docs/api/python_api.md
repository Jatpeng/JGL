# JGL Python API

> Generated from `JGL_Engine/source/python/*.cpp`.

Typical script bootstrap lives in `Game/script/init.py`:

```python
from init import create_engine, create_scene

engine = create_engine()
scene = create_scene(engine, "demo")
```

## `Engine`

Python entry point that mirrors the C++ runtime engine.

Source: `JGL_Engine/source/python/py_engine.cpp`

### Methods

| Method | Binding | Description |
| --- | --- | --- |
| `__init__` | `__init__()` | Creates an engine instance with default configuration. |
| `init` | `init(...)` | Initializes the runtime window and rendering state. |
| `run` | `run(...)` | Runs the engine main loop. |
| `tick` | `tick(...)` | Renders a single frame. |
| `create_scene` | `create_scene(...)` | Creates a new scene object. |
| `set_active_scene` | `set_active_scene(...)` | Sets the active scene. |
| `active_scene` | `active_scene(...)` | Returns the active scene. |

## `Scene`

Python-facing scene container.

Source: `JGL_Engine/source/python/py_scene.cpp`

### Properties

| Property | Binding | Description |
| --- | --- | --- |
| `name` | `name` | Readable and writable scene name. |
| `skybox_enabled` | `skybox_enabled` | Readable and writable skybox switch. |

### Methods

| Method | Binding | Description |
| --- | --- | --- |
| `create_mesh` | `create_mesh(...)` | Creates a mesh object in the scene. |
| `create_light` | `create_light(...)` | Creates a light object in the scene. |
| `remove_object` | `remove_object(...)` | Removes an object by id. |
| `find_object` | `find_object(...)` | Finds an object by name. |
| `objects` | `objects(...)` | Returns the scene object list. |
| `set_skybox_enabled` | `set_skybox_enabled(...)` | Enables or disables the skybox. |

## `Transform`

Editable transform exposed to Python.

Source: `JGL_Engine/source/python/py_object.cpp`

### Properties

| Property | Binding | Description |
| --- | --- | --- |
| `position` | `position` | Tuple-like XYZ position. |
| `rotation` | `rotation` | Tuple-like XYZ Euler rotation in degrees. |
| `scale` | `scale` | Tuple-like XYZ local scale. |

### Methods

| Method | Binding | Description |
| --- | --- | --- |
| `__init__` | `__init__()` | Creates a transform value. |

## `SceneObject`

Base Python object type shared by mesh and light objects.

Source: `JGL_Engine/source/python/py_object.cpp`

### Properties

| Property | Binding | Description |
| --- | --- | --- |
| `name` | `name` | Readable and writable object name. |
| `id` | `id` | Read-only runtime identifier. |
| `transform` | `transform` | Read-only transform handle with mutable fields. |
| `object_type` | `object_type` | Read-only type label. |

## `MeshObject`

Python-facing mesh object.

Source: `JGL_Engine/source/python/py_object.cpp`

### Methods

| Method | Binding | Description |
| --- | --- | --- |
| `set_model` | `set_model(...)` | Loads and assigns a mesh asset path. |
| `set_material` | `set_material(...)` | Loads and assigns a material asset path. |

## `LightObject`

Python-facing light object.

Source: `JGL_Engine/source/python/py_object.cpp`

### Properties

| Property | Binding | Description |
| --- | --- | --- |
| `color` | `color` | Readable and writable RGB color. |
| `strength` | `strength` | Readable and writable light intensity. |
| `enabled` | `enabled` | Readable and writable enable switch. |
