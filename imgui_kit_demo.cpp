// imgui_kit: the demo
// https://github.com/pthom/imgui_kit (MIT license)
//
// One collapsing header per topic, in the order of imgui_kit.h. Each section is marked with IMGUI_DEMO_MARKER,
// as in imgui_demo.cpp: a tool can show the code of the section under the mouse.

/*

Index of this file:
// [SECTION] Em units
// [SECTION] Layout
// [SECTION] Widgets
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
// [SECTION] Em units
//-----------------------------------------------------------------------------

void DemoEmUnits()
{
    IMGUI_DEMO_MARKER("Em units");
    if (!ImGui::CollapsingHeader("Em units"))
        return;
    ImGui::TextWrapped("Sizes in em follow the font: this button is 8 em wide and 2 em high, on any screen.");
    ImGui::Button("8 x 2 em", EmToVec2(8.f, 2.f));
    ImGui::SameLine();
    HelpMarker("EmToVec2(8, 2) gives the size in pixels. Change the font size: the button follows it.");
    ImGui::Text("1 em = %.1f px", EmSize());
}

//-----------------------------------------------------------------------------
// [SECTION] Layout
//-----------------------------------------------------------------------------

void DemoFillRow()
{
    IMGUI_DEMO_MARKER("Layout/Fill row");
    if (!ImGui::TreeNode("Fill row"))
        return;
    ImGui::TextWrapped("The slider fills the line; the arrows and the (i) stay on the right. Resize the window.");
    static int slice = 37;
    BeginFillRow("slice");
    ImGui::SliderInt("##slice", &slice, 0, 59, "Slice %d of 59");
    FillRowTail();
    if (ImGui::ArrowButton("##prev", ImGuiDir_Left) && slice > 0)
        slice--;
    ImGui::SameLine();
    if (ImGui::ArrowButton("##next", ImGuiDir_Right) && slice < 59)
        slice++;
    ImGui::SameLine();
    HelpMarker("BeginFillRow(), the slider, FillRowTail(), the arrows and this marker, EndFillRow().");
    EndFillRow();

    static char text[64] = "An InputText fills the line too";
    BeginFillRow("input");
    ImGui::InputText("##input", text, IM_ARRAYSIZE(text));
    FillRowTail();
    ImGui::Button("Send");
    EndFillRow();
    ImGui::TreePop();
}

void DemoAlignedGroup()
{
    IMGUI_DEMO_MARKER("Layout/Aligned group");
    if (!ImGui::TreeNode("Aligned group"))
        return;
    BeginAlignedGroup("right", 1.0f);
    ImGui::Button("Cancel");
    ImGui::SameLine();
    ImGui::Button("Apply");
    EndAlignedGroup();

    BeginAlignedGroup("center", 0.5f);
    ImGui::Text("A centered group");
    EndAlignedGroup();

    BeginAlignedGroup("center_two_lines", 0.5f);
    ImGui::Text("Two lines, centered as one box:");
    ImGui::Button("OK");
    EndAlignedGroup();
    ImGui::SameLine();
    HelpMarker("The group's box is centered, not each line: the button stays at the box's left.");
    ImGui::TreePop();
}

void DemoFlow()
{
    IMGUI_DEMO_MARKER("Layout/Flow");
    if (!ImGui::TreeNode("Flow"))
        return;
    ImGui::TextWrapped("The buttons stay on a line while they fit. Resize the window: they flow to the next line.");
    static int selected = 0;
    const char* presets[] = { "Lung", "Mediastinum", "Bone", "Full range", "Brain", "Liver", "Soft tissue",
                              "Abdomen", "Spine", "Angiography", "Stroke", "Temporal bone", "Custom" };
    BeginFlow("presets");
    for (int i = 0; i < IM_ARRAYSIZE(presets); i++)
    {
        FlowNext();
        bool is_selected = (i == selected);
        if (is_selected)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(presets[i]))
            selected = i;
        if (is_selected)
            ImGui::PopStyleColor();
    }
    EndFlow();
    ImGui::TreePop();
}

void DemoLayout()
{
    IMGUI_DEMO_MARKER("Layout");
    if (!ImGui::CollapsingHeader("Layout"))
        return;
    ImGui::TextWrapped("These helpers measure their parts on the previous frame. The first frame draws them hidden.");
    DemoFillRow();
    DemoAlignedGroup();
    DemoFlow();
}

//-----------------------------------------------------------------------------
// [SECTION] Widgets
//-----------------------------------------------------------------------------

void DemoHelpMarker()
{
    IMGUI_DEMO_MARKER("Widgets/Help marker");
    if (!ImGui::TreeNode("Help marker"))
        return;
    ImGui::Text("Hover the (i), or click it to pin its text:");
    ImGui::SameLine();
    HelpMarker("With a mouse, hovering shows this text, and a click pins it until a click elsewhere. "
               "On a touch screen, a tap opens it.");
    ImGui::Text("Any content, with Begin/End:");
    ImGui::SameLine();
    if (BeginHelpMarker("##custom"))
    {
        ImGui::Text("A help with widgets:");
        ImGui::BulletText("a list");
        ImGui::Button("and a button");
        EndHelpMarker();
    }
    ImGui::TreePop();
}

void DemoIconButton()
{
    IMGUI_DEMO_MARKER("Widgets/Icon button");
    if (!ImGui::TreeNode("Icon button"))
        return;
    static float zoom = 1.0f;
    if (IconButton("##minus", ImGuiKitIcon_Minus)) zoom /= 1.25f;
    ImGui::SameLine();
    if (IconButton("##plus", ImGuiKitIcon_Plus)) zoom *= 1.25f;
    ImGui::SameLine();
    if (IconButton("##zoom_in", ImGuiKitIcon_ZoomIn)) zoom *= 1.25f;
    ImGui::SameLine();
    if (IconButton("##zoom_out", ImGuiKitIcon_ZoomOut)) zoom /= 1.25f;
    ImGui::SameLine();
    if (IconButton("##zoom_1", ImGuiKitIcon_ZoomOneToOne)) zoom = 1.0f;
    ImGui::SameLine();
    if (IconButton("##zoom_fit", ImGuiKitIcon_ZoomFit)) zoom = 0.5f;
    ImGui::SameLine();
    ImGui::Text("zoom: %.2f", zoom);
    ImGui::TreePop();
}

void DemoSegmentedSwitch()
{
    IMGUI_DEMO_MARKER("Widgets/Segmented switch");
    if (!ImGui::TreeNode("Segmented switch"))
        return;
    static int view = 0;
    const char* views[] = { "Demo", "Code" };
    SegmentedSwitch("##view", &view, views, IM_ARRAYSIZE(views));
    ImGui::SameLine();
    ImGui::Text("showing: %s", views[view]);
    static int unit = 1;
    const char* units[] = { "mm", "cm", "inch" };
    SegmentedSwitch("##unit", &unit, units, IM_ARRAYSIZE(units));
    ImGui::TreePop();
}

void DemoWidgets()
{
    IMGUI_DEMO_MARKER("Widgets");
    if (!ImGui::CollapsingHeader("Widgets"))
        return;
    DemoHelpMarker();
    DemoIconButton();
    DemoSegmentedSwitch();
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
    ShowDemoContents();
    ImGui::End();
}

void ShowDemoContents()
{
    ImGui::Text("imgui_kit %s: small, standalone additions to Dear ImGui", IMGUI_KIT_VERSION);
    ImGui::Separator();
    DemoEmUnits();
    DemoLayout();
    DemoWidgets();
}

} // namespace ImGuiKit

#else

void ImGuiKit::ShowDemoWindow(bool*) {}
void ImGuiKit::ShowDemoContents() {}

#endif // #ifndef IMGUI_KIT_DISABLE_DEMO
