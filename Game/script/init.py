from pathlib import Path
import sys


MODULE_PATTERNS = ("pyjgl*.pyd", "pyjgl*.so")


def workspace_root() -> Path:
    return Path(__file__).resolve().parents[1]


def engine_search_dirs(root: Path):
    engine_root = root / "engine"
    if not engine_root.exists():
        return

    seen = set()
    preferred = [
        engine_root / "Release",
        engine_root / "Debug",
        engine_root / "RelWithDebInfo",
        engine_root / "MinSizeRel",
        engine_root,
    ]

    for candidate in preferred:
        if not candidate.exists():
            continue

        resolved = candidate.resolve()
        if resolved in seen:
            continue

        seen.add(resolved)
        yield candidate

    for candidate in sorted(engine_root.iterdir()):
        if not candidate.is_dir():
            continue

        resolved = candidate.resolve()
        if resolved in seen:
            continue

        seen.add(resolved)
        yield candidate


def has_pyjgl_module(engine_dir: Path) -> bool:
    for pattern in MODULE_PATTERNS:
        if next(engine_dir.glob(pattern), None) is not None:
            return True
    return False


def add_engine_path() -> Path:
    root = workspace_root()
    for engine_dir in engine_search_dirs(root):
        if has_pyjgl_module(engine_dir):
            sys.path.insert(0, str(engine_dir))
            return root

    raise RuntimeError("Python engine runtime not found. Build the project first.")


WORKSPACE = add_engine_path()

import pyjgl as jgl  # noqa: E402


def create_engine() -> "jgl.Engine":
    engine = jgl.Engine()
    engine.init()
    return engine


def create_scene(engine: "jgl.Engine", name: str):
    scene = engine.create_scene(name)
    engine.set_active_scene(scene)
    return scene
