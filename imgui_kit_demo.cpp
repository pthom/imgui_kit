// imgui_kit: the demo
// https://github.com/pthom/imgui_kit (MIT license)
//
// One collapsing header per topic, in the order of imgui_kit.h. Each section is marked with IMGUI_DEMO_MARKER,
// as in imgui_demo.cpp: a tool can show the code of the section under the mouse.

/*

Index of this file:
// [SECTION] Helpers
// [SECTION] Em units
// [SECTION] Demo window

*/

#include "imgui_kit.h"
#include "imgui_internal.h"     // ImGui::DemoMarker()

#ifndef IMGUI_KIT_DISABLE_DEMO

#define IMGUI_DEMO_MARKER(section)  do { ImGui::DemoMarker("imgui_kit_demo.cpp", __LINE__, section); } while (0)

namespace ImGuiKit
{
namespace
{

//-----------------------------------------------------------------------------
// [SECTION] Helpers
//-----------------------------------------------------------------------------

void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Em units
//-----------------------------------------------------------------------------

void DemoEmUnits()
{
    IMGUI_DEMO_MARKER("Em units");
    if (!ImGui::CollapsingHeader("Em units"))
        return;
    ImGui::TextWrapped("Sizes in em follow the font: this button is 8 em wide and 2 em high, on any screen.");
    ImGui::SameLine();
    HelpMarker("EmToVec2(8, 2) gives the size in pixels. Change the font size: the button follows it.");
    ImGui::Button("8 x 2 em", EmToVec2(8.f, 2.f));
    ImGui::Text("1 em = %.1f px", EmSize());
}

} // namespace

//-----------------------------------------------------------------------------
// [SECTION] Demo window
//-----------------------------------------------------------------------------

void ShowDemoWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(EmToVec2(36.f, 30.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("imgui_kit Demo", p_open))
    {
        ImGui::End();
        return;
    }
    ImGui::Text("imgui_kit %s: small, standalone additions to Dear ImGui", IMGUI_KIT_VERSION);
    ImGui::Separator();
    DemoEmUnits();
    ImGui::End();
}

} // namespace ImGuiKit

#else

void ImGuiKit::ShowDemoWindow(bool*) {}

#endif // #ifndef IMGUI_KIT_DISABLE_DEMO
