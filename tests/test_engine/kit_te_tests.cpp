// imgui_kit's tests with Dear ImGui's Test Engine: the touch layer and the wheel session, driven as a user would.
// The Test Engine moves its mouse; the tests give it a touch screen's source, so that it plays a finger.
#define IMGUI_DEFINE_MATH_OPERATORS
#include "kit_te_tests.h"
#include "imgui_kit.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_context.h"
#include "imgui_te_engine.h"
#include <math.h>
#include <string.h>

namespace
{
    // The lab: a window of lines, with a button, a slider, an item that zooms with the wheel, and a context menu
    const char* k_lab = "Touch lab";
    const ImVec2 k_lab_pos(20.f, 20.f);  // the window is pinned there

    struct Lab
    {
        int clicks = 0;
        float slider = 0.5f;
        float zoom = 1.f;
        float map_zoom = 1.f;    // the map, which reads the pinch's data (ImGuiTouch::ItemPinch)
        ImVec2 map_pan;
        int map_started = 0, map_ended = 0;
        bool menu_shown = false;
        bool reset_scroll = true;
        float scrollY = 0.f;
    };
    Lab g_lab;

    void LabGui()
    {
        ImGui::SetNextWindowPos(k_lab_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(420.f, 420.f), ImGuiCond_Always);
        ImGui::Begin(k_lab, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
        if (g_lab.reset_scroll)
        {
            ImGui::SetScrollY(0.f);
            g_lab.reset_scroll = false;
        }
        for (int i = 0; i < 10; i++)
            ImGui::Text("Line %d", i);
        if (ImGui::Button("Button"))
            g_lab.clicks++;
        ImGui::SliderFloat("Slider", &g_lab.slider, 0.f, 1.f);
        for (int i = 10; i < 16; i++)
            ImGui::Text("Line %d", i);
        // An item that zooms with the wheel, with Dear ImGui's idiom: it claims the wheel while hovered. It takes a
        // pinch too, as wheel notches of 10%
        ImGui::Button("Zoomable (the wheel zooms it)", ImVec2(-FLT_MIN, 150.f));
        ImGuiTouch::SetItemPinchAsWheel(1.1f, ImGuiTouchPinchDelivery_WheelNotches);
        if (ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY) && ImGui::GetIO().MouseWheel != 0.f)
            g_lab.zoom *= powf(1.1f, ImGui::GetIO().MouseWheel);
        for (int i = 16; i < 80; i++)
            ImGui::Text("Line %d", i);
        // A map: it reads two fingers' gesture, the zoom and the pan at once
        ImGui::InvisibleButton("Map", ImVec2(-FLT_MIN, 150.f));
        ImGuiTouchPinch pinch = ImGuiTouch::ItemPinch();
        if (pinch.Active)
        {
            g_lab.map_zoom *= pinch.ScaleDelta;
            g_lab.map_pan += pinch.CenterDelta;
        }
        g_lab.map_started += pinch.Started ? 1 : 0;
        g_lab.map_ended += pinch.Ended ? 1 : 0;
        if (ImGui::BeginPopupContextWindow("##menu"))
        {
            g_lab.menu_shown = true;
            ImGui::TextUnformatted("The context menu");
            ImGui::EndPopup();
        }
        g_lab.scrollY = ImGui::GetScrollY();
        ImGui::End();
    }

    // The mouse source is a state, not an event: io.AddMouseSourceEvent() sets the source that the next mouse events
    // take, those the Test Engine queues included. It is set again before each gesture: on Windows, the GLFW backend
    // sets it from the real mouse's messages (the visible program).
    void UseFinger(ImGuiTestContext* ctx) { ctx->UiContext->IO.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen); }
    void UseMouse(ImGuiTestContext* ctx) { ctx->UiContext->IO.AddMouseSourceEvent(ImGuiMouseSource_Mouse); }

    // Each test starts from the same lab: what moves on its own stops (a fling, a bounce), then the scroll and the
    // values go back to their start
    void ResetLab(ImGuiTestContext* ctx)
    {
        ctx->PopupCloseAll();
        ctx->SleepNoSkip(2.f, 1.f / 60.f);
        g_lab = Lab();
        ctx->Yield(2);
        ctx->SetRef(k_lab);
    }

    // A point of the lab, from its top-left corner
    ImVec2 LabPos(float x, float y) { return k_lab_pos + ImVec2(x, y); }

    // A finger pressed at `from`, moved to `to`, lifted
    void Swipe(ImGuiTestContext* ctx, ImVec2 from, ImVec2 to)
    {
        UseFinger(ctx);
        ctx->MouseMoveToPos(from);
        ctx->MouseDown();
        ctx->MouseMoveToPos(to);
        ctx->MouseUp();
    }

    ImGuiWindow* FindWindowNamed(ImGuiTestContext* ctx, const char* part)
    {
        for (ImGuiWindow* window : ctx->UiContext->Windows)
            if (strstr(window->Name, part) != nullptr)
                return window;
        return nullptr;
    }
}  // namespace

void KitTestsSetup()
{
    ImGuiTouch::Setup();
    ImGuiKit::SetupWheelSession();
}

void KitTestsRegister(ImGuiTestEngine* engine)
{
    ImGuiTest* t = nullptr;

    // A swipe that starts on a button scrolls the window, and does not click the button
    t = IM_REGISTER_TEST(engine, "touch", "swipe_from_a_button_scrolls");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        ImVec2 button = ctx->ItemInfo("Button").RectFull.GetCenter();
        Swipe(ctx, button, button - ImVec2(0.f, 150.f));
        ctx->Yield(10);
        IM_CHECK_GT(g_lab.scrollY, 100.f);
        IM_CHECK_EQ(g_lab.clicks, 0);
    };

    // A tap clicks the button (the touch layer replays it, a few frames later)
    t = IM_REGISTER_TEST(engine, "touch", "tap_clicks");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        UseFinger(ctx);
        ctx->MouseMove("Button");
        ctx->MouseDown();
        ctx->MouseUp();
        ctx->Yield(5);
        IM_CHECK_EQ(g_lab.clicks, 1);
        IM_CHECK_EQ(g_lab.scrollY, 0.f);
    };

    // A finger held still, then moved, drags the slider under it, and the window does not scroll
    t = IM_REGISTER_TEST(engine, "touch", "hold_then_drag_moves_a_slider");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        UseFinger(ctx);
        ImVec2 slider = ctx->ItemInfo("Slider").RectFull.GetCenter();
        ctx->MouseMoveToPos(slider);
        ctx->MouseDown();
        ctx->SleepNoSkip(0.3f, 1.f / 60.f);  // past the hold (0.15 s), before the long press (0.5 s)
        ctx->MouseMoveToPos(slider + ImVec2(60.f, 0.f));
        ctx->MouseUp();
        ctx->Yield(3);
        IM_CHECK_GT(g_lab.slider, 0.6f);
        IM_CHECK_EQ(g_lab.scrollY, 0.f);
    };

    // A finger still for half a second, then lifted, is a right click: the context menu opens
    t = IM_REGISTER_TEST(engine, "touch", "long_press_opens_the_context_menu");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        UseFinger(ctx);
        ctx->MouseMoveToPos(LabPos(200.f, 330.f));  // on the lines
        ctx->MouseDown();
        ctx->SleepNoSkip(0.7f, 1.f / 60.f);
        ctx->MouseUp();
        ctx->Yield(5);
        IM_CHECK(g_lab.menu_shown);
        ctx->PopupCloseAll();
    };

    // The same drag with a mouse moves nothing: the swipe is for fingers (ImGuiTouchScrollMode_Auto)
    t = IM_REGISTER_TEST(engine, "touch", "mouse_drag_does_not_scroll");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        UseMouse(ctx);
        ImVec2 from = LabPos(200.f, 330.f);
        ctx->MouseMoveToPos(from);
        ctx->MouseDown();
        ctx->MouseMoveToPos(from - ImVec2(0.f, 150.f));
        ctx->MouseUp();
        ctx->Yield(10);
        IM_CHECK_EQ(g_lab.scrollY, 0.f);
    };

    // A wheel that scrolls the page keeps scrolling it when the zoomable item comes under the still mouse: the item
    // does not zoom. Once the session ended, a wheel that starts on the item zooms it.
    t = IM_REGISTER_TEST(engine, "wheel", "session_keeps_the_page");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        UseMouse(ctx);
        ImGuiTestItemInfo zoomable = ctx->ItemInfo("Zoomable (the wheel zooms it)");
        ctx->MouseMoveToPos(ImVec2(zoomable.RectFull.GetCenter().x, zoomable.RectFull.Min.y - 30.f));
        for (int i = 0; i < 4; i++)
        {
            ctx->MouseWheelY(-1.f);
            ctx->Yield(2);
        }
        IM_CHECK_GT(g_lab.scrollY, 0.f);
        IM_CHECK_EQ(g_lab.zoom, 1.f);
        ctx->SleepNoSkip(1.f, 1.f / 60.f);  // the session ends (0.7 s)
        ctx->MouseMoveToPos(ctx->ItemInfo("Zoomable (the wheel zooms it)").RectFull.GetCenter());
        const float scrolled = g_lab.scrollY;
        ctx->MouseWheelY(1.f);
        ctx->Yield(2);
        IM_CHECK_GT(g_lab.zoom, 1.f);
        IM_CHECK_EQ(g_lab.scrollY, scrolled);
    };

    // Two fingers that spread on the zoomable item zoom it, by wheel notches, and the font does not scale. The fingers
    // come through ImGuiTouch::AddFingerEvent(), as a platform reports them; the first one is the mouse.
    t = IM_REGISTER_TEST(engine, "touch", "pinch_on_a_widget_zooms_it");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        const float font_scale = ImGui::GetStyle().FontScaleMain;
        UseFinger(ctx);
        ImVec2 c = ctx->ItemInfo("Zoomable (the wheel zooms it)").RectClipped.GetCenter();
        ctx->MouseMoveToPos(c - ImVec2(20.f, 0.f));
        ctx->MouseDown();
        ImGuiTouch::AddFingerEvent(1, c - ImVec2(20.f, 0.f), ImGuiTouchPhase_Began);
        ImGuiTouch::AddFingerEvent(2, c + ImVec2(20.f, 0.f), ImGuiTouchPhase_Began);  // 40 px apart
        ctx->Yield(2);
        // The layer released the first finger's press, so that the item is hovered: the Test Engine's mouse lifts too
        // (it sends its button's state at each frame, a platform only its changes)
        ctx->MouseUp();
        ImGuiTouch::AddFingerEvent(2, c + ImVec2(34.f, 0.f), ImGuiTouchPhase_Moved);  // 54 px: a scale of 1.35, three
                                                                                      // notches of 10%
        ctx->Yield(20);  // one notch at a time, each read before the next is sent
        ImGuiTouch::AddFingerEvent(1, ImVec2(0.f, 0.f), ImGuiTouchPhase_Ended);
        ImGuiTouch::AddFingerEvent(2, ImVec2(0.f, 0.f), ImGuiTouchPhase_Ended);
        ctx->Yield(3);
        IM_CHECK_GT(g_lab.zoom, 1.3f);
        IM_CHECK_LT(g_lab.zoom, 1.34f);
        IM_CHECK_EQ(ImGui::GetStyle().FontScaleMain, font_scale);
        IM_CHECK_EQ(g_lab.clicks, 0);
    };

    // Two fingers on an item that reads the pinch's data: it zooms by their spread and pans by their move, both in
    // the same gesture; the font does not scale, nothing is clicked
    t = IM_REGISTER_TEST(engine, "touch", "pinch_data_zooms_and_pans_at_once");
    t->GuiFunc = [](ImGuiTestContext*) { LabGui(); };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ResetLab(ctx);
        const float font_scale = ImGui::GetStyle().FontScaleMain;
        ctx->ScrollToItemY("Map");
        ctx->Yield(2);
        UseFinger(ctx);
        ImVec2 c = ctx->ItemInfo("Map").RectClipped.GetCenter();
        ctx->MouseMoveToPos(c - ImVec2(20.f, 0.f));
        ctx->MouseDown();
        ImGuiTouch::AddFingerEvent(1, c - ImVec2(20.f, 0.f), ImGuiTouchPhase_Began);
        ImGuiTouch::AddFingerEvent(2, c + ImVec2(20.f, 0.f), ImGuiTouchPhase_Began);  // 40 px apart
        ctx->Yield(2);
        ctx->MouseUp();  // the layer released the first finger's press (see pinch_on_a_widget_zooms_it)
        for (int i = 1; i <= 10; i++)  // spread to 60 px (a scale of 1.5) while their middle moves 30 px right
        {
            const float k = (float)i / 10.f;
            ImGuiTouch::AddFingerEvent(1, c + ImVec2(-20.f + 20.f * k, 0.f), ImGuiTouchPhase_Moved);
            ImGuiTouch::AddFingerEvent(2, c + ImVec2(20.f + 40.f * k, 0.f), ImGuiTouchPhase_Moved);
            ctx->Yield();
        }
        ImGuiTouch::AddFingerEvent(1, ImVec2(0.f, 0.f), ImGuiTouchPhase_Ended);
        ImGuiTouch::AddFingerEvent(2, ImVec2(0.f, 0.f), ImGuiTouchPhase_Ended);
        ctx->Yield(3);
        IM_CHECK_GT(g_lab.map_zoom, 1.48f);
        IM_CHECK_LT(g_lab.map_zoom, 1.52f);
        IM_CHECK_GT(g_lab.map_pan.x, 28.f);
        IM_CHECK_LT(g_lab.map_pan.x, 32.f);
        IM_CHECK_EQ(g_lab.map_started, 1);
        IM_CHECK_EQ(g_lab.map_ended, 1);
        IM_CHECK_EQ(ImGui::GetStyle().FontScaleMain, font_scale);
        IM_CHECK_EQ(g_lab.clicks, 0);
    };

    // The kit's demo: in its Touch section, a sideways swipe on the row of cards scrolls the row (a child in a child,
    // which the test reaches through their windows)
    t = IM_REGISTER_TEST(engine, "demo", "touch_cards_swipe_sideways");
    t->GuiFunc = [](ImGuiTestContext*) {
        ImGui::SetNextWindowPos(ImVec2(500.f, 20.f), ImGuiCond_Appearing);
        ImGuiKit::ShowDemoWindow();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("//ImGuiKit Demo");
        ctx->ItemOpen("Touch");
        ctx->Yield(2);
        ImGuiWindow* demo = ctx->GetWindowByRef("//ImGuiKit Demo");
        ImGuiWindow* zone = FindWindowNamed(ctx, "##touch_zone");
        ImGuiWindow* row = FindWindowNamed(ctx, "##sideways");
        IM_CHECK(demo != nullptr && zone != nullptr && row != nullptr);
        // The zone in the demo's view, then the row in the zone's (both scroll)
        ctx->ScrollToY(demo->ID, demo->Scroll.y + zone->Pos.y - demo->InnerRect.Min.y);
        ctx->ScrollToY(zone->ID, zone->ScrollMax.y);
        ctx->Yield(2);
        IM_CHECK_EQ(row->Scroll.x, 0.f);
        ImVec2 center = row->InnerRect.GetCenter();
        Swipe(ctx, center, center - ImVec2(120.f, 0.f));
        ctx->Yield(10);
        IM_CHECK_GT(row->Scroll.x, 50.f);
        ctx->ItemClose("Touch");
    };

    // The same row: a sideways drag that starts on its slider moves the slider (SetItemTakesDrags(ImGuiTouchAxis_X)),
    // and the row
    // does not scroll
    t = IM_REGISTER_TEST(engine, "demo", "touch_row_slider_takes_sideways_drag");
    t->GuiFunc = [](ImGuiTestContext*) {
        ImGui::SetNextWindowPos(ImVec2(500.f, 20.f), ImGuiCond_Appearing);
        ImGuiKit::ShowDemoWindow();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("//ImGuiKit Demo");
        ctx->ItemOpen("Touch");
        ctx->Yield(2);
        ImGuiWindow* demo = ctx->GetWindowByRef("//ImGuiKit Demo");
        ImGuiWindow* zone = FindWindowNamed(ctx, "##touch_zone");
        ImGuiWindow* row = FindWindowNamed(ctx, "##sideways");
        IM_CHECK(demo != nullptr && zone != nullptr && row != nullptr);
        ctx->ScrollToY(demo->ID, demo->Scroll.y + zone->Pos.y - demo->InnerRect.Min.y);
        ctx->ScrollToY(zone->ID, zone->ScrollMax.y);
        ctx->ScrollToX(row->ID, 0.f);
        ctx->SleepNoSkip(1.f, 1.f / 60.f);
        // The slider is the row's first item, on its first line
        ImVec2 slider = row->InnerRect.Min + ImVec2(ctx->UiContext->FontSize * 2.f, ctx->UiContext->FontSize * 0.8f);
        // Towards the left: the row would scroll (its content follows the finger), the slider takes it instead
        Swipe(ctx, slider, slider - ImVec2(80.f, 0.f));
        ctx->Yield(10);
        IM_CHECK_EQ(row->Scroll.x, 0.f);
        ctx->ItemClose("Touch");
    };
}
