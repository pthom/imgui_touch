# imgui_touch's touch demo on Android: stock Dear ImGui (its Android and OpenGL3 backends), the touch layer, and the fingers of
# the AMotionEvents. Built by the Gradle project in android/ (Android Studio, or `gradle assembleDebug` in there),
# which runs this file for each ABI. Dear ImGui: give -DIMGUI_TOUCH_IMGUI_DIR (the Gradle property imguiDir), or
# imgui_touch fetches it.
cmake_minimum_required(VERSION 3.22)
project(imgui_touch_android_touch_demo CXX C)

set(IMGUI_TOUCH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(IMGUI_TOUCH_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(../.. imgui_touch)

add_library(android_touch_demo SHARED main.cpp
    ${IMGUI_TOUCH_IMGUI_DIR}/backends/imgui_impl_android.cpp ${IMGUI_TOUCH_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(android_touch_demo PRIVATE
    ${IMGUI_TOUCH_IMGUI_DIR}/backends ${ANDROID_NDK}/sources/android/native_app_glue)
target_compile_definitions(android_touch_demo PRIVATE IMGUI_IMPL_OPENGL_ES3)
target_link_libraries(android_touch_demo PRIVATE imgui_touch android EGL GLESv3 log)
# The glue's entry point is referenced by nothing in the code: keep it in the shared library
target_link_options(android_touch_demo PRIVATE "-u" "ANativeActivity_onCreate")
