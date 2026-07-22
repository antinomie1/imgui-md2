#include <imgui_md2/assets.h>
#include <imgui_md2/components.h>

#include "embedded_fonts.h"

#include <algorithm>
#include <climits>
#include <filesystem>

#ifndef IMGUI_MD2_ASSET_DIR
#define IMGUI_MD2_ASSET_DIR "assets"
#endif

namespace ImGuiMD2 {
namespace {

const char* FontFileForWeight(int weight) {
    if (weight <= 300) return "Roboto-Light.ttf";
    if (weight >= 500) return "Roboto-Medium.ttf";
    return "Roboto-Regular.ttf";
}

bool Exists(const std::filesystem::path& path, std::string* error) {
    std::error_code ec;
    if (std::filesystem::is_regular_file(path, ec)) return true;
    if (error) *error = "Font asset not found: " + path.string();
    return false;
}

ImFont* AddFont(ImFontAtlas& atlas, const std::filesystem::path& path,
                float size, const ImWchar* ranges) {
    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 1;
    config.PixelSnapH = false;
    return atlas.AddFontFromFileTTF(path.string().c_str(), size, &config, ranges);
}

ImFont* AddMemoryFont(ImFontAtlas& atlas, const unsigned char* data,
                      std::size_t data_size, float size, const ImWchar* ranges) {
    if (!data || data_size == 0 || data_size > static_cast<std::size_t>(INT_MAX))
        return nullptr;
    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 1;
    config.PixelSnapH = false;
    // The bytes live in the executable's read-only image section.
    config.FontDataOwnedByAtlas = false;
    return atlas.AddFontFromMemoryTTF(const_cast<unsigned char*>(data),
                                      static_cast<int>(data_size), size, &config,
                                      ranges);
}

// Merges each configured fallback font on top of the font added most recently,
// at the same pixel size, so a text style keeps its MD2 metrics while gaining
// the fallback's extra glyphs (e.g. CJK). Empty entries are skipped silently:
// the fallbacks are a best-effort enhancement, never a hard requirement.
void ApplyMergeFonts(ImFontAtlas& atlas, float size,
                     const std::vector<FontMerge>& merge_fonts) {
    for (const FontMerge& merge : merge_fonts) {
        if (!merge.data || merge.data_size <= 0) continue;
        ImFontConfig config;
        config.MergeMode = true;
        config.OversampleH = 2;
        config.OversampleV = 1;
        config.PixelSnapH = false;
        // The buffer is owned by the caller and shared across every style.
        config.FontDataOwnedByAtlas = false;
        atlas.AddFontFromMemoryTTF(const_cast<void*>(merge.data), merge.data_size,
                                   size, &config, merge.glyph_ranges);
    }
}

// Picks the embedded weight closest to the requested one, degrading to Regular
// when the Light/Medium faces were not compiled in. This keeps the MD2 type
// scale (correct sizes and letter-spacing) intact even in the memory-optimized
// build that only embeds Roboto Regular + Material Icons.
const unsigned char* EmbeddedWeightData(int weight, std::size_t& size_out) {
    if (weight <= 300 && EmbeddedFonts::LightData() && EmbeddedFonts::LightSize()) {
        size_out = EmbeddedFonts::LightSize();
        return EmbeddedFonts::LightData();
    }
    if (weight >= 500 && EmbeddedFonts::MediumData() && EmbeddedFonts::MediumSize()) {
        size_out = EmbeddedFonts::MediumSize();
        return EmbeddedFonts::MediumData();
    }
    size_out = EmbeddedFonts::RegularSize();
    return EmbeddedFonts::RegularData();
}

// Tier 3 of the font-loading chain: reuse whatever font the host application
// already added to the atlas before calling LoadBundledFonts (its own UI font,
// or a system font it located itself). Every TextStyle degrades to that one
// font — the MD2 size/weight distinctions are lost — but the UI stays legible
// instead of the library failing outright when neither embedded nor
// filesystem fonts are available.
bool LoadHostFallbackFonts(ImFontAtlas& atlas, TypographyFonts& result) {
    if (atlas.Fonts.Size == 0) return false;
    ImFont* host_font = atlas.Fonts[0];
    result.styles.fill(host_font);
    // The host font is not expected to carry Material Icons glyphs.
    result.icons = host_font;
    return true;
}

// Makes the Body1 text the ImGui default font. Under the full MD2 type scale
// the first font added to the atlas is Headline1 (96sp); without this, every
// widget that does not push an explicit style — buttons, switches, the app bar
// title — would inherit that giant Fonts[0] as its default.
void SetDefaultFont(const TypographyFonts& result) {
    if (!ImGui::GetCurrentContext()) return;
    if (ImFont* body = result.Get(TextStyle::Body1))
        ImGui::GetIO().FontDefault = body;
}

bool LoadEmbeddedFonts(ImFontAtlas& atlas, TypographyFonts& result,
                       const FontLoadOptions& options) {
    // Regular + Material Icons are the minimum required to render the MD2 type
    // scale; Light/Medium are optional and gracefully fall back to Regular.
    if (!EmbeddedFonts::RegularData() || EmbeddedFonts::RegularSize() == 0)
        return false;

    const float scale = std::max(options.scale, 0.25f);
    const ImWchar* ranges = options.glyph_ranges ? options.glyph_ranges
                                                  : atlas.GetGlyphRangesDefault();
    if (options.full_type_scale) {
        // 完整 MD2 类型比例：为每个 TextStyle 加载对应权重和大小的字体。
        for (std::size_t i = 0; i < result.styles.size(); ++i) {
            const TypeScale& metric = Typography(static_cast<TextStyle>(i));
            std::size_t data_size = 0;
            const unsigned char* data = EmbeddedWeightData(metric.weight, data_size);
            const float size = metric.size * scale;
            result.styles[i] = AddMemoryFont(atlas, data, data_size, size, ranges);
            if (!result.styles[i]) return false;
            ApplyMergeFonts(atlas, size, options.merge_fonts);
        }
    } else {
        // 简化模式：所有 TextStyle 共享单一 Body1 字体（16sp）。
        const TypeScale& body1_metric = Typography(TextStyle::Body1);
        const float size = body1_metric.size * scale;
        ImFont* body = AddMemoryFont(atlas, EmbeddedFonts::RegularData(),
                                     EmbeddedFonts::RegularSize(), size, ranges);
        if (!body) return false;
        ApplyMergeFonts(atlas, size, options.merge_fonts);
        result.styles.fill(body);
    }

    // Material Icons 始终以 24sp 加载（MD2 图标规范）。
    static const ImWchar icon_ranges[] = {0xe000, 0xf8ff, 0};
    result.icons = AddMemoryFont(atlas, EmbeddedFonts::IconsData(),
                                 EmbeddedFonts::IconsSize(), 24.0f * scale,
                                 icon_ranges);
    return result.icons != nullptr;
}

} // namespace

ImFont* Typeface(TypeRole role) {
    switch (role) {
    case TypeRole::Medium:
        return GetTheme().fonts.Get(TextStyle::Subtitle2);
    case TypeRole::Title:
        return GetTheme().fonts.Get(TextStyle::Headline6);
    case TypeRole::Regular:
    default:
        return GetTheme().fonts.Get(TextStyle::Body1);
    }
}

void SetTypeface(TypeRole role, ImFont* font) {
    switch (role) {
    case TypeRole::Medium:
        GetTheme().fonts.styles[static_cast<std::size_t>(TextStyle::Subtitle2)] = font;
        break;
    case TypeRole::Title:
        GetTheme().fonts.styles[static_cast<std::size_t>(TextStyle::Headline6)] = font;
        break;
    case TypeRole::Regular:
    default:
        GetTheme().fonts.styles[static_cast<std::size_t>(TextStyle::Body1)] = font;
        break;
    }
}

void PushTypeface(TypeRole role) {
    if (ImFont* font = Typeface(role)) ImGui::PushFont(font);
}

void PopTypeface() { ImGui::PopFont(); }

std::string DefaultAssetDirectory() { return IMGUI_MD2_ASSET_DIR; }

bool LoadBundledFonts(ImFontAtlas& atlas, TypographyFonts& result,
                      const std::string& asset_directory,
                      const FontLoadOptions& options, std::string* error) {
    if (asset_directory.empty() && options.prefer_embedded &&
        LoadEmbeddedFonts(atlas, result, options)) {
        SetDefaultFont(result);
        return true;
    }
    const std::filesystem::path root =
        std::filesystem::path(asset_directory.empty() ? DefaultAssetDirectory()
                                                       : asset_directory) /
        "fonts";
    const auto regular = root / "Roboto-Regular.ttf";
    const auto medium = root / "Roboto-Medium.ttf";
    const auto light = root / "Roboto-Light.ttf";
    const auto icons = root / "MaterialIcons-Regular.ttf";
    if (!Exists(regular, error) || !Exists(medium, error) ||
        !Exists(light, error) || !Exists(icons, error)) {
        // Tier 3: neither embedded nor filesystem fonts are available. Fall
        // back to whatever the host application already loaded, if anything.
        if (LoadHostFallbackFonts(atlas, result)) {
            if (error) error->clear();
            SetDefaultFont(result);
            return true;
        }
        return false;
    }

    const float scale = std::max(options.scale, 0.25f);
    const ImWchar* ranges = options.glyph_ranges ? options.glyph_ranges
                                                  : atlas.GetGlyphRangesDefault();
    if (options.full_type_scale) {
        for (std::size_t i = 0; i < result.styles.size(); ++i) {
            const auto style = static_cast<TextStyle>(i);
            const TypeScale& metric = Typography(style);
            const auto file = root / FontFileForWeight(metric.weight);
            const float size = metric.size * scale;
            result.styles[i] = AddFont(atlas, file, size, ranges);
            if (!result.styles[i]) {
                if (error) *error = "ImGui could not load: " + file.string();
                return false;
            }
            ApplyMergeFonts(atlas, size, options.merge_fonts);
        }
    } else {
        const TypeScale& body1_metric = Typography(TextStyle::Body1);
        const float size = body1_metric.size * scale;
        ImFont* body = AddFont(atlas, regular, size, ranges);
        if (!body) {
            if (error) *error = "ImGui could not load: " + regular.string();
            return false;
        }
        ApplyMergeFonts(atlas, size, options.merge_fonts);
        result.styles.fill(body);
    }

    static const ImWchar icon_ranges[] = {0xe000, 0xf8ff, 0};
    result.icons = AddFont(atlas, icons, 24.0f * scale, icon_ranges);
    if (!result.icons) {
        if (error) *error = "ImGui could not load: " + icons.string();
        return false;
    }
    SetDefaultFont(result);
    return true;
}

ImFont* AddBoldFont(ImFontAtlas& atlas, float size, const ImWchar* glyph_ranges,
                    const std::vector<FontMerge>& merge_fonts) {
    const unsigned char* data = EmbeddedFonts::BoldData();
    std::size_t data_size = EmbeddedFonts::BoldSize();
    if (!data || data_size == 0) {
        data = EmbeddedFonts::MediumData();
        data_size = EmbeddedFonts::MediumSize();
    }
    if (!data || data_size == 0) {
        data = EmbeddedFonts::RegularData();
        data_size = EmbeddedFonts::RegularSize();
    }
    const ImWchar* ranges = glyph_ranges ? glyph_ranges : atlas.GetGlyphRangesDefault();
    ImFont* font = AddMemoryFont(atlas, data, data_size, size, ranges);
    if (!font) return nullptr;
    ApplyMergeFonts(atlas, size, merge_fonts);
    return font;
}

} // namespace ImGuiMD2
