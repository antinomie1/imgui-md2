#include <imgui_md2/metrics.h>

#include <algorithm>

namespace ImGuiMD2 {
namespace {
float g_dpi_scale = 1.0f;
int g_density_scale = 0;
float Densify(float base) {
    return std::max(base + static_cast<float>(g_density_scale * 4), 4.0f);
}
}

void SetDpiScale(float scale) { g_dpi_scale = std::max(scale, 0.25f); }
float DpiScale() { return g_dpi_scale; }
void SetDensityScale(int scale) { g_density_scale = std::clamp(scale, -3, 3); }
int DensityScale() { return g_density_scale; }
float Dp(float grid_units) { return grid_units * 8.0f * g_dpi_scale; }
float DensityDp(float base_dp) { return Densify(base_dp) * g_dpi_scale; }
float Sp(float size_sp) { return size_sp * g_dpi_scale; }

float Metrics::AppBarHeight() { return DensityDp(56); }
float Metrics::BottomNavigationHeight() { return DensityDp(56); }
float Metrics::ButtonMinHeight() { return DensityDp(36); }
float Metrics::ButtonMinWidth() { return DensityDp(64); }
float Metrics::ButtonHorizontalPadding() { return 16 * g_dpi_scale; }
float Metrics::FabSize() { return DensityDp(56); }
float Metrics::FabMiniSize() { return DensityDp(40); }
float Metrics::FabExtendedHeight() { return DensityDp(48); }
float Metrics::ChipHeight() { return DensityDp(32); }
float Metrics::TextFieldHeight() { return DensityDp(56); }
float Metrics::ListRowHeight() { return DensityDp(48); }
float Metrics::ListRowHeightAvatar() { return DensityDp(56); }
float Metrics::ListRowHeightTwoLine() { return DensityDp(72); }
float Metrics::ListRowHeightThreeLine() { return DensityDp(88); }
float Metrics::IconSize() { return 24 * g_dpi_scale; }
float Metrics::IconButtonSize() { return DensityDp(48); }
float Metrics::TouchTarget() { return DensityDp(48); }
float Metrics::DialogMinWidth() { return 280 * g_dpi_scale; }
float Metrics::SnackbarHeight() { return DensityDp(48); }
float Metrics::ProgressHeight() { return 4 * g_dpi_scale; }
float Metrics::SwitchTrackWidth() { return DensityDp(36); }
float Metrics::SwitchTrackHeight() { return DensityDp(14); }
float Metrics::SwitchThumbSize() { return DensityDp(20); }
float Metrics::TabHeight() { return DensityDp(48); }
float Metrics::CheckboxSize() { return 18 * g_dpi_scale; }
float Metrics::ButtonCornerRadius() { return 4 * g_dpi_scale; }
float Metrics::CardCornerRadius() { return 4 * g_dpi_scale; }
float Metrics::ChipCornerRadius() { return 16 * g_dpi_scale; }
float Metrics::FabCornerRadius() { return FabSize() * 0.5f; }
float Metrics::SnackbarCornerRadius() { return 4 * g_dpi_scale; }
float Metrics::ScreenPadding() { return 16 * g_dpi_scale; }
float Metrics::CardPadding() { return 16 * g_dpi_scale; }
float Metrics::Gap() { return 8 * g_dpi_scale; }
float Metrics::DenseGap() { return 4 * g_dpi_scale; }
float Metrics::DividerThickness() { return g_dpi_scale; }

} // namespace ImGuiMD2
