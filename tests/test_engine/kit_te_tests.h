// imgui_kit's tests with Dear ImGui's Test Engine: shared by the headless program and the visible one
#pragma once
struct ImGuiTestEngine;

// The kit's setups that the tests need, after ImGui::CreateContext() and before ImGuiTestEngine_Start(): the touch
// layer and the wheel session
void KitTestsSetup();

// Registers the tests, in the groups "touch", "wheel" and "demo"
void KitTestsRegister(ImGuiTestEngine* engine);
