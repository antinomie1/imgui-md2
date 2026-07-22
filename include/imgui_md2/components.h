#pragma once

#include "animation.h"
#include "theme.h"

#include <cstddef>

namespace ImGuiMD2 {

struct Context;

Context* CreateContext();
void DestroyContext(Context* context = nullptr);
Context& GetContext();
Theme& GetTheme();
void SetTheme(const Theme& theme, bool apply_imgui_style = true);
bool ApplyNamedTheme(const char* name, bool invert_secondary = false);
Animator& GetAnimator();
void NewFrame();

enum class ButtonVariant : std::uint8_t { Text, Outlined, Contained };
enum class TextFieldVariant : std::uint8_t { Filled, Outlined };

struct ButtonOptions {
    ButtonVariant variant = ButtonVariant::Contained;
    ImVec2 size{};
    const char* leading_icon = nullptr;
    bool enabled = true;
    bool full_width = false;
};

bool Button(const char* label, const ButtonOptions& options = {});
bool TextButton(const char* label, ImVec2 size = {});
bool OutlinedButton(const char* label, ImVec2 size = {});
bool ContainedButton(const char* label, ImVec2 size = {});
bool IconButton(const char* id, const char* icon, bool selected = false,
                bool enabled = true, float size = -1.0f);
struct FabOptions {
    const char* label = nullptr;
    bool enabled = true;
    // alpha < 0 is a sentinel meaning "use the theme default": secondary for the
    // container, on_secondary for the content (icon/label).
    Color container = Color(0, 0, 0, -1);
    Color content = Color(0, 0, 0, -1);
    // Resting elevation (shadow). Set to 0 for a flat FAB with no shadow at rest;
    // the shadow then only appears on hover/press via hover_elevation.
    int rest_elevation = 6;
    int hover_elevation = 12;
};

bool FloatingActionButton(const char* id, const char* icon,
                          const FabOptions& options);
bool FloatingActionButton(const char* id, const char* icon,
                          const char* label = nullptr, bool enabled = true);

bool Checkbox(const char* label, bool* value, bool enabled = true);
bool RadioButton(const char* label, int* value, int button_value,
                 bool enabled = true);
bool Switch(const char* label, bool* value, bool enabled = true);
bool SliderFloat(const char* label, float* value, float minimum, float maximum,
                 const char* format = "%.2f", bool enabled = true);

struct TextFieldOptions {
    TextFieldVariant variant = TextFieldVariant::Filled;
    const char* helper_text = nullptr;
    const char* leading_icon = nullptr;
    const char* trailing_icon = nullptr;
    bool error = false;
    bool enabled = true;
    ImVec2 size{};
};

bool TextField(const char* label, char* buffer, std::size_t buffer_size,
               const TextFieldOptions& options = {},
               ImGuiInputTextFlags flags = 0);
bool Select(const char* label, int* current, const char* const* items,
            int count, bool enabled = true, float width = 0.0f);

void LinearProgress(float fraction, ImVec2 size = ImVec2(-1.0f, 4.0f));
void LinearProgressIndeterminate(const char* id,
                                 ImVec2 size = ImVec2(-1.0f, 4.0f));
void CircularProgress(const char* id, float fraction = -1.0f,
                      float radius = 16.0f, float thickness = 3.0f);

bool BeginCard(const char* id, ImVec2 size = {}, int elevation = 1,
               bool outlined = false);
void EndCard();
bool ListItem(const char* label, const char* supporting_text = nullptr,
              const char* leading_icon = nullptr,
              const char* trailing_text = nullptr, bool selected = false,
              bool enabled = true);
void Divider(float inset = 0.0f);

bool Chip(const char* label, bool* selected = nullptr,
          const char* leading_icon = nullptr, bool enabled = true);
bool DismissibleChip(const char* label, bool* dismissed,
                     const char* leading_icon = nullptr, bool enabled = true);
bool Tabs(const char* id, const char* const* labels, int count, int* current,
          bool fixed = true);
void Badge(const char* text, ImVec2 anchor, Color color = Color(0, 0, 0, -1));
void Avatar(const char* id, const char* initials, float diameter = 40.0f,
            Color color = Color(0, 0, 0, -1));

bool BeginTopAppBar(const char* id, const char* title,
                    bool* navigation_clicked = nullptr,
                    const char* navigation_icon = nullptr,
                    ImVec2 size = ImVec2(-1.0f, -1.0f));
void EndTopAppBar();

bool BeginNavigationDrawer(const char* id, bool* open,
                           float width = 256.0f);
void EndNavigationDrawer();

void OpenDialog(const char* id);
bool BeginDialog(const char* id, bool* open = nullptr,
                 ImGuiWindowFlags flags = 0);
void EndDialog();

// Set `open` to true to start the timeout. The action result is returned and
// `open` becomes false after timeout or action. Pass timeout <= 0 to persist.
bool Snackbar(const char* id, const char* message, bool* open,
              const char* action = nullptr, float timeout = 4.0f);

void Tooltip(const char* text);
void Text(TextStyle style, const char* text,
          Color color = Color(0, 0, 0, -1));
void TextH1(const char* text);
void TextH2(const char* text);
void TextH3(const char* text);
void TextH4(const char* text);
void TextH5(const char* text);
void TextH6(const char* text);
void TextSubtitle1(const char* text);
void TextSubtitle2(const char* text);
void TextBody1(const char* text);
void TextBody2(const char* text);
void TextCaption(const char* text);
void TextOverline(const char* text);
void ElevationShadow(ImDrawList* draw_list, ImVec2 minimum, ImVec2 maximum,
                     float rounding, int elevation, float alpha = 1.0f);

} // namespace ImGuiMD2
