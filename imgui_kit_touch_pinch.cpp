// imgui_kit: the pinch of a touch screen (ImGuiTouch)
// https://github.com/pthom/imgui_kit (MIT license)

/*

Index of this file:
// [SECTION] Helpers: the events, the takers
// [SECTION] The claims (ItemPinch, the adapters)
// [SECTION] The fingers (AddFingerEvent)
// [SECTION] UpdateTouchPinch

*/

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_kit.h"
#include "imgui_kit_touch_internal.h"

#include "imgui.h"
#include "imgui_internal.h"  // ImClamp, the input queue

namespace ImGuiTouch
{
//-----------------------------------------------------------------------------
// [SECTION] Helpers: the events, the takers
//-----------------------------------------------------------------------------

constexpr float PINCH_THRESHOLD = 0.12f;  // the distance between the fingers changed by this much: a pinch
constexpr float DRAG_SLOP_FONT_SIZES = 0.6f;  // the fingers' middle moved this far: a drag (the two apart)

static float Distance(const ImGuiTouchFinger& a, const ImGuiTouchFinger& b)
{
    return ImSqrt(ImLengthSqr(b.pos - a.pos));
}
static ImVec2 Center(const ImGuiTouchFinger& a, const ImGuiTouchFinger& b) { return (a.pos + b.pos) * 0.5f; }

// A button or a wheel event, marked as the test engine marks its own (see the swipe layer), with the mouse
// source: the touch trickling would separate a button from the positions
static void MarkLastEvent(ImGuiContext& g, int sizeBefore)
{
    if (g.InputEventsQueue.Size > sizeBefore)
        g.InputEventsQueue.back().AddedByTestEngine = true;
}
static void ButtonEvent(ImGuiIO& io, ImGuiMouseButton button, bool down)
{
    ImGuiContext& g = *GImGui;
    ImGuiMouseSource source = io.MouseSource;
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    int before = g.InputEventsQueue.Size;
    io.AddMouseButtonEvent(button, down);
    MarkLastEvent(g, before);
    io.AddMouseSourceEvent(source);
}
static void PosEvent(ImGuiIO& io, ImVec2 pos)
{
    ImGuiContext& g = *GImGui;
    int before = g.InputEventsQueue.Size;
    io.AddMousePosEvent(pos.x, pos.y);
    MarkLastEvent(g, before);
}
static void WheelEvent(ImGuiIO& io, float notches)
{
    ImGuiContext& g = *GImGui;
    ImGuiMouseSource source = io.MouseSource;
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    int before = g.InputEventsQueue.Size;
    io.AddMouseWheelEvent(0.0f, notches);
    MarkLastEvent(g, before);
    io.AddMouseSourceEvent(source);
}

// The widget that takes the pinch: both fingers inside a rectangle noted at the last frame (the smallest, when
// several hold them: the innermost widget)
const ImGuiTouchPinchState::PinchTaker* FindTaker(const ImGuiTouchPinchState& s)
{
    const ImGuiTouchPinchState::PinchTaker* found = nullptr;
    for (const ImGuiTouchPinchState::PinchTaker& t : s.previousTakers)
        if (t.rect.Contains(s.fingers[0].pos) && t.rect.Contains(s.fingers[1].pos))
            if (found == nullptr || t.rect.GetArea() < found->rect.GetArea())
                found = &t;
    return found;
}

// The claim of a widget at this frame: an item's claims merge by its id, a rectangle's (id 0) by the rectangle.
// Null without the touch layer (the widget just draws), or when nothing of the widget shows.
static ImGuiTouchPinchState::PinchTaker* Claim(ImGuiID id, const ImRect& rect)
{
    ImGuiContext& g = *GImGui;
    ImGuiTouchContext* touch = FindTouchContext();
    if (touch == nullptr || touch->Pinch.frameCount != g.FrameCount)
        return nullptr;
    ImRect visible = rect;
    visible.ClipWith(g.CurrentWindow->ClipRect);
    if (visible.GetWidth() <= 0.0f || visible.GetHeight() <= 0.0f)
        return nullptr;
    ImGuiTouchPinchState& s = touch->Pinch;
    for (ImGuiTouchPinchState::PinchTaker& t : s.takers)
    {
        const bool same = (id != 0) ? (t.id == id)
                                    : (t.id == 0 && t.rect.Min == visible.Min && t.rect.Max == visible.Max);
        if (same)
            return &t;
    }
    ImGuiTouchPinchState::PinchTaker t;
    t.id = id;
    t.rect = visible;
    s.takers.push_back(t);
    return &s.takers.back();
}

static void ClaimWheel(ImGuiID id, const ImRect& rect, float wheelZoomFactor, ImGuiTouchPinchDelivery delivery)
{
    IM_ASSERT(wheelZoomFactor > 0.0f && wheelZoomFactor != 1.0f && "a zoom per wheel unit, e.g. 1.1 for +10%");
    if (ImGuiTouchPinchState::PinchTaker* t = Claim(id, rect))
    {
        IM_ASSERT(!t->data && "a widget gets the pinch's data (ItemPinch) or wheel events, not both");
        t->wheel = true;
        t->wheelZoomFactor = wheelZoomFactor;
        t->delivery = delivery;
    }
}

static void ClaimPan(ImGuiID id, const ImRect& rect, ImGuiMouseButton button)
{
    if (ImGuiTouchPinchState::PinchTaker* t = Claim(id, rect))
    {
        IM_ASSERT(!t->data && "a widget gets the pinch's data (ItemPinch) or a mouse drag, not both");
        t->pan = true;
        t->panButton = button;
    }
}

// The mouse button that two fingers moved together press: none for a widget that reads the gesture's data
static bool DragButton(const ImGuiTouchPinchState& s, ImGuiMouseButton* button)
{
    if (s.hasTaker && s.taker.data)
        return false;
    *button = (s.hasTaker && s.taker.pan) ? s.taker.panButton : ImGuiMouseButton_Right;
    return true;
}

// The fingers lifted (or were cancelled): the drag's button lifts, and a widget that reads the data sees Ended
static void EndGesture(ImGuiContext& g, ImGuiTouchPinchState& s)
{
    ImGuiMouseButton button;
    if (s.gesture == ImGuiTouchPinchGesture_Drag && DragButton(s, &button))
        ButtonEvent(g.IO, button, false);
    if (s.hasTaker && s.taker.data)
    {
        s.data.Active = s.data.Started = false;
        s.data.Ended = true;
        s.data.CenterDelta = ImVec2(0.0f, 0.0f);
        s.data.ScaleDelta = 1.0f;
        s.endedId = s.taker.id;
        s.endedFrame = g.FrameCount;
    }
    s.gesture = ImGuiTouchPinchGesture_None;
    s.hasTaker = false;
}

static ImGuiTouchPinch DataFor(ImGuiID id)
{
    ImGuiTouchContext* touch = FindTouchContext();
    if (touch == nullptr || id == 0)
        return ImGuiTouchPinch();
    const ImGuiTouchPinchState& s = touch->Pinch;
    const bool active = s.hasTaker && s.taker.data && s.taker.id == id && s.gesture != ImGuiTouchPinchGesture_None;
    const bool ended = s.endedId == id && s.endedFrame == GImGui->FrameCount;
    return (active || ended) ? s.data : ImGuiTouchPinch();
}

//-----------------------------------------------------------------------------
// [SECTION] The claims (ItemPinch, the adapters)
//-----------------------------------------------------------------------------

ImGuiTouchPinch ItemPinch()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.LastItemData.ID != 0 && "ItemPinch() needs an item with an id (an InvisibleButton, not an Image)");
    return RectPinch(g.LastItemData.ID, g.LastItemData.Rect.Min, g.LastItemData.Rect.Max);
}

ImGuiTouchPinch RectPinch(ImGuiID id, const ImVec2& min, const ImVec2& max)
{
    if (ImGuiTouchPinchState::PinchTaker* t = Claim(id, ImRect(min, max)))
    {
        IM_ASSERT(!t->wheel && !t->pan && "a widget gets the pinch's data (ItemPinch) or the adapters, not both");
        t->data = true;
    }
    return DataFor(id);
}

void SetItemPinchAsWheel(float wheelZoomFactor, ImGuiTouchPinchDelivery delivery)
{
    ClaimWheel(GImGui->LastItemData.ID, GImGui->LastItemData.Rect, wheelZoomFactor, delivery);
}

void SetRectPinchAsWheel(const ImVec2& min, const ImVec2& max, float wheelZoomFactor, ImGuiTouchPinchDelivery delivery)
{
    ClaimWheel(0, ImRect(min, max), wheelZoomFactor, delivery);
}

void SetItemTwoFingerPanAsMouseDrag(ImGuiMouseButton button)
{
    ClaimPan(GImGui->LastItemData.ID, GImGui->LastItemData.Rect, button);
}

void SetRectTwoFingerPanAsMouseDrag(const ImVec2& min, const ImVec2& max, ImGuiMouseButton button)
{
    ClaimPan(0, ImRect(min, max), button);
}

//-----------------------------------------------------------------------------
// [SECTION] The fingers (AddFingerEvent)
//-----------------------------------------------------------------------------

void AddFingerEvent(int id, const ImVec2& pos, ImGuiTouchPhase phase)
{
    ImGuiTouchPinchState& s = GetTouchContext().Pinch;
    const bool down = (phase == ImGuiTouchPhase_Began || phase == ImGuiTouchPhase_Moved);
    if (phase == ImGuiTouchPhase_Cancelled)
        s.cancelled = true;
    bool wasTwo = s.fingers.Size >= 2;
    int index = -1;
    for (int i = 0; i < s.fingers.Size; i++)
        if (s.fingers[i].id == id)
            index = i;
    if (!down)
    {
        if (index < 0)
            return;
        // One of the two fingers of the gesture lifts while a third is down: the third takes its place, and the
        // scale goes on from where it was (no jump), measured from the new pair
        const bool pairChanges = (index < 2 && s.fingers.Size >= 3);
        const float scaleBefore = (pairChanges && s.startDistance > 0.0f)
            ? s.scaleBase * Distance(s.fingers[0], s.fingers[1]) / s.startDistance : 1.0f;
        s.fingers.erase(s.fingers.Data + index);
        if (pairChanges)
        {
            s.scaleBase = scaleBefore;
            s.startDistance = Distance(s.fingers[0], s.fingers[1]);
            s.startCenter = Center(s.fingers[0], s.fingers[1]);
            s.lastDistance = s.startDistance;
            s.lastCenter = s.startCenter;
        }
        return;
    }
    if (index >= 0)
        s.fingers[index].pos = pos;
    else
        s.fingers.push_back({id, pos});
    if (!wasTwo && s.fingers.Size >= 2)  // the second finger landed: the pinch and the drag measure from here
    {
        s.scaleBase = 1.0f;
        s.startDistance = Distance(s.fingers[0], s.fingers[1]);
        s.startCenter = Center(s.fingers[0], s.fingers[1]);
    }
}

void ApplyFingerCancel()
{
    ImGuiContext& g = *GImGui;
    ImGuiTouchPinchState& s = GetTouchContext().Pinch;
    if (!s.cancelled)
        return;
    s.cancelled = false;
    if (s.gesture != ImGuiTouchPinchGesture_None)
        EndGesture(g, s);
    TouchScrollRelease(true);  // the swipe's press, or the widget's: neither clicks when the mouse lifts
}

// While a widget's pinch lasts (or its pan), the mouse is the fingers' center, so that the widget zooms around it. The
// platform's moves (its mouse is the first finger) are replaced: the mouse goes to the center when it is half a line
// away from it, and stays still otherwise. A move holds the next wheel event back a frame (Dear ImGui reads a wheel
// event queued after a move at the next frame): a mouse that followed each small move would halve the zoom's pace.
// Events marked as the layer's or the Test Engine's are left as they are.
void PinchBeforeNewFrame()
{
    ImGuiContext& g = *GImGui;
    ImGuiTouchContext* touch = FindTouchContext();
    if (touch == nullptr)
        return;
    ImGuiTouchPinchState& s = touch->Pinch;
    if (!s.hasTaker || s.gesture == ImGuiTouchPinchGesture_None || s.fingers.Size < 2)
        return;
    const ImVec2 center = Center(s.fingers[0], s.fingers[1]);
    const float tolerance = g.FontSize * 0.5f;
    const bool far = ImLengthSqr(g.IO.MousePos - center) > tolerance * tolerance;
    bool moved = false;
    for (int i = 0; i < g.InputEventsQueue.Size;)
    {
        ImGuiInputEvent& e = g.InputEventsQueue[i];
        if (e.Type != ImGuiInputEventType_MousePos || e.AddedByTestEngine)
            ++i;
        else if (far && !moved)
        {
            e.MousePos.PosX = center.x;
            e.MousePos.PosY = center.y;
            moved = true;
            ++i;
        }
        else
            g.InputEventsQueue.erase(g.InputEventsQueue.Data + i);
    }
    if (far && !moved)
        PosEvent(g.IO, center);
}

//-----------------------------------------------------------------------------
// [SECTION] UpdateTouchPinch
//-----------------------------------------------------------------------------

void UpdateTouchPinch(bool scalesFont)
{
    ImGuiContext& g = *GImGui;
    ImGuiTouchPinchState& s = GetTouchContext().Pinch;
    s.previousTakers.swap(s.takers);
    s.takers.resize(0);
    s.frameCount = g.FrameCount;
    bool twoFingers = s.fingers.Size >= 2;
    // The pinch's scale (their distance over their distance at the start), the move of their middle since the start
    float pinchScale = 1.0f;
    float distance = 0.0f;
    ImVec2 center, move;
    if (twoFingers)
    {
        distance = Distance(s.fingers[0], s.fingers[1]);
        center = Center(s.fingers[0], s.fingers[1]);
        if (s.startDistance > 0.0f)
            pinchScale = s.scaleBase * distance / s.startDistance;
        move = center - s.startCenter;
    }

    bool started = false;
    if (twoFingers && s.gesture == ImGuiTouchPinchGesture_None)
    {
        const ImGuiTouchPinchState::PinchTaker* taker = FindTaker(s);
        s.hasTaker = (taker != nullptr);
        if (s.hasTaker)
        {
            // A widget claims the two fingers: the swipe layer lets go for good, and the first finger's press is
            // released, so that the widget is hovered (it reads the wheel then), and is not dragged by it. The widget's
            // own press (a canvas that took the first finger) is released too.
            s.taker = *taker;
            s.notchesSent = 0.0f;
            const bool itsOwnPress = (s.taker.id != 0 && g.ActiveId == s.taker.id);
            if (!TouchScrollRelease(itsOwnPress))
            {
                s.hasTaker = false;
                return;  // another widget keeps its drag
            }
            ButtonEvent(g.IO, ImGuiMouseButton_Left, false);
            PosEvent(g.IO, center);  // the mouse goes to the fingers' center
        }
        // The first finger was a press for the swipe layer, or for a widget: the two fingers take it
        else if (!TouchScrollLetGo(false))
            return;  // a widget keeps its drag: nothing with two fingers this time
        s.gesture = ImGuiTouchPinchGesture_Undecided;
        started = true;
    }
    if (s.gesture == ImGuiTouchPinchGesture_Undecided && twoFingers)
    {
        float slop = g.FontSize * DRAG_SLOP_FONT_SIZES;
        if (ImFabs(pinchScale - 1.0f) > PINCH_THRESHOLD)
        {
            s.gesture = ImGuiTouchPinchGesture_Pinch;
            s.baseScale = ImGui::GetStyle().FontScaleMain;
            // A widget that reads the wheel's value zooms from here, without catching up the threshold at once
            if (s.hasTaker && s.taker.wheel && s.taker.delivery == ImGuiTouchPinchDelivery_WheelContinuous)
                s.notchesSent = ImLog(pinchScale) / ImLog(s.taker.wheelZoomFactor);
        }
        else if (ImLengthSqr(move) > slop * slop)
        {
            // A drag with two fingers. Unless the widget reads the data: the swipe layer lets the widgets see the
            // mouse again, the first finger's press is released, and the button goes down a frame later (a plot
            // cancels a box selection started in the frame of a left release)
            s.gesture = ImGuiTouchPinchGesture_DragStarting;
            if (!s.hasTaker)  // a taker's: released when the fingers landed
            {
                TouchScrollRelease(true);
                ButtonEvent(g.IO, ImGuiMouseButton_Left, false);
            }
        }
    }
    else if (s.gesture == ImGuiTouchPinchGesture_DragStarting && twoFingers)
    {
        s.gesture = ImGuiTouchPinchGesture_Drag;
        ImGuiMouseButton button;
        if (DragButton(s, &button))
        {
            if (button == ImGuiMouseButton_Left)
                GetTouchContext().Scroll.replayedPresses++;  // the layer's own press: the swipe does not claim it
            ButtonEvent(g.IO, button, true);
        }
    }
    if (s.gesture == ImGuiTouchPinchGesture_Pinch && twoFingers && s.hasTaker)
    {
        if (s.taker.wheel)
        {
            // The taker zooms by the wheel: the wheel units its zoom factor gives for the fingers' spread
            const float target = ImLog(pinchScale) / ImLog(s.taker.wheelZoomFactor);
            const float missing = target - s.notchesSent;
            if (s.taker.delivery == ImGuiTouchPinchDelivery_WheelContinuous)
            {
                // The exact amount at each frame: the zoom follows the fingers (amounts queued together add up exactly)
                if (ImFabs(missing) > 0.001f)
                {
                    WheelEvent(g.IO, missing);
                    s.notchesSent += missing;
                }
            }
            else
            {
                // Whole notches, one at a time: a notch queued while another waits would be read with it, in the
                // same frame (Dear ImGui reads a wheel event queued after a move at the next frame, and the fingers'
                // moves keep coming), and their sum zooms a plot once: ImPlot reads only the wheel's sign
                bool notchWaiting = false;
                for (const ImGuiInputEvent& e : g.InputEventsQueue)
                    if (e.Type == ImGuiInputEventType_MouseWheel)
                        notchWaiting = true;
                const float notch = notchWaiting ? 0.0f : (missing >= 1.0f) ? 1.0f : (missing <= -1.0f) ? -1.0f : 0.0f;
                if (notch != 0.0f)
                {
                    WheelEvent(g.IO, notch);
                    s.notchesSent += notch;
                }
            }
        }
        // A widget with the pan adapter only: a pinch on it does nothing; one that reads the data: below
    }
    else if (s.gesture == ImGuiTouchPinchGesture_Pinch && twoFingers && scalesFont)
    {
        const ImGuiTouchConfig& config = GetTouchContext().Config;
        ImGui::GetStyle().FontScaleMain =
            ImClamp(s.baseScale * pinchScale, config.PinchFontScaleMin, config.PinchFontScaleMax);
    }

    // The data, for a widget that reads it: the move and the spread since the last frame, both at each frame
    if (s.gesture != ImGuiTouchPinchGesture_None && twoFingers && s.hasTaker && s.taker.data)
    {
        ImGuiTouchPinch& d = s.data;
        if (started)
        {
            d = ImGuiTouchPinch();
            s.lastDistance = distance;
            s.lastCenter = center;
        }
        d.Active = true;
        d.Started = started;
        d.Ended = false;
        d.Center = center;
        d.CenterDelta = center - s.lastCenter;
        d.ScaleDelta = (s.lastDistance > 0.0f) ? distance / s.lastDistance : 1.0f;
        d.Scale = pinchScale;
        d.Kind = (s.gesture == ImGuiTouchPinchGesture_Pinch) ? ImGuiTouchPinchKind_Pinch
               : (s.gesture == ImGuiTouchPinchGesture_Undecided) ? ImGuiTouchPinchKind_Undecided
                                                                  : ImGuiTouchPinchKind_Pan;
        s.lastDistance = distance;
        s.lastCenter = center;
    }

    if (s.gesture != ImGuiTouchPinchGesture_None && !twoFingers)
        EndGesture(g, s);
}

}  // namespace ImGuiTouch
