// imgui_kit: small, standalone additions to Dear ImGui
// https://github.com/pthom/imgui_kit (MIT license)

/*

Index of this file:
// [SECTION] Helpers
// [SECTION] Help marker
// [SECTION] Icon button
// [SECTION] Segmented switch

*/

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_kit.h"
#include "imgui_kit_internal.h"
#include "imgui_internal.h"
#include <cmath>

namespace ImGuiKit
{
namespace
{

//-----------------------------------------------------------------------------
// [SECTION] Helpers
//-----------------------------------------------------------------------------

// Goes to the next line when an item of this width does not fit after the previous one on the same line
// (not in a fill row's tail: the row makes the room)
void NewLineIfNoRoom(float width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->DC.IsSameLine && ImGui::GetContentRegionAvail().x < width && !Internal::IsInFillRowTail())
        ImGui::NewLine();
}

//-----------------------------------------------------------------------------
// [SECTION] Help marker
//-----------------------------------------------------------------------------

enum HelpMarkerShown { HelpMarkerShown_Tooltip, HelpMarkerShown_Popup };
ImVector<int> gHelpMarkersShown;  // what each open BeginHelpMarker() shows, for EndHelpMarker()

void DefaultHelpMarkerTextRenderer(const char* text)
{
    ImGui::TextUnformatted(text);   // wrapped by the PushTextWrapPos() of BeginHelpMarker()
}
HelpMarkerTextRenderer gHelpMarkerTextRenderer = DefaultHelpMarkerTextRenderer;

// As high as the line it is on: a text's height on a line of text, a frame's after a framed widget
ImVec2 HelpMarkerSize()
{
    float base_offset = ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset;
    return ImVec2(ImTrunc(ImGui::GetFontSize() * 1.2f), ImGui::GetTextLineHeight() + 2.0f * base_offset);
}

// The (i): a circle, a dot, a bar
void DrawHelpIcon(ImDrawList* draw_list, const ImRect& bb, ImU32 color)
{
    ImVec2 c = bb.GetCenter();
    float r = ImGui::GetFontSize() * 0.45f;
    float thickness = ImMax(1.0f, ImGui::GetFontSize() * 0.08f);
    draw_list->AddCircle(c, r, color, 0, thickness);
    draw_list->AddCircleFilled(ImVec2(c.x, c.y - r * 0.45f), r * 0.13f, color);
    float half_w = r * 0.11f;
    draw_list->AddRectFilled(ImVec2(c.x - half_w, c.y - r * 0.12f), ImVec2(c.x + half_w, c.y + r * 0.55f), color, half_w);
}

//-----------------------------------------------------------------------------
// [SECTION] Icon button
//-----------------------------------------------------------------------------

void DrawMagnifier(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color, ImGuiKitIcon icon)
{
    // The glass up and right, its handle down and left: the whole icon is centered
    float r = size * 0.24f;
    ImVec2 glass(center.x + size * 0.06f, center.y - size * 0.06f);
    draw_list->AddCircle(glass, r, color, 0, size * 0.06f);
    const float d = 0.7071f;  // the handle points down-left, at 45 degrees
    draw_list->AddLine(ImVec2(glass.x - r * 1.05f * d, glass.y + r * 1.05f * d),
                       ImVec2(glass.x - r * 1.85f * d, glass.y + r * 1.85f * d), color, size * 0.1f);
    float half = r * 0.5f, t = size * 0.05f;
    if (icon == ImGuiKitIcon_ZoomIn || icon == ImGuiKitIcon_ZoomOut)
        draw_list->AddRectFilled(ImVec2(glass.x - half, glass.y - t * 0.5f), ImVec2(glass.x + half, glass.y + t * 0.5f), color);
    if (icon == ImGuiKitIcon_ZoomIn)
        draw_list->AddRectFilled(ImVec2(glass.x - t * 0.5f, glass.y - half), ImVec2(glass.x + t * 0.5f, glass.y + half), color);
    if (icon == ImGuiKitIcon_ZoomOneToOne)
    {
        // A "1": a stroke and its serif
        float h = r * 0.45f;
        draw_list->AddRectFilled(ImVec2(glass.x - t * 0.5f, glass.y - h), ImVec2(glass.x + t * 0.5f, glass.y + h), color);
        draw_list->AddLine(ImVec2(glass.x, glass.y - h), ImVec2(glass.x - r * 0.18f, glass.y - h + r * 0.18f), color, t);
    }
}

void DrawFit(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color)
{
    // Four corner brackets
    float half = size * 0.34f, arm = size * 0.22f, t = size * 0.07f;
    for (int iy = -1; iy <= 1; iy += 2)
        for (int ix = -1; ix <= 1; ix += 2)
        {
            ImVec2 corner(center.x + ix * half, center.y + iy * half);
            draw_list->AddLine(corner, ImVec2(corner.x - ix * arm, corner.y), color, t);
            draw_list->AddLine(corner, ImVec2(corner.x, corner.y - iy * arm), color, t);
        }
}

void DrawIcon(ImDrawList* draw_list, const ImRect& bb, ImGuiKitIcon icon, ImU32 color)
{
    ImVec2 c = bb.GetCenter();
    float size = ImMin(bb.GetWidth(), bb.GetHeight());
    float half = ImGui::GetFontSize() * 0.35f, t = ImMax(1.0f, ImGui::GetFontSize() * 0.09f);
    switch (icon)
    {
    case ImGuiKitIcon_Minus:
        draw_list->AddRectFilled(ImVec2(c.x - half, c.y - t), ImVec2(c.x + half, c.y + t), color);
        break;
    case ImGuiKitIcon_Plus:
        draw_list->AddRectFilled(ImVec2(c.x - half, c.y - t), ImVec2(c.x + half, c.y + t), color);
        draw_list->AddRectFilled(ImVec2(c.x - t, c.y - half), ImVec2(c.x + t, c.y + half), color);
        break;
    case ImGuiKitIcon_ZoomIn:
    case ImGuiKitIcon_ZoomOut:
    case ImGuiKitIcon_ZoomOneToOne:
        DrawMagnifier(draw_list, c, size, color, icon);
        break;
    case ImGuiKitIcon_ZoomFit:
        DrawFit(draw_list, c, size, color);
        break;
    default:
        IM_ASSERT(false && "Unknown ImGuiKitIcon");
    }
}

} // namespace

//-----------------------------------------------------------------------------
// [SECTION] Help marker
//-----------------------------------------------------------------------------

bool BeginHelpMarker(const char* str_id)
{
    ImVec2 size = HelpMarkerSize();
    NewLineIfNoRoom(size.x);
    ImGui::PushID(str_id);
    bool clicked = ImGui::InvisibleButton("##help_marker", size);
    bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    bool popup_open = ImGui::IsPopupOpen("##help_popup");
    ImU32 color = ImGui::GetColorU32((ImGui::IsItemHovered() || popup_open) ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    DrawHelpIcon(ImGui::GetWindowDrawList(), ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), color);

    if (clicked)
        ImGui::OpenPopup("##help_popup");
    float wrap_width = ImGui::GetFontSize() * 30.0f;
    if (ImGui::BeginPopup("##help_popup"))
    {
        gHelpMarkersShown.push_back(HelpMarkerShown_Popup);
        ImGui::PushTextWrapPos(wrap_width);
        return true;
    }
    // A finger has no hover: on a touch screen, only the tap shows the text
    bool touch = ImGui::GetIO().MouseSource == ImGuiMouseSource_TouchScreen;
    if (hovered && !touch && ImGui::BeginTooltip())
    {
        gHelpMarkersShown.push_back(HelpMarkerShown_Tooltip);
        ImGui::PushTextWrapPos(wrap_width);
        return true;
    }
    ImGui::PopID();
    return false;
}

void EndHelpMarker()
{
    IM_ASSERT(!gHelpMarkersShown.empty() && "EndHelpMarker() without a BeginHelpMarker() that returned true");
    ImGui::PopTextWrapPos();
    if (gHelpMarkersShown.back() == HelpMarkerShown_Popup)
        ImGui::EndPopup();
    else
        ImGui::EndTooltip();
    gHelpMarkersShown.pop_back();
    ImGui::PopID();
}

void HelpMarker(const char* text)
{
    if (BeginHelpMarker(text))
    {
        gHelpMarkerTextRenderer(text);
        EndHelpMarker();
    }
}

float HelpMarkerWidth()
{
    return HelpMarkerSize().x + ImGui::GetStyle().ItemSpacing.x;
}

void SetHelpMarkerTextRenderer(HelpMarkerTextRenderer renderer)
{
    gHelpMarkerTextRenderer = renderer ? renderer : DefaultHelpMarkerTextRenderer;
}

//-----------------------------------------------------------------------------
// [SECTION] Icon button
//-----------------------------------------------------------------------------

bool IconButton(const char* str_id, ImGuiKitIcon icon, const ImVec2& size)
{
    float h = ImGui::GetFrameHeight();
    ImVec2 button_size(size.x > 0.0f ? size.x : h, size.y > 0.0f ? size.y : h);
    ImGui::PushID(str_id);
    bool pressed = ImGui::Button("##icon_button", button_size);
    ImGui::PopID();
    DrawIcon(ImGui::GetWindowDrawList(), ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), icon,
             ImGui::GetColorU32(ImGuiCol_Text));
    return pressed;
}

//-----------------------------------------------------------------------------
// [SECTION] Segmented switch
//-----------------------------------------------------------------------------

bool SegmentedSwitch(const char* str_id, int* current, const char* const labels[], int labels_count)
{
    IM_ASSERT(labels_count > 0);
    const ImGuiStyle& style = ImGui::GetStyle();
    // Equal segments, each wide enough for the longest label
    float label_width = 0.0f;
    for (int i = 0; i < labels_count; i++)
        label_width = ImMax(label_width, ImGui::CalcTextSize(labels[i], nullptr, true).x);
    float segment_width = label_width + style.FramePadding.x * 4.0f;
    ImVec2 size(segment_width * (float)labels_count, ImGui::GetFrameHeight());
    ImVec2 p0 = ImGui::GetCursorScreenPos();

    // As a framed widget: the text that follows on its line is aligned with its labels
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;
    const ImGuiID id = window->GetID(str_id);
    const ImRect bb(p0, p0 + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;
    // The press, rather than the release: a touch screen may not report the latter where it started
    bool hovered_unused, held_unused;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered_unused, &held_unused, ImGuiButtonFlags_PressedOnClick);
    bool changed = false;
    if (pressed)
    {
        int clicked = ImClamp((int)((ImGui::GetIO().MouseClickedPos[0].x - p0.x) / segment_width), 0, labels_count - 1);
        changed = (clicked != *current);
        *current = clicked;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p1 = p0 + size;
    float rounding = size.y * 0.5f;
    auto fill_segment = [&](int i, ImGuiCol col)
    {
        ImDrawFlags flags = ImDrawFlags_RoundCornersNone;
        if (i == 0)
            flags = ImDrawFlags_RoundCornersLeft;
        if (i == labels_count - 1)
            flags = (labels_count == 1) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight;
        float x0 = p0.x + segment_width * (float)i;
        draw_list->AddRectFilled(ImVec2(x0, p0.y), ImVec2(x0 + segment_width, p1.y), ImGui::GetColorU32(col), rounding, flags);
    };
    draw_list->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);
    if (ImGui::IsItemHovered())
    {
        int over = ImClamp((int)((ImGui::GetIO().MousePos.x - p0.x) / segment_width), 0, labels_count - 1);
        if (over != *current)
            fill_segment(over, ImGuiCol_FrameBgHovered);
    }
    if (*current >= 0 && *current < labels_count)
        fill_segment(*current, ImGuiCol_ButtonActive);
    for (int i = 1; i < labels_count; i++)
    {
        float x = p0.x + segment_width * (float)i;
        draw_list->AddLine(ImVec2(x, p0.y), ImVec2(x, p1.y), ImGui::GetColorU32(ImGuiCol_Border));
    }
    draw_list->AddRect(p0, p1, ImGui::GetColorU32(ImGuiCol_Border), rounding);
    for (int i = 0; i < labels_count; i++)
    {
        ImVec2 text_size = ImGui::CalcTextSize(labels[i], nullptr, true);
        ImVec2 text_pos(p0.x + segment_width * (float)i + (segment_width - text_size.x) * 0.5f,
                        p0.y + (size.y - text_size.y) * 0.5f);
        draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), labels[i], ImGui::FindRenderedTextEnd(labels[i]));
    }
    return changed;
}

#ifdef IMGUI_KIT_USE_STL
bool SegmentedSwitch(const char* str_id, int* current, const std::vector<std::string>& labels)
{
    ImVector<const char*> c_labels;
    for (const std::string& label : labels)
        c_labels.push_back(label.c_str());
    return SegmentedSwitch(str_id, current, c_labels.Data, c_labels.Size);
}
#endif

} // namespace ImGuiKit
