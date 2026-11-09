// imgui_kit's touch demo on Android: Dear ImGui's Android + OpenGL ES 3 example, plus the kit's touch layer (with
// the fingers of the AMotionEvents: the pinch), its wheel session, and the keyboard's height from the activity.
// https://github.com/pthom/imgui_kit (MIT license)
//
// What the kit adds to Dear ImGui's example: the lines marked "imgui_kit".

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "imgui_kit.h"
#include <android/configuration.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>

static EGLDisplay           g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface           g_EglSurface = EGL_NO_SURFACE;
static EGLContext           g_EglContext = EGL_NO_CONTEXT;
static struct android_app*  g_App = nullptr;
static bool                 g_Initialized = false;
static char                 g_LogTag[] = "imgui_kit";
static std::string          g_IniFilename;

static void Init(struct android_app* app);
static void Shutdown();
static void MainLoopStep();
static int  CallActivityMethod(const char* name, bool returns_int);
static void PollUnicodeChars();

// imgui_kit: the fingers of a motion event, to the touch layer. Android reports all the pointers in one event:
// the action says which one landed or lifted (its index is in the action's high bits), a move or a cancel is for
// all of them. The ids are stable while a finger is down. Only the fingers: a mouse or a stylus is the mouse already.
static void ForwardFingerEvents(const AInputEvent* event)
{
    int32_t action = AMotionEvent_getAction(event);
    int32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    action &= AMOTION_EVENT_ACTION_MASK;
    int32_t tool_type = AMotionEvent_getToolType(event, index);
    if (tool_type != AMOTION_EVENT_TOOL_TYPE_FINGER && tool_type != AMOTION_EVENT_TOOL_TYPE_UNKNOWN)
        return;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;  // a screen was touched: thin scroll bars

    size_t count = AMotionEvent_getPointerCount(event);
    switch (action)
    {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        ImGuiTouch::AddFingerEvent(AMotionEvent_getPointerId(event, index),
            ImVec2(AMotionEvent_getX(event, index), AMotionEvent_getY(event, index)), ImGuiTouchPhase_Began);
        break;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
        ImGuiTouch::AddFingerEvent(AMotionEvent_getPointerId(event, index),
            ImVec2(AMotionEvent_getX(event, index), AMotionEvent_getY(event, index)), ImGuiTouchPhase_Ended);
        break;
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_CANCEL:
        for (size_t i = 0; i < count; i++)
            ImGuiTouch::AddFingerEvent(AMotionEvent_getPointerId(event, i),
                ImVec2(AMotionEvent_getX(event, i), AMotionEvent_getY(event, i)),
                (action == AMOTION_EVENT_ACTION_MOVE) ? ImGuiTouchPhase_Moved : ImGuiTouchPhase_Cancelled);
        break;
    default:
        break;
    }
}

static void HandleAppCmd(struct android_app* app, int32_t app_cmd)
{
    switch (app_cmd)
    {
    case APP_CMD_INIT_WINDOW:
        Init(app);
        break;
    case APP_CMD_TERM_WINDOW:
        Shutdown();
        break;
    default:
        break;
    }
}

static int32_t HandleInputEvent(struct android_app* app, AInputEvent* input_event)
{
    if (AInputEvent_getType(input_event) == AINPUT_EVENT_TYPE_MOTION)
        ForwardFingerEvents(input_event);  // imgui_kit: the fingers (the backend makes the mouse from the first one)
    return ImGui_ImplAndroid_HandleInputEvent(input_event);
}

void android_main(struct android_app* app)
{
    app->onAppCmd = HandleAppCmd;
    app->onInputEvent = HandleInputEvent;

    while (true)
    {
        int out_events;
        struct android_poll_source* out_data;
        // Poll all events. If the app is not visible, this loop blocks until g_Initialized == true.
        while (ALooper_pollOnce(g_Initialized ? 0 : -1, nullptr, &out_events, (void**)&out_data) >= 0)
        {
            if (out_data != nullptr)
                out_data->process(app, out_data);
            if (app->destroyRequested != 0)
            {
                if (!g_Initialized)
                    Shutdown();
                return;
            }
        }
        MainLoopStep();
    }
}

void Init(struct android_app* app)
{
    if (g_Initialized)
        return;

    g_App = app;
    ANativeWindow_acquire(g_App->window);

    // EGL: a window surface and an ES 3 context (Dear ImGui's example, unchanged)
    {
        g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (g_EglDisplay == EGL_NO_DISPLAY)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");
        if (eglInitialize(g_EglDisplay, 0, 0) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglInitialize() returned with an error");

        const EGLint egl_attributes[] = { EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };
        EGLint num_configs = 0;
        if (eglChooseConfig(g_EglDisplay, egl_attributes, nullptr, 0, &num_configs) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned with an error");
        if (num_configs == 0)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned 0 matching config");

        EGLConfig egl_config;
        eglChooseConfig(g_EglDisplay, egl_attributes, &egl_config, 1, &num_configs);
        EGLint egl_format;
        eglGetConfigAttrib(g_EglDisplay, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
        ANativeWindow_setBuffersGeometry(g_App->window, 0, 0, egl_format);

        const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        g_EglContext = eglCreateContext(g_EglDisplay, egl_config, EGL_NO_CONTEXT, egl_context_attributes);
        if (g_EglContext == EGL_NO_CONTEXT)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglCreateContext() returned EGL_NO_CONTEXT");

        g_EglSurface = eglCreateWindowSurface(g_EglDisplay, egl_config, g_App->window, nullptr);
        eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    g_IniFilename = std::string(app->activity->internalDataPath) + "/imgui.ini";
    io.IniFilename = g_IniFilename.c_str();
    ImGui::StyleColorsDark();

    ImGuiTouch::Setup();            // imgui_kit: the touch layer (swipe, long press, pinch)
    ImGuiKit::SetupWheelSession();  // imgui_kit: the wheel stays with the window it scrolls

    ImGui_ImplAndroid_Init(g_App->window);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    // The screen's density: 160 dpi is Android's reference, where one dp is one pixel
    float main_scale = (float)AConfiguration_getDensity(app->config) / 160.0f;
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    g_Initialized = true;
}

void MainLoopStep()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_EglDisplay == EGL_NO_DISPLAY)
        return;

    PollUnicodeChars();

    // The on-screen keyboard follows io.WantTextInput; imgui_kit: its height, so that the caret stays above it
    static bool want_text_input_last = false;
    if (io.WantTextInput != want_text_input_last)
        CallActivityMethod(io.WantTextInput ? "showSoftInput" : "hideSoftInput", false);
    want_text_input_last = io.WantTextInput;
    float keyboard_height = (float)CallActivityMethod("keyboardHeight", true);
    ImGuiTouch::SetKeyboardInsets(ImGuiTouchKeyboardInsets(0.0f, keyboard_height));

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // The kit's demo, in a window that fills the screen, above the keyboard
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - keyboard_height));
    ImGui::Begin("imgui_kit", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGuiKit::ShowDemoContents();
    ImGui::End();

    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(g_EglDisplay, g_EglSurface);
}

void Shutdown()
{
    if (!g_Initialized)
        return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (g_EglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_EglContext != EGL_NO_CONTEXT)
            eglDestroyContext(g_EglDisplay, g_EglContext);
        if (g_EglSurface != EGL_NO_SURFACE)
            eglDestroySurface(g_EglDisplay, g_EglSurface);
        eglTerminate(g_EglDisplay);
    }
    g_EglDisplay = EGL_NO_DISPLAY;
    g_EglContext = EGL_NO_CONTEXT;
    g_EglSurface = EGL_NO_SURFACE;
    ANativeWindow_release(g_App->window);
    g_Initialized = false;
}

// Calls a method of MainActivity (MainActivity.java) with no argument, returning void or an int, through JNI:
// what native code cannot do by itself (the keyboard, the typed characters). Returns 0 on a failure.
static int CallActivityMethod(const char* name, bool returns_int)
{
    JavaVM* java_vm = g_App->activity->vm;
    JNIEnv* java_env = nullptr;
    if (java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6) == JNI_ERR)
        return 0;
    if (java_vm->AttachCurrentThread(&java_env, nullptr) != JNI_OK)
        return 0;
    int result = 0;
    jclass activity_class = java_env->GetObjectClass(g_App->activity->clazz);
    jmethodID method_id = java_env->GetMethodID(activity_class, name, returns_int ? "()I" : "()V");
    if (method_id != nullptr)
    {
        if (returns_int)
            result = (int)java_env->CallIntMethod(g_App->activity->clazz, method_id);
        else
            java_env->CallVoidMethod(g_App->activity->clazz, method_id);
    }
    java_vm->DetachCurrentThread();
    return result;
}

// The native KeyEvent has no getUnicodeChar(): MainActivity queues the characters, polled here
static void PollUnicodeChars()
{
    ImGuiIO& io = ImGui::GetIO();
    int c;
    while ((c = CallActivityMethod("pollUnicodeChar", true)) != 0)
        io.AddInputCharacter((unsigned int)c);
}
