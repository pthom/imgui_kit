# imgui_kit's touch demo on iOS: Dear ImGui's Metal backend, no platform backend (UIKit's touches feed Dear ImGui).
#   cmake -S . -B build_ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DIMGUI_KIT_BUILD_EXAMPLES=ON
#   then open build_ios/imgui_kit.xcodeproj, choose your team (Signing & Capabilities), and run on your device;
#   or, for the simulator: cmake --build build_ios --config Release -- -sdk iphonesimulator
enable_language(OBJCXX)
set(imgui_dir ${IMGUI_KIT_IMGUI_DIR})
add_executable(ios_touch_demo MACOSX_BUNDLE main.mm ${imgui_dir}/backends/imgui_impl_metal.mm)
target_include_directories(ios_touch_demo PRIVATE ${imgui_dir}/backends)
target_link_libraries(ios_touch_demo PRIVATE imgui_kit "-framework UIKit" "-framework Metal" "-framework MetalKit"
    "-framework QuartzCore" "-framework CoreGraphics" "-framework Foundation")
target_compile_options(ios_touch_demo PRIVATE -fobjc-arc)
set_target_properties(ios_touch_demo PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "ImGuiKit"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.pthom.imgui-kit-touch-demo"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.github.pthom.imgui-kit-touch-demo"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")  # iPhone and iPad
