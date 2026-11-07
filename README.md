# imgui_kit

Small, standalone additions to [Dear ImGui](https://github.com/ocornut/imgui): rows that fill, groups that align, widgets, themes.

*Status: early (0.1). The first helpers land one by one; the API may still change.*

## What it is

- Helpers that need nothing but Dear ImGui (1.93 or later, docking or master).
- Organized as Dear ImGui is: one header, `imgui_kit.h`, with its `[SECTION]` index; one source, `imgui_kit.cpp`; the demo in `imgui_kit_demo.cpp` (`ImGuiKit::ShowDemoWindow()`), its sections marked with `IMGUI_DEMO_MARKER`.
- In C++ and in Python: [Dear ImGui Bundle](https://github.com/pthom/imgui_bundle) binds it, and shows its demo in its interactive explorer.

## Topics

| Topic | Status |
| --- | --- |
| Em units: sizes that follow the font (`EmSize`, `EmToVec2`) | in 0.1 |
| Layout: a row where one item fills the line, aligned groups, framed groups | next |
| Widgets: a help marker for mouse and touch, a wrapping button row, a segmented switch | next |
| Themes: tweakable themes and their editor | later (from Hello ImGui) |
| Input: touch scrolling, the wheel kept by the window it scrolls | later (from Hello ImGui) |

## Use it

Copy `imgui_kit.h` and `imgui_kit.cpp` (and `imgui_kit_demo.cpp` if you want the demo) beside Dear ImGui, and compile them with your project.

Or with CMake:

```cmake
add_subdirectory(imgui_kit)                      # uses your `imgui` target if you have one
target_link_libraries(my_app PRIVATE imgui_kit)
```

Without an `imgui` target, the CMake builds one from `IMGUI_KIT_IMGUI_DIR`, or fetches Dear ImGui (`IMGUI_KIT_FETCH_IMGUI_TAG`).

```cpp
#include "imgui_kit.h"

ImGui::Button("OK", ImGuiKit::EmToVec2(8.f, 2.f));   // 8 em wide, 2 em high, on any screen
ImGuiKit::ShowDemoWindow();
```

## Tests

`cmake -B build && cmake --build build && ctest --test-dir build`: the tests run headless, through Dear ImGui's null backend.

## Proposed upstream

Some helpers start as proposals to Dear ImGui. They live here meanwhile:

- Em units: [ocornut/imgui#6145](https://github.com/ocornut/imgui/pull/6145)
- Aligned groups: [ocornut/imgui#9330](https://github.com/ocornut/imgui/issues/9330)

## License

MIT
