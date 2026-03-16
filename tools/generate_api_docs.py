from __future__ import annotations

import argparse
import re
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


CPP_DESCRIPTIONS = {
    "Engine": "Top-level runtime entry point that owns the window, renderer, resource manager, and active scene.",
    "Engine::CreateInfo": "Configuration used before engine initialization.",
    "Engine::CreateInfo.width": "Initial window width in pixels.",
    "Engine::CreateInfo.height": "Initial window height in pixels.",
    "Engine::CreateInfo.title": "Window title shown by the native window.",
    "Engine::CreateInfo.create_default_scene": "When enabled, creates a default scene during initialization.",
    "Engine::CreateInfo.show_plane": "Controls whether the built-in ground plane is visible.",
    "Engine.init": "Initializes the rendering window and default runtime state.",
    "Engine.run": "Runs the main loop until the window closes.",
    "Engine.tick": "Renders a single frame. Useful for externally driven loops.",
    "Engine.create_scene": "Creates a scene and registers it with the engine.",
    "Engine.set_active_scene": "Switches the current scene used by the renderer and editor UI.",
    "Engine.active_scene": "Returns the current active scene.",
    "Engine.render_engine": "Returns the low-level renderer used by the editor runtime.",
    "Engine.is_initialized": "Reports whether the engine has already been initialized.",
    "Transform": "Local transform container shared by scene objects.",
    "Transform.position": "Translation in world units.",
    "Transform.rotation": "Euler rotation in degrees.",
    "Transform.scale": "Local scale multiplier.",
    "Transform.local_matrix": "Builds the local transformation matrix from position, rotation, and scale.",
    "SceneObject": "Base class for all runtime objects stored in a scene.",
    "SceneObject.id": "Stable runtime identifier.",
    "SceneObject.name": "User-visible object name.",
    "SceneObject.set_name": "Changes the object name.",
    "SceneObject.transform": "Returns the editable local transform.",
    "SceneObject.object_type": "Returns the runtime type label used by the editor.",
    "MeshObject": "Scene object that owns a model, material, and optional animation state.",
    "MeshObject.set_model": "Loads and assigns a mesh asset.",
    "MeshObject.set_material": "Loads and assigns a material asset.",
    "MeshObject.set_shader": "Overrides the shader program used by the mesh.",
    "MeshObject.model_path": "Resolved source path of the bound model asset.",
    "MeshObject.material_path": "Resolved source path of the bound material asset.",
    "MeshObject.shader_path": "Resolved source path of the bound shader asset.",
    "MeshObject.shader_name": "Base shader program name without suffixes.",
    "MeshObject.is_skinned": "Reports whether the loaded model contains skinning data.",
    "MeshObject.tick": "Advances per-frame animation state.",
    "MeshObject.apply_skinning": "Uploads current bone matrices to a shader.",
    "LightObject": "Scene object representing a point light contribution.",
    "LightObject.set_color": "Sets light RGB color.",
    "LightObject.color": "Returns light RGB color.",
    "LightObject.set_strength": "Sets emitted light intensity.",
    "LightObject.strength": "Returns emitted light intensity.",
    "LightObject.set_enabled": "Enables or disables the light.",
    "LightObject.enabled": "Returns whether the light is enabled.",
    "Scene": "Container that owns all scene objects and scene-level switches.",
    "Scene.name": "Scene display name.",
    "Scene.set_name": "Changes the scene display name.",
    "Scene.create_mesh": "Creates and registers a mesh object.",
    "Scene.create_light": "Creates and registers a light object.",
    "Scene.remove_object": "Removes an object by runtime id.",
    "Scene.find_object": "Finds an object by id or by name.",
    "Scene.objects": "Returns the full object collection.",
    "Scene.set_skybox_enabled": "Enables or disables the built-in skybox pass.",
    "Scene.skybox_enabled": "Reports whether the skybox pass is enabled.",
}


PY_DESCRIPTIONS = {
    "Engine": "Python entry point that mirrors the C++ runtime engine.",
    "Engine.__init__": "Creates an engine instance with default configuration.",
    "Engine.init": "Initializes the runtime window and rendering state.",
    "Engine.run": "Runs the engine main loop.",
    "Engine.tick": "Renders a single frame.",
    "Engine.create_scene": "Creates a new scene object.",
    "Engine.set_active_scene": "Sets the active scene.",
    "Engine.active_scene": "Returns the active scene.",
    "Scene": "Python-facing scene container.",
    "Scene.name": "Readable and writable scene name.",
    "Scene.create_mesh": "Creates a mesh object in the scene.",
    "Scene.create_light": "Creates a light object in the scene.",
    "Scene.remove_object": "Removes an object by id.",
    "Scene.find_object": "Finds an object by name.",
    "Scene.objects": "Returns the scene object list.",
    "Scene.set_skybox_enabled": "Enables or disables the skybox.",
    "Scene.skybox_enabled": "Readable and writable skybox switch.",
    "Transform": "Editable transform exposed to Python.",
    "Transform.__init__": "Creates a transform value.",
    "Transform.position": "Tuple-like XYZ position.",
    "Transform.rotation": "Tuple-like XYZ Euler rotation in degrees.",
    "Transform.scale": "Tuple-like XYZ local scale.",
    "SceneObject": "Base Python object type shared by mesh and light objects.",
    "SceneObject.name": "Readable and writable object name.",
    "SceneObject.id": "Read-only runtime identifier.",
    "SceneObject.transform": "Read-only transform handle with mutable fields.",
    "SceneObject.object_type": "Read-only type label.",
    "MeshObject": "Python-facing mesh object.",
    "MeshObject.set_model": "Loads and assigns a mesh asset path.",
    "MeshObject.set_material": "Loads and assigns a material asset path.",
    "LightObject": "Python-facing light object.",
    "LightObject.color": "Readable and writable RGB color.",
    "LightObject.strength": "Readable and writable light intensity.",
    "LightObject.enabled": "Readable and writable enable switch.",
}


@dataclass
class MemberDoc:
    name: str
    signature: str
    description: str = ""


@dataclass
class EntityDoc:
    name: str
    kind: str
    source: Path
    description: str = ""
    methods: list[MemberDoc] = field(default_factory=list)
    fields: list[MemberDoc] = field(default_factory=list)
    properties: list[MemberDoc] = field(default_factory=list)


def extract_block(text: str, anchor: str) -> str:
    match = re.search(anchor, text)
    if not match:
        raise ValueError(f"Unable to find block for: {anchor}")

    brace_start = text.find("{", match.end())
    if brace_start < 0:
        raise ValueError(f"Unable to find opening brace for: {anchor}")

    depth = 0
    for index in range(brace_start, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[brace_start + 1:index]

    raise ValueError(f"Unable to find closing brace for: {anchor}")


def remove_block(text: str, anchor: str) -> str:
    match = re.search(anchor, text)
    if not match:
        return text

    brace_start = text.find("{", match.end())
    if brace_start < 0:
        return text

    depth = 0
    for index in range(brace_start, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                semicolon_index = text.find(";", index)
                if semicolon_index < 0:
                    semicolon_index = index
                return text[:match.start()] + text[semicolon_index + 1:]

    return text


def normalize_cpp_statement(statement: str) -> str:
    statement = statement.strip()
    if not statement:
        return ""

    statement = re.sub(r"\s+", " ", statement)
    if re.search(r"(\)\s*(?:const|override|final|noexcept|\s)*)\{[^{}]*\}\s*$", statement):
        statement = re.sub(
            r"(\)\s*(?:const|override|final|noexcept|\s)*)\{[^{}]*\}\s*$",
            r"\1;",
            statement,
        )

    statement = statement.strip()
    if not statement.endswith(";"):
        statement += ";"
    return statement


def parse_public_declarations(block: str, default_access: str) -> list[str]:
    access = default_access
    declarations: list[str] = []
    current = ""

    for raw_line in block.splitlines():
        line = raw_line.split("//", 1)[0].strip()
        if not line:
            continue

        if line in {"public:", "private:", "protected:"}:
            access = line[:-1]
            continue

        if access != "public":
            continue

        normalized_line = line
        if re.search(r"(\)\s*(?:const|override|final|noexcept|\s)*)\{[^{}]*\}\s*$", normalized_line):
            normalized_line = re.sub(
                r"(\)\s*(?:const|override|final|noexcept|\s)*)\{[^{}]*\}\s*$",
                r"\1;",
                normalized_line,
            )

        current = f"{current} {normalized_line}".strip() if current else normalized_line
        if ";" in normalized_line:
            declaration = normalize_cpp_statement(current.split(";", 1)[0])
            if declaration and not declaration.startswith("class ") and not declaration.startswith("struct "):
                declarations.append(declaration)
            current = ""

    return declarations


def extract_cpp_member_name(signature: str) -> str:
    cleaned = signature.rstrip(";")
    if "(" in cleaned:
        match = re.search(r"([~A-Za-z_][A-Za-z0-9_:]*)\s*\(", cleaned)
        if match:
            return match.group(1).split("::")[-1]
    match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:=|\{|;)", cleaned)
    if match:
        return match.group(1)
    return cleaned


def parse_cpp_entity(header_path: Path, entity_name: str, entity_kind: str, block_override: str | None = None) -> EntityDoc:
    text = header_path.read_text(encoding="utf-8")
    block_anchor = block_override or rf"\b{entity_kind}\s+{re.escape(entity_name)}\b"
    block = extract_block(text, block_anchor)

    if entity_name == "Engine":
        block = remove_block(block, r"\bstruct\s+CreateInfo\b")

    default_access = "public" if entity_kind == "struct" else "private"
    declarations = parse_public_declarations(block, default_access)

    entity = EntityDoc(
        name=entity_name,
        kind=entity_kind,
        source=header_path.relative_to(ROOT),
        description=CPP_DESCRIPTIONS.get(entity_name, ""),
    )

    for declaration in declarations:
        member_name = extract_cpp_member_name(declaration)
        description_key = f"{entity_name}.{member_name}"
        member = MemberDoc(
            name=member_name,
            signature=declaration,
            description=CPP_DESCRIPTIONS.get(description_key, ""),
        )
        if "(" in declaration:
            entity.methods.append(member)
        else:
            entity.fields.append(member)

    return entity


def parse_python_entities(binding_paths: list[Path]) -> list[EntityDoc]:
    entities: dict[str, EntityDoc] = {}
    current_class: str | None = None
    pending_kind: str | None = None

    class_pattern = re.compile(r'py::class_<.*?>\(m,\s*"([^"]+)"\)')
    def_pattern = re.compile(r'\.def\("([^"]+)"')
    property_pattern = re.compile(r'\.def_property(?:_readonly)?\("([^"]+)"')
    literal_pattern = re.compile(r'"([^"]+)"')

    for binding_path in binding_paths:
        text = binding_path.read_text(encoding="utf-8")
        for raw_line in text.splitlines():
            line = raw_line.strip()
            class_match = class_pattern.search(line)
            if class_match:
                current_class = class_match.group(1)
                entities.setdefault(
                    current_class,
                    EntityDoc(
                        name=current_class,
                        kind="python-class",
                        source=binding_path.relative_to(ROOT),
                        description=PY_DESCRIPTIONS.get(current_class, ""),
                    ),
                )
                continue

            if not current_class:
                continue

            entity = entities[current_class]
            if ".def(py::init" in line:
                entity.methods.append(
                    MemberDoc(
                        name="__init__",
                        signature="__init__()",
                        description=PY_DESCRIPTIONS.get(f"{current_class}.__init__", ""),
                    )
                )
                continue

            if pending_kind:
                literal_match = literal_pattern.search(line)
                if literal_match:
                    member_name = literal_match.group(1)
                    if pending_kind == "property":
                        entity.properties.append(
                            MemberDoc(
                                name=member_name,
                                signature=member_name,
                                description=PY_DESCRIPTIONS.get(f"{current_class}.{member_name}", ""),
                            )
                        )
                    elif pending_kind == "method":
                        entity.methods.append(
                            MemberDoc(
                                name=member_name,
                                signature=f"{member_name}(...)",
                                description=PY_DESCRIPTIONS.get(f"{current_class}.{member_name}", ""),
                            )
                        )
                    pending_kind = None
                    continue

            property_match = property_pattern.search(line)
            if property_match:
                property_name = property_match.group(1)
                entity.properties.append(
                    MemberDoc(
                        name=property_name,
                        signature=property_name,
                        description=PY_DESCRIPTIONS.get(f"{current_class}.{property_name}", ""),
                    )
                )
                continue
            if ".def_property(" in line or ".def_property_readonly(" in line:
                pending_kind = "property"
                continue

            def_match = def_pattern.search(line)
            if def_match:
                method_name = def_match.group(1)
                entity.methods.append(
                    MemberDoc(
                        name=method_name,
                        signature=f"{method_name}(...)",
                        description=PY_DESCRIPTIONS.get(f"{current_class}.{method_name}", ""),
                    )
                )
                continue
            if re.search(r"\.def\(\s*$", line):
                pending_kind = "method"

    ordered_names = ["Engine", "Scene", "Transform", "SceneObject", "MeshObject", "LightObject"]
    ordered_entities: list[EntityDoc] = []
    for name in ordered_names:
        if name in entities:
            ordered_entities.append(entities[name])
    for name, entity in entities.items():
        if name not in ordered_names:
            ordered_entities.append(entity)
    return ordered_entities


def render_table(headers: list[str], rows: list[list[str]]) -> str:
    if not rows:
        return "_None._\n"

    header_line = "| " + " | ".join(headers) + " |"
    separator_line = "| " + " | ".join("---" for _ in headers) + " |"
    body_lines = ["| " + " | ".join(row) + " |" for row in rows]
    return "\n".join([header_line, separator_line, *body_lines]) + "\n"


def render_cpp_markdown(entities: list[EntityDoc]) -> str:
    lines = [
        "# JGL C++ Runtime API",
        "",
        "> Generated from `JGL_Engine/source/engine/engine.h` and `JGL_Engine/source/engine/scene.h`.",
        "",
    ]

    for entity in entities:
        lines.extend(
            [
                f"## `{entity.name}`",
                "",
                entity.description or "_No description available._",
                "",
                f"Source: `{entity.source.as_posix()}`",
                "",
            ]
        )

        if entity.fields:
            lines.extend(["### Fields", "", render_table(
                ["Field", "Declaration", "Description"],
                [[f"`{item.name}`", f"`{item.signature}`", item.description or ""] for item in entity.fields],
            ).rstrip(), ""])

        if entity.methods:
            lines.extend(["### Methods", "", render_table(
                ["Method", "Declaration", "Description"],
                [[f"`{item.name}`", f"`{item.signature}`", item.description or ""] for item in entity.methods],
            ).rstrip(), ""])

    return "\n".join(lines).rstrip() + "\n"


def render_python_markdown(entities: list[EntityDoc]) -> str:
    lines = [
        "# JGL Python API",
        "",
        "> Generated from `JGL_Engine/source/python/*.cpp`.",
        "",
        "Typical script bootstrap lives in `Game/script/init.py`:",
        "",
        "```python",
        "from init import create_engine, create_scene",
        "",
        "engine = create_engine()",
        "scene = create_scene(engine, \"demo\")",
        "```",
        "",
    ]

    for entity in entities:
        lines.extend(
            [
                f"## `{entity.name}`",
                "",
                entity.description or "_No description available._",
                "",
                f"Source: `{entity.source.as_posix()}`",
                "",
            ]
        )

        if entity.properties:
            lines.extend(["### Properties", "", render_table(
                ["Property", "Binding", "Description"],
                [[f"`{item.name}`", f"`{item.signature}`", item.description or ""] for item in entity.properties],
            ).rstrip(), ""])

        if entity.methods:
            lines.extend(["### Methods", "", render_table(
                ["Method", "Binding", "Description"],
                [[f"`{item.name}`", f"`{item.signature}`", item.description or ""] for item in entity.methods],
            ).rstrip(), ""])

    return "\n".join(lines).rstrip() + "\n"


def render_index_markdown() -> str:
    return "\n".join(
        [
            "# JGL API Docs",
            "",
            "Generated documentation entry points:",
            "",
            "- [C++ Runtime API](cpp_runtime_api.md)",
            "- [Python API](python_api.md)",
            "",
            "Regenerate with:",
            "",
            "```powershell",
            "python tools/generate_api_docs.py",
            "cmake --build build --target api_docs",
            "```",
            "",
        ]
    )


def generate_docs(output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    engine_header = ROOT / "JGL_Engine" / "source" / "engine" / "engine.h"
    scene_header = ROOT / "JGL_Engine" / "source" / "engine" / "scene.h"
    python_binding_paths = [
        ROOT / "JGL_Engine" / "source" / "python" / "py_engine.cpp",
        ROOT / "JGL_Engine" / "source" / "python" / "py_scene.cpp",
        ROOT / "JGL_Engine" / "source" / "python" / "py_object.cpp",
    ]

    cpp_entities = [
        parse_cpp_entity(engine_header, "Engine", "class"),
        parse_cpp_entity(engine_header, "CreateInfo", "struct", block_override=r"\bstruct\s+CreateInfo\b"),
        parse_cpp_entity(scene_header, "Transform", "struct"),
        parse_cpp_entity(scene_header, "SceneObject", "class"),
        parse_cpp_entity(scene_header, "MeshObject", "class"),
        parse_cpp_entity(scene_header, "LightObject", "class"),
        parse_cpp_entity(scene_header, "Scene", "class"),
    ]

    cpp_entities[1].name = "Engine::CreateInfo"
    cpp_entities[1].description = CPP_DESCRIPTIONS.get("Engine::CreateInfo", "")
    for field in cpp_entities[1].fields:
        field.description = CPP_DESCRIPTIONS.get(f"Engine::CreateInfo.{field.name}", field.description)

    python_entities = parse_python_entities(python_binding_paths)

    (output_dir / "index.md").write_text(render_index_markdown(), encoding="utf-8")
    (output_dir / "cpp_runtime_api.md").write_text(render_cpp_markdown(cpp_entities), encoding="utf-8")
    (output_dir / "python_api.md").write_text(render_python_markdown(python_entities), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate markdown API documentation for JGL.")
    parser.add_argument(
        "--output",
        default=str(ROOT / "docs" / "api"),
        help="Output directory for generated markdown files.",
    )
    args = parser.parse_args()

    generate_docs(Path(args.output))


if __name__ == "__main__":
    main()
