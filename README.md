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
  - A widget that zooms (a plot, an image) can take the pinch: it then zooms it, as wheel notches.
  - In a browser: which pointer is a finger, and the phone's *keyboard* for the text fields.
- **Wheel session**: the mouse wheel keeps scrolling the window it started on, as in a browser. A page scrolled over a plot keeps scrolling, and the plot zooms only with a wheel that starts on it. One call, after `ImGui::CreateContext()`: `ImGuiKit::SetupWheelSession()`.

Coming later, from [Hello ImGui](https://github.com/pthom/hello_imgui): tweakable themes and their editor.

## Use it

Copy the `imgui_kit*.h` and `imgui_kit*.cpp` files beside Dear ImGui, and compile them with your project. Leave out `imgui_kit_demo.cpp`, or define `IMGUI_KIT_DISABLE_DEMO`, if you do not want the demo. (The `.js` file is the source of a script embedded in a `.cpp`: not needed to build.) The git tags are the versions (`v0.5.0`); `IMGUI_KIT_VERSION` says which one you have.

Or with CMake:

```cmake
add_subdirectory(imgui_kit)                      # uses your `imgui` target if you have one
target_link_libraries(my_app PRIVATE imgui_kit)
```

Without an `imgui` target, the CMake uses a Dear ImGui package (`find_package(imgui)`: Conan, vcpkg), builds one from `IMGUI_KIT_IMGUI_DIR`, or fetches Dear ImGui (`IMGUI_KIT_FETCH_IMGUI_TAG`).

Installed (`cmake --install`, with `IMGUI_KIT_INSTALL`), the kit is a package: `find_package(imgui_kit)`, then link `imgui_kit::imgui_kit`.

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

For AI assistants: [llms.txt](llms.txt) (also at https://pthom.github.io/imgui_kit/llms.txt): where the API is, how to set the kit up, and its traps.

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
- *A widget and its drags*: a finger held still for a moment, then moved, drags the widget under it (a text selection). A widget that should take the drags at once (a plot, a canvas) calls `ImGuiTouch::SetItemTakesDrags()` after it. A slider calls `ImGuiTouch::SetItemTakesDrags(ImGuiTouchAxis_X)`: a drag along its axis moves it at once, a drag across scrolls the window, as on iOS. With a Dear ImGui that tells the kit about its draggable widgets (`ImGuiContext::ItemDragAxisCallback`, in Dear ImGui Bundle's fork), its sliders and drags do this with no call.
- *Two fingers on a widget* (instead of the font's scale), one call right after it:
  - `ImGuiTouch::ItemPinch()` returns the gesture at each frame (`ImGuiTouchPinch`: its center, its spread and its move since the last frame, its start and end). A canvas or a map applies the zoom and the pan at once, as on a phone. `RectPinch()` for a rectangle.
  - `ImGuiTouch::SetItemPinchAsWheel(wheel_zoom_factor)`: for a widget that zooms with the wheel (an image viewer, ImPlot), the pinch becomes wheel events at the fingers' center (`ImGuiTouchPinchDelivery_WheelNotches` for a widget that reads only the wheel's sign).
  - `ImGuiTouch::SetItemTwoFingerPanAsMouseDrag(button)`: two fingers moved together drag it with that button (a plot pans with the left one). It combines with the wheel. The `SetRect...` variants take a rectangle (an ImPlot plot).
- *The fingers*: the browser setup reports them. Another backend calls `ImGuiTouch::AddFingerEvent(id, pos, phase)` for each one (began, moved, ended, cancelled), for the pinch and so that a touch the system takes over ends without a click: see the [SDL3 example](examples/sdl3_touch_demo/main.cpp) (a dozen lines) and the [iOS example](examples/ios_touch_demo/main.mm) (UIKit's touches).
- *Links, in a browser*: `ImGuiTouch::SetItemOpensUrl(url)` after a link's item: the page opens it in a new tab from the tap's or the click's own handler, in every browser (WebKit allows no new tab from the frame loop, where Dear ImGui sees the click). It returns true when the page takes the link; the caller then does nothing on the click.
- *The keyboard*: `ImGuiTouch::GetKeyboardInsets()` tells how much of the screen it covers, so that a full-screen window keeps out of it. A native app sets them (`SetKeyboardInsets()`); the browser setup does it.
- *Idling*: an app that sleeps between events keeps drawing while `ImGuiKit::IsAnimating()` is true: the touch layer's motion (a fling coasts, a ring grows; `ImGuiTouch::IsAnimating()` alone) and the kit's widgets' (a segmented switch slides). A widget of yours that animates calls `ImGuiKit::RequestAnimation()`.
- *With Hello ImGui* (and Dear ImGui Bundle): nothing to call. The runner sets up the touch layer and the wheel session; their settings are `RunnerParams::touchConfig` and `RunnerParams::wheelSession`.

### What each backend reports

The touch layer needs four things from the platform. Checked in each backend's source (Dear ImGui 1.93, Nov 2026):
- *Finger or mouse*: `io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen)` for a finger's mouse events. Without it, a swipe does not scroll (unless `ScrollMode_Always`) and a long press is no right click.
- *Touch screen*: `io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen` (the thin scroll bars, while the pointer in use is a finger).
- *The fingers*: `ImGuiTouch::AddFingerEvent()` for each finger, with its phase. Without them: no pinch, no two-finger drag, and a touch the system takes over can end as a click.
- *The keyboard* (phones): `ImGuiTouch::SetKeyboardInsets()`. Without it, a text field can hide under the keyboard.

| Platform and backend | Finger or mouse | Touch screen | Fingers | Keyboard | To add |
|---|---|---|---|---|---|
| A browser, any of Dear ImGui's Emscripten backends, with `SetupEmscripten()` and `SetupEmscriptenKeyboard()` | yes | yes | yes, cancel included | yes | nothing |
| iOS, the kit's example (Metal, its own `UITouch` code) | yes | yes | yes, cancel included | yes | nothing: an app copies its touch and keyboard code |
| SDL2, SDL3 (a desktop touch screen; iOS and Android through SDL) | yes | no | no | no | SDL's finger events to `AddFingerEvent()` (down, motion, up; SDL3 also has canceled) and the flag: a dozen lines in the app's event loop, below; on a phone the keyboard's area |
| Win32, and GLFW on Windows | yes (`GetMessageExtraInfo()`) | no | no | - | `WM_POINTER` messages to `AddFingerEvent()` (their canceled flag), the flag |
| Android (`imgui_impl_android`) | yes (the tool type) | no | no: the first pointer only, as the mouse | no | each pointer of an `AMotionEvent` to `AddFingerEvent()` (`ACTION_CANCEL` too), the flag, the keyboard's area |
| GLFW on Linux and macOS | no: GLFW does not tell a finger from a mouse | no | no | - | not possible through GLFW: `ScrollMode_Always` lets the mouse swipe |
| macOS (`imgui_impl_osx`) | mouse and pen | - | - | - | a Mac has no touch screen |

The fingers from SDL, in the app's event loop (SDL2; SDL3 has `SDL_EVENT_FINGER_DOWN`, `_MOTION`, `_UP` and `_CANCELED`, the same fields):

```cpp
// After ImGui_ImplSDL2_ProcessEvent(&event). SDL gives a finger's position as a fraction of the window; a trackpad is a
// touch device too (an indirect one): only the screens. The first finger is also the mouse, through SDL.
if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERMOTION || event.type == SDL_FINGERUP)
{
    const SDL_TouchFingerEvent& finger = event.tfinger;
    if (SDL_GetTouchDeviceType(finger.touchId) == SDL_TOUCH_DEVICE_DIRECT)
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        ImGuiTouchPhase phase = (finger.type == SDL_FINGERDOWN) ? ImGuiTouchPhase_Began
                              : (finger.type == SDL_FINGERUP) ? ImGuiTouchPhase_Ended : ImGuiTouchPhase_Moved;
        ImGuiTouch::AddFingerEvent((int)finger.fingerId, ImVec2(finger.x * w, finger.y * h), phase);
    }
}
```

Hello ImGui (and Dear ImGui Bundle): in a browser, its runners call both Emscripten setups (complete). Its SDL2 runner forwards the fingers as above (the desktop, iOS and Android through SDL): the pinch works there. Its GLFW runner cannot: GLFW reports no fingers.

How it works: a touch press is held back until it is known to be a tap or a drag. A tap is then replayed to the widgets; a drag scrolls. So no widget needs to change. The cost per frame is too small to measure, with no allocation. The state lives in the Dear ImGui context (one per context), and the code reads Dear ImGui's internals (`imgui_internal.h`).

The examples (`-DIMGUI_KIT_BUILD_EXAMPLES=ON`): `examples/touch_demo` (GLFW, desktop and web, published at https://pthom.github.io/imgui_kit/), `examples/ios_touch_demo` (Metal, an iOS app with its own touch code) and, with `-DIMGUI_KIT_BUILD_SDL3_EXAMPLE=ON`, `examples/sdl3_touch_demo` (stock Dear ImGui on SDL3 with its renderer, the fingers from SDL: the desktop and iOS from one CMake; SDL3 is fetched and built).

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

The touch layer and the wheel session also have tests driven by [Dear ImGui's Test Engine](https://github.com/ocornut/imgui_test_engine), as a user would drive them: `-DIMGUI_KIT_BUILD_TEST_ENGINE_TESTS=ON` fetches it. Two programs run them:
- `imgui_kit_te_tests`: headless, at full speed (`ctest` runs it). An argument filters the tests by name or group, e.g. `touch`.
- `imgui_kit_te_tests_visible`: in a window, with the Test Engine's UI, to pick a test and watch it at the chosen speed. `--auto` runs all of them, then exits.

The Test Engine has its own license (free for open source projects: see its `LICENSE.txt`), distinct from the kit's MIT license. It is used for these tests only: the kit does not need it.

Combined with the Test Engine, the kit's setups go before `ImGuiTestEngine_Start()`: the engine draws each test's GUI from its own context hook, which must run after the kit's.

## Proposed upstream

Some helpers start as proposals to Dear ImGui. They live here meanwhile:

- Em units: [ocornut/imgui#6145](https://github.com/ocornut/imgui/pull/6145)
- Aligned groups: [ocornut/imgui#9330](https://github.com/ocornut/imgui/issues/9330)
- Touch screens: an answer to [ocornut/imgui#3379](https://github.com/ocornut/imgui/issues/3379) (drag to scroll)
- Wheel session: related to [ocornut/imgui#9587](https://github.com/ocornut/imgui/pull/9587)

## License

MIT
