# imgui_touch

A touch screen for [Dear ImGui](https://github.com/ocornut/imgui), as on a phone: an app made for the mouse works with a finger. With no change to its widgets.

**Try it online, on a phone:** [the touch demo](https://pthom.github.io/imgui_touch/).

*Status: early (0.1). The API may still change. It comes from [imgui_kit](https://github.com/pthom/imgui_kit), which keeps the layout helpers and the widgets.*

## What it has

- A *swipe* scrolls any window, with inertia and a bounce at the end, even from a button: a tap still clicks it.
- A *hold*, then a drag: the widget under the finger takes the drag (a slider, a text selection).
- A *long press* is a right click (the context menus), with a ring that shows when.
- A *pinch* scales the font. Two fingers moved together are a right drag (a node editor's pan).
- A widget that zooms (a plot, an image, a map) can take the pinch.
- *Thin scroll bars* on a touch screen.
- The phone's *keyboard*: the caret stays above it.
- In a browser: which pointer is a finger, the fingers, the phone's keyboard for the text fields, and links that open a new tab in every browser.

## Use it

- Dear ImGui 1.93 or later, docking or master. C++11, no STL, no allocation per frame.
- Organized as Dear ImGui is: one header, `imgui_touch.h`, with its `[SECTION]` index; a few sources (`imgui_touch*.cpp`); the demo in `imgui_touch_demo.cpp` (`ImGuiTouch::ShowDemoWindow()`).
- Copy the `imgui_touch*.h` and `imgui_touch*.cpp` files beside Dear ImGui, and compile them with your project. Leave out `imgui_touch_demo.cpp`, or define `IMGUI_TOUCH_DISABLE_DEMO`, if you do not want the demo. (The `.js` file is the source of a script embedded in a `.cpp`: not needed to build.) `IMGUI_TOUCH_VERSION` says which version you have.

Or with CMake:

```cmake
add_subdirectory(imgui_touch)                    # uses your `imgui` target if you have one
target_link_libraries(my_app PRIVATE imgui_touch)
```

Without an `imgui` target, the CMake uses a Dear ImGui package (`find_package(imgui)`: Conan, vcpkg), builds one from `IMGUI_TOUCH_IMGUI_DIR`, or fetches Dear ImGui (`IMGUI_TOUCH_FETCH_IMGUI_TAG`). Installed (`cmake --install`, with `IMGUI_TOUCH_INSTALL`), it is a package: `find_package(imgui_touch)`, then link `imgui_touch::imgui_touch`.

For AI assistants: [llms.txt](llms.txt) (also at https://pthom.github.io/imgui_touch/llms.txt): how to set it up, and its traps.

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
- *A widget and its drags*: a finger held still for a moment, then moved, drags the widget under it (a text selection). A widget that should take the drags at once (a plot, a canvas) calls `ImGuiTouch::SetItemTakesDrags()` after it. A slider calls `ImGuiTouch::SetItemTakesDrags(ImGuiTouchAxis_X)`: a drag along its axis moves it at once, a drag across scrolls the window, as on iOS. With a Dear ImGui that tells the layer about its draggable widgets (`ImGuiContext::ItemDragAxisCallback`, in Dear ImGui Bundle's fork), its sliders and drags do this with no call.
- *Two fingers on a widget* (instead of the font's scale), one call right after it:
  - `ImGuiTouch::ItemPinch()` returns the gesture at each frame (`ImGuiTouchPinch`: its center, its spread and its move since the last frame, its start and end). A canvas or a map applies the zoom and the pan at once, as on a phone. `RectPinch()` for a rectangle.
  - `ImGuiTouch::SetItemPinchAsWheel(wheel_zoom_factor)`: for a widget that zooms with the wheel (an image viewer, ImPlot), the pinch becomes wheel events at the fingers' center (`ImGuiTouchPinchDelivery_WheelNotches` for a widget that reads only the wheel's sign).
  - `ImGuiTouch::SetItemTwoFingerPanAsMouseDrag(button)`: two fingers moved together drag it with that button (a plot pans with the left one). It combines with the wheel. The `SetRect...` variants take a rectangle (an ImPlot plot), and optionally the id of the item that the first finger may hold (a canvas's background): the gesture takes that press over.
  - These calls run at each frame, hovered or not: the claims of the last frame decide where two fingers land, and there is no mouse between two touches.
  - `ImGuiTouch::PushClaimTransform(scale, offset)` and `PopClaimTransform()`: a canvas that shows its widgets zoomed and moved (a node editor) wraps them in these, so that their claims (these calls, and `SetItemTakesDrags()`) are where the widgets show on the screen.
- *The fingers*: the browser setup reports them. Another backend calls `ImGuiTouch::AddFingerEvent(id, pos, phase)` for each one (began, moved, ended, cancelled), for the pinch and so that a touch the system takes over ends without a click: see the [SDL3 example](examples/sdl3_touch_demo/main.cpp) (a dozen lines), the [Android example](examples/android_touch_demo/main.cpp) (the pointers of an `AMotionEvent`) and the [iOS example](examples/ios_touch_demo/main.mm) (UIKit's touches).
- *Links, in a browser*: `ImGuiTouch::SetItemOpensUrl(url)` after a link's item: the page opens it in a new tab from the tap's or the click's own handler, in every browser (WebKit allows no new tab from the frame loop, where Dear ImGui sees the click). It returns true when the page takes the link; the caller then does nothing on the click.
- *The keyboard*: `ImGuiTouch::GetKeyboardInsets()` tells how much of the screen it covers, so that a full-screen window keeps out of it. A native app sets them (`SetKeyboardInsets()`); the browser setup does it.
- *Idling*: an app that sleeps between events keeps drawing while `ImGuiTouch::IsAnimating()` is true: a fling coasts, a ring grows.
- *With Hello ImGui* (and Dear ImGui Bundle): nothing to call. The runner sets up the touch layer; its settings are `RunnerParams::touchConfig`.

### What each backend reports

The touch layer needs four things from the platform. Checked in each backend's source (Dear ImGui 1.93, Nov 2026):
- *Finger or mouse*: `io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen)` for a finger's mouse events. Without it, a swipe does not scroll (unless `ScrollMode_Always`) and a long press is no right click.
- *Touch screen*: `io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen` (the thin scroll bars, while the pointer in use is a finger).
- *The fingers*: `ImGuiTouch::AddFingerEvent()` for each finger, with its phase. Without them: no pinch, no two-finger drag, and a touch the system takes over can end as a click.
- *The keyboard* (phones): `ImGuiTouch::SetKeyboardInsets()`. Without it, a text field can hide under the keyboard.

| Platform and backend | Finger or mouse | Touch screen | Fingers | Keyboard | To add |
|---|---|---|---|---|---|
| A browser, any of Dear ImGui's Emscripten backends, with `SetupEmscripten()` and `SetupEmscriptenKeyboard()` | yes | yes | yes, cancel included | yes | nothing |
| iOS, the iOS example (Metal, its own `UITouch` code) | yes | yes | yes, cancel included | yes | nothing: an app copies its touch and keyboard code |
| SDL2, SDL3 (a desktop touch screen; iOS and Android through SDL) | yes | no | no | no | SDL's finger events to `AddFingerEvent()` (down, motion, up; SDL3 also has canceled) and the flag: a dozen lines in the app's event loop, below; on a phone the keyboard's area |
| Win32, and GLFW on Windows | yes (`GetMessageExtraInfo()`) | no | no | - | `WM_POINTER` messages to `AddFingerEvent()` (their canceled flag), the flag |
| Android (`imgui_impl_android`) | yes (the tool type) | no | no: the first pointer only, as the mouse | no | each pointer of an `AMotionEvent` to `AddFingerEvent()` (`ACTION_CANCEL` too), the flag; the keyboard's height from the activity: the [Android example](examples/android_touch_demo/main.cpp) |
| GLFW on Linux and macOS | no: GLFW does not tell a finger from a mouse | no | no | - | not possible through GLFW: `ScrollMode_Always` lets the mouse swipe |
| macOS (`imgui_impl_osx`) | mouse and pen | - | - | - | a Mac has no touch screen |

### The fingers from SDL

SDL already reports every finger of a touch screen, as events of its own (SDL2: `SDL_FINGERDOWN`, `SDL_FINGERMOTION`, `SDL_FINGERUP`; SDL3: `SDL_EVENT_FINGER_DOWN`, `_MOTION`, `_UP` and `_CANCELED`), next to the mouse events it makes from the first finger. Dear ImGui's SDL backends ignore them. The app's event loop hands them to the layer, after `ImGui_ImplSDL2_ProcessEvent(&event)`:

```cpp
if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERMOTION || event.type == SDL_FINGERUP)
{
    const SDL_TouchFingerEvent& finger = event.tfinger;
    bool direct = (SDL_GetTouchDeviceType(finger.touchId) == SDL_TOUCH_DEVICE_DIRECT)
                  || (finger.touchId == SDL_MOUSE_TOUCHID);
    if (direct)
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        ImGuiTouchPhase phase = (finger.type == SDL_FINGERDOWN) ? ImGuiTouchPhase_Began
                              : (finger.type == SDL_FINGERUP) ? ImGuiTouchPhase_Ended : ImGuiTouchPhase_Moved;
        int id = (int)(finger.fingerId ^ (finger.fingerId >> 32));
        ImGuiTouch::AddFingerEvent(id, ImVec2(finger.x * w, finger.y * h), phase);
    }
}
```

Four details carry it:
- *Which devices*: a trackpad is a touch device too, an indirect one (its fingers would become phantom pinches on a laptop): only `SDL_TOUCH_DEVICE_DIRECT`, the screens; and `SDL_MOUSE_TOUCHID`, the mouse made a touch device by the hint `SDL_MOUSE_TOUCH_EVENTS` (how to try it on a desktop).
- *The coordinates*: SDL gives a finger's position as a fraction of the window (0 to 1); the layer wants the mouse's coordinates, the window's size in points, the same units as the backend's mouse events.
- *The id*: SDL's `fingerId` is 64 bits (a pointer, on iOS); the layer's is an `int`, so the two halves are folded. Any number stable while the finger is down does.
- *The phases*: the layer's four map one to one. `Cancelled` (SDL3's `SDL_EVENT_FINGER_CANCELED`; SDL2 has none) matters when the system takes the touch over, a notification swipe: the press ends with no click.

The layer does the rest from its hooks: with two fingers down it decides pinch or pan, scales the font or feeds the widget that claimed the gesture, and parks the first finger's press (the mouse, for Dear ImGui) so that no widget is dragged by it. The whole code, for SDL3 and a desktop or iOS app: `examples/sdl3_touch_demo/main.cpp`.

On Android, the backend gets every pointer in one `AMotionEvent`: the action says which one landed or lifted (its index is in the action's high bits), a move or a cancel is for all of them; the ids are stable while a finger is down. `examples/android_touch_demo/main.cpp` forwards them in thirty lines, and asks its activity for the keyboard's height (`WindowInsets`, through JNI) for `SetKeyboardInsets()`: a text field stays above the keyboard.

Hello ImGui (and Dear ImGui Bundle): in a browser, its runners call both Emscripten setups (complete). Its SDL2 runner forwards the fingers as above (the desktop, iOS and Android through SDL): the pinch works there. Its GLFW runner cannot: GLFW reports no fingers.

How it works: a touch press is held back until it is known to be a tap or a drag. A tap is then replayed to the widgets; a drag scrolls. So no widget needs to change. The cost per frame is too small to measure, with no allocation. The state lives in the Dear ImGui context (one per context), and the code reads Dear ImGui's internals (`imgui_internal.h`).

The examples (`-DIMGUI_TOUCH_BUILD_EXAMPLES=ON`): `examples/touch_demo` (GLFW, desktop and web, published at https://pthom.github.io/imgui_touch/), `examples/ios_touch_demo` (Metal, an iOS app with its own touch code) and, with `-DIMGUI_TOUCH_BUILD_SDL3_EXAMPLE=ON`, `examples/sdl3_touch_demo` (stock Dear ImGui on SDL3 with its renderer, the fingers from SDL: the desktop and iOS from one CMake; SDL3 is fetched and built). `examples/android_touch_demo` is a Gradle project (open `android/` in Android Studio, or `gradle assembleDebug` there): stock Dear ImGui on its Android and OpenGL ES 3 backends, the fingers and the keyboard's height from the activity.

## In Python

Dear ImGui Bundle includes it as `imgui_bundle.imgui_touch`, from its next release (after 1.93.0): `imgui_touch.setup()`, once after `imgui.create_context()`. An app run by Hello ImGui (`hello_imgui.run()`, `immapp.run()`) needs no call.

## Tests

`cmake -B build && cmake --build build && ctest --test-dir build`: the tests run headless, through Dear ImGui's null backend.

The touch layer also has tests driven by [Dear ImGui's Test Engine](https://github.com/ocornut/imgui_test_engine), as a user would drive them: `-DIMGUI_TOUCH_BUILD_TEST_ENGINE_TESTS=ON` fetches it. Two programs run them:
- `imgui_touch_te_tests`: headless, at full speed (`ctest` runs it). An argument filters the tests by name or group, e.g. `touch`.
- `imgui_touch_te_tests_visible`: in a window, with the Test Engine's UI, to pick a test and watch it at the chosen speed. `--auto` runs all of them, then exits.

The Test Engine has its own license (free for open source projects: see its `LICENSE.txt`), distinct from imgui_touch's MIT license. It is used for these tests only: imgui_touch does not need it.

Combined with the Test Engine, `ImGuiTouch::Setup()` goes before `ImGuiTestEngine_Start()`: the engine draws each test's GUI from its own context hook, which must run after the layer's.

## In Dear ImGui

- An answer to [ocornut/imgui#3379](https://github.com/ocornut/imgui/issues/3379) (drag to scroll).
- Notes for a possible integration: [docs/dear_imgui_integration.md](docs/dear_imgui_integration.md).

## License

MIT
