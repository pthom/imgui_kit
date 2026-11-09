# imgui_kit and Dear ImGui: notes for a possible integration

By Pascal Thomet, 2026.

----

imgui_kit is a set of utilities for Dear ImGui. It groups some of the components and utilities I have developed since I started working in the Dear ImGui ecosystem, in 2020.

Most of them are already available in Hello ImGui and Dear ImGui Bundle. This repository is an effort to make them available to more people in the Dear ImGui world.

**It contains:**
- Some (simple) widgets: Segmented switch, icons, Resize handle, etc.
- Some layout utilities: label column, group panel, flow, etc. Aligned groups were proposed in [#9330](https://github.com/ocornut/imgui/issues/9330).
- A simple DPI handling utility: EmToVec2 (makes it easy to size widgets relative to the font size). Proposed in [#6145](https://github.com/ocornut/imgui/pull/6145).
- A touch layer (added first in Dear ImGui Bundle v1.93): handles touch and multitouch interactions on a touch screen. Also handles the touch keyboard. See [ocornut/imgui#3379](https://github.com/ocornut/imgui/issues/3379).
- A smaller patch to better handle "wheel session": a wheel movement that started on a widget should keep acting on it, and not end up acting on other ones. Related: [#9587](https://github.com/ocornut/imgui/pull/9587).

**Try it, see its manual**
- Try it inside the ImGui Explorer (together with code): https://pthom.github.io/imgui_explorer/?lib=imguikit
- Try it online (no code): https://pthom.github.io/imgui_kit/
- Phone and iOS demos: `examples/touch_demo` (GLFW, desktop and web) and `examples/ios_touch_demo` (Metal, iOS).


**Context**
* [ocornut/imgui#3379](https://github.com/ocornut/imgui/issues/3379) : drag to scroll
* [#9587](https://github.com/ocornut/imgui/pull/9587) : the wheel's owner when a lock starts


## Two independent parts

| Part | Public API | Files | Lines |
|---|---|---|---|
| Touch layer | `ImGuiTouch::` (section "Touch" of `imgui_kit.h`) | `imgui_kit_touch_internal.h`, `imgui_kit_touch*.cpp` | about 1,220 |
| Its browser glue (Emscripten) | `ImGuiTouch::SetupEmscripten()`, `SetupEmscriptenKeyboard()` | `imgui_kit_emscripten*.cpp` | about 960 |
| Wheel session | `ImGuiKit::SetupWheelSession()` (section "Wheel session") | `imgui_kit_wheel_session.cpp` | about 160 |

- Neither part uses the other.
- Both are set up once, after `ImGui::CreateContext()`, and then run through context hooks (`ImGui::AddContextHook`). An app adds nothing to its frame.
- Both keep their state per Dear ImGui context, with no static state.
- C++11, no STL, no allocation per frame, no warnings with `-Wall -Wextra -Wshadow -Wformat`.

> [!NOTE]
> Apart from those, imgui_kit also provides widget and layout utilities, which are not documented here.

## The touch layer

### What it does

- *Swipe*: a finger that drags a window's content scrolls it, with inertia and a bounce at the end, even when the drag starts on a button.
- *Tap*: still a click on the widget under the finger.
- *Hold, then drag*: the widget under the finger takes the drag (a slider, a text selection).
- *Long press*: a right click (the context menus), shown by a ring around the finger.
- *Pinch*: two fingers scale the font. Two fingers moved together are a right drag (a node editor's pan).
- *A pinch for a widget*: a widget that zooms (a plot, an image) calls `ImGuiTouch::SetItemTakesPinch()` after itself. A pinch on it then zooms it, as wheel events (it zooms as it does with the wheel, no other change): whole notches for a widget that reads only the wheel's sign (ImPlot), the exact value at each frame for one that uses it (an image viewer), and two fingers moved together drag it with its pan button.
- *Thin scrollbars* on a touch screen: indicators, since a swipe scrolls anywhere.
- *The phone's keyboard*: the caret stays above it (`SetKeyboardInsets()`, `GetKeyboardInsets()`).

### How a press reaches the widgets

The answer to the mouse-down point in #3379 ([2025-06-05](https://github.com/ocornut/imgui/issues/3379#issuecomment-2944879197): "underlying items would have already processed a mouse-down event"):

- The layer runs right after `NewFrame()`, before any widget (a `NewFramePost` hook).
- A touch press is claimed with a sentinel active id: no item is hovered while another id is active, so no widget sees the press. This is the ownership part of [the snippet in #3379](https://github.com/ocornut/imgui/issues/3379#issuecomment-2943903877) (its V3, 2025-06-05), taken before the widgets instead of after them.
- The finger moves past a slop: a swipe. The window to scroll is chosen as the wheel would choose it.
- The finger lifts before the slop: a tap. The press and the release are replayed through the input queue, and the widget gets a normal click, two frames later.
- The finger stays still past a delay: the press is handed over (a replayed release and press while the finger is down). The widget activates, and the finger drives its drag.
- So no widget needs down-then-up logic, and no widget changes.
- A widget that wants a finger's drags at once (a plot, a canvas) calls `ImGuiTouch::SetItemTakesDrags()` after itself (`ImGuiTouchAxis_Any`, the default).
- A widget dragged along one axis (a slider) calls `ImGuiTouch::SetItemTakesDrags(ImGuiTouchAxis_X)` (or `_Y`): once the finger passed the slop, a drag along that axis goes to it (the press replayed at the touch point), a drag across scrolls the window, as iOS does with a slider in a scroll view.

### The hooks

| Hook | Step |
|---|---|
| `NewFramePre` | `SimulateTouch` retags the mouse's events; the caret kept above the keyboard; in a browser, the pointer type and the fingers |
| `NewFramePost` | the swipe, then the pinch |
| `EndFramePre` | a scrollbar thumb held by a finger, drawn wider |
| `RenderPost` | the overscroll (the content drawn past its end, by moving its vertices) |
| `Shutdown` | frees the state |

### What it reads from Dear ImGui's internals

- *The press and the widgets*: `g.ActiveId` (15 reads), `SetActiveID`, `ClearActiveID` (6), `KeepAliveID`, `g.ActiveIdWindow`, `g.ActiveIdPreviousFrame`, `g.ActiveIdHasBeenPressedBefore`, `g.HoveredWindow`, `g.CurrentWindow`, `g.LastItemData.Rect`.
- *The input queue*: `g.InputEventsQueue` (read, retagged; the browser's keyboard removes the clicks under the keyboard), `ImGuiInputEvent::AddedByTestEngine`.
- *Scrolling and drawing*: `SetScrollX`, `SetScrollY`, `GetWindowScrollbarRect`, `GetForegroundDrawList`, `ImGuiWindow::Scroll`, `ScrollMax`, `InnerRect`, `ContentSize`.
- *Text input (the phone's keyboard)*: `GetInputTextState`, `g.InputTextState`, `g.PlatformImeData`, `g.PlatformImeDataPrev`, `g.NavWindow`.

### What Dear ImGui could offer

Each item would replace a workaround in the layer:

| Need | Today in the kit | A possible form in Dear ImGui                                                 |
|---|---|-------------------------------------------------------------------------------|
| Owning a press before the widgets | a sentinel active id | perhaps the `ImGuiWindowBgClickFlags` [mentioned in #3379](https://github.com/ocornut/imgui/issues/3379#issuecomment-6420664530) |
| Telling a replayed event from a backend's | `AddedByTestEngine` (the Test Engine erases events without it, so the replays need it) | a generic "not from the backend" flag                                         |
| A per-context slot for an extension | a `Shutdown` hook's `UserData`, found by scanning the hooks (as the Test Engine does) | a user-data slot in `ImGuiContext`                                            |
| Knowing which widgets a finger drags along an axis (sliders, drags) | `SetItemTakesDrags(ImGuiTouchAxis_X)` after each one; in Dear ImGui Bundle's fork, a callback that Dear ImGui's draggable widgets call ([`ItemDragAxisCallback`](https://github.com/pthom/imgui/blob/imgui_bundle/imgui_internal.h): about 20 lines, a generic commit), which the kit listens to | that callback, upstream: then every app's sliders take a finger's drag along their axis |
| Several fingers from the backend | `ImGuiTouch::AddFingerEvent(id, pos, phase)` | an `io.AddTouchPointEvent()`                                                  |
| Running before the hooks that draw a GUI (the Test Engine draws its tests from a `NewFramePost` hook) | the kit is set up before `ImGuiTestEngine_Start()`: hooks run in the order they were added | an order, or a priority, for context hooks |

### Cost

- *Per frame*: too small to measure. A window of 200 lines, a finger dragging half the time, the null backend, `-O2`: 12.5 to 13.4 µs per frame with or without the layer (three rounds of 20,000 frames).
- *Allocations per frame*: zero, with or without the layer (Dear ImGui's allocator counted over 18,000 frames).
- *In a browser*: one JS evaluation per frame for the pointer, one for the keyboard, plus one to drain what was typed when something waits.

### Where to start reading

1. `imgui_kit.h`, section "Touch (ImGuiTouch)": the API and the config.
2. `imgui_kit_touch_internal.h`, the context data for touch handling
3. `imgui_kit_touch.cpp`: the setup and the hooks.
4. `imgui_kit_touch_scroll.cpp`: the swipe, the tap, the hold, the long press. Its top comment is the design.
5. `imgui_kit_touch_pinch.cpp`, `imgui_kit_touch_keyboard.cpp`.
6. `tests/`: headless tests through the null backend. `tests/test_engine/`: the gestures driven by the Test Engine, headless or in a window.

## The wheel session

### What it does

- Without it: a page scrolled with the wheel stops when a plot passes under the mouse, and the plot zooms. On the frame of the arrival, both happen ([#9587](https://github.com/ocornut/imgui/pull/9587) fixes that part).
- With it, as in a browser: a wheel gesture that starts on the page keeps scrolling the page, and the plot sees no wheel. A gesture that starts on the plot zooms the plot.
- A session ends when the mouse moves past the drag threshold, or after Dear ImGui's lock time (`WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER`).
- Widgets claim the wheel with Dear ImGui's own idiom: `if (ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY)) ...`.

### How

- `NewFramePre`: notes the item that owned the wheel (`GetKeyOwnerData(ImGuiKey_MouseWheelY)->OwnerNext`), since `NewFrame()` gives the wheel back to the window Dear ImGui keeps scrolling.
- `NewFramePost`: when the session is the page's, the page scrolls (or Dear ImGui already scrolled it), and `io.MouseWheel` is set to 0, so that no widget sees it.
- It reads `g.WheelingWindow`, `g.WheelingWindowScrolledFrame`, `g.HoveredWindow`, `g.HoveredIdPreviousFrame`.

### In Dear ImGui

It would not stay an extension: a few lines in `UpdateMouseWheel()`, next to the window lock it extends, behind a setting such as `io.ConfigWindowsWheelSession`.

It does not replace [#9587](https://github.com/ocornut/imgui/pull/9587):
- *#9587* is the minimal fix of a bug: on the frame a wheel lock starts, nobody owns the wheel, so one notch scrolls the window and zooms the item. Two lines, for every app, with no new behavior.
- *The wheel session* goes one step further: the browser's behavior, opt-in. It needs a design discussion (a setting, its name, its default).

## Tests

Two kinds, for both parts. All of them run headless, through Dear ImGui's null backend, in the kit's CI (Linux, macOS, Windows).

### Unit tests: `tests/`

- 33 tests for the swipe, the long press and the pinch (`test_touch_scroll.cpp`), 4 for the setup and the hooks (`test_touch_setup.cpp`), 10 for the wheel session (`test_wheel_session.cpp`).
- They call the layer's steps frame by frame, with events queued by hand (positions, buttons, fingers): each one checks one rule. For example: "a long press on a slider, with a touch source, does not move it", "a flick that reaches the end overshoots, then springs back to the end", "an item whose ownership comes one frame late keeps the wheel when nothing scrolls (fast frames)".
- Run: `cmake -B build && cmake --build build && ctest --test-dir build`.

### Gestures driven by the Test Engine: `tests/test_engine/`

The same features, driven as a user would, by [Dear ImGui's Test Engine](https://github.com/ocornut/imgui_test_engine):

| Group | Test | What it checks |
|---|---|---|
| touch | `swipe_from_a_button_scrolls` | a swipe that starts on a button scrolls the window, and does not click the button |
| touch | `tap_clicks` | a tap clicks the button (the layer replays it, a few frames later) |
| touch | `hold_then_drag_moves_a_slider` | a finger held still, then moved, drags the slider; the window does not scroll |
| touch | `long_press_opens_the_context_menu` | a finger still for half a second, then lifted, opens the context menu |
| touch | `mouse_drag_does_not_scroll` | the same drag with a mouse moves nothing (the swipe is for fingers) |
| touch | `pinch_on_a_widget_zooms_it` | two fingers that spread on an item that takes the pinch zoom it, by wheel notches; the font does not scale |
| wheel | `session_keeps_the_page` | a page scrolled with the wheel keeps scrolling over a zoomable item, which does not zoom; once the session ended, a wheel that starts on the item zooms it |
| demo | `touch_cards_swipe_sideways` | in the kit's demo, a sideways swipe scrolls the row of cards (a child in a child) |
| demo | `touch_row_slider_takes_sideways_drag` | in the same row, a sideways drag on its slider moves the slider, and the row does not scroll (`SetItemTakesDrags(ImGuiTouchAxis_X)`) |

- *A finger*: the Test Engine has no API for the mouse source, but none is needed. `io.AddMouseSourceEvent()` sets a state, which the next mouse events take, those the Test Engine queues included. The tests set it to `ImGuiMouseSource_TouchScreen` before each gesture.
- *What each test proves*: without the kit's setups, the swipe, long press, pinch, wheel and demo tests fail. The three others pass either way: they check that the layer keeps what already worked (a tap still clicks, a hold hands the press to the slider, a mouse drag does not scroll).
- *Two programs run them*:
  - `imgui_kit_te_tests`: headless, at full speed (under a second); `ctest` and the CI run it. An argument filters the tests, e.g. `touch`.
  - `imgui_kit_te_tests_visible`: in a window (GLFW + OpenGL3), with the Test Engine's UI, to pick a test and watch it at the chosen speed. `--auto` plays all of them (about 20 s), then exits.
- *Build*: `-DIMGUI_KIT_BUILD_TEST_ENGINE_TESTS=ON` fetches the Test Engine. It has its own license (free for open source projects), distinct from the kit's MIT license: the kit uses it for these tests only.
- *The hook order*: the kit is set up before `ImGuiTestEngine_Start()`. The engine draws each test's GUI from its own `NewFramePost` hook, and hooks run in the order they were added: set up after it, the kit's wheel session would see the wheel only after the test's widgets.

