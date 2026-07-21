#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <imgui_md2/components.h>
#include <imgui_md2/metrics.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ImGuiMD2 {

struct Context {
    struct Ripple {
        ImVec2 center{};
        float age = 0.0f;
        float release_age = 0.0f;
        bool down = false;
        int touched_frame = 0;
    };

    Theme theme = Theme::Light();
    Animator animator;
    std::unordered_map<ImGuiID, Ripple> ripples;
    std::unordered_map<ImGuiID, double> timers;
    int last_frame = -1;
};

namespace {

std::unordered_map<ImGuiContext*, std::unique_ptr<Context>> g_contexts;

ImGuiID Salt(ImGuiID id, const char* suffix) {
    return ImHashStr(suffix, 0, id);
}

std::string VisibleLabel(const char* label) {
    const char* end = ImGui::FindRenderedTextEnd(label);
    return std::string(label, end);
}

std::string Uppercase(std::string text) {
    for (char& value : text) {
        const unsigned char c = static_cast<unsigned char>(value);
        if (c < 128) value = static_cast<char>(std::toupper(c));
    }
    return text;
}

Color StateColor(Color content, Color container, float opacity) {
    return Composite(content.WithAlpha(opacity), container);
}

float RadiusToCorners(ImVec2 center, const ImRect& rect) {
    const float x = std::max(std::abs(center.x - rect.Min.x),
                             std::abs(center.x - rect.Max.x));
    const float y = std::max(std::abs(center.y - rect.Min.y),
                             std::abs(center.y - rect.Max.y));
    return std::sqrt(x * x + y * y);
}

void DrawRipple(ImGuiID id, const ImRect& rect, float rounding, Color ink,
                bool hovered, bool held, bool enabled,
                ImDrawFlags corners = ImDrawFlags_RoundCornersAll) {
    Context& context = GetContext();
    const Theme& theme = context.theme;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (!enabled) return;

    if (hovered && !held) {
        draw->AddRectFilled(rect.Min, rect.Max,
                            ink.U32(theme.states.hover), rounding, corners);
    }

    Context::Ripple* ripple = nullptr;
    auto found = context.ripples.find(id);
    if (held && (found == context.ripples.end() || !found->second.down)) {
        auto& created = context.ripples[id];
        created.center = ImGui::IsMouseHoveringRect(rect.Min, rect.Max)
                             ? ImGui::GetIO().MousePos
                             : rect.GetCenter();
        created.age = 0.0f;
        created.release_age = 0.0f;
        created.down = true;
        created.touched_frame = ImGui::GetFrameCount();
        ripple = &created;
    } else if (found != context.ripples.end()) {
        ripple = &found->second;
    }
    if (!ripple) return;

    ripple->touched_frame = ImGui::GetFrameCount();
    if (!held && ripple->down) {
        ripple->down = false;
        ripple->release_age = 0.0f;
    }
    const float expand = Ease(Easing::Deceleration,
                              std::min(ripple->age / theme.motion.ripple_expand, 1.0f));
    float opacity = theme.states.pressed;
    if (!ripple->down) {
        opacity *= 1.0f -
                   std::min(ripple->release_age / theme.motion.ripple_fade_out, 1.0f);
    } else {
        opacity *= std::min(ripple->age / theme.motion.ripple_fade_in, 1.0f);
    }
    draw->PushClipRect(rect.Min, rect.Max, true);
    draw->AddCircleFilled(ripple->center,
                          RadiusToCorners(ripple->center, rect) * expand,
                          ink.U32(opacity), 48);
    draw->PopClipRect();
}

void RenderIcon(ImDrawList* draw, ImVec2 position, const char* icon, Color color,
                float size = 24.0f) {
    ImFont* font = GetTheme().fonts.icons;
    if (font)
        draw->AddText(font, size, position, color.U32(), icon);
    else
        draw->AddText(position, color.U32(), icon);
}

ImVec2 IconSize(const char* icon, float size = 24.0f) {
    ImFont* font = GetTheme().fonts.icons;
    return font ? font->CalcTextSizeA(size, FLT_MAX, 0.0f, icon)
                : ImGui::CalcTextSize(icon);
}

bool CustomButtonBehavior(const ImRect& rect, ImGuiID id, bool enabled,
                          bool* hovered, bool* held,
                          ImGuiButtonFlags flags = ImGuiButtonFlags_None) {
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, id)) {
        *hovered = *held = false;
        return false;
    }
    if (!enabled) {
        *hovered = *held = false;
        return false;
    }
    return ImGui::ButtonBehavior(rect, id, hovered, held, flags);
}

// 让组件的文字始终使用其在 MD2 规范中规定的 TextStyle（字号/字重/字间距），
// 而不是继承调用处当前绑定的字体。组件内部的 CalcTextSize/AddText（不带
// font 参数的重载）都读取"当前绑定字体"，所以必须在测量与绘制之间保持同一
// 个 push 生效，构成本文件字体正确性的核心机制。
struct ScopedTextStyle {
    bool pushed = false;
    explicit ScopedTextStyle(TextStyle style) {
        if (ImFont* font = GetTheme().fonts.Get(style)) {
            // 单参数重载会应用 font->LegacySize，即该字体按 AddFontXXX() 时
            // 传入的 MD2 规范尺寸加载，而不是继承调用处当前的字号。
            ImGui::PushFont(font);
            pushed = true;
        }
    }
    ~ScopedTextStyle() {
        if (pushed) ImGui::PopFont();
    }
    ScopedTextStyle(const ScopedTextStyle&) = delete;
    ScopedTextStyle& operator=(const ScopedTextStyle&) = delete;
};

Color SurfaceForElevation(const Theme& theme, int elevation) {
    if (!theme.colors.dark || elevation <= 0) return theme.colors.surface;
    static constexpr float overlays[] = {0.0f, 0.05f, 0.07f, 0.08f, 0.09f,
                                         0.11f, 0.12f, 0.14f, 0.15f, 0.16f,
                                         0.16f, 0.17f, 0.18f, 0.18f, 0.18f,
                                         0.19f, 0.20f, 0.20f, 0.20f, 0.21f,
                                         0.21f, 0.21f, 0.22f, 0.22f, 0.22f};
    const int level = std::clamp(elevation, 0, 24);
    return StateColor(Color::FromHex(0xffffff), theme.colors.surface,
                      overlays[level]);
}

} // namespace

Context* CreateContext() {
    ImGuiContext* imgui = ImGui::GetCurrentContext();
    IM_ASSERT(imgui != nullptr && "Create the ImGui context first");
    auto found = g_contexts.find(imgui);
    if (found != g_contexts.end()) return found->second.get();
    auto context = std::make_unique<Context>();
    Context* result = context.get();
    g_contexts.emplace(imgui, std::move(context));
    return result;
}

void DestroyContext(Context* context) {
    if (!context) {
        ImGuiContext* imgui = ImGui::GetCurrentContext();
        if (imgui) g_contexts.erase(imgui);
        return;
    }
    for (auto it = g_contexts.begin(); it != g_contexts.end(); ++it) {
        if (it->second.get() == context) {
            g_contexts.erase(it);
            return;
        }
    }
}

Context& GetContext() { return *CreateContext(); }
Theme& GetTheme() { return GetContext().theme; }
Animator& GetAnimator() { return GetContext().animator; }

void SetTheme(const Theme& theme, bool apply_imgui_style) {
    Context& context = GetContext();
    const bool incoming_fonts =
        theme.fonts.icons != nullptr ||
        std::any_of(theme.fonts.styles.begin(), theme.fonts.styles.end(),
                    [](ImFont* font) { return font != nullptr; });
    if (!incoming_fonts) {
        const bool current_fonts =
            context.theme.fonts.icons != nullptr ||
            std::any_of(context.theme.fonts.styles.begin(), context.theme.fonts.styles.end(),
                        [](ImFont* font) { return font != nullptr; });
        if (current_fonts) {
            Theme preserved = theme;
            preserved.fonts = context.theme.fonts;
            context.theme = preserved;
        } else {
            context.theme = theme;
        }
    } else {
        context.theme = theme;
    }
    if (apply_imgui_style) ApplyTheme(context.theme);
}

bool ApplyNamedTheme(const char* name, bool invert_secondary) {
    Theme selected = Theme::Named(name);
    // A named resource changes color roles only. Keep the host's loaded
    // fonts, density and motion/shape customization when switching at
    // runtime, matching qt-material's stylesheet behavior.
    Context& context = GetContext();
    selected.fonts = context.theme.fonts;
    selected.shapes = context.theme.shapes;
    selected.motion = context.theme.motion;
    selected.layout = context.theme.layout;
    if (invert_secondary && !selected.colors.dark) {
        // Named light resources already use the M2 Grey50/White/Grey100
        // hierarchy; the flag is retained for qt-material API compatibility.
        (void)invert_secondary;
    }
    SetTheme(selected, true);
    return selected.name != nullptr && selected.name[0] != '\0';
}

void NewFrame() {
    Context& context = GetContext();
    const int frame = ImGui::GetFrameCount();
    if (context.last_frame == frame) return;
    context.last_frame = frame;
    const float dt = ImGui::GetIO().DeltaTime;
    context.animator.NewFrame(dt, frame);
    for (auto it = context.ripples.begin(); it != context.ripples.end();) {
        it->second.age += dt;
        if (!it->second.down) it->second.release_age += dt;
        const bool finished = !it->second.down &&
                              it->second.release_age > context.theme.motion.ripple_fade_out;
        const bool stale = frame - it->second.touched_frame > 4;
        if (finished || stale)
            it = context.ripples.erase(it);
        else
            ++it;
    }
}

void ElevationShadow(ImDrawList* draw_list, ImVec2 minimum, ImVec2 maximum,
                     float rounding, int elevation, float alpha) {
    if (!draw_list || elevation <= 0 || alpha <= 0.0f) return;
    elevation = std::clamp(elevation, 1, 24);
    const float offset = 0.5f + elevation * 0.22f;
    const float spread = 1.0f + elevation * 0.32f;
    const int layers = std::clamp(2 + elevation / 3, 2, 8);
    for (int i = layers; i >= 1; --i) {
        const float t = static_cast<float>(i) / layers;
        const float grow = spread * t;
        const float layer_alpha = alpha * (0.055f / layers) * (1.2f - t * 0.35f);
        draw_list->AddRectFilled(minimum - ImVec2(grow, grow) + ImVec2(0, offset),
                                 maximum + ImVec2(grow, grow) + ImVec2(0, offset),
                                 Color::FromHex(0x000000, layer_alpha).U32(),
                                 rounding + grow);
    }
}

bool Button(const char* label, const ButtonOptions& options) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    const std::string text = Uppercase(VisibleLabel(label));
    const ScopedTextStyle text_style(TextStyle::Button);
    const ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    const float icon_width = options.leading_icon ? IconSize(options.leading_icon).x + 8.0f : 0.0f;
    ImVec2 size = options.size;
    if (size.x <= 0.0f) size.x = text_size.x + icon_width + 32.0f;
    if (options.full_width) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = theme.layout.button_height;
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + size);
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, options.enabled, &hovered, &held);
    ImDrawList* draw = window->DrawList;

    Color container(0, 0, 0, 0);
    Color content = theme.colors.primary;
    if (options.variant == ButtonVariant::Contained) {
        container = theme.colors.primary;
        content = theme.colors.on_primary;
        ElevationShadow(draw, rect.Min, rect.Max, theme.shapes.small, held ? 8 : 2,
                        options.enabled ? 1.0f : 0.35f);
        if (!options.enabled)
            container = theme.colors.on_surface.WithAlpha(theme.states.disabled_container);
        draw->AddRectFilled(rect.Min, rect.Max, container.U32(), theme.shapes.small);
    } else if (options.variant == ButtonVariant::Outlined) {
        draw->AddRect(rect.Min + ImVec2(0.5f, 0.5f), rect.Max - ImVec2(0.5f, 0.5f),
                      theme.colors.outline.U32(), theme.shapes.small, 0, 1.0f);
    }
    if (!options.enabled) content = theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    DrawRipple(id, rect, theme.shapes.small, content, hovered, held, options.enabled);

    float x = rect.GetCenter().x - (text_size.x + icon_width) * 0.5f;
    if (options.leading_icon) {
        const ImVec2 icon_size = IconSize(options.leading_icon);
        RenderIcon(draw, ImVec2(x, rect.GetCenter().y - icon_size.y * 0.5f),
                   options.leading_icon, content);
        x += icon_width;
    }
    draw->AddText(ImVec2(x, rect.GetCenter().y - text_size.y * 0.5f), content.U32(),
                  text.c_str());
    return pressed;
}

bool TextButton(const char* label, ImVec2 size) {
    ButtonOptions options;
    options.variant = ButtonVariant::Text;
    options.size = size;
    return Button(label, options);
}

bool OutlinedButton(const char* label, ImVec2 size) {
    ButtonOptions options;
    options.variant = ButtonVariant::Outlined;
    options.size = size;
    return Button(label, options);
}

bool ContainedButton(const char* label, ImVec2 size) {
    ButtonOptions options;
    options.variant = ButtonVariant::Contained;
    options.size = size;
    return Button(label, options);
}

bool IconButton(const char* id_string, const char* icon, bool selected,
                bool enabled, float size) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    if (size <= 0.0f) size = Metrics::IconButtonSize();
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(id_string);
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size, size));
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, enabled, &hovered, &held);
    Color ink = selected ? theme.colors.primary : theme.colors.on_surface;
    if (!enabled) ink = ink.WithAlpha(theme.states.disabled_content);
    if (selected)
        window->DrawList->AddCircleFilled(rect.GetCenter(), size * 0.5f,
                                           ink.U32(theme.states.selected));
    DrawRipple(id, rect, size * 0.5f, ink, hovered, held, enabled);
    const ImVec2 icon_size = IconSize(icon);
    RenderIcon(window->DrawList, rect.GetCenter() - icon_size * 0.5f, icon, ink);
    return pressed;
}

bool FloatingActionButton(const char* id_string, const char* icon,
                          const char* label, bool enabled) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(id_string);
    // FAB 扩展标签与普通按钮共享同一套 Button TextStyle（14sp/Medium/大写）。
    const ScopedTextStyle text_style(TextStyle::Button);
    const std::string visible = label ? Uppercase(VisibleLabel(label)) : std::string();
    const ImVec2 text_size = label ? ImGui::CalcTextSize(visible.c_str()) : ImVec2();
    const float fab_height = label ? Metrics::FabExtendedHeight() : Metrics::FabSize();
    const float width = label ? text_size.x + 80.0f : Metrics::FabSize();
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, fab_height));
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, enabled, &hovered, &held);
    Color container = enabled ? theme.colors.secondary
                              : theme.colors.on_surface.WithAlpha(theme.states.disabled_container);
    Color content = enabled ? theme.colors.on_secondary
                            : theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    ElevationShadow(window->DrawList, rect.Min, rect.Max, 28.0f, held ? 12 : 6);
    window->DrawList->AddRectFilled(rect.Min, rect.Max, container.U32(), 28.0f);
    DrawRipple(id, rect, 28.0f, content, hovered, held, enabled);
    const ImVec2 icon_size = IconSize(icon);
    float x = label ? rect.Min.x + 20.0f : rect.GetCenter().x - icon_size.x * 0.5f;
    RenderIcon(window->DrawList, ImVec2(x, rect.GetCenter().y - icon_size.y * 0.5f),
               icon, content);
    if (label)
        window->DrawList->AddText(ImVec2(x + 40.0f, rect.GetCenter().y - text_size.y * 0.5f),
                                  content.U32(), visible.c_str());
    return pressed;
}

bool Checkbox(const char* label, bool* value, bool enabled) {
    NewFrame();
    IM_ASSERT(value != nullptr);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    // 控件标签使用 Body1（16sp/regular），与 MD2 表单控件规范一致。
    const ScopedTextStyle text_style(TextStyle::Body1);
    const ImVec2 label_size = ImGui::CalcTextSize(VisibleLabel(label).c_str());
    const float height = std::max(Metrics::TouchTarget(), label_size.y);
    const ImRect rect(window->DC.CursorPos,
                      window->DC.CursorPos + ImVec2(Metrics::TouchTarget() + label_size.x, height));
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, enabled, &hovered, &held);
    if (pressed) *value = !*value;
    const float checked = GetAnimator().Animate(Salt(id, "checked"), *value ? 1.0f : 0.0f,
                                                0.120f, Easing::Standard);
    const ImVec2 center(rect.Min.x + Metrics::TouchTarget() * 0.5f, rect.GetCenter().y);
    const ImRect touch(center - ImVec2(20, 20), center + ImVec2(20, 20));
    Color ink = *value ? theme.colors.primary : theme.colors.on_surface;
    if (!enabled) ink = ink.WithAlpha(theme.states.disabled_content);
    DrawRipple(id, touch, 20.0f, ink, hovered, held, enabled);
    const ImRect box(center - ImVec2(9, 9), center + ImVec2(9, 9));
    if (checked > 0.001f) {
        window->DrawList->AddRectFilled(box.Min, box.Max, ink.U32(checked), 2.0f);
        const Color mark = theme.colors.on_primary.WithAlpha(checked);
        const ImVec2 p1 = center + ImVec2(-5.0f, 0.0f);
        const ImVec2 p2 = center + ImVec2(-1.5f, 4.0f);
        const ImVec2 p3 = center + ImVec2(6.0f, -5.0f);
        window->DrawList->AddLine(p1, p2, mark.U32(), 2.0f);
        window->DrawList->AddLine(p2, p3, mark.U32(), 2.0f);
    }
    if (checked < 0.999f)
        window->DrawList->AddRect(box.Min, box.Max,
                                  theme.colors.on_surface.U32(
                                      (enabled ? 0.54f : theme.states.disabled_content) *
                                      (1.0f - checked)),
                                  2.0f, 0, 2.0f);
    const Color label_color = enabled ? theme.colors.on_surface
                                      : theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    window->DrawList->AddText(ImVec2(rect.Min.x + Metrics::TouchTarget(),
                                     rect.GetCenter().y - label_size.y * 0.5f),
                              label_color.U32(), VisibleLabel(label).c_str());
    return pressed;
}

bool RadioButton(const char* label, int* value, int button_value, bool enabled) {
    NewFrame();
    IM_ASSERT(value != nullptr);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    const ScopedTextStyle text_style(TextStyle::Body1);
    const std::string visible = VisibleLabel(label);
    const ImVec2 label_size = ImGui::CalcTextSize(visible.c_str());
    const ImRect rect(window->DC.CursorPos,
                      window->DC.CursorPos +
                          ImVec2(Metrics::TouchTarget() + label_size.x,
                                 std::max(Metrics::TouchTarget(), label_size.y)));
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, enabled, &hovered, &held);
    if (pressed) *value = button_value;
    const bool selected = *value == button_value;
    const float progress = GetAnimator().Animate(Salt(id, "selected"), selected ? 1.0f : 0.0f,
                                                 0.120f, Easing::Standard);
    const ImVec2 center(rect.Min.x + Metrics::TouchTarget() * 0.5f, rect.GetCenter().y);
    Color ink = selected ? theme.colors.primary : theme.colors.on_surface;
    if (!enabled) ink = ink.WithAlpha(theme.states.disabled_content);
    DrawRipple(id, ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20)),
               20.0f, ink, hovered, held, enabled);
    window->DrawList->AddCircle(center, 9.0f, ink.U32(selected ? 1.0f : 0.54f), 24, 2.0f);
    if (progress > 0.001f)
        window->DrawList->AddCircleFilled(center, 5.0f * progress,
                                           theme.colors.primary.U32(enabled ? 1.0f
                                                                            : theme.states.disabled_content),
                                           20);
    const Color label_color = enabled ? theme.colors.on_surface
                                      : theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    window->DrawList->AddText(ImVec2(rect.Min.x + Metrics::TouchTarget(),
                                     rect.GetCenter().y - label_size.y * 0.5f),
                              label_color.U32(), visible.c_str());
    return pressed;
}

bool Switch(const char* label, bool* value, bool enabled) {
    NewFrame();
    IM_ASSERT(value != nullptr);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    const ScopedTextStyle text_style(TextStyle::Body1);
    const std::string visible = VisibleLabel(label);
    const ImVec2 label_size = ImGui::CalcTextSize(visible.c_str());
    const ImRect rect(window->DC.CursorPos,
                      window->DC.CursorPos +
                          ImVec2(Metrics::TouchTarget() + Metrics::DenseGap() + label_size.x,
                                 Metrics::TouchTarget()));
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, enabled, &hovered, &held);
    if (pressed) *value = !*value;
    const float progress = GetAnimator().Animate(Salt(id, "switch"), *value ? 1.0f : 0.0f,
                                                 0.075f, Easing::Standard);
    const float track_width = Metrics::SwitchTrackWidth();
    const float track_height = Metrics::SwitchTrackHeight();
    const float thumb_size = Metrics::SwitchThumbSize();
    const ImVec2 track_min(rect.Min.x + (Metrics::TouchTarget() - track_width) * 0.5f,
                           rect.GetCenter().y - track_height * 0.5f);
    const ImVec2 track_max(track_min.x + track_width, track_min.y + track_height);
    Color track_off = theme.colors.on_surface.WithAlpha(enabled ? 0.30f : 0.12f);
    Color track_on = theme.colors.primary.WithAlpha(enabled ? 0.50f : 0.12f);
    window->DrawList->AddRectFilled(track_min, track_max,
                                     Mix(track_off, track_on, progress).U32(),
                                     track_height * 0.5f);
    const ImVec2 thumb(track_min.x + track_height * 0.5f +
                           (track_width - track_height) * progress,
                       rect.GetCenter().y);
    Color thumb_color = Mix(enabled ? theme.colors.surface : theme.colors.on_surface,
                            theme.colors.primary, progress);
    if (!enabled) thumb_color = thumb_color.WithAlpha(theme.states.disabled_content);
    const float thumb_radius = thumb_size * 0.5f;
    ElevationShadow(window->DrawList, thumb - ImVec2(thumb_radius, thumb_radius),
                    thumb + ImVec2(thumb_radius, thumb_radius), thumb_radius, 1,
                    enabled ? 1.0f : 0.3f);
    window->DrawList->AddCircleFilled(thumb, thumb_radius, thumb_color.U32(), 24);
    const float ripple_radius = Metrics::TouchTarget() * 0.5f;
    DrawRipple(id, ImRect(thumb - ImVec2(ripple_radius, ripple_radius),
                          thumb + ImVec2(ripple_radius, ripple_radius)),
               ripple_radius,
               *value ? theme.colors.primary : theme.colors.on_surface,
               hovered, held, enabled);
    const Color label_color = enabled ? theme.colors.on_surface
                                      : theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    window->DrawList->AddText(ImVec2(rect.Min.x + Metrics::TouchTarget() + Metrics::DenseGap(),
                                     rect.GetCenter().y - label_size.y * 0.5f),
                              label_color.U32(), visible.c_str());
    return pressed;
}

bool SliderFloat(const char* label, float* value, float minimum, float maximum,
                 const char* format, bool enabled) {
    NewFrame();
    IM_ASSERT(value != nullptr && maximum > minimum);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    const float width = std::max(ImGui::CalcItemWidth(), 120.0f);
    const ImRect rect(window->DC.CursorPos,
                      window->DC.CursorPos + ImVec2(width, Metrics::TouchTarget()));
    bool hovered = false;
    bool held = false;
    CustomButtonBehavior(rect, id, enabled, &hovered, &held,
                         ImGuiButtonFlags_PressedOnClick);
    bool changed = false;
    const float left = rect.Min.x + 8.0f;
    const float right = rect.Max.x - 8.0f;
    if (held && enabled) {
        const float next = minimum +
                           std::clamp((ImGui::GetIO().MousePos.x - left) / (right - left),
                                      0.0f, 1.0f) *
                               (maximum - minimum);
        if (next != *value) {
            *value = next;
            changed = true;
            ImGui::MarkItemEdited(id);
        }
    }
    const float fraction = std::clamp((*value - minimum) / (maximum - minimum), 0.0f, 1.0f);
    const float y = rect.GetCenter().y + 5.0f;
    const float thumb_x = left + fraction * (right - left);
    const Color inactive = theme.colors.on_surface.WithAlpha(enabled ? 0.24f : 0.12f);
    const Color active = enabled ? theme.colors.primary
                                 : theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    window->DrawList->AddRectFilled(ImVec2(left, y - 2), ImVec2(right, y + 2),
                                     inactive.U32(), 2.0f);
    window->DrawList->AddRectFilled(ImVec2(left, y - 2), ImVec2(thumb_x, y + 2),
                                     active.U32(), 2.0f);
    const float thumb_radius = GetAnimator().Animate(Salt(id, "thumb"), held ? 10.0f : 6.0f,
                                                     0.075f, Easing::Standard);
    window->DrawList->AddCircleFilled(ImVec2(thumb_x, y), thumb_radius, active.U32(), 24);
    if (hovered || held)
        window->DrawList->AddCircleFilled(ImVec2(thumb_x, y), 20.0f,
                                           active.U32(held ? theme.states.pressed
                                                           : theme.states.hover),
                                           32);
    char value_text[64];
    ImFormatString(value_text, IM_ARRAYSIZE(value_text), format, *value);
    const std::string visible = VisibleLabel(label);
    const Color content = enabled ? theme.colors.on_surface
                                  : theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    {
        const ScopedTextStyle label_style(TextStyle::Body1);
        window->DrawList->AddText(rect.Min, content.U32(), visible.c_str());
    }
    {
        // 数值是次要信息，使用 Body2（14sp），与 label 的 Body1（16sp）拉开层级。
        const ScopedTextStyle value_style(TextStyle::Body2);
        const ImVec2 value_size = ImGui::CalcTextSize(value_text);
        window->DrawList->AddText(ImVec2(rect.Max.x - value_size.x, rect.Min.y),
                                  content.U32(0.60f), value_text);
    }
    return changed;
}

bool TextField(const char* label, char* buffer, std::size_t buffer_size,
               const TextFieldOptions& options, ImGuiInputTextFlags flags) {
    NewFrame();
    IM_ASSERT(buffer != nullptr && buffer_size > 0);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const float dpi = DpiScale();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = options.size.x > 0.0f ? options.size.x : ImGui::CalcItemWidth();
    const float height = options.size.y > 0.0f ? options.size.y
                                               : Metrics::TextFieldHeight();
    const float icon_inset = (options.leading_icon ? 44.0f : 12.0f) * dpi;
    const ImGuiID animation_id = window->GetID(label);
    ImGui::PushID(label);
    if (!options.enabled) ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(width);
    // 输入内容遵循 MD2 TextField 的正文层级：Subtitle1（16sp/regular）。字体
    // 必须先 push，FramePadding 的竖直居中计算才能读到正确的 GetFontSize()。
    ImFont* input_font = theme.fonts.Get(TextStyle::Subtitle1);
    if (input_font) ImGui::PushFont(input_font);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(icon_inset,
                               std::max((height - ImGui::GetFontSize()) * 0.5f, 4.0f * dpi)));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                        options.variant == TextFieldVariant::Filled ? theme.shapes.small
                                                                   : theme.shapes.medium);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,
                          (options.variant == TextFieldVariant::Filled
                               ? theme.colors.surface_variant
                               : Color(0, 0, 0, 0))
                              .Vec4());
    ImGui::PushStyleColor(ImGuiCol_Border,
                          (options.error ? theme.colors.error : theme.colors.outline).Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,
                        options.variant == TextFieldVariant::Outlined ? 1.0f : 0.0f);
    const bool changed = ImGui::InputText("##field", buffer, buffer_size, flags);
    if (input_font) ImGui::PopFont();
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    const ImRect field(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
    if (!options.enabled) ImGui::EndDisabled();
    ImGui::PopID();

    const bool floating = active || buffer[0] != '\0';
    const float float_progress = GetAnimator().Animate(Salt(animation_id, "label"),
                                                       floating ? 1.0f : 0.0f,
                                                       0.150f, Easing::Standard);
    Color accent = options.error ? theme.colors.error : theme.colors.primary;
    Color label_color = active ? accent : theme.colors.on_surface.WithAlpha(0.60f);
    if (!options.enabled) label_color = theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    // 浮动标签在 Subtitle1（16sp）与 Caption（12sp）之间连续插值字号——两者都是
    // regular 字重，用同一份字体数据按不同像素尺寸绘制即可平滑过渡，不需要在
    // 两个不同字重的字体间切换。
    const float unfloated_size = Typography(TextStyle::Subtitle1).size * dpi;
    const float floated_size = Typography(TextStyle::Caption).size * dpi;
    const float label_font_size =
        unfloated_size + (floated_size - unfloated_size) * float_progress;
    ImFont* label_font = theme.fonts.Get(TextStyle::Caption);
    const float label_y_center = field.GetCenter().y - unfloated_size * 0.5f;
    const float label_y_float = field.Min.y + 4.0f * dpi;
    const float label_y = label_y_center + (label_y_float - label_y_center) * float_progress;
    float label_x = field.Min.x + icon_inset;
    if (label_font) {
        window->DrawList->AddText(label_font, label_font_size, ImVec2(label_x, label_y),
                                  label_color.U32(), VisibleLabel(label).c_str());
    } else {
        window->DrawList->AddText(ImVec2(label_x, label_y), label_color.U32(),
                                  VisibleLabel(label).c_str());
    }
    if (options.leading_icon) {
        const ImVec2 icon_size = IconSize(options.leading_icon);
        RenderIcon(window->DrawList,
                   ImVec2(field.Min.x + 12.0f * dpi, field.GetCenter().y - icon_size.y * 0.5f),
                   options.leading_icon, label_color);
    }
    if (options.trailing_icon) {
        const ImVec2 icon_size = IconSize(options.trailing_icon);
        RenderIcon(window->DrawList,
                   ImVec2(field.Max.x - icon_size.x - 12.0f * dpi,
                          field.GetCenter().y - icon_size.y * 0.5f),
                   options.trailing_icon, label_color);
    }
    if (options.variant == TextFieldVariant::Filled) {
        const float line = (active ? 2.0f : 1.0f) * dpi;
        window->DrawList->AddLine(ImVec2(field.Min.x, field.Max.y - line * 0.5f),
                                  ImVec2(field.Max.x, field.Max.y - line * 0.5f),
                                  (active || options.error ? accent
                                                           : theme.colors.on_surface.WithAlpha(0.42f))
                                      .U32(),
                                  line);
    } else if (active || options.error || hovered) {
        window->DrawList->AddRect(field.Min, field.Max,
                                  (options.error ? theme.colors.error : theme.colors.primary).U32(),
                                  theme.shapes.medium, 0, (active ? 2.0f : 1.0f) * dpi);
    }
    if (options.helper_text) {
        const Color helper = options.error ? theme.colors.error
                                           : theme.colors.on_surface.WithAlpha(0.60f);
        const ScopedTextStyle helper_style(TextStyle::Caption);
        window->DrawList->AddText(ImVec2(start.x + 12.0f * dpi, field.Max.y + 3.0f * dpi),
                                  helper.U32(), options.helper_text);
        ImGui::Dummy(ImVec2(0.0f, 20.0f * dpi));
    }
    return changed;
}

bool Select(const char* label, int* current, const char* const* items,
            int count, bool enabled, float width) {
    NewFrame();
    IM_ASSERT(current != nullptr && items != nullptr && count > 0);
    *current = std::clamp(*current, 0, count - 1);
    if (width <= 0.0f) width = ImGui::CalcItemWidth();
    ImGui::PushID(label);
    const Color label_color = enabled ? GetTheme().colors.on_surface.WithAlpha(0.60f)
                                      : GetTheme().colors.on_surface.WithAlpha(
                                            GetTheme().states.disabled_content);
    Text(TextStyle::Caption, label, label_color);
    ButtonOptions options;
    options.variant = ButtonVariant::Outlined;
    options.size = ImVec2(width, Metrics::TextFieldHeight());
    options.enabled = enabled;
    std::string preview = std::string(items[*current]) + "###select-button";
    const bool opened = Button(preview.c_str(), options);
    if (opened) ImGui::OpenPopup("##select-popup");
    bool changed = false;
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, 320.0f));
    if (ImGui::BeginPopup("##select-popup")) {
        for (int i = 0; i < count; ++i) {
            ImGui::PushID(i);
            if (ListItem(items[i], nullptr, nullptr, nullptr, i == *current, enabled)) {
                *current = i;
                changed = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return changed;
}

void LinearProgress(float fraction, ImVec2 size) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;
    const Theme& theme = GetTheme();
    if (size.x <= 0.0f) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = Metrics::ProgressHeight();
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + size);
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, 0)) return;
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    window->DrawList->AddRectFilled(rect.Min, rect.Max,
                                     theme.colors.primary.U32(0.24f),
                                     size.y * 0.5f);
    window->DrawList->AddRectFilled(rect.Min,
                                     ImVec2(rect.Min.x + rect.GetWidth() * fraction,
                                            rect.Max.y),
                                     theme.colors.primary.U32(), size.y * 0.5f);
}

void LinearProgressIndeterminate(const char* id_string, ImVec2 size) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(id_string);
    if (size.x <= 0.0f) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = Metrics::ProgressHeight();
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + size);
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, id)) return;
    window->DrawList->AddRectFilled(rect.Min, rect.Max,
                                     theme.colors.primary.U32(0.24f),
                                     size.y * 0.5f);
    const float phase = std::fmod(static_cast<float>(ImGui::GetTime()) * 0.72f, 1.0f);
    const float head = Ease(Easing::Deceleration, phase);
    const float tail = Ease(Easing::Acceleration, std::max(phase - 0.28f, 0.0f) / 0.72f);
    const float x1 = rect.Min.x + rect.GetWidth() * tail;
    const float x2 = rect.Min.x + rect.GetWidth() * head;
    window->DrawList->AddRectFilled(ImVec2(x1, rect.Min.y), ImVec2(x2, rect.Max.y),
                                     theme.colors.primary.U32(), size.y * 0.5f);
}

void CircularProgress(const char* id_string, float fraction, float radius,
                      float thickness) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(id_string);
    const ImVec2 size(radius * 2.0f + thickness * 2.0f,
                      radius * 2.0f + thickness * 2.0f);
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + size);
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, id)) return;
    const ImVec2 center = rect.GetCenter();
    float start = -IM_PI * 0.5f;
    float end = start;
    if (fraction < 0.0f) {
        const float phase = std::fmod(static_cast<float>(ImGui::GetTime()) * 0.65f, 1.0f);
        start += phase * IM_PI * 2.0f;
        const float sweep = (0.12f + 0.70f *
                             std::abs(std::sin(static_cast<float>(ImGui::GetTime()) * 2.4f))) *
                            IM_PI * 2.0f;
        end = start + sweep;
    } else {
        fraction = std::clamp(fraction, 0.0f, 1.0f);
        window->DrawList->PathArcTo(center, radius, 0.0f, IM_PI * 2.0f, 48);
        window->DrawList->PathStroke(theme.colors.primary.U32(0.20f),
                                     ImDrawFlags_Closed, thickness);
        end = start + fraction * IM_PI * 2.0f;
    }
    window->DrawList->PathArcTo(center, radius, start, end, 40);
    window->DrawList->PathStroke(theme.colors.primary.U32(), 0, thickness);
}

bool BeginCard(const char* id, ImVec2 size, int elevation, bool outlined) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const Theme& theme = GetTheme();
    if (size.x <= 0.0f) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = 120.0f;
    const ImVec2 minimum = ImGui::GetCursorScreenPos();
    const ImVec2 maximum = minimum + size;
    ElevationShadow(window->DrawList, minimum, maximum, theme.shapes.medium,
                    outlined ? 0 : elevation);
    if (outlined)
        window->DrawList->AddRect(minimum, maximum, theme.colors.outline.U32(),
                                  theme.shapes.medium, 0, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          SurfaceForElevation(theme, elevation).Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, theme.shapes.medium);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    // AlwaysUseWindowPadding: 无边框 child window 默认不应用 WindowPadding，
    // 没有这个标志卡片内容会紧贴卡片边缘（MD2 卡片内边距规范是 16dp）。
    ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    return true;
}

void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

bool ListItem(const char* label, const char* supporting_text,
              const char* leading_icon, const char* trailing_text,
              bool selected, bool enabled) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    const float height = supporting_text ? Metrics::ListRowHeightTwoLine()
                                         : Metrics::ListRowHeight();
    const float width = ImGui::GetContentRegionAvail().x;
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, height));
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, enabled, &hovered, &held);
    Color ink = selected ? theme.colors.primary : theme.colors.on_surface;
    if (!enabled) ink = theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    if (selected)
        window->DrawList->AddRectFilled(rect.Min, rect.Max,
                                         theme.colors.primary.U32(theme.states.selected));
    DrawRipple(id, rect, 0.0f, ink, hovered, held, enabled);
    float x = rect.Min.x + 16.0f;
    if (leading_icon) {
        const ImVec2 icon_size = IconSize(leading_icon);
        RenderIcon(window->DrawList,
                   ImVec2(x, rect.GetCenter().y - icon_size.y * 0.5f),
                   leading_icon, ink);
        x += 40.0f;
    }
    const std::string visible = VisibleLabel(label);
    {
        // 主文字使用 Subtitle1（16sp/regular），MD2 列表项的正文层级。
        const ScopedTextStyle primary_style(TextStyle::Subtitle1);
        const ImVec2 primary_size = ImGui::CalcTextSize(visible.c_str());
        const float primary_y = supporting_text ? rect.Min.y + 15.0f
                                                : rect.GetCenter().y - primary_size.y * 0.5f;
        window->DrawList->AddText(ImVec2(x, primary_y), ink.U32(), visible.c_str());
    }
    if (supporting_text) {
        // 辅助说明文字使用 Body2（14sp/regular）。
        const ScopedTextStyle supporting_style(TextStyle::Body2);
        window->DrawList->AddText(ImVec2(x, rect.Min.y + 39.0f),
                                  theme.colors.on_surface.U32(enabled ? 0.60f
                                                                      : theme.states.disabled_content),
                                  supporting_text);
    }
    if (trailing_text) {
        // 行尾元信息（如时间戳）使用 Caption（12sp/regular）。
        const ScopedTextStyle trailing_style(TextStyle::Caption);
        const ImVec2 trailing_size = ImGui::CalcTextSize(trailing_text);
        window->DrawList->AddText(ImVec2(rect.Max.x - trailing_size.x - 16.0f,
                                         rect.GetCenter().y - trailing_size.y * 0.5f),
                                  theme.colors.on_surface.U32(enabled ? 0.60f
                                                                      : theme.states.disabled_content),
                                  trailing_text);
    }
    return pressed;
}

void Divider(float inset) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;
    const float width = ImGui::GetContentRegionAvail().x;
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, 1.0f));
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, 0)) return;
    window->DrawList->AddLine(ImVec2(rect.Min.x + inset, rect.Min.y + 0.5f),
                              ImVec2(rect.Max.x, rect.Min.y + 0.5f),
                              GetTheme().colors.outline.U32(), 1.0f);
}

bool Chip(const char* label, bool* selected, const char* leading_icon,
          bool enabled) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    // Chip 文字使用 Body2（14sp/regular）。
    const ScopedTextStyle text_style(TextStyle::Body2);
    const std::string visible = VisibleLabel(label);
    const ImVec2 text_size = ImGui::CalcTextSize(visible.c_str());
    const float leading = leading_icon ? 28.0f : 0.0f;
    const ImVec2 size(text_size.x + leading + 24.0f, Metrics::ChipHeight());
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + size);
    bool hovered = false;
    bool held = false;
    const bool pressed = CustomButtonBehavior(rect, id, enabled, &hovered, &held);
    if (pressed && selected) *selected = !*selected;
    const bool is_selected = selected && *selected;
    Color container = is_selected
                          ? theme.colors.primary.WithAlpha(0.12f)
                          : theme.colors.on_surface.WithAlpha(0.08f);
    Color ink = is_selected ? theme.colors.primary : theme.colors.on_surface;
    if (!enabled) {
        container = theme.colors.on_surface.WithAlpha(theme.states.disabled_container);
        ink = theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    }
    window->DrawList->AddRectFilled(rect.Min, rect.Max, container.U32(), 16.0f);
    window->DrawList->AddRect(rect.Min, rect.Max, theme.colors.outline.U32(), 16.0f, 0, 1.0f);
    DrawRipple(id, rect, 16.0f, ink, hovered, held, enabled);
    float x = rect.Min.x + 12.0f;
    if (leading_icon) {
        const ImVec2 icon_size = IconSize(leading_icon, 18.0f);
        RenderIcon(window->DrawList,
                   ImVec2(x, rect.GetCenter().y - icon_size.y * 0.5f),
                   leading_icon, ink, 18.0f);
        x += 28.0f;
    }
    window->DrawList->AddText(ImVec2(x, rect.GetCenter().y - text_size.y * 0.5f),
                              ink.U32(), visible.c_str());
    return pressed;
}

bool DismissibleChip(const char* label, bool* dismissed,
                     const char* leading_icon, bool enabled) {
    IM_ASSERT(dismissed != nullptr);
    if (*dismissed) return false;
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID id = window->GetID(label);
    const ImGuiID close_id = Salt(id, "close");
    // Chip 文字使用 Body2（14sp/regular）。
    const ScopedTextStyle text_style(TextStyle::Body2);
    const std::string visible = VisibleLabel(label);
    const ImVec2 text_size = ImGui::CalcTextSize(visible.c_str());
    const float leading = leading_icon ? 28.0f : 0.0f;
    const ImRect rect(window->DC.CursorPos,
                      window->DC.CursorPos +
                          ImVec2(text_size.x + leading + 56.0f, Metrics::ChipHeight()));
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, id)) return false;
    const ImRect body(rect.Min, ImVec2(rect.Max.x - 32.0f, rect.Max.y));
    const ImRect close(ImVec2(rect.Max.x - 32.0f, rect.Min.y), rect.Max);
    bool body_hovered = false;
    bool body_held = false;
    bool close_hovered = false;
    bool close_held = false;
    const bool pressed = enabled &&
                         ImGui::ButtonBehavior(body, id, &body_hovered, &body_held);
    const bool close_pressed = enabled &&
                               ImGui::ButtonBehavior(close, close_id, &close_hovered,
                                                     &close_held);
    if (close_pressed) *dismissed = true;
    Color ink = enabled ? theme.colors.on_surface
                        : theme.colors.on_surface.WithAlpha(theme.states.disabled_content);
    const Color container = theme.colors.on_surface.WithAlpha(
        enabled ? 0.08f : theme.states.disabled_container);
    window->DrawList->AddRectFilled(rect.Min, rect.Max, container.U32(), 16.0f);
    window->DrawList->AddRect(rect.Min, rect.Max, theme.colors.outline.U32(), 16.0f);
    DrawRipple(id, body, 16.0f, ink, body_hovered, body_held, enabled,
               ImDrawFlags_RoundCornersLeft);
    DrawRipple(close_id, close, 16.0f, ink, close_hovered, close_held, enabled,
               ImDrawFlags_RoundCornersRight);
    float x = rect.Min.x + 12.0f;
    if (leading_icon) {
        const ImVec2 icon_size = IconSize(leading_icon, 18.0f);
        RenderIcon(window->DrawList,
                   ImVec2(x, rect.GetCenter().y - icon_size.y * 0.5f),
                   leading_icon, ink, 18.0f);
        x += 28.0f;
    }
    window->DrawList->AddText(ImVec2(x, rect.GetCenter().y - text_size.y * 0.5f),
                              ink.U32(), visible.c_str());
    const char* close_icon = "\xee\x97\x8d";
    const ImVec2 icon_size = IconSize(close_icon, 18.0f);
    RenderIcon(window->DrawList, close.GetCenter() - icon_size * 0.5f,
               close_icon, ink, 18.0f);
    return pressed;
}

bool Tabs(const char* id_string, const char* const* labels, int count,
          int* current, bool fixed) {
    NewFrame();
    IM_ASSERT(labels != nullptr && count > 0 && current != nullptr);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const Theme& theme = GetTheme();
    const ImGuiID base_id = window->GetID(id_string);
    // Tab 标签与按钮共享 MD2 的 Button TextStyle（14sp/Medium/大写）。
    const ScopedTextStyle text_style(TextStyle::Button);
    std::vector<std::string> visible_labels(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
        visible_labels[static_cast<std::size_t>(i)] = Uppercase(VisibleLabel(labels[i]));
    const float available = ImGui::GetContentRegionAvail().x;
    const ImVec2 origin = window->DC.CursorPos;
    float total = 0.0f;
    std::vector<float> widths(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        widths[static_cast<std::size_t>(i)] =
            fixed ? available / count
                  : ImGui::CalcTextSize(visible_labels[static_cast<std::size_t>(i)].c_str()).x +
                        32.0f;
        total += widths[static_cast<std::size_t>(i)];
    }
    const ImRect whole(origin, origin + ImVec2(fixed ? available : total, Metrics::TabHeight()));
    ImGui::ItemSize(whole);
    bool changed = false;
    float x = origin.x;
    for (int i = 0; i < count; ++i) {
        const float width = widths[static_cast<std::size_t>(i)];
        const ImRect rect(ImVec2(x, origin.y),
                          ImVec2(x + width, origin.y + Metrics::TabHeight()));
        const ImGuiID id = ImHashStr(labels[i], 0, base_id);
        bool hovered = false;
        bool held = false;
        bool pressed = false;
        if (ImGui::ItemAdd(rect, id))
            pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
        if (pressed && *current != i) {
            *current = i;
            changed = true;
            ImGui::MarkItemEdited(id);
        }
        if (i == *current)
            window->DrawList->AddRectFilled(rect.Min, rect.Max,
                                             theme.colors.primary.U32(theme.states.selected));
        DrawRipple(id, rect, 0.0f, i == *current ? theme.colors.primary
                                                 : theme.colors.on_surface,
                   hovered, held, true);
        const char* visible = visible_labels[static_cast<std::size_t>(i)].c_str();
        const ImVec2 text_size = ImGui::CalcTextSize(visible);
        window->DrawList->AddText(rect.GetCenter() - text_size * 0.5f,
                                  (i == *current ? theme.colors.primary
                                                 : theme.colors.on_surface.WithAlpha(0.60f))
                                      .U32(),
                                  visible);
        x += width;
    }
    const float animated = GetAnimator().Animate(Salt(base_id, "indicator"),
                                                 static_cast<float>(*current),
                                                 0.225f, Easing::Standard);
    if (fixed) {
        const float tab_width = available / count;
        const float left = origin.x + tab_width * animated;
        window->DrawList->AddRectFilled(ImVec2(left, whole.Max.y - 2.0f),
                                         ImVec2(left + tab_width, whole.Max.y),
                                         theme.colors.primary.U32());
    } else {
        const int index = std::clamp(static_cast<int>(std::round(animated)), 0, count - 1);
        float left = origin.x;
        for (int i = 0; i < index; ++i) left += widths[static_cast<std::size_t>(i)];
        window->DrawList->AddRectFilled(ImVec2(left, whole.Max.y - 2.0f),
                                         ImVec2(left + widths[static_cast<std::size_t>(index)],
                                                whole.Max.y),
                                         theme.colors.primary.U32());
    }
    return changed;
}

void Badge(const char* text, ImVec2 anchor, Color color) {
    NewFrame();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const Theme& theme = GetTheme();
    if (color.a < 0.0f) color = theme.colors.error;
    const char* safe_text = text ? text : "";
    // 徽标数字使用最小的排版字号（Overline，10sp/regular）。
    const ScopedTextStyle text_style(TextStyle::Overline);
    const ImVec2 text_size = ImGui::CalcTextSize(safe_text);
    const float height = text_size.x <= 0.0f ? 8.0f : 20.0f;
    const float width = std::max(height, text_size.x + 8.0f);
    const ImRect rect(anchor - ImVec2(width * 0.5f, height * 0.5f),
                      anchor + ImVec2(width * 0.5f, height * 0.5f));
    draw->AddRectFilled(rect.Min, rect.Max, color.U32(), height * 0.5f);
    if (safe_text[0]) {
        const Color on = AccessibleOnColor(color);
        draw->AddText(rect.GetCenter() - text_size * 0.5f, on.U32(), safe_text);
    }
}

void Avatar(const char* id_string, const char* initials, float diameter,
            Color color) {
    NewFrame();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;
    const ImGuiID id = window->GetID(id_string);
    const ImRect rect(window->DC.CursorPos,
                      window->DC.CursorPos + ImVec2(diameter, diameter));
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, id)) return;
    if (color.a < 0.0f) color = GetTheme().colors.primary;
    window->DrawList->AddCircleFilled(rect.GetCenter(), diameter * 0.5f, color.U32(), 40);
    // 首字母缩写使用 Subtitle2（14sp/Medium），比正文更强调一些。
    const ScopedTextStyle text_style(TextStyle::Subtitle2);
    const ImVec2 text_size = ImGui::CalcTextSize(initials);
    window->DrawList->AddText(rect.GetCenter() - text_size * 0.5f,
                              AccessibleOnColor(color).U32(), initials);
}

bool BeginTopAppBar(const char* id, const char* title,
                    bool* navigation_clicked, const char* navigation_icon,
                    ImVec2 size) {
    NewFrame();
    const Theme& theme = GetTheme();
    const float dpi = DpiScale();
    if (size.x <= 0.0f) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = Metrics::AppBarHeight();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ElevationShadow(ImGui::GetWindowDrawList(), min, min + size, 0.0f, 4);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.app_bar.Vec4());
    ImGui::PushStyleColor(ImGuiCol_Text, theme.on_app_bar.Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f * dpi, 8.0f * dpi));
    // AlwaysUseWindowPadding: 无边框 child window 默认不应用 WindowPadding。
    ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    bool clicked = false;
    if (navigation_clicked) {
        clicked = IconButton("##navigation", navigation_icon ? navigation_icon : "\xee\x97\x92",
                    false, true, Metrics::IconButtonSize());
        *navigation_clicked = clicked;
        ImGui::SameLine(0.0f, 8.0f * dpi);
    } else {
        ImGui::SetCursorPosX(12.0f * dpi);
    }
    // 应用栏标题使用 Headline6（20sp/Medium），MD2 Top App Bar 的标题层级。
    {
        const ScopedTextStyle title_style(TextStyle::Headline6);
        ImGui::SetCursorPosY(std::max(0.0f, (size.y - ImGui::GetFontSize()) * 0.5f));
        ImGui::TextUnformatted(title);
    }
    ImGui::SameLine();
    return true;
}

void EndTopAppBar() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

bool BeginNavigationDrawer(const char* id, bool* open, float width) {
    NewFrame();
    IM_ASSERT(open != nullptr);
    ImGuiWindow* parent = ImGui::GetCurrentWindow();
    const Theme& theme = GetTheme();
    const ImGuiID animation_id = parent->GetID(id);
    const float current_width = GetAnimator().Animate(Salt(animation_id, "drawer"),
                                                      *open ? width : 0.0f,
                                                      *open ? 0.225f : 0.195f,
                                                      *open ? Easing::Deceleration
                                                            : Easing::Acceleration);
    if (current_width <= 0.5f) return false;
    const ImVec2 size(current_width, ImGui::GetContentRegionAvail().y);
    const ImVec2 minimum = ImGui::GetCursorScreenPos();
    ElevationShadow(parent->DrawList, minimum, minimum + size, 0.0f, 16);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, SurfaceForElevation(theme, 16).Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    // AlwaysUseWindowPadding: 无边框 child window 默认不应用 WindowPadding。
    ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar);
    return true;
}

void EndNavigationDrawer() {
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void OpenDialog(const char* id) { ImGui::OpenPopup(id); }

bool BeginDialog(const char* id, bool* open, ImGuiWindowFlags flags) {
    NewFrame();
    const Theme& theme = GetTheme();
    ImGui::PushStyleColor(ImGuiCol_PopupBg, SurfaceForElevation(theme, 24).Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, theme.shapes.large);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f * DpiScale(), 20.0f * DpiScale()));
    const bool visible = ImGui::BeginPopupModal(
        id, open, flags | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    if (!visible) {
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    return visible;
}

void EndDialog() {
    ImGui::EndPopup();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

bool Snackbar(const char* id_string, const char* message, bool* open,
              const char* action, float timeout) {
    NewFrame();
    IM_ASSERT(open != nullptr);
    Context& context = GetContext();
    ImGuiWindow* parent = ImGui::GetCurrentWindow();
    const ImGuiID id = parent->GetID(id_string);
    if (!*open) {
        context.timers.erase(id);
        return false;
    }
    auto inserted = context.timers.emplace(id, ImGui::GetTime());
    const double started = inserted.first->second;
    if (timeout > 0.0f && ImGui::GetTime() - started >= timeout) {
        *open = false;
        context.timers.erase(id);
        return false;
    }
    const Theme& theme = context.theme;
    const float dpi = DpiScale();
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width = std::min(344.0f * dpi, viewport->WorkSize.x - 32.0f * dpi);
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
                                   viewport->WorkPos.y + viewport->WorkSize.y - 16.0f * dpi),
                            ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(width, Metrics::SnackbarHeight()));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.snackbar.Vec4());
    ImGui::PushStyleColor(ImGuiCol_Text, theme.on_snackbar.Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Metrics::SnackbarCornerRadius());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f * dpi, 6.0f * dpi));
    std::string window_name = std::string("##snackbar_") + id_string;
    ImGui::Begin(window_name.c_str(), nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoFocusOnAppearing);
    // Snackbar 消息文字使用 Body2（14sp/regular）。
    ImFont* message_font = theme.fonts.Get(TextStyle::Body2);
    if (message_font) ImGui::PushFont(message_font);
    ImGui::SetCursorPosY((Metrics::SnackbarHeight() - ImGui::GetFontSize()) * 0.5f);
    ImGui::TextUnformatted(message);
    if (message_font) ImGui::PopFont();
    bool action_clicked = false;
    if (action) {
        const ImVec2 action_size = ImGui::CalcTextSize(action);
        ImGui::SameLine();
        ImGui::SetCursorPosX(width - action_size.x - 32.0f * dpi);
        ImGui::SetCursorPosY(6.0f * dpi);
        Theme action_theme = theme;
        action_theme.colors.primary = theme.colors.secondary;
        action_theme.colors.on_primary = theme.colors.on_secondary;
        GetContext().theme = action_theme;
        action_clicked = TextButton(action, ImVec2(action_size.x + 24.0f * dpi, Metrics::ButtonMinHeight()));
        GetContext().theme = theme;
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    if (action_clicked) {
        *open = false;
        context.timers.erase(id);
    }
    return action_clicked;
}

void Tooltip(const char* text) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) return;
    const Theme& theme = GetTheme();
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::FromHex(0x616161, 0.96f).Vec4());
    ImGui::PushStyleColor(ImGuiCol_Text, Color::FromHex(0xffffff).Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, theme.shapes.small);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Metrics::Gap(), Metrics::DenseGap()));
    ImGui::SetTooltip("%s", text);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void Text(TextStyle style, const char* text, Color color) {
    NewFrame();
    const Theme& theme = GetTheme();
    if (color.a < 0.0f) color = theme.colors.on_surface;
    ImFont* font = theme.fonts.Get(style);
    if (font) ImGui::PushFont(font);
    ImGui::PushStyleColor(ImGuiCol_Text, color.Vec4());
    std::string display = Typography(style).uppercase ? Uppercase(text) : std::string(text);
    ImGui::TextUnformatted(display.c_str());
    ImGui::PopStyleColor();
    if (font) ImGui::PopFont();
}

void TextH1(const char* text) { Text(TextStyle::Headline1, text); }
void TextH2(const char* text) { Text(TextStyle::Headline2, text); }
void TextH3(const char* text) { Text(TextStyle::Headline3, text); }
void TextH4(const char* text) { Text(TextStyle::Headline4, text); }
void TextH5(const char* text) { Text(TextStyle::Headline5, text); }
void TextH6(const char* text) { Text(TextStyle::Headline6, text); }
void TextSubtitle1(const char* text) { Text(TextStyle::Subtitle1, text); }
void TextSubtitle2(const char* text) { Text(TextStyle::Subtitle2, text); }
void TextBody1(const char* text) { Text(TextStyle::Body1, text); }
void TextBody2(const char* text) { Text(TextStyle::Body2, text); }
void TextCaption(const char* text) { Text(TextStyle::Caption, text); }
void TextOverline(const char* text) { Text(TextStyle::Overline, text); }

Color clear_color() { return GetTheme().colors.background; }

} // namespace ImGuiMD2
