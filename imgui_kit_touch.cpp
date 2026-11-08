// imgui_kit: the setup of the touch layer (ImGuiTouch) and of the wheel session, through Dear ImGui's context hooks
// https://github.com/pthom/imgui_kit (MIT license)
#include "imgui_kit.h"
#include "imgui_kit_internal.h"
#include "imgui_internal.h"  // AddContextHook

// Each step runs at a moment Dear ImGui names:
//   NewFramePre:  the wheel session notes the item that claimed the wheel (NewFrame() gives it back to a window)
//   NewFramePost: before any widget, the swipe, then the pinch; the wheel session's step, in the order of the setups
//   EndFramePre:  after the GUI, the widened scrollbar thumb (the frame's scroll is final)
//   RenderPost:   the draw data exists: the overscroll moves the content past its end
// A hook belongs to its context: Setup() installs the hooks of the current context, once (their owner tells).

namespace
{
    const ImGuiID kTouchHooksOwner = ImHashStr("ImGuiTouch");
    const ImGuiID kWheelHooksOwner = ImHashStr("ImGuiKit_WheelSession");

    ImGuiTouchContext& Ctx() { return ImGuiTouch::GetCurrentContext(); }

    // SimulateTouch: the frame's mouse events (queued by the backend, which may tag each of them as a mouse's) become a
    // touch screen's, and so does the source of what follows. When it is turned off, the source goes back to the mouse.
    void ApplySimulatedTouch(ImGuiContext& g)
    {
        if (Ctx().Config.SimulateTouch)
        {
            for (ImGuiInputEvent& e : g.InputEventsQueue)
            {
                if (e.AddedByTestEngine)  // the touch layer's own events (replayed, with the source they need)
                    continue;
                if (e.Type == ImGuiInputEventType_MousePos)
                    e.MousePos.MouseSource = ImGuiMouseSource_TouchScreen;
                else if (e.Type == ImGuiInputEventType_MouseButton)
                    e.MouseButton.MouseSource = ImGuiMouseSource_TouchScreen;
                else if (e.Type == ImGuiInputEventType_MouseWheel)
                    e.MouseWheel.MouseSource = ImGuiMouseSource_TouchScreen;
            }
            g.IO.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
        }
        else if (Ctx().SimulatedTouch)
            g.IO.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
        Ctx().SimulatedTouch = Ctx().Config.SimulateTouch;
    }

    bool HasHooks(ImGuiContext* ctx, ImGuiID owner)
    {
        for (const ImGuiContextHook& hook : ctx->Hooks)
            if (hook.Owner == owner && hook.Type != ImGuiContextHookType_PendingRemoval_)
                return true;
        return false;
    }

    void AddHook(ImGuiContext* ctx, ImGuiID owner, ImGuiContextHookType type, ImGuiContextHookCallback callback)
    {
        ImGuiContextHook hook;
        hook.Type = type;
        hook.Owner = owner;
        hook.Callback = callback;
        ImGui::AddContextHook(ctx, &hook);
    }
}

namespace ImGuiTouch
{
// The state hangs on a Shutdown hook of the Dear ImGui context (its UserData), as the Test Engine's: the hook frees it
// with the context (see imgui_kit_internal.h's top)
namespace
{
    const ImGuiID kContextHookOwner = ImHashStr("ImGuiTouch_Context");
}

ImGuiTouchContext* FindCurrentContext()
{
    ImGuiContext* ctx = GImGui;
    if (ctx == nullptr)
        return nullptr;
    for (ImGuiContextHook& hook : ctx->Hooks)
        if (hook.Owner == kContextHookOwner && hook.Type == ImGuiContextHookType_Shutdown)
            return (ImGuiTouchContext*)hook.UserData;
    return nullptr;
}

ImGuiTouchContext& GetCurrentContext()
{
    ImGuiTouchContext* state = FindCurrentContext();
    IM_ASSERT(state != nullptr && "imgui_kit's touch layer: call ImGuiTouch::Setup() first (or ImGuiKit::SetupWheelSession())");
    return *state;
}

ImGuiTouchContext& CreateCurrentContextIfNeeded()
{
    ImGuiContext* ctx = GImGui;
    IM_ASSERT(ctx != nullptr && "imgui_kit's touch layer needs a Dear ImGui context: call ImGui::CreateContext() first");
    if (ImGuiTouchContext* state = FindCurrentContext())
        return *state;
    ImGuiTouchContext* state = IM_NEW(ImGuiTouchContext)();
    ImGuiContextHook hook;
    hook.Type = ImGuiContextHookType_Shutdown;
    hook.Owner = kContextHookOwner;
    hook.UserData = state;
    hook.Callback = [](ImGuiContext*, ImGuiContextHook* h) {
        IM_DELETE((ImGuiTouchContext*)h->UserData);
        h->UserData = nullptr;
    };
    ImGui::AddContextHook(ctx, &hook);
    return *state;
}
} // namespace ImGuiTouch

namespace ImGuiTouch
{
void Setup(const ImGuiTouchConfig& config)
{
    CreateCurrentContextIfNeeded().Config = config;
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    IM_ASSERT(ctx != nullptr && "ImGuiTouch::Setup() needs a context: call ImGui::CreateContext() first");
    if (HasHooks(ctx, kTouchHooksOwner))
        return;
    AddHook(ctx, kTouchHooksOwner, ImGuiContextHookType_NewFramePre,
            [](ImGuiContext* g, ImGuiContextHook*) { ApplySimulatedTouch(*g); });
    AddHook(ctx, kTouchHooksOwner, ImGuiContextHookType_NewFramePost, [](ImGuiContext*, ImGuiContextHook*) {
        Ctx().WasAnimating = Ctx().Animating;
        Ctx().Animating = false;
        UpdateTouchScroll(Ctx().Config.ScrollMode, Ctx().Config.LongPressIsRightClick, Ctx().Config.ThinScrollbars);
        UpdateTouchPinch(Ctx().Config.PinchMode, Ctx().Config.PinchInterruptsWidgets);
    });
    AddHook(ctx, kTouchHooksOwner, ImGuiContextHookType_EndFramePre,
            [](ImGuiContext*, ImGuiContextHook*) { DrawTouchScrollbarGrab(); });
    AddHook(ctx, kTouchHooksOwner, ImGuiContextHookType_RenderPost,
            [](ImGuiContext*, ImGuiContextHook*) { ApplyTouchOverscroll(); });
}

ImGuiTouchConfig& GetConfig() { return Ctx().Config; }

// Without the touch layer, nothing records the animations: the kit's widgets animate all the same (an app that draws
// every frame), and IsAnimating() says false
bool IsAnimating()
{
    ImGuiTouchContext* state = FindCurrentContext();
    return state != nullptr && (state->Animating || state->WasAnimating);
}

void RequestAnimation()
{
    if (ImGuiTouchContext* state = FindCurrentContext())
        state->Animating = true;
}

bool ConsumeAnimationRequest()
{
    bool asked = Ctx().Animating;
    Ctx().Animating = false;
    return asked;
}
} // namespace ImGuiTouch

namespace ImGuiKit
{
void SetupWheelSession(bool enabled)
{
    ImGuiTouch::CreateCurrentContextIfNeeded().WheelSessionEnabled = enabled;
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    IM_ASSERT(ctx != nullptr && "ImGuiKit::SetupWheelSession() needs a context: call ImGui::CreateContext() first");
    if (HasHooks(ctx, kWheelHooksOwner))
        return;
    AddHook(ctx, kWheelHooksOwner, ImGuiContextHookType_NewFramePre,
            [](ImGuiContext*, ImGuiContextHook*) { WheelSessionBeforeNewFrame(); });
    AddHook(ctx, kWheelHooksOwner, ImGuiContextHookType_NewFramePost,
            [](ImGuiContext*, ImGuiContextHook*) { UpdateWheelSession(Ctx().WheelSessionEnabled); });
}
} // namespace ImGuiKit
