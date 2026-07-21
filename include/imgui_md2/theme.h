#pragma once

#include <imgui.h>

#include <array>
#include <cstdint>

namespace ImGuiMD2 {

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr Color() = default;
    constexpr Color(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}

    static constexpr Color FromHex(std::uint32_t rgb, float alpha = 1.0f) {
        return Color(static_cast<float>((rgb >> 16U) & 0xffU) / 255.0f,
                     static_cast<float>((rgb >> 8U) & 0xffU) / 255.0f,
                     static_cast<float>(rgb & 0xffU) / 255.0f, alpha);
    }

    constexpr Color WithAlpha(float alpha) const { return Color(r, g, b, alpha); }
    constexpr ImVec4 Vec4() const { return ImVec4(r, g, b, a); }
    ImU32 U32(float alpha_multiplier = 1.0f) const;
};

Color Mix(Color a, Color b, float amount);
Color Composite(Color foreground, Color background);
float RelativeLuminance(Color color);
float ContrastRatio(Color a, Color b);
Color AccessibleOnColor(Color background);
Color ParseHex(const char* hex);

// Compatibility helpers matching the md2/pluma theme vocabulary.
Color with_alpha(Color color, float alpha);
Color state_overlay(Color content, float opacity);
Color emphasize(Color on_surface, float emphasis);
Color clear_color();

enum class Swatch : std::uint8_t {
    Red,
    Pink,
    Purple,
    DeepPurple,
    Indigo,
    Blue,
    LightBlue,
    Cyan,
    Teal,
    Green,
    LightGreen,
    Lime,
    Yellow,
    Amber,
    Orange,
    DeepOrange,
    Brown,
    Grey,
    BlueGrey,
    Count
};

enum class Shade : std::uint8_t {
    S50,
    S100,
    S200,
    S300,
    S400,
    S500,
    S600,
    S700,
    S800,
    S900,
    A100,
    A200,
    A400,
    A700
};

// Returns the canonical Material Design 2 palette color. Brown, Grey and
// BlueGrey do not define accent shades; their accent requests resolve to S500.
Color Palette(Swatch swatch, Shade shade = Shade::S500);

struct ColorScheme {
    Color primary;
    Color primary_variant;
    Color secondary;
    Color secondary_variant;
    Color background;
    Color surface;
    Color surface_variant;
    Color error;
    Color on_primary;
    Color on_secondary;
    Color on_background;
    Color on_surface;
    Color on_error;
    Color outline;
    Color scrim;
    bool dark = false;
};

struct StateLayers {
    static constexpr float hover = 0.04f;
    static constexpr float hover_on_surface = 0.08f;
    static constexpr float focus = 0.12f;
    static constexpr float pressed = 0.12f;
    static constexpr float dragged = 0.16f;
    static constexpr float selected = 0.12f;
    static constexpr float selected_strong = 0.16f;
    static constexpr float activated = 0.24f;
    static constexpr float disabled_bg = 0.12f;
};

struct Emphasis {
    static constexpr float high = 0.87f;
    static constexpr float medium = 0.60f;
    static constexpr float disabled = 0.38f;
};

enum class TextStyle : std::uint8_t {
    Headline1,
    Headline2,
    Headline3,
    Headline4,
    Headline5,
    Headline6,
    Subtitle1,
    Subtitle2,
    Body1,
    Body2,
    Button,
    Caption,
    Overline,
    Count
};

struct TypeScale {
    float size;
    float line_height;
    float letter_spacing;
    int weight;
    bool uppercase;
};

const TypeScale& Typography(TextStyle style);

struct TypographyFonts {
    std::array<ImFont*, static_cast<std::size_t>(TextStyle::Count)> styles{};
    ImFont* icons = nullptr;

    ImFont* Get(TextStyle style) const {
        return styles[static_cast<std::size_t>(style)];
    }
};

struct ShapeScale {
    float small = 4.0f;
    float medium = 4.0f;
    float large = 4.0f;
    float full = 999.0f;
};

struct StateOpacity {
    float hover = 0.04f;
    float hover_on_surface = 0.08f;
    float focus = 0.12f;
    float pressed = 0.12f;
    float dragged = 0.16f;
    float selected = 0.12f;
    float selected_strong = 0.16f;
    float activated = 0.24f;
    float disabled_content = 0.38f;
    float disabled_container = 0.12f;
};

struct MotionScale {
    float short_duration = 0.075f;
    float medium_duration = 0.150f;
    float long_duration = 0.225f;
    float complex_duration = 0.300f;
    float ripple_fade_in = 0.075f;
    float ripple_fade_out = 0.150f;
    float ripple_expand = 0.225f;
};

struct LayoutScale {
    float density = 1.0f;
    float touch_target = 48.0f;
    float button_height = 36.0f;
    float text_field_height = 56.0f;
    float app_bar_height = 56.0f;
    float grid = 8.0f;
};

struct ThemeResource {
    Color primary = Color(0, 0, 0, 0);
    Color primary_light = Color(0, 0, 0, 0);
    Color secondary = Color(0, 0, 0, 0);
    Color secondary_light = Color(0, 0, 0, 0);
    Color secondary_dark = Color(0, 0, 0, 0);
    Color primary_text = Color(0, 0, 0, 0);
    Color secondary_text = Color(0, 0, 0, 0);
    bool dark = false;
    const char* name = "resource";
};

struct Theme {
    ColorScheme colors;
    ShapeScale shapes;
    StateOpacity states;
    MotionScale motion;
    LayoutScale layout;
    TypographyFonts fonts;
    Color primary_light;
    Color disabled;
    Color app_bar = Color(0, 0, 0, 0);
    Color on_app_bar = Color(0, 0, 0, 0);
    Color snackbar = Color(0, 0, 0, 0);
    Color on_snackbar = Color(0, 0, 0, 0);
    bool is_dark = false;
    const char* name = "custom";

    static Theme Light();
    static Theme Light(Color primary, Color secondary);
    static Theme Dark();
    static Theme Dark(Color primary, Color secondary);
    static Theme LightFromPrimary(Swatch primary, Swatch secondary = Swatch::Amber);
    static Theme DarkFromPrimary(Swatch primary, Swatch secondary = Swatch::Teal);
    static Theme BaselineLight();
    static Theme BaselineDark();
    static Theme FromResource(const ThemeResource& resource,
                              bool invert_secondary = false);
    static Theme Named(const char* name);
    static const char* const* named_ids();
    static int named_count();
};

// Applies the current MD2 theme to ImGui's global style. Call after creating
// the ImGui context, or use ThemeScope for a temporary application.
void ApplyTheme(const Theme& theme, ImGuiStyle* destination = nullptr);

class ThemeScope {
public:
    explicit ThemeScope(const Theme& theme);
    ~ThemeScope();
    ThemeScope(const ThemeScope&) = delete;
    ThemeScope& operator=(const ThemeScope&) = delete;

private:
    ImGuiStyle backup_{};
};

} // namespace ImGuiMD2
