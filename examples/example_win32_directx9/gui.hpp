#pragma once

#define  IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "color_t.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

using namespace std;

class c_gui {

public:

    float m_anim = 0.f;
    int m_tab = 0;

    int m_rage_subtab = 0;
    vector < const char* > rage_subtabs = { "General", "Anti aim", "Subtab" };

    color_t accent_color = { 0.3f, 0.49f, 1.f, 1.f };

    color_t text = { 1.f, 1.f, 1.f, 1.f };
    color_t text_disabled = { 0.51f, 0.52f, 0.56f, 1.f };

    color_t border = { 1.f, 1.f, 1.f, 0.03f };

    color_t frame_inactive = { 0.023f, 0.039f, 0.07f, 1.f };
    color_t frame_active = { 0.043f, 0.07f, 0.137f, 1.f };

    color_t button = { 0.031f, 0.035f, 0.058f, 1.f };
    color_t button_hovered = { 0.050f, 0.054f, 0.078f, 1.f };
    color_t button_active = { 0.07f, 0.074f, 0.098f, 1.f };

    color_t group_box_bg = { 0.019f, 0.035f, 0.062f, 1.f };

    // --- дизайн меню: 0 = Classic (исходный), 1 = Modern (стекло в стиле вотермарки)
    int m_design = 0;
    color_t window_bg = { 0.043f, 0.051f, 0.075f, 1.f };   // Modern: фон окна
    float m_bg_alpha = 0.96f;                                // Modern: непрозрачность стекла
    float m_rounding = 10.f;                                 // Modern: скругление окна/панелей
    bool m_shimmer = true;                                   // Modern: бегущий блик под шапкой
    bool m_glow = true;                                      // Modern: акцентный отсвет слева

    // Палитра виджетов. Classic хранит исходные цвета, чтобы к нему можно было вернуться.
    struct palette_t {
        color_t accent, frame_inactive, frame_active, button, button_hovered, button_active, group_box_bg, border;
    };
    static palette_t classic_palette( ) {
        return { { 0.3f, 0.49f, 1.f, 1.f }, { 0.023f, 0.039f, 0.07f, 1.f }, { 0.043f, 0.07f, 0.137f, 1.f },
                 { 0.031f, 0.035f, 0.058f, 1.f }, { 0.050f, 0.054f, 0.078f, 1.f }, { 0.07f, 0.074f, 0.098f, 1.f },
                 { 0.019f, 0.035f, 0.062f, 1.f }, { 1.f, 1.f, 1.f, 0.03f } };
    }
    void apply_palette( const palette_t& p ) {
        accent_color = p.accent; frame_inactive = p.frame_inactive; frame_active = p.frame_active;
        button = p.button; button_hovered = p.button_hovered; button_active = p.button_active;
        group_box_bg = p.group_box_bg; border = p.border;
    }

    // Фон окна Modern (вызывается из ImGui::Begin для окна меню)
    void draw_modern_window( ImDrawList* draw, ImVec2 pos, ImVec2 size );
    // Логотип-плитка как у вотермарки: [молния] DLTB.RIP
    void draw_modern_logo( ImDrawList* draw, ImVec2 pos, ImFont* bold );
    // Статус в подвале сайдбара с пульсирующей точкой
    void draw_modern_status( ImDrawList* draw, ImVec2 pos, const char* label, const char* status, bool ok );

    void render_circle_for_horizontal_bar( ImVec2 pos, ImColor color, float alpha );

    // Подпись слева от виджета: если не влезает до поля (max_x) — обрезается с "...",
    // полный текст показывается во всплывающей подсказке при наведении.
    void side_label( ImDrawList* draw, ImVec2 pos, float max_x, ImU32 col, const char* label );

    // Сегментированный переключатель подвкладок (вместо ряда обычных кнопок)
    bool subtabs( const char* id, const char* const* names, int count, int* current );

    inline void group_title( const char* name ) {

        SetCursorPosX( GetCursorPosX( ) + 10 );
        PushStyleColor( ImGuiCol_Text, ImVec4( 1.f, 1.f, 1.f, 0.5f ) );
        Text( name );
        PopStyleColor( );
    }

    void group_box( const char* name, ImVec2 size_arg );
    void end_group_box( );

    bool tab( const char* icon, const char* label, bool selected );
    bool subtab( const char* label, bool selected, int size, ImDrawFlags flags );

};

inline c_gui gui;
