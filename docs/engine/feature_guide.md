# 功能说明

本文档集中描述当前仓库已经接入、并能在运行时或编辑器中直接控制的主要功能。

## 方向光阴影

方向光支持阴影映射，并已接入 Forward / Deferred 两条主渲染路径。

可调参数：

- `Cast Shadows`
- `Shadow Bias Min`
- `Shadow Bias Max`
- `Shadow Filter Radius`

编辑器入口：

- `Inspector -> Light`

## Shader 热重载

运行时支持 shader 热重载。

行为约定：

- 新 program 只有在编译和链接成功后才会替换旧 program
- 编译失败时会保留当前正在使用的 program
- 支持 mesh shader、内置 shader、post-process shader
- 重新加载时会重建当前环境图对应的 IBL 资源

入口：

- `Inspector -> Mesh Assets -> Reload Shader`
- `Render Settings -> Render -> Reload All Shaders`
- `Scene` 工具栏
- `F5`

## HDR 环境图与 IBL

环境光照支持从 `.hdr` 文件生成 IBL 资源。

当前默认导入的环境资源位于：

- `Assets/environments/newport_loft/Newport_Loft_Env.hdr`

编辑器入口：

- `Render Settings -> Environment`

可用操作：

- 从仓库内 `.hdr` 资源中切换
- 通过文件对话框加载外部 `.hdr`
- 回退到内置 skybox

## 默认展示场景

默认场景已经从硬编码创建改成资源驱动。

场景资源文件：

- `Assets/scenes/default_scene.xml`

当前这份场景会默认加载：

- `Assets/models/showcase/bunny.obj`
- `Assets/models/showcase/teapot.obj`
- `Assets/environments/newport_loft/Newport_Loft_Env.hdr`

场景资源支持的主要字段：

- `Scene`: `Name`、`ShowPlane`、`SkyboxEnabled`、`EnvironmentMap`
- `Mesh`: `Name`、`Model`、`Material`、`Shader`、`Position`、`Rotation`、`Scale`、`Color`
- `Light`: `Name`、`Type`、`Position`、`Direction`、`Color`、`Strength`、`CastShadows`

如果默认场景资源加载失败，引擎会回退到内置的简单场景，保证编辑器仍能启动。

## 编辑器 Panel 拆分

编辑器已经拆分为多个独立 panel，便于后续继续扩展。

当前 panel：

- `Scene Hierarchy`
- `Inspector`
- `Render Settings`
- `Scene`

这种拆分方式的目的：

- 降低单文件复杂度
- 把场景结构、对象属性和全局渲染设置分开维护
- 让后续新增 panel 时改动范围更小

## 脚本入口

Game 工作区默认脚本入口已切换到展示场景：

- `Game/script/showcase_scene.py`
- `Game/run_script.bat`
- `Game/run_script.sh`

该脚本不再手动创建场景，而是直接运行引擎默认加载的资源化场景。
