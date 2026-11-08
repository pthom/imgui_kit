# imgui_kit

Small, standalone additions to [Dear ImGui](https://github.com/ocornut/imgui): rows that fill, groups that align, labels in a column, a few widgets, touch screens, and a mouse wheel that stays with what it scrolls.

**Try it online:** [the kit's demo](https://pthom.github.io/imgui_explorer/?lib=imguikit), with its code beside it, in C++ or Python. On a phone: [the touch demo](https://pthom.github.io/imgui_kit/).

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
  - A *group panel*: a frame with a label around a group.
- **Widgets**:
  - A *help marker*: an (i) that shows its text on hover, and on a click or a tap (a touch screen has no hover). Its text can be rendered as Markdown, through a renderer you provide.
  - *Icon buttons*: plus, minus, zoom in, zoom out, 1:1, fit, drawn with the draw list (no icon font needed).
  - A *segmented switch*: exclusive choices in one frame, such as "Demo | Code". Its corners follow the style's `FrameRounding`.
  - *Fit text*: a text cut to a width and a number of lines, with an ellipsis.
  - A *resize handle*: a corner that resizes a widget.
- **Touch screens** (`ImGuiTouch`): an app made for the mouse works with a finger. See [Touch screens](#touch-screens).
  - A *swipe* scrolls any window, with inertia, even from a button: a tap still clicks it.
  - A *long press* is a right click (the context menus), with a ring that shows when.
  - A *pinch* scales the font. Two fingers moved together are a right drag (a node editor's pan).
  - In a browser: which pointer is a finger, and the phone's *keyboard* for the text fields.
- **Wheel session**: the mouse wheel keeps scrolling the window it started on, as in a browser. A page scrolled over a plot keeps scrolling, and the plot zooms only with a wheel that starts on it. One call, after `ImGui::CreateContext()`: `ImGuiKit::SetupWheelSession()`.

Coming later, from [Hello ImGui](https://github.com/pthom/hello_imgui): tweakable themes and their editor.

## Use it

Copy the `imgui_kit*.h` and `imgui_kit*.cpp` files beside Dear ImGui, and compile them with your project. Leave out `imgui_kit_demo.cpp`, or define `IMGUI_KIT_DISABLE_DEMO`, if you do not want the demo.

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

## Touch screens

One call after `ImGui::CreateContext()`, and each frame goes through it (Dear ImGui's context hooks): nothing to call per frame.

```cpp
ImGuiTouch::Setup();                    // the swipe, the long press, the pinch
#ifdef __EMSCRIPTEN__
ImGuiTouch::SetupEmscripten();          // in a browser: which pointer is a finger, and the fingers
ImGuiTouch::SetupEmscriptenKeyboard();  // the phone's keyboard, for the text fields
#endif
```

- *Settings*: `ImGuiTouch::GetConfig()` (`ImGuiTouchConfig`), read at each frame. `SimulateTouch` lets the mouse play a finger, to try it on a desktop.
- *A widget and its drags*: a finger held still for a moment, then moved, drags the widget under it (a slider, a text selection). A widget that should take the drags at once (a plot, a canvas) calls `ImGuiTouch::SetItemTakesDrags()` after it.
- *The fingers*: the browser setup reports them. Another backend calls `ImGuiTouch::AddFingerEvent(id, pos, down)` for each one, for the pinch: see the [iOS example](examples/ios_touch_demo/main.mm).
- *The keyboard*: `ImGuiTouch::GetKeyboardInsets()` tells how much of the screen it covers, so that a full-screen window keeps out of it. A native app sets them (`SetKeyboardInsets()`); the browser setup does it.
- *Idling*: an app that sleeps between events keeps drawing while `ImGuiTouch::IsAnimating()` is true (a fling coasts, a ring grows).
- *With Hello ImGui* (and Dear ImGui Bundle): nothing to call. The runner sets up the touch layer and the wheel session; their settings are `RunnerParams::touchConfig` and `RunnerParams::wheelSession`.

How it works: a touch press is held back until it is known to be a tap or a drag. A tap is then replayed to the widgets; a drag scrolls. So no widget needs to change. The cost per frame is too small to measure, with no allocation. The state lives in the Dear ImGui context (one per context), and the code reads Dear ImGui's internals (`imgui_internal.h`).

The examples (`-DIMGUI_KIT_BUILD_EXAMPLES=ON`): `examples/touch_demo` (GLFW, desktop and web, published at https://pthom.github.io/imgui_kit/) and `examples/ios_touch_demo` (Metal, an iOS app).

## In Python

Dear ImGui Bundle includes the kit as `imgui_bundle.imgui_kit`, from its next release (after 1.93.0). Each `begin_x()` / `end_x()` pair is also a context manager named `x`:

```python
from imgui_bundle import imgui, imgui_kit

with imgui_kit.label_column("audio"):
    imgui_kit.label("Volume")
    _, volume = imgui.slider_float("##volume", volume, 0.0, 1.0)
```

The input: `imgui_kit.touch.setup()` and `imgui_kit.setup_wheel_session()`, once after `imgui.create_context()`. An app run by Hello ImGui (`hello_imgui.run()`, `immapp.run()`) needs neither.

The [book page](https://imgui-bundle.pages.dev/doc/addons/imgui-kit/) and the [API reference](https://imgui-bundle.pages.dev/doc/api/imgui-kit/imgui-kit/) give the rest.

## Tests

`cmake -B build && cmake --build build && ctest --test-dir build`: the tests run headless, through Dear ImGui's null backend.

## Proposed upstream

Some helpers start as proposals to Dear ImGui. They live here meanwhile:

- Em units: [ocornut/imgui#6145](https://github.com/ocornut/imgui/pull/6145)
- Aligned groups: [ocornut/imgui#9330](https://github.com/ocornut/imgui/issues/9330)
- Touch screens: an answer to [ocornut/imgui#3379](https://github.com/ocornut/imgui/issues/3379) (drag to scroll)
- Wheel session: related to [ocornut/imgui#9587](https://github.com/ocornut/imgui/pull/9587)

## License

MIT
