// imgui_kit: small, standalone additions to Dear ImGui
// https://github.com/pthom/imgui_kit (MIT license)
//
// Two files: imgui_kit.h and imgui_kit.cpp, beside Dear ImGui. imgui_kit_demo.cpp holds the demo (optional).
// They need only Dear ImGui (its docking or master branch, 1.93 or later).

/*

Index of this file:
// [SECTION] Header mess
// [SECTION] Demo
// [SECTION] Em units

*/

#pragma once

//-----------------------------------------------------------------------------
// [SECTION] Header mess
//-----------------------------------------------------------------------------

#define IMGUI_KIT_VERSION       "0.1.0"
#define IMGUI_KIT_VERSION_NUM   100     // major * 10000 + minor * 100 + patch

#include "imgui.h"

namespace ImGuiKit
{

//-----------------------------------------------------------------------------
// [SECTION] Demo
//-----------------------------------------------------------------------------

// The kit's demo window, as ImGui::ShowDemoWindow(). Removed by IMGUI_KIT_DISABLE_DEMO.
IMGUI_API void ShowDemoWindow(bool* p_open = nullptr);

//-----------------------------------------------------------------------------
// [SECTION] Em units
//-----------------------------------------------------------------------------
// Sizes in multiples of the current font size, as the em unit of CSS: they follow the font,
// so they stay right on high-DPI screens and when the user zooms the text.

IMGUI_API float  EmSize();                          // The current font size, in pixels
IMGUI_API float  EmSize(float nb_lines);            // The height of nb_lines lines of text, in pixels
IMGUI_API ImVec2 EmToVec2(float x, float y);        // A size or a position in em, in pixels
IMGUI_API ImVec2 EmToVec2(ImVec2 v);
IMGUI_API ImVec2 PixelsToEm(ImVec2 pixels);         // Pixels, in em
IMGUI_API float  PixelSizeToEm(float pixel_size);

} // namespace ImGuiKit
