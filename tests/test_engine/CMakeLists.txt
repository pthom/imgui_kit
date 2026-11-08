# imgui_kit's tests with Dear ImGui's Test Engine (https://github.com/ocornut/imgui_test_engine): the touch layer and
# the wheel session, driven as a user would. Two programs run the same tests:
#   - imgui_kit_te_tests: headless (Dear ImGui's null backend), at full speed; ctest and the CI run it;
#   - imgui_kit_te_tests_visible: in a window (GLFW + OpenGL3), with the Test Engine's UI, to watch them.
# The Test Engine has its own license (free for open source, see its LICENSE.txt), distinct from the kit's MIT license.
set(IMGUI_KIT_FETCH_TEST_ENGINE_TAG "v1.93.0" CACHE STRING "The Test Engine tag fetched for the tests")
include(FetchContent)
FetchContent_Declare(imgui_test_engine GIT_REPOSITORY https://github.com/ocornut/imgui_test_engine.git
    GIT_TAG ${IMGUI_KIT_FETCH_TEST_ENGINE_TAG})
FetchContent_MakeAvailable(imgui_test_engine)
set(te_dir ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine)

# Dear ImGui calls the Test Engine's hooks: its configuration names them
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${te_dir})
target_compile_definitions(imgui PUBLIC IMGUI_USER_CONFIG="kit_te_imconfig.h")

file(GLOB te_sources ${te_dir}/*.cpp)
add_library(imgui_test_engine STATIC ${te_sources})
target_link_libraries(imgui_test_engine PUBLIC imgui)
find_package(Threads REQUIRED)
target_link_libraries(imgui_test_engine PUBLIC Threads::Threads)
target_link_libraries(imgui INTERFACE imgui_test_engine)  # every user of this Dear ImGui calls the hooks

add_executable(imgui_kit_te_tests main_headless.cpp kit_te_tests.cpp ${IMGUI_KIT_IMGUI_DIR}/backends/imgui_impl_null.cpp)
target_include_directories(imgui_kit_te_tests PRIVATE ${IMGUI_KIT_IMGUI_DIR}/backends)
target_link_libraries(imgui_kit_te_tests PRIVATE imgui_kit)
add_test(NAME imgui_kit_te_tests COMMAND imgui_kit_te_tests)

if (NOT EMSCRIPTEN)
    if (NOT TARGET glfw)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
        FetchContent_MakeAvailable(glfw)
    endif()
    find_package(OpenGL REQUIRED)
    add_executable(imgui_kit_te_tests_visible main_visible.cpp kit_te_tests.cpp
        ${IMGUI_KIT_IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_KIT_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(imgui_kit_te_tests_visible PRIVATE ${IMGUI_KIT_IMGUI_DIR}/backends)
    target_link_libraries(imgui_kit_te_tests_visible PRIVATE imgui_kit glfw OpenGL::GL)
endif()
