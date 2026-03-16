# JGL C++ Runtime API

> Generated from `JGL_Engine/source/engine/engine.h` and `JGL_Engine/source/engine/scene.h`.

## `Engine`

Top-level runtime entry point that owns the window, renderer, resource manager, and active scene.

Source: `JGL_Engine/source/engine/engine.h`

### Methods

| Method | Declaration | Description |
| --- | --- | --- |
| `Engine` | `explicit Engine(const CreateInfo& create_info = {});` |  |
| `~Engine` | `~Engine();` |  |
| `init` | `bool init();` | Initializes the rendering window and default runtime state. |
| `run` | `void run();` | Runs the main loop until the window closes. |
| `tick` | `void tick();` | Renders a single frame. Useful for externally driven loops. |
| `create_scene` | `std::shared_ptr<Scene> create_scene(const std::string& name);` | Creates a scene and registers it with the engine. |
| `set_active_scene` | `void set_active_scene(std::shared_ptr<Scene> scene);` | Switches the current scene used by the renderer and editor UI. |
| `active_scene` | `std::shared_ptr<Scene> active_scene() const;` | Returns the current active scene. |
| `render_engine` | `RenderEngine* render_engine() const;` | Returns the low-level renderer used by the editor runtime. |
| `is_initialized` | `bool is_initialized() const;` | Reports whether the engine has already been initialized. |

## `Engine::CreateInfo`

Configuration used before engine initialization.

Source: `JGL_Engine/source/engine/engine.h`

### Fields

| Field | Declaration | Description |
| --- | --- | --- |
| `width` | `int width = 1024;` | Initial window width in pixels. |
| `height` | `int height = 720;` | Initial window height in pixels. |
| `title` | `std::string title = "JGL_Engine";` | Window title shown by the native window. |
| `create_default_scene` | `bool create_default_scene = true;` | When enabled, creates a default scene during initialization. |
| `show_plane` | `bool show_plane = false;` | Controls whether the built-in ground plane is visible. |

## `Transform`

Local transform container shared by scene objects.

Source: `JGL_Engine/source/engine/scene.h`

### Fields

| Field | Declaration | Description |
| --- | --- | --- |
| `position` | `glm::vec3 position { 0.0f, 0.0f, 0.0f };` | Translation in world units. |
| `rotation` | `glm::vec3 rotation { 0.0f, 0.0f, 0.0f };` | Euler rotation in degrees. |
| `scale` | `glm::vec3 scale { 1.0f, 1.0f, 1.0f };` | Local scale multiplier. |

### Methods

| Method | Declaration | Description |
| --- | --- | --- |
| `local_matrix` | `glm::mat4 local_matrix() const;` | Builds the local transformation matrix from position, rotation, and scale. |

## `SceneObject`

Base class for all runtime objects stored in a scene.

Source: `JGL_Engine/source/engine/scene.h`

### Methods

| Method | Declaration | Description |
| --- | --- | --- |
| `~SceneObject` | `virtual ~SceneObject() = default;` |  |
| `id` | `uint64_t id() const;` | Stable runtime identifier. |
| `name` | `const std::string& name() const;` | User-visible object name. |
| `set_name` | `void set_name(const std::string& name);` | Changes the object name. |
| `transform` | `Transform& transform();` | Returns the editable local transform. |
| `transform` | `const Transform& transform() const;` | Returns the editable local transform. |
| `object_type` | `virtual const char* object_type() const = 0;` | Returns the runtime type label used by the editor. |

## `MeshObject`

Scene object that owns a model, material, and optional animation state.

Source: `JGL_Engine/source/engine/scene.h`

### Methods

| Method | Declaration | Description |
| --- | --- | --- |
| `MeshObject` | `MeshObject(uint64_t id, std::string name, std::shared_ptr<IResourceManager> resources);` |  |
| `~MeshObject` | `~MeshObject() override;` |  |
| `set_model` | `bool set_model(const std::string& path);` | Loads and assigns a mesh asset. |
| `set_material` | `bool set_material(const std::string& path);` | Loads and assigns a material asset. |
| `set_shader` | `bool set_shader(const std::string& path);` | Overrides the shader program used by the mesh. |
| `model` | `std::shared_ptr<nelems::Model> model() const;` |  |
| `material` | `std::shared_ptr<Material> material() const;` |  |
| `shader` | `nshaders::Shader* shader() const;` |  |
| `model_path` | `const std::string& model_path() const;` | Resolved source path of the bound model asset. |
| `material_path` | `const std::string& material_path() const;` | Resolved source path of the bound material asset. |
| `shader_path` | `const std::string& shader_path() const;` | Resolved source path of the bound shader asset. |
| `shader_name` | `const std::string& shader_name() const;` | Base shader program name without suffixes. |
| `is_skinned` | `bool is_skinned() const;` | Reports whether the loaded model contains skinning data. |
| `tick` | `void tick(float delta_time);` | Advances per-frame animation state. |
| `apply_skinning` | `void apply_skinning(nshaders::Shader* shader) const;` | Uploads current bone matrices to a shader. |
| `object_type` | `const char* object_type() const override;` |  |

## `LightObject`

Scene object representing a point light contribution.

Source: `JGL_Engine/source/engine/scene.h`

### Methods

| Method | Declaration | Description |
| --- | --- | --- |
| `LightObject` | `LightObject(uint64_t id, std::string name);` |  |
| `set_color` | `void set_color(const glm::vec3& color);` | Sets light RGB color. |
| `color` | `const glm::vec3& color() const;` | Returns light RGB color. |
| `set_strength` | `void set_strength(float strength);` | Sets emitted light intensity. |
| `strength` | `float strength() const;` | Returns emitted light intensity. |
| `set_enabled` | `void set_enabled(bool enabled);` | Enables or disables the light. |
| `enabled` | `bool enabled() const;` | Returns whether the light is enabled. |
| `object_type` | `const char* object_type() const override;` |  |

## `Scene`

Container that owns all scene objects and scene-level switches.

Source: `JGL_Engine/source/engine/scene.h`

### Methods

| Method | Declaration | Description |
| --- | --- | --- |
| `Scene` | `explicit Scene(std::string name, std::shared_ptr<IResourceManager> resources);` |  |
| `name` | `const std::string& name() const;` | Scene display name. |
| `set_name` | `void set_name(const std::string& name);` | Changes the scene display name. |
| `create_mesh` | `std::shared_ptr<MeshObject> create_mesh(const std::string& name);` | Creates and registers a mesh object. |
| `create_light` | `std::shared_ptr<LightObject> create_light(const std::string& name);` | Creates and registers a light object. |
| `remove_object` | `void remove_object(uint64_t id);` | Removes an object by runtime id. |
| `find_object` | `std::shared_ptr<SceneObject> find_object(uint64_t id) const;` | Finds an object by id or by name. |
| `find_object` | `std::shared_ptr<SceneObject> find_object(const std::string& name) const;` | Finds an object by id or by name. |
| `objects` | `const std::vector<std::shared_ptr<SceneObject>>& objects() const;` | Returns the full object collection. |
| `set_skybox_enabled` | `void set_skybox_enabled(bool enabled);` | Enables or disables the built-in skybox pass. |
| `skybox_enabled` | `bool skybox_enabled() const;` | Reports whether the skybox pass is enabled. |
