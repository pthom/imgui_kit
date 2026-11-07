// imgui_kit: what its sources share, not part of its API
// https://github.com/pthom/imgui_kit (MIT license)
#pragma once
#include "imgui_kit.h"

namespace ImGuiKit
{
namespace Internal
{
    // True between FillRowTail() and EndFillRow(): the fill row makes the room, a widget must not wrap
    bool IsInFillRowTail();
}
}
