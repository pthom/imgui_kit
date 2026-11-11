// The swipe layer (imgui_kit_touch_scroll.cpp) through Dear ImGui's null backend: the mouse and its source are
// injected, the scroll of the windows is read back. The layer's steps are called directly, frame by frame.
#define IMGUI_DEFINE_MATH_OPERATORS
#include "doctest.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_null.h"
#include "imgui_kit.h"
#include "imgui_kit_internal.h"

namespace
{

// A window at (0,0), 400x300: a button, a slider, a child of 30 lines, a wide child of one line, then 100 lines
struct Bench
{
    Bench()
    {
        ImGui::CreateContext();
        ImGuiTouch::CreateCurrentContextIfNeeded();  // the steps are called directly, without the setups
        ImGui_ImplNull_Init();
    }
    ~Bench()
    {
        ImGui_ImplNull_Shutdown();
        ImGui::DestroyContext();
    }

    ImGuiTouchScrollMode mode = ImGuiTouchScrollMode_Always;
    bool LongPressIsRightClick = true;
    bool ThinScrollbars = false;
    bool item_popup_open = false, window_popup_open = false;  // the context menus (right clicks) of the button and the
                                                          // window
    bool right_down = false;  // io.MouseDown[1], as of the last frame
    int right_clicks = 0;     // the right clicks seen by the widgets
    bool pinch_scales_font = true;
    bool short_content = false;  // the window's lines fit: nothing to scroll (the child still scrolls)
    bool canvas = false;        // a canvas below the wide child: an item that sums its drags
    bool canvas_takes_drags = false;  // it calls SetItemTakesDrags()
    bool canvas_long_press_is_right_click = true;  // its argument
    int canvas_drag_axis = -1;   // >= 0: it calls SetItemTakesDrags(axis) instead
    bool wide_content = false;  // the window's content is wider than it: it scrolls sideways too
    bool canvas_takes_pinch = false;  // the adapters: SetItemPinchAsWheel() (1.1 per notch) and
                                    // SetItemTwoFingerPanAsMouseDrag()
    bool canvas_pan_only = false;     // only SetItemTwoFingerPanAsMouseDrag()
    ImGuiMouseButton canvas_pan_button = ImGuiMouseButton_Right;
    ImGuiTouchPinchDelivery canvas_delivery = ImGuiTouchPinchDelivery_WheelNotches;
    bool canvas_pinch_data = false;   // it reads the gesture (ItemPinch) instead
    ImGuiTouchPinch canvas_pinch;    // the last frame's
    float canvas_data_zoom = 1.f;     // the products of ScaleDelta, the sums of CenterDelta
    ImVec2 canvas_data_pan;
    int canvas_started = 0, canvas_ended = 0, canvas_active_frames = 0;
    ImGuiTouchPinchKind canvas_first_kind = -1;  // the first Kind other than Undecided
    float canvas_zoom = 1.f;
    ImVec2 canvas_wheel_pos;  // the mouse when the canvas last zoomed
    int canvas_rect_claim = 0;  // 1: the canvas's rectangle takes the pinch (SetRectPinchAsWheel); 2: with its id
    int claim_canvases = 0;  // > 0: a rectangle (0,0)-(50,25) takes the pinch, inside 1 or 2 canvases (PushClaimTransform)
    float claim_wheel = 0.f;  // the wheel seen while it is claimed
    ImGuiID canvas_id = 0;
    ImVec2 canvas_drag;
    ImGuiID button_id = 0;
    bool steal = false;  // the GUI takes the active id while the button is down (a widget that wants a long press)
    ImGuiID steal_id = 0;
    int clicks = 0;
    int repeats = 0;  // the presses of a button that repeats while held
    int double_clicks = 0;  // the frames where the widgets would see a double click (read after the layer ran)
    float slider = 0.5f;
    float scrollY = 0.f, child_scroll_y = 0.f, wide_scroll_x = 0.f;  // as of the last frame's Begin()
    float scroll_max_y = 0.f;
    float last_vtx_y = 0.f;  // the y of the window's last drawn vertex (its last visible line): the overscroll moves it
    // Screen coordinates, as of the last frame
    ImRect button_rect, slider_rect, repeat_rect, child_rect, wide_rect, canvasRect, lines_rect;
    ImRect held_bar_rect;  // what DrawTouchScrollbarGrab() drew in the last frame: an inverted rect when nothing
    ImGuiWindow* bench_window = nullptr;  // their scroll bars: ImGui::GetWindowScrollbarRect()
    ImGuiWindow* wide_window = nullptr;
    ImGuiWindow* child_window = nullptr;
    ImVec2 mouse;

    void Frame()
    {
        ImGuiTouch::PinchBeforeNewFrame();  // as the hooks do, before ImGui::NewFrame()
        ImGui_ImplNull_NewFrame();
        ImGui::NewFrame();
        // As the hooks do: the layer runs before any widget (it acts on the frame's input before the widgets see it)
        ImGuiTouch::ApplyFingerCancel();
        ImGuiTouch::UpdateTouchScroll(mode, LongPressIsRightClick, ThinScrollbars);
        ImGuiTouch::UpdateTouchPinch(pinch_scales_font);
        if (ImGui::GetIO().MouseClickedCount[ImGuiMouseButton_Left] == 2)
            double_clicks++;
        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400.f, 300.f), ImGuiCond_Always);
        ImGui::Begin("Bench", nullptr,
                     ImGuiWindowFlags_NoTitleBar | (wide_content ? ImGuiWindowFlags_HorizontalScrollbar : 0));
        bench_window = ImGui::GetCurrentWindow();
        scrollY = ImGui::GetScrollY();
        scroll_max_y = ImGui::GetScrollMaxY();
        right_down = ImGui::GetIO().MouseDown[1];
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            right_clicks++;
        if (steal && ImGui::IsMouseDown(0))
        {
            steal_id = ImGui::GetID("steal");
            ImGui::SetActiveID(steal_id, ImGui::GetCurrentWindow());
            ImGui::KeepAliveID(steal_id);
        }
        if (ImGui::Button("Button"))
            clicks++;
        button_id = ImGui::GetItemID();
        button_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        item_popup_open = false;
        if (ImGui::BeginPopupContextItem("button_menu"))
        {
            item_popup_open = true;
            ImGui::EndPopup();
        }
        ImGui::SliderFloat("Slider", &slider, 0.f, 1.f);
        slider_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::Button("Repeat"))
            repeats++;
        ImGui::PopItemFlag();
        repeat_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::BeginChild("Child", ImVec2(0.f, 80.f), ImGuiChildFlags_Borders);
        child_scroll_y = ImGui::GetScrollY();
        child_window = ImGui::GetCurrentWindow();
        for (int i = 0; i < 30; ++i)
            ImGui::Text("Child line %d", i);
        ImGui::EndChild();
        child_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        // Tall enough: one line plus its scrollbar, no vertical scroll
        ImGui::BeginChild("Wide", ImVec2(0.f, 60.f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        wide_scroll_x = ImGui::GetScrollX();
        wide_window = ImGui::GetCurrentWindow();
        ImGui::Text("A wide line, wider than the window, so that this child scrolls sideways and not vertically");
        ImGui::EndChild();
        wide_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        if (canvas)
        {
            ImGui::InvisibleButton("Canvas", ImVec2(300.f, 50.f));
            canvas_id = ImGui::GetItemID();
            canvasRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            if (ImGui::IsItemActive())
                canvas_drag += ImGui::GetIO().MouseDelta;
            if (canvas_takes_drags)
                ImGuiTouch::SetItemTakesDrags(ImGuiTouchAxis_Any, canvas_long_press_is_right_click);
            if (canvas_drag_axis >= 0)
                ImGuiTouch::SetItemTakesDrags(canvas_drag_axis, canvas_long_press_is_right_click);
            if (canvas_pinch_data)
            {
                canvas_pinch = ImGuiTouch::ItemPinch();
                if (canvas_pinch.Active)
                {
                    canvas_data_zoom *= canvas_pinch.ScaleDelta;
                    canvas_data_pan += canvas_pinch.CenterDelta;
                    canvas_active_frames++;
                    if (canvas_first_kind < 0 && canvas_pinch.Kind != ImGuiTouchPinchKind_Undecided)
                        canvas_first_kind = canvas_pinch.Kind;
                }
                canvas_started += canvas_pinch.Started ? 1 : 0;
                canvas_ended += canvas_pinch.Ended ? 1 : 0;
            }
            if (canvas_pan_only)
                ImGuiTouch::SetItemTwoFingerPanAsMouseDrag(canvas_pan_button);
            if (canvas_rect_claim > 0)
            {
                ImGuiTouch::SetRectPinchAsWheel(canvasRect.Min, canvasRect.Max, 1.1f, ImGuiTouchPinchDelivery_WheelNotches,
                                                canvas_rect_claim == 2 ? canvas_id : 0);
                if (ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY) && ImGui::GetIO().MouseWheel != 0.f)
                    canvas_zoom *= powf(1.1f, ImGui::GetIO().MouseWheel);
            }
            if (canvas_takes_pinch)
            {
                ImGuiTouch::SetItemPinchAsWheel(1.1f, canvas_delivery);
                ImGuiTouch::SetItemTwoFingerPanAsMouseDrag(canvas_pan_button);
                if (ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY) && ImGui::GetIO().MouseWheel != 0.f)
                {
                    canvas_zoom *= powf(1.1f, ImGui::GetIO().MouseWheel);
                    canvas_wheel_pos = ImGui::GetIO().MousePos;
                }
            }
        }
        if (claim_canvases > 0)
        {
            // The canvases show it at 2x: one from (100,150) on the screen; two nested, from (140,170)
            if (claim_canvases == 1)
                ImGuiTouch::PushClaimTransform(2.f, ImVec2(100.f, 150.f));
            else
            {
                ImGuiTouch::PushClaimTransform(4.f, ImVec2(60.f, 130.f));
                ImGuiTouch::PushClaimTransform(0.5f, ImVec2(20.f, 10.f));
            }
            ImGuiTouch::SetRectPinchAsWheel(ImVec2(0.f, 0.f), ImVec2(50.f, 25.f), 1.1f,
                                            ImGuiTouchPinchDelivery_WheelNotches);
            for (int i = 0; i < claim_canvases; ++i)
                ImGuiTouch::PopClaimTransform();
            claim_wheel += ImGui::GetIO().MouseWheel;
        }
        if (wide_content)
            ImGui::Dummy(ImVec2(800.f, 1.f));  // twice the window's width
        ImVec2 lines_pos = ImGui::GetCursorScreenPos();
        for (int i = 0; i < (short_content ? 2 : 100); ++i)
            ImGui::Text("Line %d", i);
        lines_rect = ImRect(lines_pos, ImVec2(lines_pos.x + 300.f, 290.f));
        window_popup_open = false;
        if (ImGui::BeginPopupContextWindow("window_menu",
                                           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            window_popup_open = true;
            ImGui::EndPopup();
        }
        ImGui::End();
        ImDrawList* foreground = ImGui::GetForegroundDrawList();
        const int vtx_before = foreground->VtxBuffer.Size;
        ImGuiTouch::DrawTouchScrollbarGrab();  // as the hooks do, after the GUI
        held_bar_rect = ImRect(ImVec2(FLT_MAX, FLT_MAX), ImVec2(-FLT_MAX, -FLT_MAX));
        for (int i = vtx_before; i < foreground->VtxBuffer.Size; ++i)
            held_bar_rect.Add(foreground->VtxBuffer[i].pos);
        ImGui::Render();
        ImGuiTouch::ApplyTouchOverscroll();
        last_vtx_y = ImGui::FindWindowByName("Bench")->DrawList->VtxBuffer.back().pos.y;
        ImGui_ImplNullRender_RenderDrawData(ImGui::GetDrawData());
    }
    void Frames(int n) { for (int i = 0; i < n; ++i) Frame(); }

    void MoveTo(ImVec2 p)
    {
        mouse = p;
        ImGui::GetIO().AddMousePosEvent(p.x, p.y);
        Frame();
    }
    void Press(ImVec2 p)
    {
        MoveTo(p);
        ImGui::GetIO().AddMouseButtonEvent(0, true);
        Frame();
    }
    // The finger moves by `delta`, in steps of one frame
    void Drag(ImVec2 delta, int steps)
    {
        for (int i = 1; i <= steps; ++i)
            MoveTo(mouse + delta * ((float)i / (float)steps) - delta * ((float)(i - 1) / (float)steps));
    }
    // A release after a pause: the finger speed has decayed, no inertia follows
    void ReleaseStill()
    {
        Frames(15);
        Release();
    }
    void Release()
    {
        ImGui::GetIO().AddMouseButtonEvent(0, false);
        Frames(4);  // the replayed press and release, the scroll target, a frame to read
    }
    void Source(ImGuiMouseSource source) { ImGui::GetIO().AddMouseSourceEvent(source); }
};

bool Near(float a, float b, float tol = 3.f) { return ImFabs(a - b) <= tol; }
}  // namespace

TEST_CASE("Touch scroll: a swipe on the lines scrolls the window, a tap does not")
{
    Bench b;
    b.Frames(3);
    REQUIRE(b.scrollY == 0.f);
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.ReleaseStill();
    CHECK(Near(b.scrollY, 50.f));
    CHECK(ImGui::GetCurrentContext()->ActiveId == 0);

    float before = b.scrollY;
    b.Press(b.lines_rect.GetCenter());
    b.ReleaseStill();
    CHECK(b.scrollY == before);
    CHECK(b.clicks == 0);
}

TEST_CASE("Touch scroll: Auto acts with a touch source only, Disabled never")
{
    Bench b;
    b.mode = ImGuiTouchScrollMode_Auto;
    b.Frames(3);
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.ReleaseStill();
    CHECK(b.scrollY == 0.f);
    b.Press(b.button_rect.GetCenter());
    CHECK(ImGui::GetCurrentContext()->ActiveId != 0);  // the button, at once
    b.Release();
    CHECK(b.clicks == 1);

    b.Source(ImGuiMouseSource_TouchScreen);
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.ReleaseStill();
    CHECK(Near(b.scrollY, 50.f));

    b.mode = ImGuiTouchScrollMode_Disabled;
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.ReleaseStill();
    CHECK(Near(b.scrollY, 50.f));
}

TEST_CASE("Touch scroll: a tap on a button clicks it when the finger lifts, a swipe from it scrolls")
{
    Bench b;
    b.Frames(3);
    b.Press(b.button_rect.GetCenter());
    b.Frames(2);
    CHECK(b.clicks == 0);  // nothing yet: the press is held back
    b.Release();
    CHECK(b.clicks == 1);  // replayed
    CHECK(b.scrollY == 0.f);

    b.Press(b.button_rect.GetCenter());
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.ReleaseStill();
    CHECK(Near(b.scrollY, 50.f));
    CHECK(b.clicks == 1);
}

TEST_CASE("Touch scroll: two taps are a double click, a tap after a pause is not, one tap never is")
{
    Bench b;
    b.Frames(3);
    b.Press(b.button_rect.GetCenter());
    b.Release();
    CHECK(b.clicks == 1);
    CHECK(b.double_clicks == 0);  // the replayed press does not pair with the finger's own press
    b.Press(b.button_rect.GetCenter());  // about 0.1 s after the first tap
    b.Release();
    CHECK(b.clicks == 2);
    CHECK(b.double_clicks == 1);
    b.Frames(30);  // half a second
    b.Press(b.button_rect.GetCenter());
    b.Release();
    CHECK(b.clicks == 3);
    CHECK(b.double_clicks == 1);
    // A finger's two taps land apart: 12 px (ImGui's 6 px suit a mouse), and may take 0.35 s
    b.Frames(30);
    b.Source(ImGuiMouseSource_TouchScreen);
    b.Press(b.button_rect.GetCenter());
    b.Release();
    b.Frames(14);  // about 0.35 s from the first tap's press to the second's
    b.Press(b.button_rect.GetCenter() + ImVec2(12.f, 0.f));
    b.Release();
    CHECK(b.clicks == 5);
    CHECK(b.double_clicks == 2);
}

TEST_CASE("Touch scroll: a hold hands the press to the widget under the finger")
{
    Bench b;
    b.Frames(3);
    b.Press(b.slider_rect.GetCenter());
    b.Frames(20);  // past the hold delay, the slider got the press
    b.Drag(ImVec2(60.f, 0.f), 5);
    b.ReleaseStill();
    CHECK(b.slider > 0.6f);
    CHECK(b.scrollY == 0.f);

    b.Press(b.lines_rect.GetCenter());  // a hold on the void: nothing
    b.Frames(20);
    b.Release();
    CHECK(b.scrollY == 0.f);
    CHECK(b.clicks == 0);

    b.Press(b.button_rect.GetCenter());  // a long tap still clicks
    b.Frames(20);
    b.Release();
    CHECK(b.clicks == 1);
}

TEST_CASE("Touch scroll: on a touch screen, a lift leaves no pointer, so nothing is hovered while the content coasts")
{
    Bench b;
    b.mode = ImGuiTouchScrollMode_Auto;
    b.Frames(3);
    b.Press(b.button_rect.GetCenter());  // a mouse: the pointer stays, the button is hovered after the click
    b.Release();
    CHECK(b.clicks == 1);
    CHECK(ImGui::IsMousePosValid());
    CHECK(ImGui::GetCurrentContext()->HoveredId != 0);

    b.Source(ImGuiMouseSource_TouchScreen);
    b.Press(b.button_rect.GetCenter());  // a finger: the tap clicks, then no pointer, nothing hovered
    b.Release();
    CHECK(b.clicks == 2);
    CHECK(!ImGui::IsMousePosValid());
    CHECK(ImGui::GetCurrentContext()->HoveredId == 0);

    b.Press(b.lines_rect.GetCenter());  // a flick: nothing hovered while the content coasts
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.Release();
    b.Frames(10);
    CHECK(b.scrollY > 50.f);
    CHECK(!ImGui::IsMousePosValid());
    CHECK(ImGui::GetCurrentContext()->HoveredId == 0);

    b.MoveTo(b.button_rect.GetCenter());  // the next touch brings a position back
    b.Frame();
    CHECK(ImGui::IsMousePosValid());

}

TEST_CASE("Touch scroll: on a touch screen, the release of a handover is not a lift, the pointer stays")
{
    Bench b;
    b.mode = ImGuiTouchScrollMode_Auto;
    b.Frames(3);
    b.Source(ImGuiMouseSource_TouchScreen);
    b.Press(b.slider_rect.GetCenter());  // a hold: the slider gets the press, with the pointer where the finger is
    b.Frames(20);
    CHECK(ImGui::IsMousePosValid());
    CHECK(ImGui::GetCurrentContext()->ActiveId != 0);
    b.Release();
    CHECK(!ImGui::IsMousePosValid());
}

// Two fingers on the screen (ids 1 and 2), as a platform reports them; their lift
void Fingers(ImVec2 a, ImVec2 b)
{
    ImGuiTouch::AddFingerEvent(1, a, ImGuiTouchPhase_Moved);
    ImGuiTouch::AddFingerEvent(2, b, ImGuiTouchPhase_Moved);
}
void LiftFingers()
{
    ImGuiTouch::AddFingerEvent(1, ImVec2(0, 0), ImGuiTouchPhase_Ended);
    ImGuiTouch::AddFingerEvent(2, ImVec2(0, 0), ImGuiTouchPhase_Ended);
}

TEST_CASE("Touch: a cancelled finger ends its press with no click, on a button and on the swipe")
{
    Bench b;
    b.Frames(3);
    const ImVec2 p = b.button_rect.GetCenter();
    b.Press(p);
    ImGuiTouch::AddFingerEvent(1, p, ImGuiTouchPhase_Began);
    b.Frames(20);  // held: the button has the press
    ImGuiTouch::AddFingerEvent(1, p, ImGuiTouchPhase_Cancelled);
    b.Release();   // the platform's mouse lifts too
    CHECK(b.clicks == 0);

    // A tap (a short press) that the system cancels: no click either
    b.Press(p);
    ImGuiTouch::AddFingerEvent(1, p, ImGuiTouchPhase_Began);
    b.Frame();
    ImGuiTouch::AddFingerEvent(1, p, ImGuiTouchPhase_Cancelled);
    b.Release();
    CHECK(b.clicks == 0);

    // And an ended finger still clicks
    b.Press(p);
    ImGuiTouch::AddFingerEvent(1, p, ImGuiTouchPhase_Began);
    b.Frame();
    ImGuiTouch::AddFingerEvent(1, p, ImGuiTouchPhase_Ended);
    b.Release();
    CHECK(b.clicks == 1);
}

TEST_CASE("Touch pinch: two fingers scale the font, and take the press from the swipe layer")
{
    Bench b;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -30.f), 3);
    Fingers(ImVec2(100, 100), ImVec2(200, 100));  // the second finger lands, 100 px from the first
    b.Frame();
    Fingers(ImVec2(100, 100), ImVec2(250, 100));  // the fingers spread: 150 px, a scale of 1.5
    b.Frames(2);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.5f);
    b.Drag(ImVec2(0.f, -30.f), 3);  // the first finger's motion scrolls no more
    CHECK(Near(b.scrollY, 30.f));
    Fingers(ImVec2(100, 100), ImVec2(700, 100));  // a scale of 6: clamped
    b.Frame();
    CHECK(ImGui::GetStyle().FontScaleMain == 5.f);
    LiftFingers();  // the scale stays, no tap, no inertia
    b.Release();
    b.Frames(10);
    CHECK(ImGui::GetStyle().FontScaleMain == 5.f);
    CHECK(Near(b.scrollY, 30.f));
    CHECK(b.clicks == 0);
    ImGui::GetStyle().FontScaleMain = 1.f;
}

TEST_CASE("Touch pinch: two fingers that move together are a right drag, the left press is released")
{
    Bench b;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    b.Press(b.lines_rect.GetCenter());
    Fingers(ImVec2(100, 100), ImVec2(200, 100));  // the second finger lands
    b.Frames(2);
    CHECK(!b.right_down);
    Fingers(ImVec2(100, 70), ImVec2(202, 70));  // the two move together, 30 px up (and spread a little: 1.02)
    b.Frames(4);
    CHECK(b.right_down);
    CHECK(!ImGui::GetIO().MouseDown[0]);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
    b.Drag(ImVec2(0.f, -30.f), 3);  // the first finger moves: no scroll (it is a right drag now)
    CHECK(b.scrollY == 0.f);
    LiftFingers();
    b.Release();
    CHECK(!b.right_down);
    CHECK(b.clicks == 0);
}

TEST_CASE("Touch pinch: a widget that holds the press keeps it")
{
    Bench b;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    b.Press(b.slider_rect.GetCenter());
    b.Frames(20);  // the hold: the slider got the press
    Fingers(ImVec2(100, 100), ImVec2(200, 100));
    b.Frame();
    Fingers(ImVec2(100, 100), ImVec2(250, 100));  // a scale of 1.5
    b.Frames(2);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.f);  // the slider keeps its drag
    b.Drag(ImVec2(60.f, 0.f), 5);
    CHECK(b.slider > 0.6f);
    LiftFingers();
    b.Release();
    ImGui::GetStyle().FontScaleMain = 1.f;
}

TEST_CASE("Touch scroll: a press is not held back when nothing can scroll, nor a drag along an axis nothing scrolls")
{
    {
        Bench b;
        b.short_content = true;
        b.Frames(3);
        b.Press(b.button_rect.GetCenter());  // the window fits: the button gets the press at once
        CHECK(ImGui::GetCurrentContext()->ActiveId == b.button_id);
        b.Release();
        CHECK(b.clicks == 1);
        b.Press(b.child_rect.GetCenter());  // the child scrolls: a swipe there still works
        b.Drag(ImVec2(0.f, -30.f), 3);
        b.ReleaseStill();
        CHECK(Near(b.child_scroll_y, 30.f));
    }  // one ImGui context at a time
    Bench c;
    c.Frames(3);
    c.Press(c.slider_rect.GetCenter());  // the window scrolls vertically only: a horizontal drag goes to the slider
    c.Drag(ImVec2(60.f, 0.f), 5);       // at once, no hold
    c.ReleaseStill();
    CHECK(c.slider > 0.6f);
    CHECK(c.scrollY == 0.f);
}

TEST_CASE("Touch scroll: a widget that takes the drags gets the press at once, also along the axis the window scrolls")
{
    {
        Bench b;
        b.canvas = true;
        b.Frames(3);
        b.Press(b.canvasRect.GetCenter());  // a plain item: the press is held back, a vertical drag scrolls the window
        CHECK(ImGui::GetCurrentContext()->ActiveId != b.canvas_id);
        b.Drag(ImVec2(0.f, -40.f), 4);
        b.ReleaseStill();
        CHECK(b.scrollY > 0.f);
        CHECK(b.canvas_drag.y == 0.f);
    }  // one ImGui context at a time
    Bench c;
    c.canvas = c.canvas_takes_drags = true;
    c.Frames(3);
    c.Press(c.canvasRect.GetCenter());  // SetItemTakesDrags(): the canvas has the press at once, and the drag
    CHECK(ImGui::GetCurrentContext()->ActiveId == c.canvas_id);
    c.Drag(ImVec2(0.f, -40.f), 4);
    c.ReleaseStill();
    CHECK(c.scrollY == 0.f);
    CHECK(Near(c.canvas_drag.y, -40.f));
    c.Press(c.lines_rect.GetCenter());  // elsewhere, the window still scrolls
    c.Drag(ImVec2(0.f, -40.f), 4);
    c.ReleaseStill();
    CHECK(c.scrollY > 0.f);
}

TEST_CASE("Touch scroll: a long press is a right click when the finger lifts, a shorter one or a moving one is not")
{
    Bench b;
    b.Frames(3);
    b.Press(b.button_rect.GetCenter());
    b.Frames(40);  // 0.66 s still: the hold gave the button the press, the long press is armed, nothing happens yet
    CHECK(!b.item_popup_open);
    CHECK(ImGui::GetIO().MouseDown[0]);
    CHECK(ImGui::GetCurrentContext()->ActiveId == b.button_id);
    b.Release();  // the lift: a right click, and the button does not click
    CHECK(b.item_popup_open);
    CHECK(b.clicks == 0);
    CHECK(b.right_clicks == 1);
    b.Press(b.lines_rect.GetCenter());  // away from the menu: it closes
    b.Release();
    CHECK(!b.item_popup_open);

    b.Press(b.lines_rect.GetCenter());  // on the void
    b.Frames(40);
    CHECK(!b.window_popup_open);
    b.Release();
    CHECK(b.window_popup_open);
    b.Press(ImVec2(b.lines_rect.Min.x + 10.f, b.lines_rect.Min.y + 10.f));  // away from the menu: it closes
    b.Release();
    CHECK(!b.window_popup_open);

    b.Press(b.button_rect.GetCenter());  // a shorter hold: a click when the finger lifts
    b.Frames(15);
    b.Release();
    CHECK(b.clicks == 1);
    CHECK(!b.item_popup_open);

    b.Press(b.lines_rect.GetCenter());  // a finger that moves before the delay: a swipe, no menu
    b.Frames(10);
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.Frames(40);
    CHECK(!b.window_popup_open);
    b.ReleaseStill();

    b.LongPressIsRightClick = false;
    b.Press(b.button_rect.GetCenter());
    b.Frames(40);
    b.Release();
    CHECK(!b.item_popup_open);
    CHECK(b.clicks == 2);
}

TEST_CASE("Touch scroll: the hold's and the long press's delays come from the config")
{
    Bench b;
    b.Frames(3);
    ImGuiTouchConfig& config = ImGuiTouch::GetConfig();
    b.Press(b.button_rect.GetCenter());
    b.Frames(15);  // 0.25 s: past the default hold, the button has the press
    CHECK(ImGui::GetCurrentContext()->ActiveId == b.button_id);
    b.Release();
    config.HoldSeconds = 1.f;
    b.Press(b.button_rect.GetCenter());
    b.Frames(15);  // not yet: the layer still holds it
    CHECK(ImGui::GetCurrentContext()->ActiveId != b.button_id);
    b.Frames(50);  // 1.08 s: now
    CHECK(ImGui::GetCurrentContext()->ActiveId == b.button_id);
    b.Release();
    config.HoldSeconds = 0.15f;

    config.LongPressSeconds = 2.f;
    const int right_clicks = b.right_clicks;  // the hold of one second above lifted after the long press: one
    b.Press(b.button_rect.GetCenter());
    b.Frames(40);  // 0.66 s: a long press by default, not with this setting
    b.Release();
    CHECK(!b.item_popup_open);
    CHECK(b.right_clicks == right_clicks);
    config.LongPressSeconds = 0.5f;
}

TEST_CASE("Touch scroll: a widget that takes the drags may keep a still finger past the long press")
{
    {
        Bench b;
        b.canvas = b.canvas_takes_drags = true;
        b.Frames(3);
        b.Press(b.canvasRect.GetCenter());
        b.Frames(40);  // 0.66 s still: the canvas keeps the press; the lift will be a right click
        CHECK(ImGui::GetCurrentContext()->ActiveId == b.canvas_id);
        b.Release();
        CHECK(b.right_clicks == 1);
    }  // one ImGui context at a time
    Bench c;
    c.canvas = c.canvas_takes_drags = true;
    c.canvas_long_press_is_right_click = false;  // a piano key: held as long as the finger stays, and no right click
    c.Frames(3);
    c.Press(c.canvasRect.GetCenter());
    c.Frames(40);
    CHECK(ImGui::GetCurrentContext()->ActiveId == c.canvas_id);
    CHECK(ImGui::GetIO().MouseDown[0]);
    c.Release();
    CHECK(c.right_clicks == 0);
}

TEST_CASE("Touch scroll: a button that repeats keeps the finger past the long press, and repeats")
{
    Bench b;
    b.Frames(3);
    b.Source(ImGuiMouseSource_TouchScreen);
    b.Press(b.repeat_rect.GetCenter());
    b.Frames(60);  // one second: the hold hands the press over, the repeats start, the long press would fire at 0.5 s
    CHECK(b.repeats > 5);
    CHECK(b.window_popup_open == false);
    CHECK(b.item_popup_open == false);
    int repeats_so_far = b.repeats;
    b.Frames(12);
    CHECK(b.repeats > repeats_so_far);  // still repeating: the press was not taken away
    b.Release();
}

TEST_CASE("Touch scroll: a long press on a slider, with a touch source, does not move it")
{
    Bench b;
    b.mode = ImGuiTouchScrollMode_Auto;
    b.Frames(3);
    b.Source(ImGuiMouseSource_TouchScreen);
    b.Press(b.slider_rect.GetCenter());
    b.Frames(15);  // the hold gave the slider the press, at the finger
    float held = b.slider;
    CHECK(held > 0.3f);
    b.Frames(30);  // the long press is armed: the slider keeps the press
    CHECK(b.slider == held);
    CHECK(ImGui::GetIO().MouseDown[0]);
    b.Release();  // a right click, at no position for the slider: it stays
    CHECK(b.slider == held);
    CHECK(b.right_clicks == 1);
}

TEST_CASE("Touch scroll: after a long press, a move keeps the widget's drag, and cancels the right click")
{
    Bench b;
    b.mode = ImGuiTouchScrollMode_Auto;
    b.Frames(3);
    b.Source(ImGuiMouseSource_TouchScreen);
    b.Press(b.slider_rect.GetCenter());
    b.Frames(45);  // the hold gave the slider the press, the long press is armed
    float held = b.slider;
    b.Drag(ImVec2(60.f, 0.f), 6);  // the drag after a pause, as on a phone
    CHECK(b.slider > held);
    CHECK(ImGui::GetCurrentContext()->ActiveId != 0);
    b.Release();
    CHECK(b.right_clicks == 0);
    CHECK(!b.item_popup_open);
    CHECK(!b.window_popup_open);
}

TEST_CASE("Touch scroll: with a finger, a swipe on a scroll bar scrolls the content with the finger")
{
    {
        Bench b;
        b.Frames(3);
        b.Source(ImGuiMouseSource_TouchScreen);
        b.Press(b.lines_rect.GetCenter());  // some scroll first
        b.Drag(ImVec2(0.f, -100.f), 5);
        b.ReleaseStill();
        float before = b.scrollY;
        REQUIRE(before > 50.f);
        b.Press(ImGui::GetWindowScrollbarRect(b.bench_window, ImGuiAxis_Y).GetCenter());
        b.Drag(ImVec2(0.f, 40.f), 4);  // down, on the vertical bar
        b.ReleaseStill();
        CHECK(Near(b.scrollY, before - 40.f));  // the content followed the finger, as on the content

        b.Press(ImGui::GetWindowScrollbarRect(b.wide_window, ImGuiAxis_X).GetCenter());
        b.Drag(ImVec2(-40.f, 0.f), 4);  // left, on the horizontal bar of the wide child
        b.ReleaseStill();
        CHECK(Near(b.wide_scroll_x, 40.f));
    }  // one ImGui context at a time
    Bench c;  // with the mouse, the bar is a control: the same drag moves the content forward (its thumb, a page)
    c.Frames(3);
    c.Press(c.lines_rect.GetCenter());
    c.Drag(ImVec2(0.f, -100.f), 5);
    c.ReleaseStill();
    float before = c.scrollY;
    c.Press(ImGui::GetWindowScrollbarRect(c.bench_window, ImGuiAxis_Y).GetCenter());
    c.Drag(ImVec2(0.f, 40.f), 4);
    c.ReleaseStill();
    CHECK(c.scrollY > before);
}

TEST_CASE("Touch scroll: thin scroll bars on a touch screen, when asked")
{
    Bench b;
    b.Frames(2);
    const float theme_size = ImGui::GetStyle().ScrollbarSize;
    b.ThinScrollbars = true;
    b.Frames(2);
    CHECK(ImGui::GetStyle().ScrollbarSize == theme_size);  // not a touch screen
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    b.Frames(2);
    CHECK(ImGui::GetStyle().ScrollbarSize < theme_size);
    CHECK(ImGui::GetStyle().ScrollbarSize >= 1.f);
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_IsTouchScreen;
}

TEST_CASE("Touch scroll: a touch laptop driven by its mouse keeps the theme's bars, thin again at the next finger")
{
    Bench b;
    b.ThinScrollbars = true;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    b.Frames(2);
    const float thin_size = ImGui::GetStyle().ScrollbarSize;
    b.Source(ImGuiMouseSource_Mouse);
    b.MoveTo(ImVec2(100.f, 100.f));
    b.MoveTo(ImVec2(110.f, 100.f));
    b.Frames(2);
    CHECK(ImGui::GetStyle().ScrollbarSize > thin_size);  // the theme's
    b.Source(ImGuiMouseSource_TouchScreen);  // tags the next event: a finger that moves
    b.MoveTo(ImVec2(111.f, 100.f));
    b.Frames(2);
    CHECK(ImGui::GetStyle().ScrollbarSize == thin_size);
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_IsTouchScreen;
}

TEST_CASE("Touch scroll: thin scroll bars turned off give the theme's bars back, a theme applied meanwhile included")
{
    Bench b;
    b.Frames(2);
    ImGuiStyle& style = ImGui::GetStyle();
    const float theme_size = style.ScrollbarSize, theme_rounding = style.ScrollbarRounding;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    b.ThinScrollbars = true;
    b.Frames(2);
    CHECK(style.ScrollbarSize < theme_size);
    b.ThinScrollbars = false;
    b.Frames(2);
    CHECK(style.ScrollbarSize == theme_size);
    CHECK(style.ScrollbarRounding == theme_rounding);

    // A theme applied while the bars are thin: its values come back
    b.ThinScrollbars = true;
    b.Frames(2);
    style.ScrollbarSize = 20.f;
    style.ScrollbarRounding = 2.f;
    b.Frames(2);
    CHECK(style.ScrollbarSize < 20.f);
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_IsTouchScreen;  // no longer a touch screen: the same
    b.Frames(2);
    CHECK(style.ScrollbarSize == 20.f);
    CHECK(style.ScrollbarRounding == 2.f);
    style.ScrollbarSize = theme_size;
    style.ScrollbarRounding = theme_rounding;
}

TEST_CASE("Touch scroll: a thin scroll bar held by a finger widens over the content")
{
    Bench b;
    b.ThinScrollbars = true;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    b.Frames(3);
    b.Source(ImGuiMouseSource_TouchScreen);
    // The child's bar: the bench window's is under its resize border, which would take the hold
    const ImRect bar = ImGui::GetWindowScrollbarRect(b.child_window, ImGuiAxis_Y);
    b.Press(bar.GetCenter());
    CHECK(b.held_bar_rect.Min.x > b.held_bar_rect.Max.x);  // a swipe so far: nothing drawn
    b.Frames(20);  // past the hold delay, the bar got the press
    CHECK(ImGui::GetActiveID() == ImGui::GetWindowScrollbarID(b.child_window, ImGuiAxis_Y));
    CHECK(b.held_bar_rect.Min.x < bar.Min.x - 3.f);  // its thumb, wider, over the content
    CHECK(b.held_bar_rect.Max.x <= bar.Max.x + 1.f);  // along the window's edge
    CHECK(b.held_bar_rect.Min.y >= bar.Min.y - 1.f);
    CHECK(b.held_bar_rect.Max.y <= bar.Max.y + 1.f);
    b.Release();
    CHECK(b.held_bar_rect.Min.x > b.held_bar_rect.Max.x);
}

TEST_CASE("Touch scroll: a widget that takes the active id after the press ends the swipe")
{
    Bench b;
    b.Frames(3);
    b.Press(b.lines_rect.GetCenter());
    b.steal = true;
    b.Frame();
    CHECK(ImGui::GetCurrentContext()->ActiveId == b.steal_id);
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.ReleaseStill();
    CHECK(b.scrollY == 0.f);
}

TEST_CASE("Touch scroll: a child scrolls itself, and its parent when it cannot scroll that way")
{
    Bench b;
    b.Frames(3);
    b.Press(b.child_rect.GetCenter());
    b.Drag(ImVec2(0.f, -30.f), 3);
    b.ReleaseStill();
    CHECK(Near(b.child_scroll_y, 30.f));
    CHECK(b.scrollY == 0.f);

    b.Press(b.wide_rect.GetCenter());
    b.Drag(ImVec2(-30.f, 0.f), 3);
    b.ReleaseStill();
    CHECK(Near(b.wide_scroll_x, 30.f));
    CHECK(b.scrollY == 0.f);

    b.Press(b.wide_rect.GetCenter());
    b.Drag(ImVec2(0.f, -30.f), 3);
    b.ReleaseStill();
    CHECK(Near(b.scrollY, 30.f));
    CHECK(Near(b.wide_scroll_x, 30.f));
}

TEST_CASE("Touch scroll: a flick keeps scrolling, then stops")
{
    Bench b;
    b.Frames(3);
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -50.f), 5);  // 10 px per frame: 600 px/s
    b.Release();
    float at_release = b.scrollY;
    b.Frames(10);
    CHECK(b.scrollY > at_release + 20.f);
    b.Frames(120);
    float stopped = b.scrollY;
    b.Frames(10);
    CHECK(b.scrollY == stopped);
}

TEST_CASE("Touch scroll: while the content coasts, the app does not idle; once it stops, it may")
{
    // The inertia moves without input events: it asks for a refresh at each frame, as an animation would
    Bench b;
    b.Frames(3);
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -50.f), 5);
    b.Release();
    ImGuiTouch::ConsumeAnimationRequest();
    b.Frame();
    CHECK(ImGuiTouch::ConsumeAnimationRequest());
    b.Frames(120);
    ImGuiTouch::ConsumeAnimationRequest();
    b.Frame();
    CHECK(!ImGuiTouch::ConsumeAnimationRequest());
}

TEST_CASE("Touch scroll: a drag past the start pulls the content, which springs back at the lift")
{
    Bench b;
    b.Frames(3);
    float rest = b.last_vtx_y;
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, 100.f), 5);  // down, while the window is at its start: nothing scrolls, the content follows
    CHECK(b.scrollY == 0.f);
    float pulled = b.last_vtx_y;
    CHECK(pulled > rest + 20.f);
    CHECK(pulled < rest + 100.f);  // less than the finger: the rubber band
    b.Frames(15);
    CHECK(b.last_vtx_y == pulled);  // held there by the finger
    b.Drag(ImVec2(0.f, -40.f), 2);  // the finger comes back: the content follows it, still nothing scrolls
    CHECK(b.last_vtx_y < pulled);
    CHECK(b.last_vtx_y > rest);
    CHECK(b.scrollY == 0.f);
    b.Release();
    b.Frames(60);
    CHECK(b.last_vtx_y == rest);
    CHECK(b.scrollY == 0.f);
}

TEST_CASE("Touch scroll: a flick that reaches the end overshoots, then springs back to the end")
{
    Bench b;
    b.Frames(3);
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -600.f), 6);  // 100 px per frame: thousands of px/s, enough to reach the end
    b.Release();
    bool overshot = false;
    for (int i = 0; i < 120 && !overshot; ++i)
    {
        b.Frame();
        if (b.scrollY == b.scroll_max_y)
        {
            float at_end = b.last_vtx_y;
            b.Frames(2);
            overshot = b.last_vtx_y < at_end;  // the content keeps going up past its end
        }
    }
    CHECK(overshot);
    b.Frames(60);
    CHECK(b.scrollY == b.scroll_max_y);
    float settled = b.last_vtx_y;
    b.Frames(5);
    CHECK(b.last_vtx_y == settled);
}

TEST_CASE("Touch pinch: on a widget that takes the pinch, it zooms the widget by wheel notches, and not the font")
{
    Bench b;
    b.canvas = true;
    b.canvas_takes_pinch = true;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c);
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));  // the second finger lands, 40 px from the first
    b.Frame();
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(34.f, 0.f));  // 54 px: a scale of 1.35, three notches of 10%
    b.Frames(6);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
    CHECK(b.canvas_zoom == doctest::Approx(1.331f));
    CHECK(Near(b.canvas_wheel_pos.x, c.x + 7.f, 1.f));  // around the fingers' center, not the first finger (c - 20)
    CHECK(Near(b.canvas_wheel_pos.y, c.y, 1.f));
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));  // back to 40 px: the notches go back
    b.Frames(6);
    CHECK(b.canvas_zoom == doctest::Approx(1.f));
    LiftFingers();
    b.Release();
    CHECK(b.clicks == 0);
    CHECK(b.scrollY == 0.f);
}

TEST_CASE("Touch pinch: outside the widgets that take it, the pinch scales the font")
{
    Bench b;
    b.canvas = true;
    b.canvas_takes_pinch = true;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    b.Press(b.lines_rect.GetCenter());
    Fingers(ImVec2(100, 100), ImVec2(200, 100));  // not on the canvas
    b.Frame();
    Fingers(ImVec2(100, 100), ImVec2(250, 100));
    b.Frames(2);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.5f);
    CHECK(b.canvas_zoom == 1.f);
    LiftFingers();
    b.Release();
    ImGui::GetStyle().FontScaleMain = 1.f;
}

TEST_CASE("Touch pinch: a widget that reads the data zooms and pans at once, from Started to Ended")
{
    Bench b;
    b.canvas = true;
    b.canvas_pinch_data = true;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c - ImVec2(20.f, 0.f));
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));  // 40 px apart
    b.Frame();
    CHECK(b.canvas_started == 1);
    CHECK(b.canvas_pinch.Active);
    CHECK(b.canvas_pinch.ScaleDelta == 1.f);
    for (int i = 1; i <= 10; i++)  // spread to 60 px (a scale of 1.5) while their middle moves 30 px right
    {
        const float k = (float)i / 10.f;
        Fingers(c + ImVec2(-20.f + 20.f * k, 0.f), c + ImVec2(20.f + 40.f * k, 0.f));
        b.Frame();
    }
    CHECK(b.canvas_pinch.Scale == doctest::Approx(1.5f));
    CHECK(b.canvas_data_zoom == doctest::Approx(1.5f));
    CHECK(b.canvas_data_pan.x == doctest::Approx(30.f));
    CHECK(b.canvas_data_pan.y == doctest::Approx(0.f));
    CHECK(b.canvas_pinch.Center.x == doctest::Approx(c.x + 30.f));
    CHECK(b.canvas_first_kind == ImGuiTouchPinchKind_Pinch);  // the spread passed 12% before the middle passed 0.6 em
    LiftFingers();
    b.Frame();
    CHECK(b.canvas_ended == 1);
    CHECK(!b.canvas_pinch.Active);
    b.Release();
    CHECK(b.canvas_ended == 1);  // one frame only
    CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
    CHECK(b.clicks == 0);
    CHECK(b.canvas_drag.x == 0.f);  // the first finger's press did not drag the canvas
}

TEST_CASE("Touch pinch: a third finger takes the place of one that lifts, and the scale goes on without a jump")
{
    Bench b;
    b.canvas = true;
    b.canvas_pinch_data = true;
    b.Frames(3);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c - ImVec2(20.f, 0.f));
    ImGuiTouch::AddFingerEvent(1, c - ImVec2(20.f, 0.f), ImGuiTouchPhase_Began);
    ImGuiTouch::AddFingerEvent(2, c + ImVec2(20.f, 0.f), ImGuiTouchPhase_Began);  // 40 px apart
    b.Frame();
    ImGuiTouch::AddFingerEvent(3, c + ImVec2(0.f, 30.f), ImGuiTouchPhase_Began);  // a third, resting below
    b.Frame();
    ImGuiTouch::AddFingerEvent(2, c + ImVec2(40.f, 0.f), ImGuiTouchPhase_Moved);  // 60 px: a scale of 1.5
    b.Frames(2);
    CHECK(b.canvas_pinch.Scale == doctest::Approx(1.5f));
    ImGuiTouch::AddFingerEvent(1, c - ImVec2(20.f, 0.f), ImGuiTouchPhase_Ended);  // the first lifts: the pair is 2, 3
    b.Frame();
    CHECK(b.canvas_pinch.Active);
    CHECK(b.canvas_pinch.ScaleDelta == doctest::Approx(1.f));  // no jump
    CHECK(b.canvas_pinch.Scale == doctest::Approx(1.5f));      // continuous
    // The new pair spreads: 2 at (c + 40, 0), 3 at (c + 0, 30): 50 px; 3 moves to (c + 0, 60): 72 px, a scale of 1.44
    ImGuiTouch::AddFingerEvent(3, c + ImVec2(0.f, 60.f), ImGuiTouchPhase_Moved);
    b.Frame();
    CHECK(b.canvas_pinch.ScaleDelta == doctest::Approx(72.11f / 50.f).epsilon(0.01));
    CHECK(b.canvas_pinch.Scale == doctest::Approx(1.5f * 72.11f / 50.f).epsilon(0.01));
    ImGuiTouch::AddFingerEvent(2, c, ImGuiTouchPhase_Ended);
    ImGuiTouch::AddFingerEvent(3, c, ImGuiTouchPhase_Ended);
    b.Release();
}

TEST_CASE("Touch scroll: a release at an invalid position (a backend that lost the pointer) starts no fling")
{
    Bench b;
    b.Frames(3);
    b.Press(b.lines_rect.GetCenter());
    b.Drag(ImVec2(0.f, -40.f), 4);  // a swipe up, fast
    const float dragged = b.scrollY;
    CHECK(dragged > 0.f);
    ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    b.Frames(34);
    CHECK(b.scrollY == dragged);  // nothing coasted
    // Without the guard, the lift speed is infinite: the overscroll goes to infinity, then NaN, the content is
    // drawn there and the bounce never ends
    CHECK(ImGuiTouch::GetTouchContext().Scroll.Overscroll == 0.f);
    ImGuiTouch::ConsumeAnimationRequest();  // (the bench has no hook that clears the flag at each frame)
    b.Frame();
    CHECK(!ImGuiTouch::ConsumeAnimationRequest());
}

TEST_CASE("Touch scroll: the long press watch ends when the layer turns off (the source becomes a mouse)")
{
    Bench b;
    b.mode = ImGuiTouchScrollMode_Auto;
    b.Source(ImGuiMouseSource_TouchScreen);
    b.Frames(3);
    b.Press(b.button_rect.GetCenter());
    b.Frames(2);
    CHECK(ImGuiTouch::GetTouchContext().Scroll.WatchingLongPress);
    b.Source(ImGuiMouseSource_Mouse);  // a mouse takes over while the finger is down (the source tags the next event)
    const ImVec2 p = b.button_rect.GetCenter();
    ImGui::GetIO().AddMousePosEvent(p.x + 1.f, p.y);  // a move below the slop: the watch would stay, but for the mouse
    b.Frames(2);
    CHECK(!ImGuiTouch::GetTouchContext().Scroll.WatchingLongPress);
    b.Release();
}

TEST_CASE("Touch pinch: the data's Kind is Pan for two fingers that move together, and a cancel ends the gesture")
{
    Bench b;
    b.canvas = true;
    b.canvas_pinch_data = true;
    b.Frames(3);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c - ImVec2(20.f, 0.f));
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
    b.Frame();
    CHECK(b.canvas_pinch.Kind == ImGuiTouchPinchKind_Undecided);
    Fingers(c + ImVec2(-20.f, 15.f), c + ImVec2(20.f, 15.f));  // together, 15 px down
    b.Frame();
    CHECK(b.canvas_pinch.Kind == ImGuiTouchPinchKind_Pan);
    CHECK(b.canvas_data_pan.y == doctest::Approx(15.f));
    CHECK(b.canvas_data_zoom == doctest::Approx(1.f));
    ImGuiTouch::AddFingerEvent(2, c, ImGuiTouchPhase_Cancelled);
    b.Frame();
    CHECK(b.canvas_ended == 1);
    LiftFingers();
    b.Release();
    CHECK(b.clicks == 0);
}

TEST_CASE("Touch pinch: a widget with the pan adapter only: a pinch on it does nothing, not even the font")
{
    Bench b;
    b.canvas = true;
    b.canvas_pan_only = true;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c - ImVec2(20.f, 0.f));
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
    b.Frame();
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(40.f, 0.f));  // a scale of 1.5
    b.Frames(3);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
    CHECK(b.canvas_zoom == 1.f);
    LiftFingers();
    b.Release();
}

TEST_CASE("Touch pinch: without PinchScalesFont, a pinch outside the widgets leaves the font, and a widget still zooms")
{
    Bench b;
    b.canvas = true;
    b.canvas_takes_pinch = true;
    b.pinch_scales_font = false;
    b.Frames(3);
    ImGui::GetStyle().FontScaleMain = 1.f;
    b.Press(b.lines_rect.GetCenter());
    Fingers(ImVec2(100, 100), ImVec2(200, 100));  // not on the canvas
    b.Frame();
    Fingers(ImVec2(100, 100), ImVec2(250, 100));
    b.Frames(2);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
    LiftFingers();
    b.Release();
    b.Frames(2);

    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c);
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
    b.Frame();
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(34.f, 0.f));  // a scale of 1.35, three notches of 10%
    b.Frames(6);
    CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
    CHECK(b.canvas_zoom == doctest::Approx(1.331f));
    LiftFingers();
    b.Release();
}

TEST_CASE("Touch pinch: two fingers that move together on a widget that takes the pinch drag it with its pan button")
{
    Bench b;
    b.canvas = true;
    b.canvas_takes_pinch = true;
    b.canvas_pan_button = ImGuiMouseButton_Left;  // a plot pans with the left button
    b.Frames(3);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c);
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
    b.Frames(2);
    Fingers(c - ImVec2(20.f, 12.f), c + ImVec2(20.f, -12.f));  // the two move together, 12 px up
    b.Frames(3);
    CHECK(ImGui::GetIO().MouseDown[0]);
    CHECK(!b.right_down);
    Fingers(c - ImVec2(20.f, 22.f), c + ImVec2(20.f, -22.f));  // the fingers move on, 10 px up
    b.Drag(ImVec2(0.f, -10.f), 2);  // the platform's mouse (the first finger) too: moved to the fingers' center
    CHECK(b.canvas_drag.y < -5.f);
    CHECK(Near(ImGui::GetIO().MousePos.x, c.x, 1.f));
    CHECK(b.scrollY == 0.f);
    LiftFingers();
    b.Release();
    CHECK(!ImGui::GetIO().MouseDown[0]);
}

TEST_CASE("Touch pinch: when two fingers that drag a widget lift, its drag ends at their center, not at the first finger")
{
    Bench b;
    b.canvas = true;
    b.canvas_takes_pinch = true;
    b.canvas_pan_button = ImGuiMouseButton_Left;
    b.Frames(3);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c);
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
    b.Frames(2);
    Fingers(c - ImVec2(20.f, 12.f), c + ImVec2(20.f, -12.f));  // the two move together: the drag starts
    b.Frames(3);
    Fingers(c - ImVec2(20.f, 22.f), c + ImVec2(20.f, -22.f));
    b.Drag(ImVec2(0.f, -10.f), 2);
    REQUIRE(ImGui::GetIO().MouseDown[0]);
    const ImVec2 drag = b.canvas_drag;
    LiftFingers();
    b.MoveTo(c + ImVec2(-20.f, -22.f));  // the platform's last move, in the frame of the lift: the first finger's
    b.Frames(2);
    CHECK(!ImGui::GetIO().MouseDown[0]);
    CHECK(Near(b.canvas_drag.x, drag.x, 1.f));
    CHECK(Near(b.canvas_drag.y, drag.y, 1.f));
    b.Release();
}

TEST_CASE("Touch pinch: while the fingers move at each frame, each wheel notch reaches the widget in a frame of its "
          "own")
{
    Bench b;
    b.canvas = true;
    b.canvas_takes_pinch = true;
    b.Frames(3);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c - ImVec2(20.f, 0.f));
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
    b.Frame();
    float zoom = b.canvas_zoom;
    for (int i = 1; i <= 30; ++i)  // a fast pinch: the spread grows from 40 px to 80 px in 10 frames (7 notches of 10%)
    {
        Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f + 4.f * (float)ImMin(i, 10), 0.f));
        if (i % 2 == 0)  // the platform's mouse (the first finger) moves every other frame: a jitter
            b.MoveTo(c - ImVec2(20.f, (float)(i % 4) * 0.5f));
        else
            b.Frame();
        CHECK(b.canvas_zoom <= zoom * 1.1f * 1.001f);  // at most one notch per frame: two read together zoom once
        zoom = b.canvas_zoom;
    }
    LiftFingers();
    b.Release();
    CHECK(b.canvas_zoom == doctest::Approx(powf(1.1f, 7.f)));
}

TEST_CASE("Touch pinch: a widget that reads the wheel's value gets the exact amount at each frame, and follows the "
          "fingers")
{
    Bench b;
    b.canvas = true;
    b.canvas_takes_pinch = true;
    b.canvas_delivery = ImGuiTouchPinchDelivery_WheelContinuous;  // the canvas zooms by 1.1 ^ wheel
    b.Frames(3);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c - ImVec2(20.f, 0.f));
    Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
    b.Frame();
    float zoom = b.canvas_zoom, biggest_step = 1.f;
    int frames_zoomed = 0;
    for (int i = 1; i <= 30; ++i)  // the spread grows from 40 px to 80 px in 20 frames: a scale of 2
    {
        Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f + 2.f * (float)ImMin(i, 20), 0.f));
        if (i % 2 == 0)  // the platform's mouse (the first finger) moves every other frame
            b.MoveTo(c - ImVec2(20.f, (float)(i % 4) * 0.5f));
        else
            b.Frame();
        biggest_step = ImMax(biggest_step, b.canvas_zoom / zoom);
        frames_zoomed += (b.canvas_zoom != zoom) ? 1 : 0;
        zoom = b.canvas_zoom;
    }
    LiftFingers();
    b.Release();
    // The zoom starts where the pinch is recognized (a spread of 1.15, past the 12% threshold): 2 / 1.15
    CHECK(b.canvas_zoom == doctest::Approx(2.f / 1.15f).epsilon(0.005));
    CHECK(biggest_step < 1.08f);  // small steps: no jump at the start, no notch of 10%
    CHECK(frames_zoomed >= 14);   // nearly each frame of the spread (17), not every other one: the mouse moves to the
                                 // fingers' center only every half line, and a move holds the wheel back a frame
}

TEST_CASE("Touch pinch: a rectangle claimed with an item's id takes that item's press over; without the id, the item keeps it")
{
    for (int with_id = 0; with_id <= 1; ++with_id)
    {
        CAPTURE(with_id);
        Bench b;
        b.canvas = true;
        b.canvas_takes_drags = true;  // the canvas gets the first finger's press at once, and holds it
        b.canvas_rect_claim = with_id ? 2 : 1;
        b.Frames(3);
        ImGui::GetStyle().FontScaleMain = 1.f;
        const ImVec2 c = b.canvasRect.GetCenter();
        b.Press(c);
        CHECK(ImGui::GetActiveID() == b.canvas_id);
        Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
        b.Frame();
        Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(34.f, 0.f));  // a scale of 1.35: three notches of 10%
        b.Frames(6);
        CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
        CHECK(b.canvas_zoom == doctest::Approx(with_id ? 1.331f : 1.f));
        LiftFingers();
        b.Release();
    }
}

TEST_CASE("Touch pinch: inside a canvas (PushClaimTransform), a claim is where the canvas shows it on the screen")
{
    for (int canvases = 1; canvases <= 2; ++canvases)
    {
        CAPTURE(canvases);
        Bench b;
        b.claim_canvases = canvases;
        b.Frames(3);
        ImGui::GetStyle().FontScaleMain = 1.f;
        const ImVec2 c = (canvases == 1) ? ImVec2(150.f, 175.f) : ImVec2(190.f, 195.f);  // the claim's center
        b.Press(c);
        Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(20.f, 0.f));
        b.Frame();
        Fingers(c - ImVec2(20.f, 0.f), c + ImVec2(34.f, 0.f));  // a scale of 1.35: three notches of 10%
        b.Frames(6);
        CHECK(ImGui::GetStyle().FontScaleMain == 1.f);
        CHECK(b.claim_wheel == doctest::Approx(3.f));
        LiftFingers();
        b.Release();

        // On the rectangle's own coordinates, (0,0)-(50,25), nothing claims: the pinch scales the font
        b.Press(b.lines_rect.GetCenter());
        Fingers(ImVec2(10.f, 12.f), ImVec2(40.f, 12.f));
        b.Frame();
        Fingers(ImVec2(10.f, 12.f), ImVec2(55.f, 12.f));
        b.Frames(2);
        CHECK(ImGui::GetStyle().FontScaleMain == 1.5f);
        LiftFingers();
        b.Release();
        ImGui::GetStyle().FontScaleMain = 1.f;
    }
}

TEST_CASE("Touch scroll: a widget that takes the drags along X gets a sideways drag, even where the page scrolls "
          "sideways")
{
    Bench b;
    b.canvas = true;
    b.canvas_drag_axis = ImGuiTouchAxis_X;  // a slider
    b.wide_content = true;                 // the page scrolls both ways
    b.Frames(3);
    REQUIRE(b.bench_window->ScrollMax.x > 0.f);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c);
    b.Drag(ImVec2(40.f, 0.f), 4);  // sideways: the canvas's axis
    b.Frames(2);
    CHECK(b.canvas_drag.x > 20.f);  // the canvas was dragged (from past the slop)
    CHECK(b.bench_window->Scroll.x == 0.f);  // the page did not scroll
    b.ReleaseStill();

    b.canvas_drag = ImVec2(0.f, 0.f);
    b.Press(c);
    b.Drag(ImVec2(0.f, -40.f), 4);  // vertical: not its axis, the page scrolls
    b.ReleaseStill();
    CHECK(b.scrollY > 20.f);
    CHECK(b.canvas_drag.y == 0.f);
}

TEST_CASE("Touch scroll: without SetItemTakesDrags(X), a sideways drag on a widget scrolls a page that scrolls "
          "sideways")
{
    Bench b;
    b.canvas = true;
    b.wide_content = true;
    b.Frames(3);
    const ImVec2 c = b.canvasRect.GetCenter();
    b.Press(c);
    b.Drag(ImVec2(-40.f, 0.f), 4);
    b.ReleaseStill();
    CHECK(b.bench_window->Scroll.x > 20.f);
    CHECK(b.canvas_drag.x == 0.f);
}

