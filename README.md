# imgui-md2

`imgui-md2` 是一套面向 Dear ImGui 的 Material Design 2（M2）实现。它保留
ImGui 的即时模式调用方式，同时提供独立的颜色/排版/形状/密度/状态/动效令牌、
逐控件动画状态和常用 M2 组件。

本项目明确实现 **Material Design 2**，不是 Material You / Material 3。

![19 个色族 × 14 个色阶的完整 M2 调色板](docs/screenshots/palette.png)

每个控件的独立截图见 [中文 API 文档](docs/API.zh-CN.md)；全部截图由
`tools/screenshot`（`xmake build imgui-md2-screenshots` 后运行）自动生成。

## 能力

- 完整 M2 调色板：19 个色族、`50..900` 与 `A100..A700` 色阶；
- 亮/暗语义主题：primary、secondary、surface、background、error 和全部
  `on-*` 色，支持 WCAG 对比度计算与自定义品牌色；
- 参考 `pluma/md2` 与 `qt-material` 的 26 个命名主题（支持 `.xml` 后缀和
  `default` 别名），并提供 `ThemeResource` 接口导入同类资源；
- 主题令牌：13 级 Roboto 排版、形状、8dp 栅格、密度、48dp 触控目标、
  状态层、强调级别、暗色 elevation overlay，以及可调的 DP/SP 指标；
- 动画：M2 standard / deceleration / acceleration / sharp 贝塞尔曲线、
  可重定向动画轨道、自动状态回收、75/150/225ms ripple；
- 组件：按钮、图标按钮、FAB、Checkbox、Radio、Switch、Slider、TextField、
  Select、线性/环形进度、Card、List、Divider、Chip、Tabs、Badge、Avatar、
  Top App Bar、Navigation Drawer、Dialog、Snackbar、Tooltip；
- 资源：默认将 Roboto Regular + Material Icons（约 520KB）编译进库，
  可选完整 Light/Regular/Medium 字体，另有外部字体回退和可重复执行的下载脚本；
- 统一使用 Dear ImGui v1.92.8、C++17 及以上版本。

## 快速集成

父项目已经使用 xmake 时：

```lua
includes("imgui_md2")

target("my-app")
    add_deps("imgui-md2")
```

独立构建：

```powershell
xmake f -P . -p mingw -a x86_64 -m release -y
xmake -P .
```

初始化必须发生在创建 ImGui context 之后、后端上传字体纹理之前：

```cpp
#include <imgui_md2/imgui_md2.h>

ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();

ImGuiMD2::Theme theme = ImGuiMD2::Theme::Light(
    ImGuiMD2::Palette(ImGuiMD2::Swatch::Indigo, ImGuiMD2::Shade::S500),
    ImGuiMD2::Palette(ImGuiMD2::Swatch::Pink, ImGuiMD2::Shade::A200));

std::string font_error;
if (!ImGuiMD2::LoadBundledFonts(*io.Fonts, theme.fonts, "assets", {}, &font_error)) {
    // 记录 font_error；仍可使用后端已经配置的 ImGui 默认字体。
}
ImGuiMD2::SetTheme(theme);

// 也可以直接使用内置的 qt-material 风格命名主题：
ImGuiMD2::ApplyNamedTheme("light_blue");
```

使用 ImGui 1.92+ 的 GLFW/OpenGL3 等新版渲染后端时，不要手动调用
`io.Fonts->Build()`；后端设置 `ImGuiBackendFlags_RendererHasTextures` 后，
`NewFrame()` 会自动构建并上传字体 atlas。

每帧直接调用组件；组件会自动推进 MD2 上下文，无需手工维护动画：

```cpp
static bool enabled = true;
static char query[128] = {};

ImGuiMD2::Switch("Enable notifications", &enabled);

ImGuiMD2::TextFieldOptions field;
field.variant = ImGuiMD2::TextFieldVariant::Outlined;
field.leading_icon = ImGuiMD2::Icons::Search;
field.helper_text = "Search by title";
ImGuiMD2::TextField("Search", query, sizeof(query), field);

if (ImGuiMD2::ContainedButton("SAVE")) {
    // handle action
}
```

销毁时先释放与当前 ImGui context 绑定的 MD2 状态：

```cpp
ImGuiMD2::DestroyContext();
ImGui::DestroyContext();
```

接口说明见 [中文 API 文档](docs/API.zh-CN.md)，令牌来源和即时模式适配说明见
[设计文档](docs/DESIGN.zh-CN.md)。

## 资源刷新

仓库已包含运行时字体。需要重新从上游拉取时：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/fetch_assets.ps1 -Force
```

默认构建会将 Regular Roboto 和 Material Icons 编译进库；刷新 TTF 后同步
更新内置数组：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/generate_embedded_fonts.ps1
```

如需同时内置 Light/Medium：

```powershell
xmake f --imgui_md2_embed_full_fonts=true
```

字体许可证位于 `assets/fonts`，项目自身采用 BSD-2-Clause 许可证。

## 规范与参考实现

- [Dear ImGui v1.92.8](https://github.com/ocornut/imgui/releases/tag/v1.92.8)
- [Material Design 2](https://m2.material.io/design)
- [Material Components Web v14.0.0](https://github.com/material-components/material-components-web/tree/v14.0.0)
- [qt-material-widgets](https://github.com/laserpants/qt-material-widgets)
- [qt-material](https://github.com/dunderlab/qt-material)

`material-components-web` 的源码只作为令牌和行为参考，本库没有链接或复制其
运行时代码。
