cmake_minimum_required(VERSION 3.21)  # PROJECT_IS_TOP_LEVEL
project(imgui_touch VERSION 0.1.0 LANGUAGES CXX)

# imgui_touch needs Dear ImGui. Four ways, in this order:
#   - a target named `imgui` or `imgui::imgui` already exists (your project, or a library that brings Dear ImGui): used;
#   - find_package(imgui) finds a package (Conan, vcpkg: the target `imgui::imgui`): used;
#   - IMGUI_TOUCH_IMGUI_DIR names a Dear ImGui folder: a target `imgui` is built from it;
#   - else Dear ImGui is fetched (IMGUI_TOUCH_FETCH_IMGUI_TAG, its docking branch).
set(IMGUI_TOUCH_IMGUI_DIR "" CACHE PATH "A Dear ImGui folder, used when no target `imgui` exists")
set(IMGUI_TOUCH_FETCH_IMGUI_TAG "v1.93.0-docking" CACHE STRING "The Dear ImGui tag fetched when no folder is given")
option(IMGUI_TOUCH_BUILD_TESTS "Build the tests (Dear ImGui's null backend, doctest)" ${PROJECT_IS_TOP_LEVEL})
option(IMGUI_TOUCH_BUILD_EXAMPLES "Build the examples (the touch demo: GLFW + OpenGL3, desktop or Emscripten)" OFF)
option(IMGUI_TOUCH_BUILD_TEST_ENGINE_TESTS "Build the tests driven by Dear ImGui's Test Engine (headless, and visible)" OFF)
option(IMGUI_TOUCH_INSTALL "Install the library, its header and its CMake package (find_package(imgui_touch))" ${PROJECT_IS_TOP_LEVEL})
option(IMGUI_TOUCH_BUILD_SDL3_EXAMPLE "With the examples: the SDL3 touch demo too (desktop and iOS; SDL3 is fetched and built)" OFF)
if (IMGUI_TOUCH_BUILD_SDL3_EXAMPLE AND NOT EMSCRIPTEN)
    enable_language(C)  # SDL3 is C, and Objective-C on Apple's platforms: enabled before any target
    if (APPLE)
        enable_language(OBJC)
    endif()
endif()

set(CMAKE_CXX_STANDARD 11)  # as Dear ImGui
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT TARGET imgui AND NOT TARGET imgui::imgui)
    find_package(imgui CONFIG QUIET)
endif()
if (TARGET imgui::imgui AND NOT TARGET imgui)
    set(IMGUI_TOUCH_IMGUI_TARGET imgui::imgui)
else()
    set(IMGUI_TOUCH_IMGUI_TARGET imgui)
endif()
if (NOT TARGET ${IMGUI_TOUCH_IMGUI_TARGET})
    if (NOT IMGUI_TOUCH_IMGUI_DIR)
        include(FetchContent)
        FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG ${IMGUI_TOUCH_FETCH_IMGUI_TAG})
        FetchContent_MakeAvailable(imgui)
        # Written to the (empty) cache entry: a parent project builds Dear ImGui's backends from the same folder
        set(IMGUI_TOUCH_IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "A Dear ImGui folder, used when no target `imgui` exists" FORCE)
    endif()
    add_library(imgui STATIC
        ${IMGUI_TOUCH_IMGUI_DIR}/imgui.cpp ${IMGUI_TOUCH_IMGUI_DIR}/imgui_demo.cpp ${IMGUI_TOUCH_IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_TOUCH_IMGUI_DIR}/imgui_tables.cpp ${IMGUI_TOUCH_IMGUI_DIR}/imgui_widgets.cpp)
    target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${IMGUI_TOUCH_IMGUI_DIR}> $<INSTALL_INTERFACE:include>)
    # imgui_touch uses no obsolete name of Dear ImGui: apps that disable them (as Dear ImGui Bundle does) can build it
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
    set(IMGUI_TOUCH_BUILT_IMGUI ON)
endif()

if (PROJECT_IS_TOP_LEVEL)
    # The page's script (imgui_touch_emscripten_keyboard.js) is copied into its .cpp by tools/embed_js.py, which writes
    # the file's sha1 in the marker line: a stale copy stops a developer's build here, not in the CI
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/imgui_touch_emscripten_keyboard.js IMGUI_TOUCH_KEYBOARD_JS)
    string(REPLACE "\r\n" "\n" IMGUI_TOUCH_KEYBOARD_JS "${IMGUI_TOUCH_KEYBOARD_JS}")  # a checkout with CRLF: the same file
    string(SHA1 IMGUI_TOUCH_KEYBOARD_JS_SHA1 "${IMGUI_TOUCH_KEYBOARD_JS}")
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/imgui_touch_emscripten_keyboard.cpp IMGUI_TOUCH_KEYBOARD_JS_MARKER
         REGEX "<embedded file=\"imgui_touch_emscripten_keyboard.js\" sha1=\"")
    if (NOT IMGUI_TOUCH_KEYBOARD_JS_MARKER MATCHES "sha1=\"${IMGUI_TOUCH_KEYBOARD_JS_SHA1}\"")
        message(FATAL_ERROR "imgui_touch_emscripten_keyboard.cpp holds a stale copy of imgui_touch_emscripten_keyboard.js: "
                            "run tools/embed_js.py (edit the .js, never the copy)")
    endif()
endif()

file(GLOB IMGUI_TOUCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imgui_touch*.cpp)  # imgui_touch.cpp, imgui_touch_scroll.cpp...
add_library(imgui_touch STATIC ${IMGUI_TOUCH_SOURCES})
add_library(imgui_touch::imgui_touch ALIAS imgui_touch)
# Installed in include/imgui_touch/, reachable both as "imgui_touch.h" (as from add_subdirectory) and
# "imgui_touch/imgui_touch.h" (as from Dear ImGui Bundle)
target_include_directories(imgui_touch PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include> $<INSTALL_INTERFACE:include/imgui_touch>)
target_link_libraries(imgui_touch PUBLIC ${IMGUI_TOUCH_IMGUI_TARGET})
if (PROJECT_IS_TOP_LEVEL)  # imgui_touch's own builds (its CI): Dear ImGui's warning level
    if (MSVC)
        target_compile_options(imgui_touch PRIVATE /W4)
    else()
        target_compile_options(imgui_touch PRIVATE -Wall -Wextra -Wshadow -Wformat)
    endif()
endif()

if (IMGUI_TOUCH_BUILD_TESTS)
    if (NOT IMGUI_TOUCH_BUILT_IMGUI)
        message(FATAL_ERROR "imgui_touch's tests build Dear ImGui's null backend: give IMGUI_TOUCH_IMGUI_DIR, or no `imgui` target")
    endif()
    enable_testing()
    add_executable(imgui_touch_tests tests/test_main.cpp tests/test_touch_scroll.cpp tests/test_touch_setup.cpp
        ${IMGUI_TOUCH_IMGUI_DIR}/backends/imgui_impl_null.cpp)
    target_include_directories(imgui_touch_tests PRIVATE tests ${IMGUI_TOUCH_IMGUI_DIR}/backends)
    target_link_libraries(imgui_touch_tests PRIVATE imgui_touch)
    add_test(NAME imgui_touch_tests COMMAND imgui_touch_tests)
endif()

if (IMGUI_TOUCH_BUILD_TEST_ENGINE_TESTS)
    if (NOT IMGUI_TOUCH_BUILT_IMGUI)
        message(FATAL_ERROR "imgui_touch's Test Engine tests build Dear ImGui with the Test Engine's hooks: give IMGUI_TOUCH_IMGUI_DIR, or no `imgui` target")
    endif()
    enable_testing()
    add_subdirectory(tests/test_engine)
endif()

if (IMGUI_TOUCH_BUILD_EXAMPLES)
    if (NOT IMGUI_TOUCH_BUILT_IMGUI)
        message(FATAL_ERROR "imgui_touch's examples use Dear ImGui's backends: give IMGUI_TOUCH_IMGUI_DIR, or no `imgui` target")
    endif()
    if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        add_subdirectory(examples/ios_touch_demo)
    else()
        add_subdirectory(examples/touch_demo)
    endif()
    if (IMGUI_TOUCH_BUILD_SDL3_EXAMPLE AND NOT EMSCRIPTEN)
        add_subdirectory(examples/sdl3_touch_demo)
    endif()
endif()

if (IMGUI_TOUCH_INSTALL AND IMGUI_TOUCH_BUILT_IMGUI AND IMGUI_TOUCH_BUILD_TEST_ENGINE_TESTS)
    # That Dear ImGui is a test build (the Test Engine's hooks and imconfig): not a package
    message(STATUS "imgui_touch: no install with IMGUI_TOUCH_BUILD_TEST_ENGINE_TESTS (its Dear ImGui is a test build)")
elseif (IMGUI_TOUCH_INSTALL)
    # The package: find_package(imgui_touch) gives imgui_touch::imgui_touch. With a Dear ImGui imgui_touch built itself, it is
    # installed too (its target and headers); with one from a package (imgui::imgui), the config finds that package.
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(IMGUI_TOUCH_INSTALL_TARGETS imgui_touch)
    set(IMGUI_TOUCH_CONFIG_FIND_IMGUI "")
    if (IMGUI_TOUCH_BUILT_IMGUI)
        list(APPEND IMGUI_TOUCH_INSTALL_TARGETS imgui)
        install(DIRECTORY ${IMGUI_TOUCH_IMGUI_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h"
            PATTERN "backends" EXCLUDE PATTERN "examples" EXCLUDE PATTERN "misc" EXCLUDE PATTERN "docs" EXCLUDE)
    elseif (IMGUI_TOUCH_IMGUI_TARGET STREQUAL "imgui::imgui")
        set(IMGUI_TOUCH_CONFIG_FIND_IMGUI "include(CMakeFindDependencyMacro)\nfind_dependency(imgui)")
    endif()
    install(TARGETS ${IMGUI_TOUCH_INSTALL_TARGETS} EXPORT imgui_touchTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES imgui_touch.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui_touch)
    install(EXPORT imgui_touchTargets NAMESPACE imgui_touch:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui_touch)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/imgui_touchConfig.cmake.in
        "@PACKAGE_INIT@\n@IMGUI_TOUCH_CONFIG_FIND_IMGUI@\ninclude(\"\${CMAKE_CURRENT_LIST_DIR}/imgui_touchTargets.cmake\")\n")
    configure_package_config_file(${CMAKE_CURRENT_BINARY_DIR}/imgui_touchConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/imgui_touchConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui_touch)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/imgui_touchConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/imgui_touchConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/imgui_touchConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui_touch)
endif()
