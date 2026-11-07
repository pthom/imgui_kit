// The layout helpers, through Dear ImGui's null backend: what they measure on a frame places the next one
#include "doctest.h"
#include "imgui.h"
#include "imgui_impl_null.h"
#include "imgui_kit.h"
#include "imgui_internal.h"
#include <string>
#include <string.h>

namespace
{
    struct Bench
    {
        Bench()
        {
            ImGui::CreateContext();
            ImGui::GetIO().IniFilename = nullptr;
            ImGui_ImplNull_Init();
        }
        ~Bench() { ImGui_ImplNull_Shutdown(); ImGui::DestroyContext(); }
        // One frame with a window of the given width, the gui inside
        template <typename F> void Frame(float window_width, F gui)
        {
            ImGui_ImplNull_NewFrame();
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(window_width, 400));
            ImGui::Begin("bench", nullptr, ImGuiWindowFlags_NoDecoration);
            gui();
            ImGui::End();
            ImGui::Render();
        }
    };

    float WindowContentWidth() { return ImGui::GetCurrentWindow()->ContentSize.x; }
}

TEST_CASE("Fill row: from the second frame, the fill item ends where the tail starts, and nothing overflows")
{
    Bench bench;
    float slider_right = 0, tail_right = 0, region_right = 0, content_width = 0;
    auto gui = [&] {
        static int v = 0;
        region_right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        ImGuiKit::BeginFillRow("row");
        ImGui::SliderInt("##v", &v, 0, 10);
        slider_right = ImGui::GetItemRectMax().x;
        ImGuiKit::FillRowTail();
        ImGui::Button("A");
        ImGui::SameLine();
        ImGui::Button("Bbbbbb");
        tail_right = ImGui::GetItemRectMax().x;
        ImGuiKit::EndFillRow();
        content_width = WindowContentWidth();
    };
    bench.Frame(400, gui);
    CHECK(slider_right == doctest::Approx(region_right));   // first frame: the slider takes the whole line
    for (int i = 0; i < 2; i++)
        bench.Frame(400, gui);
    CHECK(tail_right == doctest::Approx(region_right));      // the tail ends at the region's right edge
    CHECK(slider_right < tail_right);
    CHECK(content_width <= 400.0f);                          // no horizontal overflow
    bench.Frame(300, gui);                                   // a narrower window: the tail keeps its width
    bench.Frame(300, gui);
    CHECK(tail_right == doctest::Approx(region_right));
}

TEST_CASE("Aligned group: right-aligned from the second frame")
{
    Bench bench;
    float group_right = 0, region_right = 0;
    auto gui = [&] {
        region_right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        ImGuiKit::BeginAlignedGroup("g", 1.0f);
        ImGui::Button("Cancel");
        ImGui::SameLine();
        ImGui::Button("Apply");
        ImGuiKit::EndAlignedGroup();
        group_right = ImGui::GetItemRectMax().x;
    };
    for (int i = 0; i < 3; i++)
        bench.Frame(400, gui);
    CHECK(group_right == doctest::Approx(region_right));
}

TEST_CASE("Column: an aligned group inside it is centered in the column, not on the line")
{
    Bench bench;
    float column_left = 0, column_right = 0, small_left = 0, small_right = 0;
    auto gui = [&] {
        ImGuiKit::BeginColumn("c");
        ImGui::Button("A wide button, wider than the small one", ImVec2(200, 0));
        ImGuiKit::BeginAlignedGroup("g", 0.5f);
        ImGui::Button("x", ImVec2(40, 0));
        ImGuiKit::EndAlignedGroup();
        small_left = ImGui::GetItemRectMin().x;
        small_right = ImGui::GetItemRectMax().x;
        ImGuiKit::EndColumn();
        column_left = ImGui::GetItemRectMin().x;
        column_right = ImGui::GetItemRectMax().x;
    };
    for (int i = 0; i < 3; i++)
        bench.Frame(800, gui);
    CHECK(column_right - column_left == doctest::Approx(200));
    CHECK(small_left - column_left == doctest::Approx(column_right - small_right));
}

TEST_CASE("Label column: the widgets after labels of different widths start at the same x")
{
    Bench bench;
    float x_short = 0, x_long = 0, label_right_short = 0, label_right_long = 0;
    const float aligns[] = { 0.0f, 1.0f };
    for (float align : aligns)
    {
        auto gui = [&] {
            ImGui::PushID(align == 0.0f ? "left" : "right");
            ImGuiKit::BeginLabelColumn("lc", align);
            ImGuiKit::Label("A");
            label_right_short = ImGui::GetItemRectMax().x;
            ImGui::Button("short widget");
            x_short = ImGui::GetItemRectMin().x;
            ImGuiKit::Label("A much longer label");
            label_right_long = ImGui::GetItemRectMax().x;
            ImGui::Button("long widget");
            x_long = ImGui::GetItemRectMin().x;
            ImGuiKit::EndLabelColumn();
            ImGui::PopID();
        };
        for (int i = 0; i < 3; i++)
            bench.Frame(600, gui);
        CHECK(x_short == doctest::Approx(x_long));
        if (align == 1.0f)
            CHECK(label_right_short == doctest::Approx(label_right_long));
    }
}

TEST_CASE("Flow: items stay on a line while they fit, then wrap")
{
    Bench bench;
    float ys[6];
    auto gui = [&] {
        ImGuiKit::BeginFlow("f");
        for (int i = 0; i < 6; i++)
        {
            ImGuiKit::FlowNext();
            ImGui::PushID(i);
            ImGui::Button("Button", ImVec2(80, 0));
            ImGui::PopID();
            ys[i] = ImGui::GetItemRectMin().y;
        }
        ImGuiKit::EndFlow();
    };
    for (int i = 0; i < 3; i++)
        bench.Frame(300, gui);   // room for 3 buttons of 80 px a line
    CHECK(ys[0] == ys[1]);
    CHECK(ys[1] == ys[2]);
    CHECK(ys[3] > ys[2]);
    CHECK(ys[3] == ys[5]);
    for (int i = 0; i < 2; i++)
        bench.Frame(1000, gui);  // a wide window: all on one line
    CHECK(ys[5] == ys[0]);
}

TEST_CASE("Help marker, icon button, segmented switch: they draw, and the switch changes at the press")
{
    Bench bench;
    int current = 0;
    bool changed = false;
    ImVec2 switch_min, switch_max;
    auto gui = [&] {
        ImGuiKit::HelpMarker("help");
        ImGuiKit::IconButton("##zoom", ImGuiKitIcon_ZoomIn);
        const char* labels[] = { "A", "B" };
        changed = ImGuiKit::SegmentedSwitch("##s", &current, labels, 2);
        switch_min = ImGui::GetItemRectMin();
        switch_max = ImGui::GetItemRectMax();
    };
    bench.Frame(400, gui);
    // Press on the right half: the second segment is chosen at the press, without a release
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(switch_min.x + (switch_max.x - switch_min.x) * 0.75f, (switch_min.y + switch_max.y) * 0.5f);
    bench.Frame(400, gui);
    io.AddMouseButtonEvent(0, true);
    bench.Frame(400, gui);
    CHECK(current == 1);
    CHECK(changed);
}

TEST_CASE("Fill row: a help marker in the tail stays on the line, and the tail ends at the right edge")
{
    Bench bench;
    float slider_y = 0, marker_y = 0, marker_right = 0, region_right = 0;
    auto gui = [&] {
        static int v = 0;
        region_right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        ImGuiKit::BeginFillRow("row");
        ImGui::SliderInt("##v", &v, 0, 10);
        slider_y = ImGui::GetItemRectMin().y;
        ImGuiKit::FillRowTail();
        ImGui::ArrowButton("##next", ImGuiDir_Right);
        ImGui::SameLine();
        ImGuiKit::HelpMarker("help");
        marker_y = ImGui::GetItemRectMin().y;
        marker_right = ImGui::GetItemRectMax().x;
        ImGuiKit::EndFillRow();
    };
    for (int i = 0; i < 3; i++)
        bench.Frame(400, gui);
    CHECK(marker_y == doctest::Approx(slider_y));
    CHECK(marker_right == doctest::Approx(region_right));
}

TEST_CASE("Fit text: the text itself when it fits, else cut at a word, with an ellipsis, within the lines")
{
    Bench bench;
    const char* short_text = "Short";
    const char* long_text = "A summary much longer than the two lines it may take on a narrow card, so it gets cut";
    const char* fitted_short = nullptr;
    float fitted_height = 0, line_height = 0;
    std::string fitted_long;
    auto gui = [&] {
        fitted_short = ImGuiKit::FitText(short_text, 120.0f, 2);
        const char* r = ImGuiKit::FitText(long_text, 120.0f, 2);
        fitted_long = r;
        fitted_height = ImGui::CalcTextSize(r, nullptr, false, 120.0f).y;
        line_height = ImGui::GetTextLineHeight();
    };
    bench.Frame(400, gui);
    CHECK(fitted_short == short_text);
    CHECK(fitted_long.size() < strlen(long_text));
    CHECK(fitted_long.compare(fitted_long.size() - 3, 3, "\xE2\x80\xA6") == 0);
    CHECK(std::string(long_text).rfind(fitted_long.substr(0, fitted_long.size() - 3), 0) == 0);  // a prefix
    CHECK(fitted_height <= 2 * line_height + 1);
}
