#pragma once

#include <cstddef>

namespace ImGuiMD2::EmbeddedFonts {

const unsigned char* RegularData();
std::size_t RegularSize();
const unsigned char* IconsData();
std::size_t IconsSize();
const unsigned char* LightData();
std::size_t LightSize();
const unsigned char* MediumData();
std::size_t MediumSize();
const unsigned char* BoldData();
std::size_t BoldSize();
bool HasFullTypeScale();

} // namespace ImGuiMD2::EmbeddedFonts
