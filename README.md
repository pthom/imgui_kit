# imgui_kit

Small, standalone additions to [Dear ImGui](https://github.com/ocornut/imgui): rows that fill, groups that align, labels in a column, a few widgets.

**Try it online:** [the kit's demo](https://pthom.github.io/imgui_explorer/?lib=imguikit), with its code beside it, in C++ or Python.

*Status: early (0.1). The API may still change.*

## What it is

- Helpers that need nothing but Dear ImGui (1.93 or later, docking or master).
- Organized as Dear ImGui is: one header, `imgui_kit.h`, with its `[SECTION]` index; a few sources (`imgui_kit*.cpp`); the demo in `imgui_kit_demo.cpp` (`ImGuiKit::ShowDemoWindow()`), its sections marked with `IMGUI_DEMO_MARKER`.
- In C++ and in Python: [Dear ImGui Bundle](https://github.com/pthom/imgui_bundle) binds it, and shows its demo in its interactive explorer.

## What it has

- **Em units**: sizes that follow the font (`EmSize()`, `EmToVec2()`), so they stay right on high-DPI screens and when the user zooms the text.
- **Layout**: helpers that measure their parts on the previous frame.
  - A *fill row*: a slider or an input fills the line, and the items after it stay on the right.
  - An *aligned group*: a group placed at the right of the line, or centered.
  - A *column*: a group that knows its width, so that what is inside can be centered under a knob or a slider.
  - A *label column*: labels on the left, and the widgets after them aligned at the widest label.
  - A *flow*: items on a line while they fit, then on the next one, as words in a paragraph.
- **Widgets**:
  - A *help marker*: an (i) that shows its text on hover, and on a click or a tap (a touch screen has no hover). Its text can be rendered as Markdown, through a renderer you provide.
  - *Icon buttons*: plus, minus, zoom in, zoom out, 1:1, fit, drawn with the draw list (no icon font needed).
  - A *segmented switch*: exclusive choices in one frame, such as "Demo | Code". Its corners follow the style's `FrameRounding`.

Coming later, from [Hello ImGui](https://github.com/pthom/hello_imgui): tweakable themes and their editor; touch scrolling and a mouse wheel kept by the window it scrolls.

## Use it

Copy `imgui_kit.h`, `imgui_kit_internal.h` and the `imgui_kit*.cpp` files beside Dear ImGui, and compile them with your project. Leave out `imgui_kit_demo.cpp`, or define `IMGUI_KIT_DISABLE_DEMO`, if you do not want the demo.

Or with CMake:

```cmake
add_subdirectory(imgui_kit)                      # uses your `imgui` target if you have one
target_link_libraries(my_app PRIVATE imgui_kit)
```

Without an `imgui` target, the CMake builds one from `IMGUI_KIT_IMGUI_DIR`, or fetches Dear ImGui (`IMGUI_KIT_FETCH_IMGUI_TAG`).

```cpp
#include "imgui_kit.h"

// A slider that fills the line, with an arrow and a help marker on its right
ImGuiKit::BeginFillRow("slice");
ImGui::SliderInt("##slice", &slice, 0, 59);
ImGuiKit::FillRowTail();                        // the items after this call stay on the right
ImGui::ArrowButton("##next", ImGuiDir_Right);
ImGui::SameLine();
ImGuiKit::HelpMarker("Drag the slider, or click the arrow.");
ImGuiKit::EndFillRow();

// Labels on the left, their widgets aligned
ImGuiKit::BeginLabelColumn("audio");
ImGuiKit::Label("Volume");
ImGui::SliderFloat("##volume", &volume, 0.0f, 1.0f);
ImGuiKit::Label("Output");
ImGui::Checkbox("Muted", &muted);
ImGuiKit::EndLabelColumn();

ImGuiKit::ShowDemoWindow();                     // all of it, with its code
```

Define `IMGUI_KIT_USE_STL` for overloads that take `std::string` and `std::vector`.

## In Python

Dear ImGui Bundle includes the kit as `imgui_bundle.imgui_kit`, from its next release (after 1.93.0). Each `begin_x()` / `end_x()` pair is also a context manager named `x`:

```python
from imgui_bundle import imgui, imgui_kit

with imgui_kit.label_column("audio"):
    imgui_kit.label("Volume")
    _, volume = imgui.slider_float("##volume", volume, 0.0, 1.0)
```

The [book page](https://imgui-bundle.pages.dev/doc/addons/imgui-kit/) and the [API reference](https://imgui-bundle.pages.dev/doc/api/imgui-kit/imgui-kit/) give the rest.

## Tests

`cmake -B build && cmake --build build && ctest --test-dir build`: the tests run headless, through Dear ImGui's null backend.

## Proposed upstream

Some helpers start as proposals to Dear ImGui. They live here meanwhile:

- Em units: [ocornut/imgui#6145](https://github.com/ocornut/imgui/pull/6145)
- Aligned groups: [ocornut/imgui#9330](https://github.com/ocornut/imgui/issues/9330)

## License

MIT
