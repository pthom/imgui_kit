// imgui_kit: the demo
// https://github.com/pthom/imgui_kit (MIT license)
//
// One collapsing header per topic, in the order of imgui_kit.h. Each section is marked with IMGUI_DEMO_MARKER,
// as in imgui_demo.cpp: a tool can show the code of the section under the mouse.

/*

Index of this file:
// [SECTION] Toolbar: theme and font size
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
// [SECTION] Toolbar: theme and font size
//-----------------------------------------------------------------------------

// The theme and the frame rounding apply to the whole application; "App" restores the colors it had.
// The font size applies to the demo.
void DemoToolbar(float* font_size)
{
    IMGUI_DEMO_MARKER("Toolbar");
    static ImVec4 app_colors[ImGuiCol_COUNT];
    static bool app_colors_saved = false;
    ImGuiStyle& style = ImGui::GetStyle();
    if (!app_colors_saved)
    {
        for (int i = 0; i < ImGuiCol_COUNT; i++)
            app_colors[i] = style.Colors[i];
        app_colors_saved = true;
    }

    BeginFlow("toolbar");
    FlowNext();
    ImGui::BeginGroup();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Theme");
    ImGui::SameLine();
    static int theme = 0;
    const char* themes[] = { "App", "Dark", "Light", "Classic" };
    if (SegmentedSwitch("##theme", &theme, themes, IM_ARRAYSIZE(themes)))
    {
        if (theme == 0)
            for (int i = 0; i < ImGuiCol_COUNT; i++)
                style.Colors[i] = app_colors[i];
        else if (theme == 1)
            ImGui::StyleColorsDark();
        else if (theme == 2)
            ImGui::StyleColorsLight();
        else
            ImGui::StyleColorsClassic();
    }
    ImGui::EndGroup();

    FlowNext();
    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(EmSize(8.f));
    ImGui::SliderFloat("##font_size", font_size, 8.0f, 40.0f, "Font %.0f px");
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
        *font_size = style.FontSizeBase;
    ImGui::EndGroup();

    FlowNext();
    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(EmSize(10.f));
    ImGui::SliderFloat("##rounding", &style.FrameRounding, 0.0f, 12.0f, "Frame rounding %.0f px");
    ImGui::SameLine();
    HelpMarker("The style's FrameRounding: ImGui::GetStyle().FrameRounding. Buttons, sliders, input fields and "
               "the segmented switch round their corners with it. 0 gives square frames; half the frame's height "
               "gives a pill.");
    ImGui::EndGroup();
    EndFlow();
}

//-----------------------------------------------------------------------------
// [SECTION] Em units
//-----------------------------------------------------------------------------

void DemoEmUnits()
{
    IMGUI_DEMO_MARKER("Em units");
    if (!ImGui::CollapsingHeader("Em units"))
        return;
    ImGui::TextWrapped("Sizes in em follow the font: the first button is 8 em wide and 2 em high, on any screen. "
                       "Change the font size at the top: it grows with the text, as a standard button does. "
                       "The last button keeps its size in pixels: at a large font, its label gets cut.");
    ImGui::Button("8 x 2 em", EmToVec2(8.f, 2.f));
    ImGui::SameLine();
    ImGui::Button("A standard button");
    ImGui::SameLine();
    ImGui::Button("100 x 30 px", ImVec2(100.f, 30.f));
    ImGui::SameLine();
    HelpMarker("EmToVec2(8, 2) gives the size in pixels, from the current font size.");
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
    const char* presets[] = { "Coral", "Lavender", "Mint", "Sunflower yellow", "Sky", "Peach", "Forest green",
                              "Sand", "Honey", "Midnight blue", "Rose", "Turquoise", "Custom" };
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
    ImGui::TextWrapped("A touch screen has no hover: a tooltip never shows there. The (i) opens on a tap as well, "
                       "so its text reaches every user.");
    ImGui::BulletText("With a mouse: hover to read, click to pin.");
    ImGui::BulletText("On a touch screen: tap to open, tap elsewhere to close.");
    ImGui::Text("Try it:");
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
    ImGui::TextWrapped("The icons are drawn with the window's draw list (lines, rectangles, circles): "
                       "no icon font to load, and they follow the font size.");
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
    ImGui::TextWrapped("Its corners follow the style's FrameRounding, as a button's do: change it at the top.");
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
    if (!ImGui::Begin("ImGuiKit Demo", p_open))
    {
        ImGui::End();
        return;
    }
    ShowDemoContents();
    ImGui::End();
}

void ShowDemoContents()
{
    ImGui::Text("ImGuiKit %s: small, standalone additions to Dear ImGui", IMGUI_KIT_VERSION);
    static float font_size = 0.0f;  // 0: the application's size, read at the first frame
    if (font_size == 0.0f)
        font_size = ImGui::GetStyle().FontSizeBase;
    DemoToolbar(&font_size);
    ImGui::Separator();
    ImGui::PushFont(nullptr, font_size);
    DemoEmUnits();
    DemoLayout();
    DemoWidgets();
    ImGui::PopFont();
}

} // namespace ImGuiKit

#else

void ImGuiKit::ShowDemoWindow(bool*) {}
void ImGuiKit::ShowDemoContents() {}

#endif // #ifndef IMGUI_KIT_DISABLE_DEMO
