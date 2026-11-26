# How imgui_touch works inside

By Pascal Thomet, 2026.

----

imgui_touch is a touch layer for Dear ImGui: it handles touch and multitouch interactions on a touch screen, and the phone's keyboard. It was added first in Dear ImGui Bundle v1.93. See [ocornut/imgui#3379](https://github.com/ocornut/imgui/issues/3379).

These notes are for a reader of the code: how the layer works on a stock Dear ImGui, what it reads from its internals, and what Dear ImGui could offer to make it simpler.

It started inside [imgui_kit](https://github.com/pthom/imgui_kit), a set of small utilities for Dear ImGui (widgets, layout, em units, a wheel session). It is now a library of its own: touch only.

**Try it, see its manual**
- Try it inside the ImGui Explorer (together with code): https://pthom.github.io/imgui_explorer/?lib=imguitouch
- Try it online (no code): https://pthom.github.io/imgui_touch/
- Phone demos: `examples/touch_demo` (GLFW, desktop and web), `examples/sdl3_touch_demo` (SDL3, desktop and iOS, the fingers from SDL), `examples/ios_touch_demo` (Metal, iOS, UIKit's touches) and `examples/android_touch_demo` (imgui_impl_android, the pointers of the motion events, the keyboard's height).

**Context**
* [ocornut/imgui#3379](https://github.com/ocornut/imgui/issues/3379) : drag to scroll
* The wheel session (a mouse wheel, not a finger) stays in imgui_kit: [its notes](https://github.com/pthom/imgui_kit/blob/main/docs/dear_imgui_integration.md).


## The parts

| Part | Public API | Files | Lines |
|---|---|---|---|
| Touch layer | `ImGuiTouch::` (`imgui_touch.h`) | `imgui_touch_internal.h`, `imgui_touch*.cpp` | about 1,900 |
| Its browser glue (Emscripten) | `ImGuiTouch::SetupEmscripten()`, `SetupEmscriptenKeyboard()` | `imgui_touch_emscripten*.cpp` | about 1,100 |

- Set up once, after `ImGui::CreateContext()`, it then runs through context hooks (`ImGui::AddContextHook`). An app adds nothing to its frame.
- It keeps its state per Dear ImGui context, with no static state.
- C++11, no STL, no allocation per frame, no warnings with `-Wall -Wextra -Wshadow -Wformat`.

## The touch layer

### What it does

- *Swipe*: a finger that drags a window's content scrolls it, with inertia and a bounce at the end, even when the drag starts on a button.
- *Tap*: still a click on the widget under the finger.
- *Hold, then drag*: the widget under the finger takes the drag (a slider, a text selection).
- *Long press*: a right click (the context menus), shown by a ring around the finger.
- *Pinch*: two fingers scale the font. Two fingers moved together are a right drag (a node editor's pan).
- *Two fingers on a widget*: a widget claims them with a call after itself. `ImGuiTouch::ItemPinch()` returns the gesture (its center, its spread and move since the last frame): the widget zooms and pans at once. For an existing widget, two adapters instead: `SetItemPinchAsWheel()`, a pinch as wheel events (whole notches for a widget that reads only the wheel's sign, ImPlot; the exact value at each frame for an image viewer), and `SetItemTwoFingerPanAsMouseDrag()`, two fingers moved together as a drag with a mouse button. The claims of the last frame decide where the fingers land.
- *Thin scrollbars* on a touch screen, while the pointer in use is a finger (a touch laptop driven by its mouse keeps the theme's bars): indicators, since a swipe scrolls anywhere.
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

### What the layer copies from Dear ImGui

These rules are copied, not called (they are inside functions of Dear ImGui that do more). At a Dear ImGui bump, each one is checked against its source; the tests named catch a drift in the behaviour.

| Where | What | From (`imgui.cpp`) | Test |
|---|---|---|---|
| `imgui_touch_scroll.cpp`, `ScrollTarget` and `ScrollsWithPointer` | the window a wheel scrolls: the hovered one, or the first parent that can scroll that way; `NoScrollWithMouse` refuses, unless `NoMouseInputs` | `FindBestWheelingWindow` | `test_touch_scroll.cpp` ("a child scrolls itself, and its parent when it cannot scroll that way") |
| `imgui_touch_scroll.cpp`, `DrawTouchScrollbarGrab` | the thumb's position and length along the bar | `ScrollbarEx` | `test_touch_scroll.cpp` ("a thin scroll bar held by a finger widens over the content") |
| `imgui_touch_scroll.cpp`, `ReplayPress` | the double click's bookkeeping: `MouseClickedTime`, `MouseClickedPos`, `MouseClickedLastCount` | `UpdateMouseInputs` | `test_touch_scroll.cpp` ("two taps are a double click, a tap after a pause is not") |

### What Dear ImGui could offer

Each item would replace a workaround in the layer:

| Need | Today in the layer | A possible form in Dear ImGui                                                 |
|---|---|-------------------------------------------------------------------------------|
| Owning a press before the widgets | a sentinel active id | perhaps the `ImGuiWindowBgClickFlags` [mentioned in #3379](https://github.com/ocornut/imgui/issues/3379#issuecomment-6420664530) |
| Telling a replayed event from a backend's | `AddedByTestEngine` (the Test Engine erases events without it, so the replays need it) | a generic "not from the backend" flag                                         |
| A per-context slot for an extension | a `Shutdown` hook's `UserData`, found by scanning the hooks (as the Test Engine does) | a user-data slot in `ImGuiContext`                                            |
| Knowing which widgets a finger drags along an axis (sliders, drags) | `SetItemTakesDrags(ImGuiTouchAxis_X)` after each one; in Dear ImGui Bundle's fork, a callback that Dear ImGui's draggable widgets call ([`ItemDragAxisCallback`](https://github.com/pthom/imgui/blob/imgui_bundle/imgui_internal.h): about 20 lines, a generic commit), which the layer listens to | that callback, upstream: then every app's sliders take a finger's drag along their axis |
| Several fingers from the backend | `ImGuiTouch::AddFingerEvent(id, pos, phase)` | an `io.AddTouchEvent()`                                                       |
| A link that opens in a new tab in every browser (WebKit allows a new tab only from the gesture's own handler, not from the frame loop where Dear ImGui sees the click) | `ImGuiTouch::SetItemOpensUrl(url)` after the link: the page keeps the links' rectangles and opens from its own handler | a callback from `TextLinkOpenURL` with each link's rectangle and url, per frame, which a browser backend would use the same way |
| Running before the hooks that draw a GUI (the Test Engine draws its tests from a `NewFramePost` hook) | the layer is set up before `ImGuiTestEngine_Start()`: hooks run in the order they were added | an order, or a priority, for context hooks |

### Cost

- *Per frame*: too small to measure. A window of 200 lines, a finger dragging half the time, the null backend, `-O2`: 12.5 to 13.4 µs per frame with or without the layer (three rounds of 20,000 frames).
- *Allocations per frame*: zero, with or without the layer (Dear ImGui's allocator counted over 18,000 frames).
- *In a browser*: one JS evaluation per frame for the pointer, one for the keyboard, plus one to drain what was typed when something waits.

### Where to start reading

1. `imgui_touch.h`: the API and the config.
2. `imgui_touch_internal.h`, the context data for touch handling
3. `imgui_touch.cpp`: the setup and the hooks.
4. `imgui_touch_scroll.cpp`: the swipe, the tap, the hold, the long press. Its top comment is the design.
5. `imgui_touch_pinch.cpp`, `imgui_touch_keyboard.cpp`.
6. `tests/`: headless tests through the null backend. `tests/test_engine/`: the gestures driven by the Test Engine, headless or in a window.

## Tests

Two kinds. All of them run headless, through Dear ImGui's null backend, in the CI (Linux, macOS, Windows).

### Unit tests: `tests/`

- 47 tests for the swipe, the long press and the pinch (`test_touch_scroll.cpp`), 5 for the setup and the hooks (`test_touch_setup.cpp`).
- They call the layer's steps frame by frame, with events queued by hand (positions, buttons, fingers): each one checks one rule. For example: "a long press on a slider, with a touch source, does not move it", "a flick that reaches the end overshoots, then springs back to the end".
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
| touch | `pinch_on_a_widget_zooms_it` | two fingers that spread on an item with the wheel adapter zoom it, by wheel notches; the font does not scale |
| touch | `pinch_data_zooms_and_pans_at_once` | two fingers that spread and move on an item that reads the gesture (`ItemPinch()`) zoom it and pan it in the same gesture; the font does not scale, nothing is clicked |
| demo | `touch_cards_swipe_sideways` | in the demo, a sideways swipe scrolls the row of cards (a child in a child) |
| demo | `touch_row_slider_takes_sideways_drag` | in the same row, a sideways drag on its slider moves the slider, and the row does not scroll (`SetItemTakesDrags(ImGuiTouchAxis_X)`) |

- *A finger*: the Test Engine has no API for the mouse source, but none is needed. `io.AddMouseSourceEvent()` sets a state, which the next mouse events take, those the Test Engine queues included. The tests set it to `ImGuiMouseSource_TouchScreen` before each gesture.
- *What each test proves*: without `ImGuiTouch::Setup()`, the swipe, long press, pinch and demo tests fail. The three others pass either way: they check that the layer keeps what already worked (a tap still clicks, a hold hands the press to the slider, a mouse drag does not scroll).
- *Two programs run them*:
  - `imgui_touch_te_tests`: headless, at full speed (under a second); `ctest` and the CI run it. An argument filters the tests, e.g. `touch`.
  - `imgui_touch_te_tests_visible`: in a window (GLFW + OpenGL3), with the Test Engine's UI, to pick a test and watch it at the chosen speed. `--auto` plays all of them (about 20 s), then exits.
- *Build*: `-DIMGUI_TOUCH_BUILD_TEST_ENGINE_TESTS=ON` fetches the Test Engine. It has its own license (free for open source projects), distinct from imgui_touch's MIT license: it is used for these tests only.
- *The hook order*: `ImGuiTouch::Setup()` goes before `ImGuiTestEngine_Start()`. The engine draws each test's GUI from its own `NewFramePost` hook, and hooks run in the order they were added: set up after it, the layer would see a press only after the test's widgets.
