// imgui_kit's Test Engine tests, in a window: the Test Engine's UI lists the tests, runs them at the chosen speed, and
// draws the mouse, which plays a finger.
// Usage: imgui_kit_te_tests_visible          (pick and run the tests in the "Dear ImGui Test Engine" window)
//        imgui_kit_te_tests_visible --auto   (runs all of them at a watchable speed, then exits with their result)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_te_engine.h"
#include "imgui_te_ui.h"
#include "kit_te_tests.h"
#include <stdio.h>
#include <string.h>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

int main(int argc, char** argv)
{
    const bool autoRun = argc > 1 && strcmp(argv[1], "--auto") == 0;
    if (!glfwInit())
        return 1;
#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    GLFWwindow* window = glfwCreateWindow((int)(1300 * main_scale), (int)(900 * main_scale),
                                          "imgui_kit: the Test Engine tests", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigRunSpeed = ImGuiTestRunSpeed_Normal;
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.ConfigLogToTTY = autoRun;
    // The kit's hooks before the engine's: the engine runs each test's GUI from its own NewFramePost hook,
    // which must come after the touch layer and the wheel session
    KitTestsSetup();
    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());

    KitTestsRegister(engine);
    if (autoRun)
        ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Tests, nullptr, ImGuiTestRunFlags_RunFromCommandLine);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiTestEngine_ShowTestEngineWindows(engine, nullptr);
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        ImGuiTestEngine_PostSwap(engine);
        if (autoRun && ImGuiTestEngine_IsTestQueueEmpty(engine))
            break;
    }

    ImGuiTestEngine_Stop(engine);
    ImGuiTestEngineResultSummary summary;
    ImGuiTestEngine_GetResultSummary(engine, &summary);
    if (autoRun)
        printf("imgui_kit_te_tests_visible: %d of %d passed\n", summary.CountSuccess, summary.CountTested);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImGuiTestEngine_DestroyContext(engine);
    glfwDestroyWindow(window);
    glfwTerminate();
    return (!autoRun || (summary.CountTested > 0 && summary.CountSuccess == summary.CountTested)) ? 0 : 1;
}
