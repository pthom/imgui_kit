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
- A widget that wants a finger's drags at once (a plot, a canvas) calls `ImGuiTouch::SetItemTakesDrags()` after itself.

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
| Several fingers from the backend | `ImGuiTouch::AddFingerEvent(id, pos, down)` | an `io.AddTouchPointEvent()`                                                  |

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
