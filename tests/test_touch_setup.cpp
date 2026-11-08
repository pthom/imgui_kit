// The setups through Dear ImGui's context hooks: no step called by hand, only ImGui::NewFrame() and ImGui::Render()
#include "doctest.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_null.h"
#include "imgui_kit.h"

namespace
{
    // A window of 100 lines, 400x300 at (0,0), the touch layer and the wheel session set up
    struct HookBench
    {
        float scrollY = 0.f;
        HookBench()
        {
            ImGui::CreateContext();
            ImGui::GetIO().IniFilename = nullptr;
            ImGui_ImplNull_Init();
            ImGuiTouchConfig config;
            config.ScrollMode = ImGuiTouchScrollMode_Always;  // the mouse drives it, as a finger would
            ImGuiTouch::Setup(config);
            ImGuiTouch::Setup(config);  // a second call installs nothing more
            ImGuiKit::SetupWheelSession();
        }
        ~HookBench() { ImGui_ImplNull_Shutdown(); ImGui::DestroyContext(); }
        void Frame()
        {
            ImGui_ImplNull_NewFrame();
            ImGui::GetIO().DeltaTime = 1.f / 60.f;
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(400, 300));
            ImGui::Begin("lines", nullptr, ImGuiWindowFlags_NoDecoration);
            for (int i = 0; i < 100; i++)
                ImGui::Text("Line %d", i);
            scrollY = ImGui::GetScrollY();
            ImGui::End();
            ImGui::Render();
        }
    };
}

TEST_CASE("Touch setup: the hooks run the layer: a drag scrolls, the release coasts and animates")
{
    HookBench b;
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < 3; i++)
        b.Frame();
    int hooks = 0;
    for (const ImGuiContextHook& hook : ImGui::GetCurrentContext()->Hooks)
        hooks += (hook.Type != ImGuiContextHookType_PendingRemoval_);
    CHECK(hooks == 6);  // touch: NewFramePre, NewFramePost, EndFramePre, RenderPost; the wheel session: NewFramePre, NewFramePost

    io.AddMousePosEvent(200, 200);
    b.Frame();
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    b.Frame();
    for (int i = 1; i <= 8; i++)
    {
        io.AddMousePosEvent(200, 200 - 15.f * i);  // drag up: the content follows, it scrolls down
        b.Frame();
    }
    float dragged = b.scrollY;
    CHECK(dragged > 50.f);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    b.Frame();
    b.Frame();
    CHECK(ImGuiTouch::IsAnimating());   // the inertia
    CHECK(b.scrollY > dragged);         // it coasts
    for (int i = 0; i < 300; i++)
        b.Frame();
    CHECK(!ImGuiTouch::IsAnimating());  // it stopped
}

TEST_CASE("Touch setup: SimulateTouch tags the mouse's events as a touch screen's, whatever the backend tells")
{
    HookBench b;
    ImGuiIO& io = ImGui::GetIO();
    b.Frame();
    ImGuiTouch::GetConfig().SimulateTouch = true;
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);  // a backend that tags each of its events (SDL does)
    io.AddMousePosEvent(100, 100);
    b.Frame();
    CHECK(io.MouseSource == ImGuiMouseSource_TouchScreen);

    ImGuiTouch::GetConfig().SimulateTouch = false;
    b.Frame();
    io.AddMousePosEvent(110, 100);
    b.Frame();
    CHECK(io.MouseSource == ImGuiMouseSource_Mouse);
}
