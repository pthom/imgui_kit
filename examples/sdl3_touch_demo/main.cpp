// imgui_kit's touch demo on SDL3: Dear ImGui's SDL3 + SDL_Renderer example, plus the kit's touch layer (with SDL's
// fingers: the pinch) and wheel session. The desktop and iOS (SDL_Renderer draws with Metal there).
// https://github.com/pthom/imgui_kit (MIT license)
//
// What the kit adds to Dear ImGui's example: the lines marked "imgui_kit".

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_kit.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>  // iOS needs SDL's own main

// imgui_kit: a finger of a touch screen, to the touch layer. SDL gives its position as a fraction of the window: in
// the window's coordinates here, as the mouse's (the first finger is also the mouse, through SDL). A trackpad is a
// touch device too, an indirect one: only the screens, and the mouse when SDL makes it a touch device
// (SDL_HINT_MOUSE_TOUCH_EVENTS).
static void ForwardFingerEvent(const SDL_TouchFingerEvent& finger, SDL_Window* window)
{
    bool direct = (SDL_GetTouchDeviceType(finger.touchID) == SDL_TOUCH_DEVICE_DIRECT)
                  || (finger.touchID == SDL_MOUSE_TOUCHID);
    if (!direct)
        return;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;  // a screen was touched: thin scroll bars
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    ImGuiTouchPhase phase = (finger.type == SDL_EVENT_FINGER_DOWN) ? ImGuiTouchPhase_Began
                          : (finger.type == SDL_EVENT_FINGER_UP) ? ImGuiTouchPhase_Ended
                          : (finger.type == SDL_EVENT_FINGER_CANCELED) ? ImGuiTouchPhase_Cancelled
                          : ImGuiTouchPhase_Moved;
    int id = (int)(finger.fingerID ^ (finger.fingerID >> 32));  // SDL's id, folded to an int
    ImGuiTouch::AddFingerEvent(id, ImVec2(finger.x * (float)w, finger.y * (float)h), phase);
}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("imgui_kit touch demo (SDL3)", (int)(600 * main_scale),
                                          (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr)
    {
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGuiTouch::Setup();            // imgui_kit: the touch layer (swipe, long press, pinch)
    ImGuiKit::SetupWheelSession();  // imgui_kit: the wheel stays with the window it scrolls

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool done = false;
    while (!done)
    {
        // imgui_kit: an app that sleeps when nothing happens keeps drawing while something of the kit moves
        if (!ImGuiKit::IsAnimating())
            SDL_WaitEventTimeout(nullptr, 100);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_MOTION
                || event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED)
                ForwardFingerEvent(event.tfinger, window);  // imgui_kit: the fingers
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // The kit's demo, in a window that fills the screen (on a phone, the whole display)
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("imgui_kit", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGuiKit::ShowDemoContents();
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0.06f, 0.06f, 0.06f, 1.0f);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
