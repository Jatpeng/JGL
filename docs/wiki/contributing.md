# Wiki 使用与维护

`docs/wiki/index.md` 是自动生成的 Wiki 首页，用来把 `docs/` 下的文档组织成统一入口。

## 目标

- 给现有 Markdown 文档提供一个按分类浏览的知识库首页
- 自动汇总标题、摘要和最近更新时间，减少手工维护目录
- 保持 `docs/index.md` 作为总入口，`docs/wiki/index.md` 作为知识库入口

## 新增文档时怎么接入 Wiki

1. 把新文档放到 `docs/` 下的合适目录，例如 `docs/engine/`、`docs/sections/` 或 `docs/api/`
2. 在文档首行使用一级标题，例如 `# 渲染管线`
3. 在标题后尽快给出一段简短说明，Wiki 会把第一段正文提取为摘要
4. 运行 `python tools/generate_wiki_docs.py` 更新 Wiki 首页

## 摘要提取规则

- 取正文中的第一段普通段落作为摘要
- 列表、表格、引用块和代码块不会作为摘要
- 如果没有可提取的普通段落，Wiki 会显示“暂无摘要”

## 推荐命令

```powershell
python tools/generate_wiki_docs.py
cmake --build build --target wiki_docs
cmake --build build --target docs_all
```

## 维护建议

- `docs/index.md` 放稳定入口和人工整理后的推荐阅读
- `docs/wiki/index.md` 放自动汇总结果
- 修改文档标题或首段后，重新生成 Wiki，避免目录摘要过期
