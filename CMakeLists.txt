cmake_minimum_required(VERSION 3.16)
project(imgui_kit VERSION 0.1.0 LANGUAGES CXX)

# imgui_kit needs Dear ImGui. Three ways, in this order:
#   - a target named `imgui` already exists (your project, or a library that brings Dear ImGui): it is used;
#   - IMGUI_KIT_IMGUI_DIR names a Dear ImGui folder: a target `imgui` is built from it;
#   - else Dear ImGui is fetched (IMGUI_KIT_FETCH_IMGUI_TAG, its docking branch).
set(IMGUI_KIT_IMGUI_DIR "" CACHE PATH "A Dear ImGui folder, used when no target `imgui` exists")
set(IMGUI_KIT_FETCH_IMGUI_TAG "v1.93.0-docking" CACHE STRING "The Dear ImGui tag fetched when no folder is given")
option(IMGUI_KIT_BUILD_TESTS "Build the tests (Dear ImGui's null backend, doctest)" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT TARGET imgui)
    if (NOT IMGUI_KIT_IMGUI_DIR)
        include(FetchContent)
        FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG ${IMGUI_KIT_FETCH_IMGUI_TAG})
        FetchContent_MakeAvailable(imgui)
        set(IMGUI_KIT_IMGUI_DIR ${imgui_SOURCE_DIR})
    endif()
    add_library(imgui STATIC
        ${IMGUI_KIT_IMGUI_DIR}/imgui.cpp ${IMGUI_KIT_IMGUI_DIR}/imgui_demo.cpp ${IMGUI_KIT_IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_KIT_IMGUI_DIR}/imgui_tables.cpp ${IMGUI_KIT_IMGUI_DIR}/imgui_widgets.cpp)
    target_include_directories(imgui PUBLIC ${IMGUI_KIT_IMGUI_DIR})
    set(IMGUI_KIT_BUILT_IMGUI ON)
endif()

add_library(imgui_kit STATIC imgui_kit.cpp imgui_kit_demo.cpp)
target_include_directories(imgui_kit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui_kit PUBLIC imgui)

if (IMGUI_KIT_BUILD_TESTS)
    if (NOT IMGUI_KIT_BUILT_IMGUI)
        message(FATAL_ERROR "imgui_kit's tests build Dear ImGui's null backend: give IMGUI_KIT_IMGUI_DIR, or no `imgui` target")
    endif()
    enable_testing()
    add_executable(imgui_kit_tests tests/test_main.cpp tests/test_em.cpp ${IMGUI_KIT_IMGUI_DIR}/backends/imgui_impl_null.cpp)
    target_include_directories(imgui_kit_tests PRIVATE tests ${IMGUI_KIT_IMGUI_DIR}/backends)
    target_link_libraries(imgui_kit_tests PRIVATE imgui_kit)
    add_test(NAME imgui_kit_tests COMMAND imgui_kit_tests)
endif()
