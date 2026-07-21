# 设计与规范映射

## 目标

本实现把 Material Design 2 的设计变量抽成平台无关的 C++ token，再用 Dear
ImGui 的 draw list 与 item behavior 构造控件。调用方仍按即时模式提交 UI，库内
context 只保存不可避免的跨帧视觉状态（动画轨道、ripple、Snackbar 计时器）。

## 规范基线

实现以以下资料为基线：

- [M2 Design](https://m2.material.io/design)：颜色、排版、状态、motion、组件；
- [M2 Resources](https://m2.material.io/resources)：Roboto 与 Material Icons；
- [Material Components Web v14.0.0](https://github.com/material-components/material-components-web/tree/v14.0.0)：
  M2 Web 组件的最终主版本；
- [qt-material-widgets](https://github.com/laserpants/qt-material-widgets)：桌面 widget
  的状态机、ripple、调色板和组件拆分；
- [qt-material](https://github.com/dunderlab/qt-material)：桌面场景下亮/暗主题与资源组织。

参考版本固定为 MDC Web `v14.0.0`，避免 Material 3 token 混入 M2 命名。

## Token 映射

| M2 概念 | 本库 | 默认值 |
|---|---|---|
| primary / secondary | `ColorScheme` | Green 500 / Amber A200（默认） |
| background / surface | `ColorScheme` | Grey 50 / White（亮色）；`#121212` / `#1E1E1E`（暗色） |
| error | `ColorScheme::error` | `#B00020` |
| 高/中/禁用强调 | alpha | 0.87 / 0.60(旧文本为 0.54) / 0.38 |
| hover / focus / pressed | `StateOpacity` | 0.04 / 0.12 / 0.12 |
| hover on surface / dragged | `StateOpacity` | 0.08 / 0.16 |
| selected / activated | `StateOpacity` | 0.12 / 0.24 |
| standard curve | `Easing::Standard` | `(0.4, 0, 0.2, 1)` |
| enter curve | `Easing::Deceleration` | `(0, 0, 0.2, 1)` |
| permanent exit | `Easing::Acceleration` | `(0.4, 0, 1, 1)` |
| temporary exit | `Easing::Sharp` | `(0.4, 0, 0.6, 1)` |
| ripple | `MotionScale` | 75 / 150 / 225ms |
| grid / touch target | `LayoutScale` | 8 / 48px |

`Theme::Named()` 内置了 `dunderlab/qt-material` 的 26 个命名资源（dark/light
色系），并把 XML 的 `primaryColor`、`primaryLightColor`、`secondaryColor`、
`primaryTextColor` 等字段转换为 M2 `ColorScheme`。`Metrics` 采用 pluma/md2 的
密度公式：`DensityDp(base) = max(base + 4 * density, 4) * dpi`，其中 density
限制在 [-3, +3]；8dp 栅格通过 `Dp()`，文字通过 `Sp()` 缩放。

亮色文本使用 M2 的 87%/54%/38% 规则；新的语义辅助文本使用 60% emphasis。
暗色主题的高强调文字为纯白，低强调通过 alpha 表达。

## 暗色 elevation

M2 暗色表面不依赖黑色阴影表达层级，而是在 `#121212` surface 上叠加白色
overlay。本库的 `SurfaceForElevation` 使用 1..24dp 对应的 5%..22% overlay，
Card、Drawer、Dialog 自动采用。亮色主题使用多层矩形近似环境阴影与关键光阴影。

## 即时模式适配

DOM/Qt 组件可以持有对象状态，ImGui 组件每帧重新调用。因此本库采用：

1. ImGui ID 作为状态键；
2. 每个 ImGui context 一份 MD2 context；
3. target 改变时从当前插值值重新定向，避免动画跳变；
4. 600 frame 未触及的轨道与完成退场的 ripple 自动删除；
5. 控件在内部幂等调用 `NewFrame()`，减少接入步骤。

这让切换主题、条件显示和虚拟列表仍符合 ImGui 习惯。调用方必须遵守 ImGui ID
唯一性规则。

## 已知的渲染边界

- Dear ImGui 默认不做复杂文本 shaping。Material Icons 使用码点而不是 ligature；
- `TypeScale::letter_spacing` 完整保留给自定义文本排版，`Text()` 使用 ImGui 的
  基础字形布局，不对 UTF-8 文本逐字插入 tracking；
- elevation shadow 是无 shader 近似。如应用已有高斯模糊/九宫格阴影 renderer，
  可替换 `ElevationShadow`；
- keyboard/gamepad 导航沿用 ImGui `ButtonBehavior` 和 popup 导航；
- M2 的响应式 breakpoint 由宿主窗口系统决定，本库只提供可变宽组件与 drawer。

## 资源与许可证

Roboto 和 Material Icons 均按 Apache-2.0 单独授权，许可证和字体一起存放在
`assets/fonts`。资源脚本输出 SHA-256，便于供应链固定和离线打包。库源码使用
BSD-2-Clause 许可证。
