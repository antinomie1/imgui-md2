#pragma once

#include <cstddef>

namespace ImGuiMD2 {

// Host-controlled logical scale. 1.0 is 96 DPI. These values are independent
// from ImGui's global font scale and are intended for layout calculations.
void SetDpiScale(float scale);
float DpiScale();
void SetDensityScale(int scale); // clamped to [-3, +3], 4dp per step
int DensityScale();
float Dp(float grid_units);       // grid_units * 8dp * dpi
float DensityDp(float base_dp);   // (base_dp + 4*density) * dpi
float Sp(float size_sp);          // size_sp * dpi

struct Metrics {
    static float AppBarHeight();
    static float BottomNavigationHeight();
    static float ButtonMinHeight();
    static float ButtonMinWidth();
    static float ButtonHorizontalPadding();
    static float FabSize();
    static float FabMiniSize();
    static float FabExtendedHeight();
    static float ChipHeight();
    static float TextFieldHeight();
    static float ListRowHeight();
    static float ListRowHeightAvatar();
    static float ListRowHeightTwoLine();
    static float ListRowHeightThreeLine();
    static float IconSize();
    static float IconButtonSize();
    static float TouchTarget();
    static float DialogMinWidth();
    static float SnackbarHeight();
    static float ProgressHeight();
    static float SwitchTrackWidth();
    static float SwitchTrackHeight();
    static float SwitchThumbSize();
    static float TabHeight();
    static float CheckboxSize();
    static float ButtonCornerRadius();
    static float CardCornerRadius();
    static float ChipCornerRadius();
    static float FabCornerRadius();
    static float SnackbarCornerRadius();
    static float ScreenPadding();
    static float CardPadding();
    static float Gap();
    static float DenseGap();
    static float DividerThickness();
};

} // namespace ImGuiMD2
