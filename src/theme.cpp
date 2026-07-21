#include <imgui_md2/theme.h>
#include <imgui_md2/metrics.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ImGuiMD2 {
namespace {

constexpr std::size_t kSwatchCount = static_cast<std::size_t>(Swatch::Count);
constexpr std::size_t kShadeCount = 14;

constexpr std::uint32_t kPalette[kSwatchCount][kShadeCount] = {
    {0xffebee, 0xffcdd2, 0xef9a9a, 0xe57373, 0xef5350, 0xf44336, 0xe53935,
     0xd32f2f, 0xc62828, 0xb71c1c, 0xff8a80, 0xff5252, 0xff1744, 0xd50000},
    {0xfce4ec, 0xf8bbd0, 0xf48fb1, 0xf06292, 0xec407a, 0xe91e63, 0xd81b60,
     0xc2185b, 0xad1457, 0x880e4f, 0xff80ab, 0xff4081, 0xf50057, 0xc51162},
    {0xf3e5f5, 0xe1bee7, 0xce93d8, 0xba68c8, 0xab47bc, 0x9c27b0, 0x8e24aa,
     0x7b1fa2, 0x6a1b9a, 0x4a148c, 0xea80fc, 0xe040fb, 0xd500f9, 0xaa00ff},
    {0xede7f6, 0xd1c4e9, 0xb39ddb, 0x9575cd, 0x7e57c2, 0x673ab7, 0x5e35b1,
     0x512da8, 0x4527a0, 0x311b92, 0xb388ff, 0x7c4dff, 0x651fff, 0x6200ea},
    {0xe8eaf6, 0xc5cae9, 0x9fa8da, 0x7986cb, 0x5c6bc0, 0x3f51b5, 0x3949ab,
     0x303f9f, 0x283593, 0x1a237e, 0x8c9eff, 0x536dfe, 0x3d5afe, 0x304ffe},
    {0xe3f2fd, 0xbbdefb, 0x90caf9, 0x64b5f6, 0x42a5f5, 0x2196f3, 0x1e88e5,
     0x1976d2, 0x1565c0, 0x0d47a1, 0x82b1ff, 0x448aff, 0x2979ff, 0x2962ff},
    {0xe1f5fe, 0xb3e5fc, 0x81d4fa, 0x4fc3f7, 0x29b6f6, 0x03a9f4, 0x039be5,
     0x0288d1, 0x0277bd, 0x01579b, 0x80d8ff, 0x40c4ff, 0x00b0ff, 0x0091ea},
    {0xe0f7fa, 0xb2ebf2, 0x80deea, 0x4dd0e1, 0x26c6da, 0x00bcd4, 0x00acc1,
     0x0097a7, 0x00838f, 0x006064, 0x84ffff, 0x18ffff, 0x00e5ff, 0x00b8d4},
    {0xe0f2f1, 0xb2dfdb, 0x80cbc4, 0x4db6ac, 0x26a69a, 0x009688, 0x00897b,
     0x00796b, 0x00695c, 0x004d40, 0xa7ffeb, 0x64ffda, 0x1de9b6, 0x00bfa5},
    {0xe8f5e9, 0xc8e6c9, 0xa5d6a7, 0x81c784, 0x66bb6a, 0x4caf50, 0x43a047,
     0x388e3c, 0x2e7d32, 0x1b5e20, 0xb9f6ca, 0x69f0ae, 0x00e676, 0x00c853},
    {0xf1f8e9, 0xdcedc8, 0xc5e1a5, 0xaed581, 0x9ccc65, 0x8bc34a, 0x7cb342,
     0x689f38, 0x558b2f, 0x33691e, 0xccff90, 0xb2ff59, 0x76ff03, 0x64dd17},
    {0xf9fbe7, 0xf0f4c3, 0xe6ee9c, 0xdce775, 0xd4e157, 0xcddc39, 0xc0ca33,
     0xafb42b, 0x9e9d24, 0x827717, 0xf4ff81, 0xeeff41, 0xc6ff00, 0xaeea00},
    {0xfffde7, 0xfff9c4, 0xfff59d, 0xfff176, 0xffee58, 0xffeb3b, 0xfdd835,
     0xfbc02d, 0xf9a825, 0xf57f17, 0xffff8d, 0xffff00, 0xffea00, 0xffd600},
    {0xfff8e1, 0xffecb3, 0xffe082, 0xffd54f, 0xffca28, 0xffc107, 0xffb300,
     0xffa000, 0xff8f00, 0xff6f00, 0xffe57f, 0xffd740, 0xffc400, 0xffab00},
    {0xfff3e0, 0xffe0b2, 0xffcc80, 0xffb74d, 0xffa726, 0xff9800, 0xfb8c00,
     0xf57c00, 0xef6c00, 0xe65100, 0xffd180, 0xffab40, 0xff9100, 0xff6d00},
    {0xfbe9e7, 0xffccbc, 0xffab91, 0xff8a65, 0xff7043, 0xff5722, 0xf4511e,
     0xe64a19, 0xd84315, 0xbf360c, 0xff9e80, 0xff6e40, 0xff3d00, 0xdd2c00},
    {0xefebe9, 0xd7ccc8, 0xbcaaa4, 0xa1887f, 0x8d6e63, 0x795548, 0x6d4c41,
     0x5d4037, 0x4e342e, 0x3e2723, 0x795548, 0x795548, 0x795548, 0x795548},
    {0xfafafa, 0xf5f5f5, 0xeeeeee, 0xe0e0e0, 0xbdbdbd, 0x9e9e9e, 0x757575,
     0x616161, 0x424242, 0x212121, 0x9e9e9e, 0x9e9e9e, 0x9e9e9e, 0x9e9e9e},
    {0xeceff1, 0xcfd8dc, 0xb0bec5, 0x90a4ae, 0x78909c, 0x607d8b, 0x546e7a,
     0x455a64, 0x37474f, 0x263238, 0x607d8b, 0x607d8b, 0x607d8b, 0x607d8b},
};

constexpr TypeScale kTypography[] = {
    {96.0f, 96.0f, -1.50f, 300, false},
    {60.0f, 60.0f, -0.50f, 300, false},
    {48.0f, 50.0f, 0.00f, 400, false},
    {34.0f, 40.0f, 0.25f, 400, false},
    {24.0f, 32.0f, 0.00f, 400, false},
    {20.0f, 32.0f, 0.15f, 500, false},
    {16.0f, 28.0f, 0.15f, 400, false},
    {14.0f, 22.0f, 0.10f, 500, false},
    {16.0f, 24.0f, 0.50f, 400, false},
    {14.0f, 20.0f, 0.25f, 400, false},
    {14.0f, 36.0f, 1.25f, 500, true},
    {12.0f, 20.0f, 0.40f, 400, false},
    {10.0f, 32.0f, 1.50f, 400, true},
};

struct NamedDef {
    const char* id;
    bool dark;
    std::uint32_t primary;
    std::uint32_t primary_light;
    std::uint32_t secondary;
    std::uint32_t secondary_light;
    std::uint32_t secondary_dark;
    std::uint32_t primary_text;
    std::uint32_t secondary_text;
};

// Embedded equivalents of qt-material's resources/themes/*.xml. Keeping the
// table compiled in makes runtime theme switching independent of a working
// directory while retaining the upstream names.
constexpr NamedDef kNamed[] = {
    {"dark_amber", true, 0xffd740, 0xffff74, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_blue", true, 0x448aff, 0x83b9ff, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_cyan", true, 0x4dd0e1, 0x88ffff, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_lightgreen", true, 0x8bc34a, 0xbef67a, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_pink", true, 0xff4081, 0xff79b0, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_purple", true, 0xab47bc, 0xdf78ef, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_red", true, 0xff1744, 0xff616f, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_teal", true, 0x1de9b6, 0x6effe8, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"dark_yellow", true, 0xffff00, 0xffff5a, 0x232629, 0x4f5b62, 0x31363b, 0xffffff, 0xffffff},
    {"light_amber", false, 0xffc400, 0xfff64f, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_blue", false, 0x2979ff, 0x75a7ff, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_blue_500", false, 0x03a9f4, 0x67daff, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_cyan", false, 0x00e5ff, 0x6effff, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_cyan_500", false, 0x00bcd4, 0x62efff, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_lightgreen", false, 0x64dd17, 0x9cff57, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_lightgreen_500", false, 0x8bc34a, 0xbef67a, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_orange", false, 0xff3d00, 0xff7539, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_pink", false, 0xff4081, 0xff79b0, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_pink_500", false, 0xe91e63, 0xff6090, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_purple", false, 0xe040fb, 0xff79ff, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_purple_500", false, 0x9c27b0, 0xd05ce3, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_red", false, 0xff1744, 0xff616f, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_red_500", false, 0xf44336, 0xff7961, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_teal", false, 0x1de9b6, 0x6effe8, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_teal_500", false, 0x009688, 0x52c7b8, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
    {"light_yellow", false, 0xffea00, 0xffff56, 0xf5f5f5, 0xffffff, 0xe6e6e6, 0x3c3c3c, 0x555555},
};

constexpr int kNamedCount = static_cast<int>(sizeof(kNamed) / sizeof(kNamed[0]));
const char* kNamedIds[kNamedCount + 1] = {};

struct NamedIdsInit {
    NamedIdsInit() {
        for (int i = 0; i < kNamedCount; ++i) kNamedIds[i] = kNamed[i].id;
    }
} kNamedIdsInit;

float LinearChannel(float value) {
    return value <= 0.03928f ? value / 12.92f
                            : std::pow((value + 0.055f) / 1.055f, 2.4f);
}

Color SolidState(Color content, Color container, float opacity) {
    return Composite(content.WithAlpha(opacity), container);
}

} // namespace

ImU32 Color::U32(float alpha_multiplier) const {
    return ImGui::ColorConvertFloat4ToU32(
        ImVec4(r, g, b, std::clamp(a * alpha_multiplier, 0.0f, 1.0f)));
}

Color Mix(Color a, Color b, float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    return Color(a.r + (b.r - a.r) * amount, a.g + (b.g - a.g) * amount,
                 a.b + (b.b - a.b) * amount, a.a + (b.a - a.a) * amount);
}

Color Composite(Color foreground, Color background) {
    const float alpha = foreground.a + background.a * (1.0f - foreground.a);
    if (alpha <= 0.0f) return {};
    const float scale = (1.0f - foreground.a) * background.a;
    return Color((foreground.r * foreground.a + background.r * scale) / alpha,
                 (foreground.g * foreground.a + background.g * scale) / alpha,
                 (foreground.b * foreground.a + background.b * scale) / alpha,
                 alpha);
}

float RelativeLuminance(Color color) {
    return 0.2126f * LinearChannel(color.r) +
           0.7152f * LinearChannel(color.g) +
           0.0722f * LinearChannel(color.b);
}

float ContrastRatio(Color a, Color b) {
    const float la = RelativeLuminance(a) + 0.05f;
    const float lb = RelativeLuminance(b) + 0.05f;
    return std::max(la, lb) / std::min(la, lb);
}

Color AccessibleOnColor(Color background) {
    const Color dark = Color::FromHex(0x000000, 0.87f);
    const Color light = Color::FromHex(0xffffff);
    return ContrastRatio(background, light) >= ContrastRatio(background, dark)
               ? light
               : dark;
}

Color ParseHex(const char* hex) {
    if (!hex || !*hex) return Color::FromHex(0x000000);
    if (*hex == '#') ++hex;
    auto nibble = [](char value) -> unsigned {
        value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        if (value >= '0' && value <= '9') return static_cast<unsigned>(value - '0');
        if (value >= 'a' && value <= 'f') return static_cast<unsigned>(value - 'a' + 10);
        return 0;
    };
    if (std::strlen(hex) < 6) return Color::FromHex(0x000000);
    const std::uint32_t red = (nibble(hex[0]) << 4U) | nibble(hex[1]);
    const std::uint32_t green = (nibble(hex[2]) << 4U) | nibble(hex[3]);
    const std::uint32_t blue = (nibble(hex[4]) << 4U) | nibble(hex[5]);
    return Color::FromHex((red << 16U) | (green << 8U) | blue);
}

Color with_alpha(Color color, float alpha) { return color.WithAlpha(alpha); }
Color state_overlay(Color content, float opacity) { return content.WithAlpha(opacity); }
Color emphasize(Color on_surface, float emphasis) {
    return on_surface.WithAlpha(on_surface.a * emphasis);
}

Color Palette(Swatch swatch, Shade shade) {
    const auto family = std::min(static_cast<std::size_t>(swatch), kSwatchCount - 1);
    const auto tone = std::min(static_cast<std::size_t>(shade), kShadeCount - 1);
    return Color::FromHex(kPalette[family][tone]);
}

const TypeScale& Typography(TextStyle style) {
    const auto index = std::min(static_cast<std::size_t>(style),
                                static_cast<std::size_t>(TextStyle::Count) - 1);
    return kTypography[index];
}

namespace {

void FinishTheme(Theme& theme, bool dark) {
    theme.colors.dark = dark;
    theme.is_dark = dark;
    theme.colors.error = dark ? Palette(Swatch::Red, Shade::S200)
                              : Palette(Swatch::Red, Shade::S700);
    theme.colors.on_error = dark ? Color::FromHex(0x000000, Emphasis::high)
                                 : Color::FromHex(0xffffff);
    theme.colors.outline = dark ? Color::FromHex(0xffffff, 0.12f)
                                : Color::FromHex(0x000000, 0.12f);
    theme.disabled = dark ? Color::FromHex(0xffffff, Emphasis::disabled)
                          : Color::FromHex(0x000000, Emphasis::disabled);
    theme.snackbar = Color::FromHex(0x323232);
    theme.on_snackbar = Color::FromHex(0xffffff);
    if (theme.app_bar.a <= 0.0f) theme.app_bar = theme.colors.primary;
    if (theme.on_app_bar.a <= 0.0f) theme.on_app_bar = theme.colors.on_primary;
    if (theme.name == nullptr) theme.name = dark ? "dark" : "light";
}

Color AccentOrShade(Swatch swatch, Shade accent, Shade fallback) {
    // Brown/Grey/BlueGrey have no accents; Palette intentionally falls back
    // to S500 for those families.
    return Palette(swatch, accent).a > 0.0f ? Palette(swatch, accent)
                                           : Palette(swatch, fallback);
}

} // namespace

Theme Theme::Light() {
    Theme theme = LightFromPrimary(Swatch::Green, Swatch::Amber);
    theme.name = "light";
    return theme;
}

Theme Theme::Light(Color primary, Color secondary) {
    Theme theme;
    auto& c = theme.colors;
    c.primary = primary;
    c.primary_variant = Mix(primary, Color::FromHex(0x000000), 0.18f);
    theme.primary_light = Mix(primary, Color::FromHex(0xffffff), 0.35f);
    c.secondary = secondary;
    c.secondary_variant = Mix(secondary, Color::FromHex(0x000000), 0.16f);
    c.background = Palette(Swatch::Grey, Shade::S50);
    c.surface = Color::FromHex(0xffffff);
    c.surface_variant = Palette(Swatch::Grey, Shade::S100);
    c.on_primary = AccessibleOnColor(c.primary);
    c.on_secondary = AccessibleOnColor(c.secondary);
    c.on_background = Color::FromHex(0x000000, Emphasis::high);
    c.on_surface = Color::FromHex(0x000000, Emphasis::high);
    c.on_error = Color::FromHex(0xffffff);
    theme.app_bar = c.primary;
    theme.on_app_bar = c.on_primary;
    FinishTheme(theme, false);
    return theme;
}

Theme Theme::Dark() {
    Theme theme = DarkFromPrimary(Swatch::Green, Swatch::Amber);
    theme.name = "dark";
    return theme;
}

Theme Theme::Dark(Color primary, Color secondary) {
    Theme theme;
    auto& c = theme.colors;
    c.primary = primary;
    c.primary_variant = Mix(primary, Color::FromHex(0x000000), 0.55f);
    theme.primary_light = Mix(primary, Color::FromHex(0xffffff), 0.20f);
    c.secondary = secondary;
    c.secondary_variant = Mix(secondary, Color::FromHex(0xffffff), 0.08f);
    c.background = Color::FromHex(0x121212);
    c.surface = Color::FromHex(0x1e1e1e);
    c.surface_variant = Color::FromHex(0x2c2c2c);
    c.on_primary = AccessibleOnColor(c.primary);
    c.on_secondary = AccessibleOnColor(c.secondary);
    c.on_background = Color::FromHex(0xffffff);
    c.on_surface = Color::FromHex(0xffffff);
    c.on_error = Color::FromHex(0x000000, Emphasis::high);
    theme.app_bar = c.primary_variant;
    theme.on_app_bar = Color::FromHex(0xffffff);
    FinishTheme(theme, true);
    return theme;
}

Theme Theme::LightFromPrimary(Swatch primary, Swatch secondary) {
    Theme theme = Light(Palette(primary, Shade::S500),
                        AccentOrShade(secondary, Shade::A200, Shade::S200));
    theme.colors.primary_variant = Palette(primary, Shade::S700);
    theme.primary_light = Palette(primary, Shade::S200);
    theme.colors.secondary_variant = Palette(secondary, Shade::S700);
    theme.name = "light_from_primary";
    return theme;
}

Theme Theme::DarkFromPrimary(Swatch primary, Swatch secondary) {
    Theme theme = Dark(Palette(primary, Shade::S200),
                       AccentOrShade(secondary, Shade::A200, Shade::S200));
    theme.colors.primary_variant = Palette(primary, Shade::S700);
    theme.primary_light = Palette(primary, Shade::S100);
    theme.colors.secondary_variant = Palette(secondary, Shade::S200);
    theme.app_bar = Palette(primary, Shade::S700);
    theme.on_app_bar = Color::FromHex(0xffffff);
    theme.name = "dark_from_primary";
    return theme;
}

Theme Theme::BaselineLight() {
    Theme theme = LightFromPrimary(Swatch::DeepPurple, Swatch::Teal);
    theme.name = "baseline_light";
    return theme;
}

Theme Theme::BaselineDark() {
    Theme theme = DarkFromPrimary(Swatch::Purple, Swatch::Teal);
    theme.name = "baseline_dark";
    return theme;
}

Theme Theme::FromResource(const ThemeResource& resource, bool invert_secondary) {
    Theme theme;
    auto& c = theme.colors;
    theme.name = resource.name ? resource.name : "resource";
    c.primary = resource.primary;
    theme.primary_light = resource.primary_light;
    c.primary_variant = resource.dark ? resource.primary : resource.primary;
    c.secondary = resource.primary_light;
    c.secondary_variant = resource.primary;
    Color background = resource.secondary;
    Color surface = resource.secondary_dark;
    Color surface_variant = resource.secondary_light;
    if (!resource.dark) {
        // qt-material's secondary fields describe scaffold surfaces. For a
        // light theme, use the M2 Grey50/White/Grey100 surface hierarchy.
        background = Palette(Swatch::Grey, Shade::S50);
        surface = Color::FromHex(0xffffff);
        surface_variant = Palette(Swatch::Grey, Shade::S100);
        // The reference implementation accepts invert_secondary for
        // qt-material compatibility, but still normalizes light resources to
        // the M2 Grey50/White/Grey100 surface hierarchy.
        (void)invert_secondary;
    }
    c.background = background;
    c.surface = surface;
    c.surface_variant = surface_variant;
    c.on_primary = AccessibleOnColor(c.primary);
    c.on_secondary = resource.primary_text.a > 0.0f
                         ? resource.primary_text
                         : AccessibleOnColor(c.secondary);
    c.on_background = resource.secondary_text.a > 0.0f
                          ? resource.secondary_text
                          : Color::FromHex(resource.dark ? 0xffffff : 0x000000,
                                           Emphasis::high);
    c.on_surface = resource.primary_text.a > 0.0f
                       ? resource.primary_text
                       : Color::FromHex(resource.dark ? 0xffffff : 0x000000,
                                        Emphasis::high);
    theme.app_bar = c.primary;
    theme.on_app_bar = resource.primary_text.a > 0.0f
                           ? resource.primary_text
                           : c.on_primary;
    FinishTheme(theme, resource.dark);
    return theme;
}

Theme Theme::Named(const char* requested_name) {
    if (!requested_name || !*requested_name) return BaselineDark();
    char name[64]{};
    std::snprintf(name, sizeof(name), "%s", requested_name);
    const std::size_t length = std::strlen(name);
    if (length > 4 && std::strcmp(name + length - 4, ".xml") == 0)
        name[length - 4] = '\0';
    if (std::strcmp(name, "default") == 0 ||
        std::strcmp(name, "default_dark") == 0)
        return Named("dark_teal");
    if (std::strcmp(name, "default_light") == 0)
        return Named("light_cyan_500");
    for (const NamedDef& definition : kNamed) {
        if (std::strcmp(name, definition.id) != 0) continue;
        ThemeResource resource;
        resource.name = definition.id;
        resource.dark = definition.dark;
        resource.primary = Color::FromHex(definition.primary);
        resource.primary_light = Color::FromHex(definition.primary_light);
        resource.secondary = Color::FromHex(definition.secondary);
        resource.secondary_light = Color::FromHex(definition.secondary_light);
        resource.secondary_dark = Color::FromHex(definition.secondary_dark);
        resource.primary_text = Color::FromHex(definition.primary_text);
        resource.secondary_text = Color::FromHex(definition.secondary_text);
        return FromResource(resource, !definition.dark);
    }
    return BaselineDark();
}

const char* const* Theme::named_ids() { return kNamedIds; }
int Theme::named_count() { return kNamedCount; }

void ApplyTheme(const Theme& theme, ImGuiStyle* destination) {
    ImGuiStyle& style = destination ? *destination : ImGui::GetStyle();
    const auto& c = theme.colors;
    const float d = std::max(theme.layout.density, 0.5f) * DpiScale();
    const Color hover = SolidState(c.on_surface, c.surface,
                                   theme.states.hover_on_surface);
    const Color focus = SolidState(c.on_surface, c.surface, theme.states.focus);
    const Color primary_hover = SolidState(c.on_primary, c.primary, theme.states.hover);
    const Color primary_press = SolidState(c.on_primary, c.primary, theme.states.pressed);

    style.Colors[ImGuiCol_Text] = c.on_surface.Vec4();
    style.Colors[ImGuiCol_TextDisabled] = c.on_surface.WithAlpha(theme.states.disabled_content).Vec4();
    style.Colors[ImGuiCol_WindowBg] = c.background.Vec4();
    style.Colors[ImGuiCol_ChildBg] = c.surface.Vec4();
    style.Colors[ImGuiCol_PopupBg] = c.surface.Vec4();
    style.Colors[ImGuiCol_Border] = c.outline.Vec4();
    style.Colors[ImGuiCol_BorderShadow] = Color::FromHex(0x000000, 0.0f).Vec4();
    style.Colors[ImGuiCol_FrameBg] = c.surface_variant.Vec4();
    style.Colors[ImGuiCol_FrameBgHovered] = hover.Vec4();
    style.Colors[ImGuiCol_FrameBgActive] = focus.Vec4();
    style.Colors[ImGuiCol_TitleBg] = theme.app_bar.Vec4();
    style.Colors[ImGuiCol_TitleBgActive] = theme.app_bar.Vec4();
    style.Colors[ImGuiCol_TitleBgCollapsed] = theme.app_bar.Vec4();
    style.Colors[ImGuiCol_MenuBarBg] = c.surface.Vec4();
    style.Colors[ImGuiCol_ScrollbarBg] = Color(0, 0, 0, 0).Vec4();
    style.Colors[ImGuiCol_ScrollbarGrab] = c.on_surface.WithAlpha(0.20f).Vec4();
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = c.on_surface.WithAlpha(0.32f).Vec4();
    style.Colors[ImGuiCol_ScrollbarGrabActive] = c.on_surface.WithAlpha(0.44f).Vec4();
    style.Colors[ImGuiCol_CheckMark] = c.primary.Vec4();
    style.Colors[ImGuiCol_SliderGrab] = c.primary.Vec4();
    style.Colors[ImGuiCol_SliderGrabActive] = c.primary_variant.Vec4();
    style.Colors[ImGuiCol_Button] = c.primary.Vec4();
    style.Colors[ImGuiCol_ButtonHovered] = primary_hover.Vec4();
    style.Colors[ImGuiCol_ButtonActive] = primary_press.Vec4();
    style.Colors[ImGuiCol_Header] = c.primary.WithAlpha(theme.states.selected).Vec4();
    style.Colors[ImGuiCol_HeaderHovered] = hover.Vec4();
    style.Colors[ImGuiCol_HeaderActive] = focus.Vec4();
    style.Colors[ImGuiCol_Separator] = c.outline.Vec4();
    style.Colors[ImGuiCol_SeparatorHovered] = c.primary.WithAlpha(0.54f).Vec4();
    style.Colors[ImGuiCol_SeparatorActive] = c.primary.Vec4();
    style.Colors[ImGuiCol_ResizeGrip] = c.primary.WithAlpha(0.12f).Vec4();
    style.Colors[ImGuiCol_ResizeGripHovered] = c.primary.WithAlpha(0.38f).Vec4();
    style.Colors[ImGuiCol_ResizeGripActive] = c.primary.WithAlpha(0.54f).Vec4();
    style.Colors[ImGuiCol_Tab] = c.surface.Vec4();
    style.Colors[ImGuiCol_TabHovered] = hover.Vec4();
    style.Colors[ImGuiCol_TabSelected] = c.primary.WithAlpha(0.12f).Vec4();
    style.Colors[ImGuiCol_TabDimmed] = c.surface_variant.Vec4();
    style.Colors[ImGuiCol_TabDimmedSelected] = c.primary.WithAlpha(0.08f).Vec4();
    style.Colors[ImGuiCol_PlotLines] = c.primary.Vec4();
    style.Colors[ImGuiCol_PlotLinesHovered] = c.secondary.Vec4();
    style.Colors[ImGuiCol_PlotHistogram] = c.secondary.Vec4();
    style.Colors[ImGuiCol_PlotHistogramHovered] = c.secondary_variant.Vec4();
    style.Colors[ImGuiCol_TableHeaderBg] = c.surface_variant.Vec4();
    style.Colors[ImGuiCol_TableBorderStrong] = c.outline.Vec4();
    style.Colors[ImGuiCol_TableBorderLight] = c.outline.WithAlpha(0.08f).Vec4();
    style.Colors[ImGuiCol_TableRowBg] = Color(0, 0, 0, 0).Vec4();
    style.Colors[ImGuiCol_TableRowBgAlt] = c.on_surface.WithAlpha(0.025f).Vec4();
    style.Colors[ImGuiCol_TextLink] = c.primary.Vec4();
    style.Colors[ImGuiCol_TextSelectedBg] = c.primary.WithAlpha(0.24f).Vec4();
    style.Colors[ImGuiCol_DragDropTarget] = c.secondary.Vec4();
    style.Colors[ImGuiCol_NavCursor] = c.primary.Vec4();
    style.Colors[ImGuiCol_NavWindowingHighlight] = c.on_surface.WithAlpha(0.70f).Vec4();
    style.Colors[ImGuiCol_NavWindowingDimBg] = c.scrim.Vec4();
    style.Colors[ImGuiCol_ModalWindowDimBg] = c.scrim.Vec4();

    style.WindowPadding = ImVec2(16.0f * d, 16.0f * d);
    style.FramePadding = ImVec2(12.0f * d, 8.0f * d);
    style.CellPadding = ImVec2(16.0f * d, 8.0f * d);
    style.ItemSpacing = ImVec2(8.0f * d, 8.0f * d);
    style.ItemInnerSpacing = ImVec2(8.0f * d, 4.0f * d);
    style.IndentSpacing = 24.0f * d;
    style.ScrollbarSize = 12.0f * d;
    style.GrabMinSize = 20.0f * d;
    style.WindowRounding = theme.shapes.medium;
    style.ChildRounding = theme.shapes.medium;
    style.FrameRounding = theme.shapes.small;
    style.PopupRounding = theme.shapes.medium;
    style.ScrollbarRounding = theme.shapes.full;
    style.GrabRounding = theme.shapes.full;
    style.TabRounding = theme.shapes.small;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
}

ThemeScope::ThemeScope(const Theme& theme) : backup_(ImGui::GetStyle()) {
    ApplyTheme(theme);
}

ThemeScope::~ThemeScope() { ImGui::GetStyle() = backup_; }

} // namespace ImGuiMD2
