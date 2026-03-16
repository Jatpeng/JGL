# API Docs

This repository ships a source-driven markdown API generator.

Commands:

```powershell
python tools/generate_api_docs.py
cmake --build build --target api_docs
```

Generated files:

- `docs/api/index.md`
- `docs/api/cpp_runtime_api.md`
- `docs/api/python_api.md`
