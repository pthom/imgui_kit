// The setups through Dear ImGui's context hooks: no step called by hand, only ImGui::NewFrame() and ImGui::Render()
#include "doctest.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_null.h"
#include "imgui_kit.h"
#include "imgui_kit_internal.h"  // FindTouchContext

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
    CHECK(hooks == 7);  // touch: NewFramePre, NewFramePost, EndFramePre, RenderPost; the wheel session: NewFramePre,
                        // NewFramePost; the touch state: Shutdown (it frees the state with the context)

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

TEST_CASE("Keyboard insets: the active text field scrolls above the keyboard")
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui_ImplNull_Init();
    static char text[64] = "";
    float fieldBottom = 0.f;
    // A full-screen window that keeps out of the keyboard, 20 lines, then the field
    auto frame = [&](bool focus) {
        ImGui_ImplNull_NewFrame();
        io.DeltaTime = 1.f / 60.f;
        ImGui::NewFrame();
        ImGuiTouchKeyboardInsets keyboard = ImGuiTouch::GetKeyboardInsets();
        ImGui::SetNextWindowPos(ImVec2(0, keyboard.Top));
        ImGui::SetNextWindowSize(ImVec2(400, 300 - keyboard.Top - keyboard.Bottom));
        ImGui::Begin("page", nullptr, ImGuiWindowFlags_NoDecoration);
        for (int i = 0; i < 12; i++)
            ImGui::Text("Line %d", i);
        if (focus)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("field", text, IM_ARRAYSIZE(text));
        fieldBottom = ImGui::GetItemRectMax().y;
        ImGui::End();
        ImGui::Render();
    };
    io.DisplaySize = ImVec2(400, 300);
    frame(false);
    frame(true);
    for (int i = 0; i < 3; i++)  // the focus request, the activation, then WantTextInput
        frame(false);
    REQUIRE(io.WantTextInput);
    CHECK(fieldBottom > 150.f);  // under where the keyboard will be

    ImGuiTouch::SetKeyboardInsets(ImGuiTouchKeyboardInsets{0.f, 150.f});  // a keyboard over the bottom half
    for (int i = 0; i < 6; i++)
        frame(false);
    CHECK(io.WantTextInput);       // the field kept the keyboard
    CHECK(fieldBottom <= 150.f);   // and is above it

    ImGuiTouch::SetKeyboardInsets(ImGuiTouchKeyboardInsets{});
    ImGui_ImplNull_Shutdown();
    ImGui::DestroyContext();
}

TEST_CASE("Url zones: outside a browser, the caller opens its links itself")
{
    HookBench b;
    b.Frame();
    ImGui_ImplNull_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("links");
    ImGui::TextLink("Docs");
    CHECK(!ImGuiTouch::SetItemOpensUrl("https://example.com"));
    CHECK(!ImGuiTouch::SetRectOpensUrl(ImVec2(0, 0), ImVec2(10, 10), "https://example.com"));
    ImGui::End();
    ImGui::Render();
}

TEST_CASE("Touch context: each Dear ImGui context has its own touch state, freed with it")
{
    ImGuiContext* a = ImGui::CreateContext();
    ImGuiTouch::Setup();
    ImGuiTouch::GetConfig().ScrollMode = ImGuiTouchScrollMode_Always;
    ImGuiTouch::SetKeyboardInsets(ImGuiTouchKeyboardInsets{0.f, 100.f});

    ImGuiContext* b = ImGui::CreateContext();
    ImGui::SetCurrentContext(b);
    CHECK(ImGuiTouch::FindTouchContext() == nullptr);  // no touch state before a setup
    ImGuiTouch::Setup();
    CHECK(ImGuiTouch::GetConfig().ScrollMode == ImGuiTouchScrollMode_Auto);  // b starts afresh
    CHECK(ImGuiTouch::GetKeyboardInsets().Bottom == 0.f);
    ImGuiTouch::GetConfig().ScrollMode = ImGuiTouchScrollMode_Disabled;
    ImGui::DestroyContext(b);

    ImGui::SetCurrentContext(a);
    CHECK(ImGuiTouch::GetConfig().ScrollMode == ImGuiTouchScrollMode_Always);  // a kept its own
    CHECK(ImGuiTouch::GetKeyboardInsets().Bottom == 100.f);
    ImGui::DestroyContext(a);

    ImGuiContext* c = ImGui::CreateContext();  // may reuse a destroyed context's address: it still starts afresh
    CHECK(ImGuiTouch::FindTouchContext() == nullptr);
    ImGuiTouch::Setup();
    CHECK(ImGuiTouch::GetConfig().ScrollMode == ImGuiTouchScrollMode_Auto);
    ImGui::DestroyContext(c);
}
