// imgui_kit's Test Engine tests: Dear ImGui's configuration (IMGUI_USER_CONFIG) when they are built
#pragma once
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL 1  // the tests run in a coroutine, on a std::thread
#define IMGUI_TEST_ENGINE_ENABLE_STD_FUNCTION 1              // the tests are lambdas that capture their state
#define IMGUI_TEST_ENGINE_ENABLE_CAPTURE 0                   // no screen capture
#include "imgui_te_imconfig.h"
