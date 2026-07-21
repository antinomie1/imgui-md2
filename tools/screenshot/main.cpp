// Dev-only tool: renders every imgui-md2 component once, crops each capture
// to the widget's own item rect (plus a small padding margin) and writes a
// PNG per control into docs/screenshots/. Not part of the shipped library.
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_md2/imgui_md2.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// The imgui package build defines GLFW_INCLUDE_NONE project-wide (it manages
// GL entry points itself via imgui_impl_opengl3's loader). This tool only
// needs the legacy GL1.1 entry points (glViewport/glClear/glReadPixels, all
// exported directly by opengl32.lib), so undo that and let GLFW pull in the
// plain <GL/gl.h> declarations instead.
#undef GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "png_writer.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

using namespace ImGuiMD2;

namespace {

GLFWwindow* g_window = nullptr;
std::string g_out_dir;

struct PendingShot {
    std::string file;
    ImVec2 min;
    ImVec2 max;
};
std::vector<PendingShot> g_pending;

ImVec2 UnionMin(ImVec2 a, ImVec2 b) { return ImVec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
ImVec2 UnionMax(ImVec2 a, ImVec2 b) { return ImVec2(std::max(a.x, b.x), std::max(a.y, b.y)); }

void QueueShot(const std::string& file, ImVec2 mn, ImVec2 mx) {
    g_pending.push_back({file, mn, mx});
}

constexpr ImGuiWindowFlags kCanvasFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus;

void BeginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    // Fixed, generous delta time: every MD2 state transition is at most
    // 0.3s, and the animator snaps directly to target on an id's first
    // touch anyway, so this only matters for the hover-delay timer the
    // tooltip capture relies on.
    io.DeltaTime = 0.5f;
    // Keep the capture deterministic regardless of where the real cursor
    // happens to be; shots that need hover set io.MousePos explicitly
    // afterwards, every frame, before ImGui::NewFrame().
    io.MousePos = ImVec2(-9999.0f, -9999.0f);
    for (bool& down : io.MouseDown) down = false;
}

void BeginImGuiFrame() {
    ImGui::NewFrame();
    ImGuiMD2::NewFrame();
}

void BeginCanvas(const char* id = "##canvas") {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, GetTheme().colors.background.Vec4());
    ImGui::Begin(id, nullptr, kCanvasFlags);
}

void EndCanvas() {
    ImGui::End();
    ImGui::PopStyleColor();
}

// Renders the queued draw list, reads back the pixels for every pending
// shot and writes each crop to its own PNG, then clears the queue.
void EndFrameAndCapture() {
    ImGui::Render();
    int fb_w = 0, fb_h = 0;
    glfwGetFramebufferSize(g_window, &fb_w, &fb_h);
    glViewport(0, 0, fb_w, fb_h);
    const Color bg = GetTheme().colors.background;
    glClearColor(bg.r, bg.g, bg.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    constexpr float kPadding = 12.0f;
    for (const PendingShot& shot : g_pending) {
        ImVec2 mn(std::max(0.0f, shot.min.x - kPadding), std::max(0.0f, shot.min.y - kPadding));
        ImVec2 mx(shot.max.x + kPadding, shot.max.y + kPadding);
        int x0 = std::clamp(static_cast<int>(mn.x * scale.x), 0, fb_w);
        int y0 = std::clamp(static_cast<int>(mn.y * scale.y), 0, fb_h);
        int x1 = std::clamp(static_cast<int>(std::ceil(mx.x * scale.x)), 0, fb_w);
        int y1 = std::clamp(static_cast<int>(std::ceil(mx.y * scale.y)), 0, fb_h);
        const int w = std::max(1, x1 - x0);
        const int h = std::max(1, y1 - y0);

        std::vector<unsigned char> raw(static_cast<size_t>(w) * h * 4);
        glReadPixels(x0, fb_h - y1, w, h, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());

        std::vector<unsigned char> flipped(raw.size());
        for (int row = 0; row < h; ++row) {
            std::memcpy(&flipped[static_cast<size_t>(row) * w * 4],
                        &raw[static_cast<size_t>(h - 1 - row) * w * 4],
                        static_cast<size_t>(w) * 4);
        }
        const std::string path = g_out_dir + "/" + shot.file;
        ScreenshotTool::WritePng(path, w, h, flipped.data());
        std::printf("wrote %s (%dx%d)\n", path.c_str(), w, h);
    }
    g_pending.clear();
    glfwSwapBuffers(g_window);
}

// --- individual shots -------------------------------------------------

void ShotButtons() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    ImGui::SetCursorPos(ImVec2(40, 40));
    TextButton("TEXT");
    QueueShot("button_text.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SetCursorPos(ImVec2(40, 110));
    OutlinedButton("OUTLINED");
    QueueShot("button_outlined.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SetCursorPos(ImVec2(40, 180));
    ButtonOptions options;
    options.leading_icon = Icons::Add;
    Button("CONTAINED", options);
    QueueShot("button_contained.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SetCursorPos(ImVec2(40, 260));
    IconButton("icon-button-demo", Icons::Favorite);
    QueueShot("icon_button.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SetCursorPos(ImVec2(40, 330));
    FloatingActionButton("fab-demo", Icons::Add);
    QueueShot("fab.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SetCursorPos(ImVec2(140, 330));
    FloatingActionButton("fab-extended-demo", Icons::Add, "CREATE");
    QueueShot("fab_extended.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    EndCanvas();
    EndFrameAndCapture();
}

void ShotSelectionControls() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();

    ImGui::SetCursorPos(ImVec2(40, 40));
    bool unchecked = false;
    Checkbox("Unchecked", &unchecked);
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    ImGui::SetCursorPos(ImVec2(40, 90));
    bool checked = true;
    Checkbox("Checked", &checked);
    mn = UnionMin(mn, ImGui::GetItemRectMin());
    mx = UnionMax(mx, ImGui::GetItemRectMax());
    QueueShot("checkbox.png", mn, mx);

    ImGui::SetCursorPos(ImVec2(40, 150));
    int radio_value = 0;
    RadioButton("Option A", &radio_value, 0);
    mn = ImGui::GetItemRectMin();
    mx = ImGui::GetItemRectMax();
    ImGui::SetCursorPos(ImVec2(40, 200));
    RadioButton("Option B", &radio_value, 1);
    mn = UnionMin(mn, ImGui::GetItemRectMin());
    mx = UnionMax(mx, ImGui::GetItemRectMax());
    QueueShot("radio_button.png", mn, mx);

    ImGui::SetCursorPos(ImVec2(40, 260));
    bool switch_off = false;
    Switch("Off", &switch_off);
    mn = ImGui::GetItemRectMin();
    mx = ImGui::GetItemRectMax();
    ImGui::SetCursorPos(ImVec2(40, 310));
    bool switch_on = true;
    Switch("On", &switch_on);
    mn = UnionMin(mn, ImGui::GetItemRectMin());
    mx = UnionMax(mx, ImGui::GetItemRectMax());
    QueueShot("switch.png", mn, mx);

    ImGui::SetCursorPos(ImVec2(40, 380));
    ImGui::PushItemWidth(220.0f);
    float slider_value = 0.65f;
    SliderFloat("Volume", &slider_value, 0.0f, 1.0f);
    ImGui::PopItemWidth();
    QueueShot("slider.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    EndCanvas();
    EndFrameAndCapture();
}

void ShotFields() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();

    ImGui::SetCursorPos(ImVec2(40, 40));
    ImGui::PushItemWidth(240.0f);
    char filled_buffer[64] = "Ada Lovelace";
    TextFieldOptions filled_options;
    filled_options.variant = TextFieldVariant::Filled;
    filled_options.leading_icon = Icons::Person;
    filled_options.helper_text = "Full name";
    // TextField() ends with a Dummy() to reserve room for the helper text
    // when one is set, so GetItemRectMin/Max() after the call would only
    // return that trailing Dummy's tiny rect. Span the cursor position
    // before/after the call (full field + helper text) instead.
    const ImVec2 filled_top_left = ImGui::GetCursorScreenPos();
    TextField("Name", filled_buffer, sizeof(filled_buffer), filled_options);
    const ImVec2 filled_bottom = ImGui::GetCursorScreenPos();
    QueueShot("textfield_filled.png", filled_top_left,
              ImVec2(filled_top_left.x + 240.0f, filled_bottom.y));

    ImGui::SetCursorPos(ImVec2(40, 170));
    char outlined_buffer[64] = "";
    TextFieldOptions outlined_options;
    outlined_options.variant = TextFieldVariant::Outlined;
    outlined_options.leading_icon = Icons::Search;
    TextField("Search", outlined_buffer, sizeof(outlined_buffer), outlined_options);
    QueueShot("textfield_outlined.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopItemWidth();

    ImGui::SetCursorPos(ImVec2(40, 260));
    static const char* kItems[] = {"Compact", "Comfortable", "Spacious"};
    int current = 1;
    // Select() draws a caption label above the button as a separate item, so
    // GetItemRectMin/Max() (the button only) would crop the label off — span
    // the cursor position before/after instead, as with the filled TextField.
    const ImVec2 select_top_left = ImGui::GetCursorScreenPos();
    Select("Density", &current, kItems, 3, true, 220.0f);
    const ImVec2 select_bottom = ImGui::GetCursorScreenPos();
    QueueShot("select.png", select_top_left, ImVec2(select_top_left.x + 220.0f, select_bottom.y));

    EndCanvas();
    EndFrameAndCapture();
}

void ShotProgress() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();

    ImGui::SetCursorPos(ImVec2(40, 40));
    ImGui::PushItemWidth(240.0f);
    LinearProgress(0.62f, ImVec2(240.0f, 4.0f));
    QueueShot("linear_progress.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SetCursorPos(ImVec2(40, 90));
    LinearProgressIndeterminate("linear-indeterminate-demo", ImVec2(240.0f, 4.0f));
    QueueShot("linear_progress_indeterminate.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopItemWidth();

    ImGui::SetCursorPos(ImVec2(40, 140));
    CircularProgress("circular-demo", 0.7f);
    QueueShot("circular_progress.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    ImGui::SetCursorPos(ImVec2(140, 140));
    CircularProgress("circular-indeterminate-demo");
    QueueShot("circular_progress_indeterminate.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    EndCanvas();
    EndFrameAndCapture();
}

void ShotCardAndList() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();

    ImGui::SetCursorPos(ImVec2(40, 40));
    BeginCard("screenshot-card", ImVec2(360.0f, 150.0f), 1, false);
    TextH6("Card title");
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    TextBody2("Supporting text that spans a couple of\nlines to show card layout and elevation.");
    ImVec2 card_min = ImGui::GetWindowPos();
    ImVec2 card_max = card_min + ImGui::GetWindowSize();
    EndCard();
    QueueShot("card.png", card_min, card_max);

    // ListItem/Divider always span ImGui::GetContentRegionAvail().x (a full
    // row), so bound them to a child window instead of the whole canvas —
    // otherwise the crop stretches across nearly the entire capture surface.
    ImGui::SetCursorPos(ImVec2(40, 220));
    ImGui::BeginChild("list-demo-bounds", ImVec2(360.0f, 96.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ListItem("Inbox", "12 unread messages", Icons::Home, "09:41");
    QueueShot("list_item.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(40, 340));
    ImGui::BeginChild("divider-demo-bounds", ImVec2(360.0f, 8.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    Divider();
    QueueShot("divider.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::EndChild();

    EndCanvas();
    EndFrameAndCapture();
}

void ShotChipsAndTabs() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();

    ImGui::SetCursorPos(ImVec2(40, 40));
    bool chip_selected = false;
    Chip("Filter", &chip_selected, Icons::Search);
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 16.0f);
    bool chip_selected2 = true;
    Chip("Selected", &chip_selected2);
    mn = UnionMin(mn, ImGui::GetItemRectMin());
    mx = UnionMax(mx, ImGui::GetItemRectMax());
    QueueShot("chip.png", mn, mx);

    ImGui::SetCursorPos(ImVec2(40, 90));
    bool dismissed = false;
    DismissibleChip("Removable", &dismissed, Icons::Star);
    QueueShot("dismissible_chip.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    // Tabs(), like ListItem/Divider, sizes itself from
    // GetContentRegionAvail().x rather than the pushed item width, so it
    // needs to be bounded by a child window too.
    ImGui::SetCursorPos(ImVec2(40, 150));
    ImGui::BeginChild("tabs-demo-bounds", ImVec2(360.0f, 72.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    static const char* kTabs[] = {"RECENTS", "FAVORITES", "NEARBY"};
    int current_tab = 0;
    // Tabs() calls ItemAdd() per-tab, so GetItemRectMin/Max() after the call
    // would only return the *last* tab's rect. It does ItemSize() once for
    // the whole bar though, so the cursor position before/after spans it.
    const ImVec2 tabs_top_left = ImGui::GetCursorScreenPos();
    Tabs("tabs-demo", kTabs, 3, &current_tab);
    const ImVec2 tabs_bottom = ImGui::GetCursorScreenPos();
    ImGui::EndChild();
    QueueShot("tabs.png", tabs_top_left, ImVec2(tabs_top_left.x + 360.0f, tabs_bottom.y));

    EndCanvas();
    EndFrameAndCapture();
}

void ShotBadgeAndAvatar() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();

    ImGui::SetCursorPos(ImVec2(40, 40));
    IconButton("badge-anchor-demo", Icons::Info);
    ImVec2 anchor_min = ImGui::GetItemRectMin();
    ImVec2 anchor_max = ImGui::GetItemRectMax();
    Badge("3", ImVec2(anchor_max.x - 6.0f, anchor_min.y + 6.0f));
    QueueShot("badge.png", ImVec2(anchor_min.x, anchor_min.y - 6.0f),
              ImVec2(anchor_max.x + 6.0f, anchor_max.y));

    ImGui::SetCursorPos(ImVec2(140, 40));
    Avatar("avatar-demo", "AB", 48.0f);
    QueueShot("avatar.png", ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    EndCanvas();
    EndFrameAndCapture();
}

void ShotTypography() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    // SetCursorPos() only offsets the very next item — every following line
    // resets its x back to the window's normal left margin on wrap, not to
    // the position we set. Indent() persists across lines instead, so every
    // TextStyle shares the same left edge our crop below assumes.
    ImGui::SetCursorPosY(40.0f);
    ImGui::Indent(40.0f);
    ImVec2 mn = ImGui::GetCursorScreenPos();
    TextH1("Headline 1");
    TextH2("Headline 2");
    TextH3("Headline 3");
    TextH4("Headline 4");
    TextH5("Headline 5");
    TextH6("Headline 6");
    TextSubtitle1("Subtitle 1");
    TextSubtitle2("Subtitle 2");
    TextBody1("Body 1 — the quick brown fox jumps over the lazy dog");
    TextBody2("Body 2 — the quick brown fox jumps over the lazy dog");
    TextCaption("Caption text");
    TextOverline("Overline text");
    ImVec2 mx = ImGui::GetCursorScreenPos();
    mx.x = mn.x + 420.0f;
    QueueShot("typography.png", mn, mx);
    EndCanvas();
    EndFrameAndCapture();
}

void ShotElevation() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    const int levels[] = {1, 2, 4, 8, 16, 24};
    ImVec2 origin = ImVec2(60.0f, 60.0f);
    const float box = 64.0f;
    const float gap = 32.0f;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    for (int i = 0; i < 6; ++i) {
        ImVec2 mn(origin.x + i * (box + gap), origin.y);
        ImVec2 mx = mn + ImVec2(box, box);
        ElevationShadow(draw, mn, mx, 8.0f, levels[i]);
        draw->AddRectFilled(mn, mx, GetTheme().colors.surface.U32(), 8.0f);
    }
    ImVec2 total_min = origin;
    ImVec2 total_max = ImVec2(origin.x + 6 * (box + gap) - gap, origin.y + box);
    QueueShot("elevation.png", total_min, total_max);
    EndCanvas();
    EndFrameAndCapture();
}

void ShotTopAppBar() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    ImGui::SetCursorPos(ImVec2(40, 40));
    bool nav_clicked = false;
    BeginTopAppBar("top-app-bar-demo", "imgui-md2", &nav_clicked, Icons::Menu, ImVec2(480.0f, 56.0f));
    ImVec2 bar_min = ImGui::GetWindowPos();
    ImVec2 bar_max = bar_min + ImGui::GetWindowSize();
    EndTopAppBar();
    QueueShot("top_app_bar.png", bar_min, bar_max);
    EndCanvas();
    EndFrameAndCapture();
}

void ShotNavigationDrawer() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    ImGui::SetCursorPos(ImVec2(40, 40));
    // BeginNavigationDrawer() fills GetContentRegionAvail().y of its parent;
    // bound that to a child window instead of the whole canvas, or the crop
    // stretches the full capture height.
    ImGui::BeginChild("drawer-demo-bounds", ImVec2(256.0f, 420.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    bool open = true;
    BeginNavigationDrawer("nav-drawer-demo", &open, 256.0f);
    ImVec2 drawer_min = ImGui::GetWindowPos();
    ImVec2 drawer_max = drawer_min + ImGui::GetWindowSize();
    ListItem("Inbox", nullptr, Icons::Home, nullptr, true);
    ListItem("Starred", nullptr, Icons::Star);
    ListItem("Settings", nullptr, Icons::Settings);
    EndNavigationDrawer();
    ImGui::EndChild();
    QueueShot("navigation_drawer.png", drawer_min, drawer_max);
    EndCanvas();
    EndFrameAndCapture();
}

// BeginDialog() forces ImGuiWindowFlags_AlwaysAutoResize, and ImGui only
// knows a freshly-opened auto-resize window's content size *after* it has
// rendered once — the very first frame comes back tiny. Render once to let
// it measure, then render again and capture.
void ShotDialog(bool capture) {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    OpenDialog("dialog-demo");
    // NoTitleBar: MD2 dialogs render their own headline text as the first
    // line of body content (below) rather than an OS-style title bar/close
    // button.
    if (BeginDialog("dialog-demo", nullptr, ImGuiWindowFlags_NoTitleBar)) {
        TextH6("Delete file?");
        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        TextBody2("This action cannot be undone.");
        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        TextButton("CANCEL");
        ImGui::SameLine();
        TextButton("DELETE");
        ImVec2 dialog_min = ImGui::GetWindowPos();
        ImVec2 dialog_max = dialog_min + ImGui::GetWindowSize();
        EndDialog();
        if (capture) QueueShot("dialog.png", dialog_min, dialog_max);
    }
    EndCanvas();
    EndFrameAndCapture();
}

void ShotSnackbar() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    bool open = true;
    Snackbar("snackbar-demo", "Message sent", &open, "UNDO");
    if (ImGuiWindow* window = ImGui::FindWindowByName("##snackbar_snackbar-demo")) {
        QueueShot("snackbar.png", window->Pos, window->Pos + window->Size);
    }
    EndCanvas();
    EndFrameAndCapture();
}

void ShotTooltip() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    ImGui::SetCursorPos(ImVec2(60, 60));
    IconButton("tooltip-anchor-demo", Icons::Info);
    const ImVec2 anchor_min = ImGui::GetItemRectMin();
    const ImVec2 anchor_max = ImGui::GetItemRectMax();

    // Mirrors ImGuiMD2::Tooltip()'s styling directly instead of the real
    // hover-delay path, so a single frame is enough for a deterministic
    // capture.
    const Theme& theme = GetTheme();
    const char* const tooltip_text = "Search by title";
    // AlwaysAutoResize windows only know their content size after a prior
    // frame has rendered it, so a freshly-opened one reports a near-empty
    // size on frame one (see the Dialog capture for the same issue). Size
    // this window explicitly instead, since the content is a single
    // known line of text — no warmup frame needed.
    const ImVec2 text_size = ImGui::CalcTextSize(tooltip_text);
    const ImVec2 padding(Metrics::Gap(), Metrics::DenseGap());
    const float tooltip_height = text_size.y + padding.y * 2.0f;
    // Center the bubble on the anchor's full touch-target rect, not its
    // top edge — IconButton's item rect is the 48dp touch target, much
    // taller than the glyph or this single-line bubble, so top-aligning
    // the two left the bubble visibly floating above the icon it points to.
    ImGui::SetNextWindowPos(ImVec2(anchor_max.x + Metrics::Gap(),
                                   anchor_min.y + (anchor_max.y - anchor_min.y - tooltip_height) * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(text_size.x + padding.x * 2.0f, tooltip_height));
    // A plain ImGui::Begin() window is styled by WindowBg, not PopupBg (the
    // real Tooltip() uses BeginTooltip(), which does draw from PopupBg) —
    // push WindowBg here or this inherits BeginCanvas()'s light background,
    // leaving near-white text on a near-white window.
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Color::FromHex(0x616161, 0.96f).Vec4());
    ImGui::PushStyleColor(ImGuiCol_Text, Color::FromHex(0xffffff).Vec4());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, theme.shapes.small);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::Begin("##tooltip-demo", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoResize);
    ImGui::TextUnformatted(tooltip_text);
    const ImVec2 tooltip_min = ImGui::GetWindowPos();
    const ImVec2 tooltip_max = tooltip_min + ImGui::GetWindowSize();
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    QueueShot("tooltip.png", UnionMin(anchor_min, tooltip_min), UnionMax(anchor_max, tooltip_max));
    EndCanvas();
    EndFrameAndCapture();
}

void ShotPalette() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    ImGui::SetCursorPos(ImVec2(24, 24));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    static const char* kNames[] = {"Red",    "Pink",       "Purple", "DeepPurple", "Indigo",
                                   "Blue",   "LightBlue",  "Cyan",   "Teal",       "Green",
                                   "LightGreen", "Lime",   "Yellow", "Amber",      "Orange",
                                   "DeepOrange", "Brown",  "Grey",   "BlueGrey"};
    static const Shade kShades[] = {Shade::S50,  Shade::S100, Shade::S200, Shade::S300,
                                    Shade::S400, Shade::S500, Shade::S600, Shade::S700,
                                    Shade::S800, Shade::S900, Shade::A100, Shade::A200,
                                    Shade::A400, Shade::A700};
    constexpr float kBox = 26.0f;
    constexpr float kGap = 3.0f;
    constexpr float kLabelWidth = 96.0f;
    constexpr float kRowHeight = kBox + kGap;

    ImFont* label_font = GetTheme().fonts.Get(TextStyle::Caption);
    if (label_font) ImGui::PushFont(label_font);
    for (int row = 0; row < 19; ++row) {
        const float y = origin.y + row * kRowHeight;
        draw->AddText(ImVec2(origin.x, y + (kBox - ImGui::GetFontSize()) * 0.5f),
                      GetTheme().colors.on_background.U32(), kNames[row]);
        for (int col = 0; col < 14; ++col) {
            const Color color = Palette(static_cast<Swatch>(row), kShades[col]);
            const ImVec2 mn(origin.x + kLabelWidth + col * (kBox + kGap), y);
            const ImVec2 mx = mn + ImVec2(kBox, kBox);
            draw->AddRectFilled(mn, mx, color.U32());
        }
    }
    if (label_font) ImGui::PopFont();
    const ImVec2 total_max(origin.x + kLabelWidth + 14 * (kBox + kGap) - kGap,
                           origin.y + 19 * kRowHeight - kGap);
    // Register an item spanning the manually drawn area — otherwise ImGui
    // logs a "SetCursorPos extends window boundaries" warning, since the
    // draw-list calls above never advanced the window's tracked content size.
    ImGui::Dummy(total_max - origin);
    QueueShot("palette.png", origin, total_max);
    EndCanvas();
    EndFrameAndCapture();
}

void ShotThemes() {
    BeginFrame();
    BeginImGuiFrame();
    BeginCanvas();
    ImGui::SetCursorPos(ImVec2(24, 24));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    constexpr int kColumns = 4;
    constexpr float kCardW = 168.0f;
    constexpr float kCardH = 76.0f;
    constexpr float kGap = 10.0f;
    const int count = Theme::named_count();
    const char* const* ids = Theme::named_ids();

    ImFont* label_font = GetTheme().fonts.Get(TextStyle::Caption);
    if (label_font) ImGui::PushFont(label_font);
    for (int i = 0; i < count; ++i) {
        const int col = i % kColumns;
        const int row = i / kColumns;
        const ImVec2 mn(origin.x + col * (kCardW + kGap), origin.y + row * (kCardH + kGap));
        const ImVec2 mx = mn + ImVec2(kCardW, kCardH);
        const Theme t = Theme::Named(ids[i]);
        draw->AddRectFilled(mn, mx, t.colors.background.U32(), 6.0f);
        draw->AddRect(mn, mx, t.colors.outline.U32(), 6.0f);
        draw->AddRectFilled(mn + ImVec2(10, 10), mn + ImVec2(34, 34), t.colors.primary.U32(), 4.0f);
        draw->AddRectFilled(mn + ImVec2(38, 10), mn + ImVec2(62, 34), t.colors.secondary.U32(), 4.0f);
        draw->AddText(ImVec2(mn.x + 10.0f, mn.y + 42.0f), t.colors.on_background.U32(), ids[i]);
    }
    if (label_font) ImGui::PopFont();
    const int rows = (count + kColumns - 1) / kColumns;
    const ImVec2 total_max(origin.x + kColumns * (kCardW + kGap) - kGap,
                           origin.y + rows * (kCardH + kGap) - kGap);
    ImGui::Dummy(total_max - origin);
    QueueShot("themes.png", origin, total_max);
    EndCanvas();
    EndFrameAndCapture();
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    g_window = glfwCreateWindow(1000, 900, "imgui-md2 screenshot tool", nullptr, nullptr);
    if (!g_window) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    CreateContext();
    Theme theme = Theme::Light(Palette(Swatch::Indigo, Shade::S500),
                               Palette(Swatch::Pink, Shade::A200));
    std::string font_error;
    if (!LoadBundledFonts(*io.Fonts, theme.fonts, "", {}, &font_error)) {
        std::fprintf(stderr, "LoadBundledFonts: %s\n", font_error.c_str());
    }
    SetTheme(theme);

    const std::filesystem::path out_dir =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() /
        "docs" / "screenshots";
    std::filesystem::create_directories(out_dir);
    g_out_dir = out_dir.string();

    ShotButtons();
    ShotSelectionControls();
    ShotFields();
    ShotProgress();
    ShotCardAndList();
    ShotChipsAndTabs();
    ShotBadgeAndAvatar();
    ShotTypography();
    ShotElevation();
    ShotTopAppBar();
    ShotNavigationDrawer();
    ShotDialog(false);
    ShotDialog(true);
    ShotSnackbar();
    ShotTooltip();
    ShotPalette();
    ShotThemes();

    ImGuiMD2::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(g_window);
    glfwTerminate();
    std::printf("done: %s\n", g_out_dir.c_str());
    return 0;
}
