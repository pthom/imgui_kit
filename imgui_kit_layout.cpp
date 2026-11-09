// imgui_kit: small, standalone additions to Dear ImGui
// https://github.com/pthom/imgui_kit (MIT license)

/*

Index of this file:
// [SECTION] Helpers
// [SECTION] Fill row
// [SECTION] Aligned group
// [SECTION] Column
// [SECTION] Label column
// [SECTION] Group panel
// [SECTION] Flow

*/

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_kit.h"
#include "imgui_kit_internal.h"
#include "imgui_internal.h"

namespace ImGuiKit
{

//-----------------------------------------------------------------------------
// [SECTION] Helpers
//-----------------------------------------------------------------------------
// The layout helpers keep what they measured in the window's state storage, under a key made of their str_id's ID
// and a seed per helper: the bare ID would be Dear ImGui's own key for an item with that label (a tree node's open
// state), and two helpers with one str_id would share a value.
// A value never measured reads as IMGUI_KIT_UNKNOWN: the helper then draws hidden on that frame (Alpha = 0, no input)
// and// keeps its overflow out of the window's content size, so that a first frame neither flashes nor adds a scrollbar.
// Their Begin/End stacks are in ImGuiKitContext, one per Dear ImGui context (imgui_kit_internal.h).

static ImGuiID StorageKey(const char* str_id, ImGuiID helper_seed)
{
    return ImHashData(&helper_seed, sizeof(helper_seed), ImGui::GetID(str_id));
}
const ImGuiID FILL_ROW_SEED = ImHashStr("##kit_fill_row");
const ImGuiID ALIGNED_GROUP_SEED = ImHashStr("##kit_aligned_group");
const ImGuiID COLUMN_SEED = ImHashStr("##kit_column");
const ImGuiID LABEL_COLUMN_SEED = ImHashStr("##kit_label_column");
const ImGuiID FLOW_SEED = ImHashStr("##kit_flow");

static float StoredFloat(ImGuiID key)
{
    return ImGui::GetStateStorage()->GetFloat(key, IMGUI_KIT_UNKNOWN);
}

static void StoreFloat(ImGuiID key, float v)
{
    ImGui::GetStateStorage()->SetFloat(key, v);
}

// A flow's item, by its rank
static ImGuiID FlowItemKey(const ImGuiKitFlow& flow, int index)
{
    return ImHashData(&index, sizeof(index), flow.id);
}

bool IsInFillRowTail()
{
    return !GetCurrentContext().FillRows.empty() && GetCurrentContext().FillRows.back().tailStarted;
}

//-----------------------------------------------------------------------------
// [SECTION] Fill row
//-----------------------------------------------------------------------------

float BeginFillRow(const char* str_id)
{
    ImGuiKitFillRow row;
    row.id = StorageKey(str_id, FILL_ROW_SEED);
    float tail = StoredFloat(row.id);
    float width = ImMax(1.0f, ImGui::GetContentRegionAvail().x - (tail == IMGUI_KIT_UNKNOWN ? 0.0f : tail));
    ImGui::SetNextItemWidth(width);
    GetCurrentContext().FillRows.push_back(row);
    return width;
}

void FillRowTail()
{
    IM_ASSERT(!GetCurrentContext().FillRows.empty() && "FillRowTail() needs BeginFillRow()");
    ImGuiKitFillRow& row = GetCurrentContext().FillRows.back();
    IM_ASSERT(!row.tailStarted && "FillRowTail() called twice");
    row.fillRight = ImGui::GetItemRectMax().x;
    row.tailStarted = true;
    if (StoredFloat(row.id) == IMGUI_KIT_UNKNOWN)
        row.hidden.Begin();
    ImGui::SameLine();
}

void EndFillRow()
{
    IM_ASSERT(!GetCurrentContext().FillRows.empty() && "EndFillRow() without BeginFillRow()");
    ImGuiKitFillRow& row = GetCurrentContext().FillRows.back();
    // The tail's width: from the fill item's right edge to the tail's last item, the spacing included
    float tail = row.tailStarted ? ImMax(0.0f, ImGui::GetItemRectMax().x - row.fillRight) : 0.0f;
    StoreFloat(row.id, tail);
    row.hidden.End();
    GetCurrentContext().FillRows.pop_back();
}

//-----------------------------------------------------------------------------
// [SECTION] Aligned group
//-----------------------------------------------------------------------------

void BeginAlignedGroup(const char* str_id, float align_x)
{
    ImGuiKitAlignedGroup group;
    group.id = StorageKey(str_id, ALIGNED_GROUP_SEED);
    float width = StoredFloat(group.id);
    // The room to align in: up to the innermost column's right edge, else up to the line's end
    float room = ImGui::GetContentRegionAvail().x;
    if (!GetCurrentContext().Columns.empty())
    {
        const ImGuiKitColumn& column = GetCurrentContext().Columns.back();
        room = (column.width == IMGUI_KIT_UNKNOWN)
                   ? IMGUI_KIT_UNKNOWN : column.left + column.width - ImGui::GetCursorScreenPos().x;
    }
    if (width == IMGUI_KIT_UNKNOWN || room == IMGUI_KIT_UNKNOWN)
        group.hidden.Begin();
    else
    {
        // A whole pixel: content that snaps to whole pixels (a child window, an image) would change the measured
        // width with the offset's fraction, and the group would move at each frame
        float offset = ImTrunc((room - width) * align_x);
        if (offset > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
    }
    GetCurrentContext().AlignedGroups.push_back(group);
    ImGui::BeginGroup();
}

void EndAlignedGroup()
{
    IM_ASSERT(!GetCurrentContext().AlignedGroups.empty() && "EndAlignedGroup() without BeginAlignedGroup()");
    ImGui::EndGroup();
    ImGuiKitAlignedGroup& group = GetCurrentContext().AlignedGroups.back();
    StoreFloat(group.id, ImGui::GetItemRectSize().x);
    group.hidden.End();
    GetCurrentContext().AlignedGroups.pop_back();
}

//-----------------------------------------------------------------------------
// [SECTION] Column
//-----------------------------------------------------------------------------

void BeginColumn(const char* str_id)
{
    ImGuiKitColumn column;
    column.id = StorageKey(str_id, COLUMN_SEED);
    column.left = ImGui::GetCursorScreenPos().x;
    column.width = StoredFloat(column.id);
    GetCurrentContext().Columns.push_back(column);
    ImGui::BeginGroup();
}

void EndColumn()
{
    IM_ASSERT(!GetCurrentContext().Columns.empty() && "EndColumn() without BeginColumn()");
    ImGui::EndGroup();
    StoreFloat(GetCurrentContext().Columns.back().id, ImGui::GetItemRectSize().x);
    GetCurrentContext().Columns.pop_back();
}

//-----------------------------------------------------------------------------
// [SECTION] Label column
//-----------------------------------------------------------------------------

void BeginLabelColumn(const char* str_id, float labels_align_x)
{
    ImGuiKitLabelColumn lc;
    lc.id = StorageKey(str_id, LABEL_COLUMN_SEED);
    lc.left = ImGui::GetCursorScreenPos().x;
    lc.alignX = labels_align_x;
    lc.column = StoredFloat(lc.id);
    GetCurrentContext().LabelColumns.push_back(lc);
    if (lc.column == IMGUI_KIT_UNKNOWN)
        GetCurrentContext().LabelColumns.back().hidden.Begin();
}

void Label(const char* text)
{
    IM_ASSERT(!GetCurrentContext().LabelColumns.empty() && "Label() needs BeginLabelColumn()");
    ImGuiKitLabelColumn& lc = GetCurrentContext().LabelColumns.back();
    const char* text_end = ImGui::FindRenderedTextEnd(text);
    float width = ImGui::CalcTextSize(text, text_end).x;
    lc.widest = ImMax(lc.widest, width);
    // A label wider than the last frame's widest (a text that grew) takes its own width this frame
    float column = (lc.column == IMGUI_KIT_UNKNOWN) ? width : ImMax(lc.column, width);

    ImGui::AlignTextToFramePadding();
    float y = ImGui::GetCursorScreenPos().y;
    ImGui::SetCursorScreenPos(ImVec2(lc.left + (column - width) * lc.alignX, y));
    ImGui::TextUnformatted(text, text_end);
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(lc.left + column + ImGui::GetStyle().ItemSpacing.x,
                                     ImGui::GetCursorScreenPos().y));
}

void EndLabelColumn()
{
    IM_ASSERT(!GetCurrentContext().LabelColumns.empty() && "EndLabelColumn() without BeginLabelColumn()");
    ImGuiKitLabelColumn& lc = GetCurrentContext().LabelColumns.back();
    StoreFloat(lc.id, lc.widest);
    lc.hidden.End();
    GetCurrentContext().LabelColumns.pop_back();
}

//-----------------------------------------------------------------------------
// [SECTION] Group panel
//-----------------------------------------------------------------------------

// The frame is drawn at the end, around the content's group: no clip rects or item spacing to push during the content.
// The label is pushed as an ID: two panels may hold items with the same labels.
void BeginGroupPanel(const char* label, const ImVec2& size)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGuiKitGroupPanel panel;
    panel.topLeft = ImGui::GetCursorScreenPos();
    panel.minSize = size;
    ImGui::PushID(label);
    ImGui::BeginGroup();  // the whole panel

    // The label, in the frame's top border
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    panel.labelLeft = panel.topLeft.x + style.FramePadding.x * 2.0f;
    panel.labelRight = panel.labelLeft + ImGui::CalcTextSize(label, label_end).x;
    ImGui::SetCursorScreenPos(ImVec2(panel.labelLeft, panel.topLeft.y));
    ImGui::TextUnformatted(label, label_end);

    // The content, inside the frame's padding
    ImGui::SetCursorScreenPos(ImVec2(panel.topLeft.x + style.FramePadding.x * 2.0f,
                                     panel.topLeft.y + ImGui::GetTextLineHeight() + style.FramePadding.y));
    ImGui::BeginGroup();
    GetCurrentContext().GroupPanels.push_back(panel);
}

void EndGroupPanel()
{
    IM_ASSERT(!GetCurrentContext().GroupPanels.empty() && "EndGroupPanel() without BeginGroupPanel()");
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGuiKitGroupPanel panel = GetCurrentContext().GroupPanels.back();
    GetCurrentContext().GroupPanels.pop_back();
    ImGui::EndGroup();  // the content
    ImVec2 content_max = ImGui::GetItemRectMax();

    float right = ImMax(content_max.x + style.FramePadding.x * 2.0f, panel.labelRight + style.FramePadding.x * 2.0f);
    right = ImMax(right, panel.topLeft.x + panel.minSize.x);
    float bottom = ImMax(content_max.y + style.FramePadding.y * 2.0f, panel.topLeft.y + panel.minSize.y);
    ImGui::SetCursorScreenPos(panel.topLeft);
    ImGui::Dummy(ImVec2(right - panel.topLeft.x, bottom - panel.topLeft.y));  // the panel's whole size
    ImGui::EndGroup();
    ImGui::PopID();

    // The frame, from the middle of the label's line, with a gap for the label: drawn three times, clipped
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImRect frame(ImVec2(panel.topLeft.x, panel.topLeft.y + ImGui::GetTextLineHeight() * 0.5f), ImVec2(right, bottom));
    float gap = style.ItemInnerSpacing.x;
    ImRect clips[3] = {
        ImRect(ImVec2(frame.Min.x - 1.0f, frame.Min.y - 1.0f), ImVec2(panel.labelLeft - gap, frame.Max.y + 1.0f)),
        ImRect(ImVec2(panel.labelRight + gap, frame.Min.y - 1.0f), ImVec2(frame.Max.x + 1.0f, frame.Max.y + 1.0f)),
        ImRect(ImVec2(panel.labelLeft - gap, frame.Min.y + 1.0f), ImVec2(panel.labelRight + gap, frame.Max.y + 1.0f)),
    };
    for (const ImRect& clip : clips)
    {
        draw_list->PushClipRect(clip.Min, clip.Max, true);
        draw_list->AddRect(frame.Min, frame.Max, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding);
        draw_list->PopClipRect();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Flow
//-----------------------------------------------------------------------------

void BeginFlow(const char* str_id)
{
    ImGuiKitFlow flow;
    flow.id = StorageKey(str_id, FLOW_SEED);
    flow.rightEdge = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    if (StoredFloat(flow.id) == IMGUI_KIT_UNKNOWN)  // never drawn: no widths yet
        flow.hidden.Begin();
    GetCurrentContext().Flows.push_back(flow);
}

void FlowNext()
{
    IM_ASSERT(!GetCurrentContext().Flows.empty() && "FlowNext() needs BeginFlow()");
    ImGuiKitFlow& flow = GetCurrentContext().Flows.back();
    if (flow.index > 0)
    {
        // The previous item is drawn: keep its width, and decide where the next one goes
        StoreFloat(FlowItemKey(flow, flow.index - 1), ImGui::GetItemRectSize().x);
        // An item never measured (new since the last frame) starts a line: for one frame, rather than overflow
        float nextWidth = StoredFloat(FlowItemKey(flow, flow.index));
        float nextLeft = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
        if (nextWidth != IMGUI_KIT_UNKNOWN && nextLeft + nextWidth <= flow.rightEdge)
            ImGui::SameLine();
    }
    ++flow.index;
}

void EndFlow()
{
    IM_ASSERT(!GetCurrentContext().Flows.empty() && "EndFlow() without BeginFlow()");
    ImGuiKitFlow& flow = GetCurrentContext().Flows.back();
    if (flow.index > 0)
        StoreFloat(FlowItemKey(flow, flow.index - 1), ImGui::GetItemRectSize().x);
    StoreFloat(flow.id, (float)flow.index);  // the flow was drawn (its items' count)
    flow.hidden.End();
    GetCurrentContext().Flows.pop_back();
}

} // namespace ImGuiKit
