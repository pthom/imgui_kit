// imgui_kit's Test Engine tests, headless: Dear ImGui's null backend, at full speed. Exits with 0 when all passed.
// Usage: imgui_kit_te_tests [filter]   (a test's name or group, e.g. "touch"; all by default)
#include "imgui.h"
#include "imgui_impl_null.h"
#include "imgui_te_engine.h"
#include "kit_te_tests.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplNull_Init();

    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigRunSpeed = ImGuiTestRunSpeed_Fast;
    test_io.ConfigNoThrottle = true;
    test_io.ConfigLogToTTY = true;
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    // The kit's hooks before the engine's: the engine runs each test's GUI from its own NewFramePost hook,
    // which must come after the touch layer and the wheel session
    KitTestsSetup();
    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());

    KitTestsRegister(engine);
    ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Tests, argc > 1 ? argv[1] : nullptr, ImGuiTestRunFlags_RunFromCommandLine);
    while (!ImGuiTestEngine_IsTestQueueEmpty(engine))
    {
        ImGui_ImplNull_NewFrame();
        ImGui::NewFrame();
        ImGui::Render();
        ImGuiTestEngine_PostSwap(engine);
    }

    ImGuiTestEngine_Stop(engine);
    ImGuiTestEngineResultSummary summary;
    ImGuiTestEngine_GetResultSummary(engine, &summary);
    printf("imgui_kit_te_tests: %d of %d passed\n", summary.CountSuccess, summary.CountTested);
    ImGui_ImplNull_Shutdown();
    ImGui::DestroyContext();
    ImGuiTestEngine_DestroyContext(engine);
    return (summary.CountTested > 0 && summary.CountSuccess == summary.CountTested) ? 0 : 1;
}
