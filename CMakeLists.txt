cmake_minimum_required(VERSION 3.21)  # PROJECT_IS_TOP_LEVEL
project(imgui_kit VERSION 0.5.0 LANGUAGES CXX)

# imgui_kit needs Dear ImGui. Four ways, in this order:
#   - a target named `imgui` or `imgui::imgui` already exists (your project, or a library that brings Dear ImGui): used;
#   - find_package(imgui) finds a package (Conan, vcpkg: the target `imgui::imgui`): used;
#   - IMGUI_KIT_IMGUI_DIR names a Dear ImGui folder: a target `imgui` is built from it;
#   - else Dear ImGui is fetched (IMGUI_KIT_FETCH_IMGUI_TAG, its docking branch).
set(IMGUI_KIT_IMGUI_DIR "" CACHE PATH "A Dear ImGui folder, used when no target `imgui` exists")
set(IMGUI_KIT_FETCH_IMGUI_TAG "v1.93.0-docking" CACHE STRING "The Dear ImGui tag fetched when no folder is given")
option(IMGUI_KIT_BUILD_TESTS "Build the tests (Dear ImGui's null backend, doctest)" ${PROJECT_IS_TOP_LEVEL})
option(IMGUI_KIT_BUILD_EXAMPLES "Build the examples (the touch demo: GLFW + OpenGL3, desktop or Emscripten)" OFF)
option(IMGUI_KIT_BUILD_TEST_ENGINE_TESTS "Build the tests driven by Dear ImGui's Test Engine (headless, and visible)" OFF)
option(IMGUI_KIT_INSTALL "Install the library, its header and its CMake package (find_package(imgui_kit))" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 11)  # as Dear ImGui
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT TARGET imgui AND NOT TARGET imgui::imgui)
    find_package(imgui CONFIG QUIET)
endif()
if (TARGET imgui::imgui AND NOT TARGET imgui)
    set(IMGUI_KIT_IMGUI_TARGET imgui::imgui)
else()
    set(IMGUI_KIT_IMGUI_TARGET imgui)
endif()
if (NOT TARGET ${IMGUI_KIT_IMGUI_TARGET})
    if (NOT IMGUI_KIT_IMGUI_DIR)
        include(FetchContent)
        FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG ${IMGUI_KIT_FETCH_IMGUI_TAG})
        FetchContent_MakeAvailable(imgui)
        set(IMGUI_KIT_IMGUI_DIR ${imgui_SOURCE_DIR})
    endif()
    add_library(imgui STATIC
        ${IMGUI_KIT_IMGUI_DIR}/imgui.cpp ${IMGUI_KIT_IMGUI_DIR}/imgui_demo.cpp ${IMGUI_KIT_IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_KIT_IMGUI_DIR}/imgui_tables.cpp ${IMGUI_KIT_IMGUI_DIR}/imgui_widgets.cpp)
    target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${IMGUI_KIT_IMGUI_DIR}> $<INSTALL_INTERFACE:include>)
    # The kit uses no obsolete name of Dear ImGui: apps that disable them (as Dear ImGui Bundle does) can build it
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
    set(IMGUI_KIT_BUILT_IMGUI ON)
endif()

if (PROJECT_IS_TOP_LEVEL)
    # The page's script (imgui_kit_emscripten_keyboard.js) is copied into its .cpp by tools/embed_js.py, which writes
    # the file's sha1 in the marker line: a stale copy stops a developer's build here, not in the CI
    file(SHA1 ${CMAKE_CURRENT_SOURCE_DIR}/imgui_kit_emscripten_keyboard.js IMGUI_KIT_KEYBOARD_JS_SHA1)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/imgui_kit_emscripten_keyboard.cpp IMGUI_KIT_KEYBOARD_JS_MARKER
         REGEX "<embedded file=\"imgui_kit_emscripten_keyboard.js\" sha1=\"")
    if (NOT IMGUI_KIT_KEYBOARD_JS_MARKER MATCHES "sha1=\"${IMGUI_KIT_KEYBOARD_JS_SHA1}\"")
        message(FATAL_ERROR "imgui_kit_emscripten_keyboard.cpp holds a stale copy of imgui_kit_emscripten_keyboard.js: "
                            "run tools/embed_js.py (edit the .js, never the copy)")
    endif()
endif()

file(GLOB IMGUI_KIT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imgui_kit*.cpp)  # imgui_kit.cpp, imgui_kit_layout.cpp...
add_library(imgui_kit STATIC ${IMGUI_KIT_SOURCES})
add_library(imgui_kit::imgui_kit ALIAS imgui_kit)
# Installed in include/imgui_kit/, reachable both as "imgui_kit.h" (as from add_subdirectory) and "imgui_kit/imgui_kit.h"
# (as from Dear ImGui Bundle)
target_include_directories(imgui_kit PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include> $<INSTALL_INTERFACE:include/imgui_kit>)
target_link_libraries(imgui_kit PUBLIC ${IMGUI_KIT_IMGUI_TARGET})
if (PROJECT_IS_TOP_LEVEL)  # the kit's own builds (its CI): Dear ImGui's warning level
    if (MSVC)
        target_compile_options(imgui_kit PRIVATE /W4)
    else()
        target_compile_options(imgui_kit PRIVATE -Wall -Wextra -Wshadow -Wformat)
    endif()
endif()

if (IMGUI_KIT_BUILD_TESTS)
    if (NOT IMGUI_KIT_BUILT_IMGUI)
        message(FATAL_ERROR "imgui_kit's tests build Dear ImGui's null backend: give IMGUI_KIT_IMGUI_DIR, or no `imgui` target")
    endif()
    enable_testing()
    add_executable(imgui_kit_tests tests/test_main.cpp tests/test_em.cpp tests/test_layout.cpp
        tests/test_touch_scroll.cpp tests/test_wheel_session.cpp tests/test_touch_setup.cpp ${IMGUI_KIT_IMGUI_DIR}/backends/imgui_impl_null.cpp)
    target_include_directories(imgui_kit_tests PRIVATE tests ${IMGUI_KIT_IMGUI_DIR}/backends)
    target_link_libraries(imgui_kit_tests PRIVATE imgui_kit)
    add_test(NAME imgui_kit_tests COMMAND imgui_kit_tests)
endif()

if (IMGUI_KIT_BUILD_TEST_ENGINE_TESTS)
    if (NOT IMGUI_KIT_BUILT_IMGUI)
        message(FATAL_ERROR "imgui_kit's Test Engine tests build Dear ImGui with the Test Engine's hooks: give IMGUI_KIT_IMGUI_DIR, or no `imgui` target")
    endif()
    enable_testing()
    add_subdirectory(tests/test_engine)
endif()

if (IMGUI_KIT_BUILD_EXAMPLES)
    if (NOT IMGUI_KIT_BUILT_IMGUI)
        message(FATAL_ERROR "imgui_kit's examples use Dear ImGui's backends: give IMGUI_KIT_IMGUI_DIR, or no `imgui` target")
    endif()
    if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        add_subdirectory(examples/ios_touch_demo)
    else()
        add_subdirectory(examples/touch_demo)
    endif()
endif()

if (IMGUI_KIT_INSTALL)
    # The package: find_package(imgui_kit) gives imgui_kit::imgui_kit. With a Dear ImGui the kit built itself, it is
    # installed too (its target and headers); with one from a package (imgui::imgui), the config finds that package.
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(IMGUI_KIT_INSTALL_TARGETS imgui_kit)
    set(IMGUI_KIT_CONFIG_FIND_IMGUI "")
    if (IMGUI_KIT_BUILT_IMGUI)
        list(APPEND IMGUI_KIT_INSTALL_TARGETS imgui)
        install(DIRECTORY ${IMGUI_KIT_IMGUI_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h"
            PATTERN "backends" EXCLUDE PATTERN "examples" EXCLUDE PATTERN "misc" EXCLUDE PATTERN "docs" EXCLUDE)
    elseif (IMGUI_KIT_IMGUI_TARGET STREQUAL "imgui::imgui")
        set(IMGUI_KIT_CONFIG_FIND_IMGUI "include(CMakeFindDependencyMacro)\nfind_dependency(imgui)")
    endif()
    install(TARGETS ${IMGUI_KIT_INSTALL_TARGETS} EXPORT imgui_kitTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES imgui_kit.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui_kit)
    install(EXPORT imgui_kitTargets NAMESPACE imgui_kit:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui_kit)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/imgui_kitConfig.cmake.in
        "@PACKAGE_INIT@\n@IMGUI_KIT_CONFIG_FIND_IMGUI@\ninclude(\"\${CMAKE_CURRENT_LIST_DIR}/imgui_kitTargets.cmake\")\n")
    configure_package_config_file(${CMAKE_CURRENT_BINARY_DIR}/imgui_kitConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/imgui_kitConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui_kit)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/imgui_kitConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/imgui_kitConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/imgui_kitConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui_kit)
endif()
