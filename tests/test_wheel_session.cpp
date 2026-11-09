// The wheel session (internal/wheel_session.cpp) through Dear ImGui's null backend: a window of lines with an item
// that takes the wheel to zoom, as ImPlot's plot and ImmVision's image do (it owns ImGuiKey_MouseWheelY while
// hovered, and reads io.MouseWheel).
#define IMGUI_DEFINE_MATH_OPERATORS
#include "doctest.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_null.h"
#include "imgui_kit.h"
#include "imgui_kit_internal.h"

namespace
{
struct Bench
{
    Bench()
    {
        ImGui::CreateContext();
        ImGui_ImplNull_Init();
    }
    ~Bench()
    {
        ImGui_ImplNull_Shutdown();
        ImGui::DestroyContext();
    }

    float scrollY = 0.f;   // as of the last frame's Begin()
    int zooms = 0;         // the wheel notches the zooming item saw
    bool owner_lapse = false;  // the item does not claim the wheel this frame (as a widget whose hover flickers)
    bool short_content = false;  // the window's lines fit: nothing scrolls (a node editor's window)
    bool item_owns_wheel = true;  // false: the item reads the wheel without owning it (a node editor's zoom)
    int wheels_seen = 0;  // the wheel notches a non-owning item saw
    bool session_enabled = true;  // SetupWheelSession(enabled)
    ImRect zoom_rect, lines_rect;

    void Frame()
    {
        ImGui_ImplNull_NewFrame();
        ImGuiKit::WheelSessionBeforeNewFrame();
        ImGui::NewFrame();
        ImGuiKit::UpdateWheelSession(session_enabled);
        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400.f, 300.f), ImGuiCond_Always);
        ImGui::Begin("Bench", nullptr, ImGuiWindowFlags_NoTitleBar);
        scrollY = ImGui::GetScrollY();
        for (int i = 0; i < (short_content ? 3 : 10); ++i)
            ImGui::Text("Line %d", i);
        ImGui::InvisibleButton("zoom", ImVec2(300.f, 100.f));  // the zooming item, as ImPlot and ImmVision do it
        zoom_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        if (ImGui::IsItemHovered())
        {
            if (!owner_lapse && item_owns_wheel)
                ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
            if (ImGui::GetIO().MouseWheel != 0.f)
                zooms++;
        }
        for (int i = 10; i < (short_content ? 12 : 100); ++i)
            ImGui::Text("Line %d", i);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplNullRender_RenderDrawData(ImGui::GetDrawData());
    }
    void Frames(int n) { for (int i = 0; i < n; ++i) Frame(); }
    void MoveTo(ImVec2 p)
    {
        ImGui::GetIO().AddMousePosEvent(p.x, p.y);
        Frame();
    }
    void Wheel(float notches)
    {
        ImGui::GetIO().AddMouseWheelEvent(0.f, notches);
        Frame();
    }
};
}  // namespace

TEST_CASE("Wheel session: a wheel on the lines, then the mouse moves onto the zooming item: the item takes the wheel")
{
    // A move ends the session, as it ends ImGui's lock of the scrolled window
    Bench b;
    b.Frames(3);
    b.MoveTo(ImVec2(100.f, 20.f));  // on the lines
    b.Wheel(-1.f);
    float after_first = b.scrollY;
    CHECK(after_first > 0.f);
    b.MoveTo(b.zoom_rect.GetCenter());  // over the item (its rectangle is on the screen, as of the last frame)
    b.Frames(2);
    CHECK(b.zoom_rect.Contains(ImGui::GetIO().MousePos));
    b.Wheel(-1.f);
    b.Wheel(-1.f);
    CHECK(b.scrollY == after_first);
    CHECK(b.zooms == 2);
}

TEST_CASE("Wheel session: the mouse still, the page scrolls the zooming item under it, and keeps scrolling")
{
    // A wheel or a trackpad: the hand does not move the mouse. ImGui's lock keeps the wheel on the page, and gives
    // its ownership back to the page at each NewFrame(): the item's claim is gone when the session looks
    Bench b;
    b.Frames(3);
    b.MoveTo(ImVec2(100.f, b.zoom_rect.Min.y - 10.f));  // on the line just above the item
    float scroll = b.scrollY;
    for (int i = 0; i < 6; ++i)
    {
        b.Wheel(-1.f);
        CHECK(b.scrollY > scroll);  // each notch scrolls the page, also while the item is under the mouse
        scroll = b.scrollY;
    }
    CHECK(b.zooms == 0);
}

TEST_CASE("Wheel session: disabled, the item that arrives under the still mouse takes the wheel (Dear ImGui's "
          "behavior)")
{
    Bench b;
    b.session_enabled = false;
    b.Frames(3);
    b.MoveTo(ImVec2(100.f, b.zoom_rect.Min.y - 10.f));  // on the line just above the item
    for (int i = 0; i < 6; ++i)
        b.Wheel(-1.f);
    CHECK(b.zooms > 0);
}

TEST_CASE("Wheel session: a wheel that started on the zooming item zooms, and does not scroll")
{
    Bench b;
    b.Frames(3);
    b.MoveTo(b.zoom_rect.GetCenter());
    b.Frames(2);  // the item owns the wheel from the frame before
    b.Wheel(-1.f);
    b.Wheel(-1.f);
    CHECK(b.zooms == 2);
    CHECK(b.scrollY == 0.f);
    b.MoveTo(ImVec2(100.f, 20.f));  // back on the lines: the page scrolls, as before
    b.Frames(2);
    b.Wheel(-1.f);
    CHECK(b.scrollY > 0.f);
}

TEST_CASE("Wheel session: a session ends after a pause, and the item takes the next wheel")
{
    Bench b;
    b.Frames(3);
    b.MoveTo(ImVec2(100.f, 20.f));
    b.Wheel(-1.f);
    b.MoveTo(b.zoom_rect.GetCenter());
    b.Frames(60);  // one second: past the session's 0.7 s
    float scroll = b.scrollY;
    b.Wheel(-1.f);
    CHECK(b.zooms == 1);
    CHECK(b.scrollY == scroll);
}

TEST_CASE("Wheel session: during the item's session, a frame where its ownership lapsed does not scroll the page")
{
    Bench b;
    b.Frames(3);
    b.MoveTo(b.zoom_rect.GetCenter());
    b.Frames(2);
    b.Wheel(-1.f);
    b.owner_lapse = true;  // the next frame's owner is nobody
    b.Frame();
    b.owner_lapse = false;
    b.Wheel(-1.f);  // ImGui would scroll: the owner lapsed; the session cancels it
    b.Frames(2);
    CHECK(b.zooms == 2);
    CHECK(b.scrollY == 0.f);
}

TEST_CASE("Wheel session: a trackpad's small momentum events extend a session by little")
{
    Bench b;
    b.Frames(3);
    b.MoveTo(ImVec2(100.f, b.zoom_rect.Min.y - 30.f));  // the mouse still above the item: the first notch brings it
                                                       // under
    b.Wheel(-1.f);
    for (int i = 0; i < 60; ++i)  // a second of a dying tail: 0.01 notch per frame (each adds 7 ms, a frame takes 17)
        b.Wheel(-0.01f);
    CHECK(b.zooms == 0);  // the session held while it lived
    b.Frames(12);
    CHECK(b.zoom_rect.Contains(ImGui::GetIO().MousePos));
    b.Wheel(-1.f);  // the session ended with the tail (ImGui's timer): the item takes the wheel
    CHECK(b.zooms == 1);
}

TEST_CASE("Wheel session: a widget that reads the wheel without owning it keeps it when nothing scrolls (a node "
          "editor's zoom)")
{
    Bench b;
    b.short_content = true;
    b.item_owns_wheel = false;
    b.Frames(3);
    b.MoveTo(ImVec2(100.f, 20.f));  // a session starts on the lines: nothing scrolls there
    b.Wheel(-1.f);
    b.MoveTo(b.zoom_rect.GetCenter());
    b.Frames(2);
    b.Wheel(-1.f);
    b.Wheel(-1.f);
    CHECK(b.zooms == 2);
    CHECK(b.scrollY == 0.f);
}

TEST_CASE("Wheel session: a widget that reads the wheel without owning it sees none while the page scrolls over it")
{
    Bench b;
    b.item_owns_wheel = false;
    b.Frames(3);
    b.MoveTo(ImVec2(100.f, 20.f));
    b.Wheel(-1.f);
    float after_first = b.scrollY;
    b.MoveTo(b.zoom_rect.GetCenter());
    b.Frames(2);
    b.Wheel(-1.f);
    CHECK(b.scrollY > after_first);  // ImGui scrolled the page: the session's
    CHECK(b.zooms == 0);
}

TEST_CASE("Wheel session: an item whose ownership comes one frame late keeps the wheel when nothing scrolls (fast "
          "frames)")
{
    Bench b;
    b.short_content = true;
    b.Frames(3);
    b.owner_lapse = true;  // the hover frame does not claim the wheel yet (the claim is for the next frame)
    b.MoveTo(b.zoom_rect.GetCenter());
    b.owner_lapse = false;
    b.Wheel(-1.f);  // nobody owns the wheel at this event: the item reads it, and claims it for the next frame
    b.Wheel(-1.f);  // the item owns it now, nothing scrolled: the session is the item's
    b.Wheel(-1.f);
    CHECK(b.zooms == 3);
    CHECK(b.scrollY == 0.f);
}
