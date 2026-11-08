// imgui_kit's touch demo: Dear ImGui's GLFW + OpenGL3 example, plus the kit's touch layer and wheel session.
// It builds for the desktop and for the web (Emscripten): open it on a phone to try the gestures.
// https://github.com/pthom/imgui_kit (MIT license)
//
// What the kit adds to Dear ImGui's example: the lines marked "imgui_kit".

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_kit.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include "emscripten_mainloop_stub.h"  // from Dear ImGui's examples/libs/emscripten
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL ES 3.0 + GLSL 300 es in a browser, GL 3.2 + GLSL 150 on macOS, GL 3.0 + GLSL 130 elsewhere
#if defined(__EMSCRIPTEN__)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
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
    GLFWwindow* window = glfwCreateWindow((int)(600 * main_scale), (int)(800 * main_scale), "imgui_kit touch demo", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    style.FontSizeBase = 16.0f;  // a size that reads on a phone

    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiTouch::Setup();            // imgui_kit: the touch layer (swipe, long press, pinch)
    ImGuiKit::SetupWheelSession();  // imgui_kit: the wheel stays with the window it scrolls
#ifdef __EMSCRIPTEN__
    ImGuiTouch::SetupEmscripten();  // imgui_kit: in a browser, which pointer is a finger, and the fingers
    ImGuiTouch::SetupEmscriptenKeyboard();  // imgui_kit: the phone's keyboard, for the text fields
#endif

#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // The kit's demo, in a window that fills the screen (a phone's page), out of the phone's keyboard
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiTouchKeyboardInsets keyboard = ImGuiTouch::GetKeyboardInsets();  // imgui_kit
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + keyboard.Top));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - keyboard.Top - keyboard.Bottom));
        ImGui::Begin("imgui_kit touch demo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextWrapped("Open \"Touch\" below, then try it with a finger. On a desktop: tick \"Simulate a touch "
                           "source\", and the mouse plays a finger.");
        ImGui::TextLinkOpenURL("See the full interactive explorer", "https://pthom.github.io/imgui_explorer/?lib=imguikit");
        ImGuiKit::ShowDemoContents();
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
