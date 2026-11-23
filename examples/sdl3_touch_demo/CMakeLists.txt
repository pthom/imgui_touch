# imgui_touch's touch demo on SDL3: stock Dear ImGui (its SDL3 and SDL_Renderer backends), the touch layer, and SDL3's fingers.
# The desktop (Windows, Linux, macOS) and iOS from this one file; SDL3 is fetched and built here (a few minutes).
#   Desktop:  cmake -S . -B build -DIMGUI_TOUCH_BUILD_EXAMPLES=ON -DIMGUI_TOUCH_BUILD_SDL3_EXAMPLE=ON
#   iOS:      cmake -S . -B build_ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
#                   -DIMGUI_TOUCH_BUILD_EXAMPLES=ON -DIMGUI_TOUCH_BUILD_SDL3_EXAMPLE=ON
#             then open build_ios/imgui_touch.xcodeproj, choose your team (Signing & Capabilities), run on your device.
set(imgui_dir ${IMGUI_TOUCH_IMGUI_DIR})

include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.30 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL3)

add_executable(sdl3_touch_demo MACOSX_BUNDLE main.cpp
    ${imgui_dir}/backends/imgui_impl_sdl3.cpp ${imgui_dir}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(sdl3_touch_demo PRIVATE ${imgui_dir}/backends)
target_link_libraries(sdl3_touch_demo PRIVATE imgui_touch SDL3::SDL3)
if (APPLE)
    set_target_properties(sdl3_touch_demo PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "ImGuiTouch SDL3"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.pthom.imgui-touch-sdl3-touch-demo"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.github.pthom.imgui-touch-sdl3-touch-demo"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")  # iPhone and iPad
endif()
