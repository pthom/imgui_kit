// The em functions, through Dear ImGui's null backend
#include "doctest.h"
#include "imgui.h"
#include "imgui_impl_null.h"
#include "imgui_kit.h"
#include "imgui_internal.h"

namespace
{
    struct Bench
    {
        Bench() { ImGui::CreateContext(); ImGui::GetIO().IniFilename = nullptr; ImGui_ImplNull_Init(); }
        ~Bench() { ImGui_ImplNull_Shutdown(); ImGui::DestroyContext(); }
        template <typename F> void Frame(F gui)
        {
            ImGui_ImplNull_NewFrame();
            ImGui::NewFrame();
            gui();
            ImGui::Render();
        }
    };
}

TEST_CASE("Em sizes follow the font")
{
    Bench bench;
    bench.Frame([] {
        ImGui::Begin("bench");
        float em = ImGuiKit::EmSize();
        CHECK(em == ImGui::GetFontSize());
        CHECK(ImGuiKit::EmSize(3.f) == doctest::Approx(3.f * em));
        ImVec2 v = ImGuiKit::EmToVec2(2.f, 0.5f);
        CHECK(v.x == doctest::Approx(2.f * em));
        CHECK(v.y == doctest::Approx(0.5f * em));
        CHECK(ImGuiKit::PixelSizeToEm(4.f * em) == doctest::Approx(4.f));
        ImVec2 back = ImGuiKit::PixelsToEm(v);
        CHECK(back.x == doctest::Approx(2.f));
        CHECK(back.y == doctest::Approx(0.5f));
        ImGui::End();
    });
}

TEST_CASE("The demo window runs, its sections open")
{
    Bench bench;
    bench.Frame([] { ImGuiKit::ShowDemoWindow(); });  // the window exists from here
    ImGuiWindow* window = ImGui::FindWindowByName("imgui_kit Demo");
    REQUIRE(window != nullptr);
    for (const char* section : {"Em units"})  // open every header, as a click would
        window->StateStorage.SetInt(window->GetID(section), 1);
    for (int i = 0; i < 3; ++i)
        bench.Frame([] { ImGuiKit::ShowDemoWindow(); });
}
