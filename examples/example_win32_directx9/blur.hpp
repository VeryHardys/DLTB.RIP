#pragma once
#include <d3d9.h>
#include "imgui.h"

namespace blur {
    inline IDirect3DDevice9* device = nullptr;
    inline void draw() {}
    inline void draw_blur(ImDrawList* draw_list) {}
}

inline void draw_blur(ImDrawList* draw_list) {}
inline void begin_blur(const ImDrawList* parent_list, const ImDrawCmd* cmd) {}
inline void end_blur(const ImDrawList* parent_list, const ImDrawCmd* cmd) {}
