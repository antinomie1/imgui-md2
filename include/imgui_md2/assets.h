#pragma once

#include "theme.h"

#include <string>
#include <vector>

namespace ImGuiMD2 {

enum class TypeRole : std::uint8_t { Regular, Medium, Title };

void SetTypeface(TypeRole role, ImFont* font);
ImFont* Typeface(TypeRole role);
void PushTypeface(TypeRole role);
void PopTypeface();

// A secondary font (typically a system font discovered at runtime) merged on
// top of every text-style font. Used to extend glyph coverage — e.g. adding
// CJK glyphs from a platform font over the bundled Roboto. Merges apply to the
// text styles only, never to the Material Icons font.
//
// The bytes are referenced, not copied: the same buffer is shared across every
// text style (avoiding one multi-megabyte load per style) and is read lazily by
// ImGui's dynamic rasterizer. The caller must keep `data` alive for as long as
// the font atlas is in use.
struct FontMerge {
    const void* data = nullptr;            // font file bytes, owned by the caller
    int data_size = 0;                     // size of `data` in bytes
    const ImWchar* glyph_ranges = nullptr; // codepoints this font supplies
};

struct FontLoadOptions {
    float scale = 1.0f;
    bool full_type_scale = true;
    const ImWchar* glyph_ranges = nullptr;
    // With an empty asset_directory, use the compiled-in Regular/Icon fonts
    // when available. Set false to force filesystem loading.
    bool prefer_embedded = true;
    // Fallback fonts merged over each text style, in order. Empty by default so
    // the library has no dependency on any system font.
    std::vector<FontMerge> merge_fonts;
};

// Path compiled from IMGUI_MD2_ASSET_DIR, or "assets" when the project did
// not define one. It can be overridden at runtime.
std::string DefaultAssetDirectory();

// Adds the MD2 type scale (Roboto Light/Regular/Medium at their spec sizes)
// and Material Icons to the supplied ImGui atlas, following a three-tier
// priority chain — each tier is tried only if the previous one is
// unavailable, so the library never hard-fails just because one font source
// is missing:
//
//   1. Embedded (default). The Regular + Icons pair compiled into the
//      library, tried first whenever asset_directory is empty and
//      options.prefer_embedded is true (the default). Light/Medium are
//      included too when the library was built with
//      IMGUI_MD2_EMBED_FULL_FONTS; otherwise those weights gracefully
//      degrade to Regular while sizes/letter-spacing stay spec-correct.
//   2. Filesystem assets. Roboto-{Light,Regular,Medium}.ttf and
//      MaterialIcons-Regular.ttf read from asset_directory (or
//      DefaultAssetDirectory() when empty). Used when embedded fonts were
//      skipped or unavailable.
//   3. Host-supplied fallback. If the caller already populated `atlas`
//      with its own font(s) before calling this function (e.g. the host
//      application's UI font, or a system font it located itself) and
//      both tiers above failed, that host font is reused for every
//      TextStyle. This sacrifices the MD2 size/weight distinctions but
//      keeps the UI legible instead of failing outright.
//
// options.merge_fonts layers additional glyph coverage (e.g. a CJK system
// font discovered by the host) on top of whichever tier above provided the
// base font — this is the intended way for an application to extend
// coverage with system fonts without the library depending on any
// platform-specific font-discovery code itself.
//
// For ImGui 1.92+ renderer backends, initialize the backend and let it build
// and upload the atlas; only legacy/manual backends need an explicit Build()
// after the backend flags have been set.
bool LoadBundledFonts(ImFontAtlas& atlas, TypographyFonts& result,
                      const std::string& asset_directory = {},
                      const FontLoadOptions& options = {},
                      std::string* error = nullptr);

namespace Icons {
inline constexpr const char* Add = "\xee\x85\x85";
inline constexpr const char* ArrowBack = "\xee\x97\x84";
inline constexpr const char* ArrowForward = "\xee\x97\x88";
inline constexpr const char* Check = "\xee\x97\x8a";
inline constexpr const char* ChevronLeft = "\xee\x97\x8b";
inline constexpr const char* ChevronRight = "\xee\x97\x8c";
inline constexpr const char* Close = "\xee\x97\x8d";
inline constexpr const char* Delete = "\xee\xa1\xb2";
inline constexpr const char* Edit = "\xee\x8f\x89";
inline constexpr const char* Favorite = "\xee\xa1\xbd";
inline constexpr const char* Home = "\xee\xa2\x8a";
inline constexpr const char* Info = "\xee\xa2\x8e";
inline constexpr const char* Menu = "\xee\x97\x92";
inline constexpr const char* MoreVert = "\xee\x97\x94";
inline constexpr const char* Person = "\xee\x9f\xbd";
inline constexpr const char* Refresh = "\xee\x97\x95";
inline constexpr const char* Search = "\xee\xa2\xb6";
inline constexpr const char* Settings = "\xee\xa2\xb8";
inline constexpr const char* Star = "\xee\xa0\xb8";
inline constexpr const char* Warning = "\xee\x80\x82";
} // namespace Icons

} // namespace ImGuiMD2
