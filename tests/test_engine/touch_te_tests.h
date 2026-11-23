// imgui_touch's tests with Dear ImGui's Test Engine: shared by the headless program and the visible one
#pragma once
struct ImGuiTestEngine;

// The setup that the tests need, after ImGui::CreateContext() and before ImGuiTestEngine_Start(): the touch layer
void TouchTestsSetup();

// Registers the tests, in the groups "touch" and "demo"
void TouchTestsRegister(ImGuiTestEngine* engine);
