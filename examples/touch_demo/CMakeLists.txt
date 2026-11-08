# imgui_kit's touch demo: Dear ImGui's GLFW + OpenGL3 backends, for the desktop and the web (Emscripten).
# Needs the Dear ImGui folder (its backends and its Emscripten shell): built with the kit's own Dear ImGui.
set(imgui_dir ${IMGUI_KIT_IMGUI_DIR})
add_executable(touch_demo main.cpp ${imgui_dir}/backends/imgui_impl_glfw.cpp ${imgui_dir}/backends/imgui_impl_opengl3.cpp)
target_include_directories(touch_demo PRIVATE ${imgui_dir}/backends)
target_link_libraries(touch_demo PRIVATE imgui_kit)

if (EMSCRIPTEN)
    # The emscripten-glfw port (contrib.glfw3), as Dear ImGui's example: a better support of high DPI displays
    target_compile_options(touch_demo PRIVATE --use-port=contrib.glfw3)
    target_include_directories(touch_demo PRIVATE ${imgui_dir}/examples/libs/emscripten)
    target_link_options(touch_demo PRIVATE --use-port=contrib.glfw3 -sWASM=1 -sALLOW_MEMORY_GROWTH=1
        -sNO_FILESYSTEM=1 -sDISABLE_EXCEPTION_CATCHING=1 --shell-file ${imgui_dir}/examples/libs/emscripten/shell_minimal.html)
    set_target_properties(touch_demo PROPERTIES SUFFIX .html OUTPUT_NAME index)
else()
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
    FetchContent_MakeAvailable(glfw)
    find_package(OpenGL REQUIRED)
    target_link_libraries(touch_demo PRIVATE glfw OpenGL::GL)
endif()
