#include "gui.hpp"
#include "hashes.hpp"
#include <cmath>
using namespace ImGui;

// Акцент с заданной прозрачностью (без учёта style.Alpha — для фона окна)
static ImU32 accent_a( float a ) { return ImColor( gui.accent_color.r, gui.accent_color.g, gui.accent_color.b, a ); }

void c_gui::render_circle_for_horizontal_bar( ImVec2 pos, ImColor color, float alpha ) {

    auto draw = GetWindowDrawList( );
    draw->AddCircleFilled( pos, 6, ImColor( color.Value.x, color.Value.y, color.Value.z, alpha * GetStyle( ).Alpha ) );
}

void c_gui::side_label( ImDrawList* draw, ImVec2 pos, float max_x, ImU32 col, const char* label ) {

    const char* end = FindRenderedTextEnd( label );
    if ( end == label ) return;
    ImVec2 full = CalcTextSize( label, end );
    if ( pos.x + full.x <= max_x ) { draw->AddText( pos, col, label, end ); return; }

    // обрезаем по символам (UTF-8), пока текст + "..." не влезет
    const float dots = CalcTextSize( "..." ).x;
    const char* cut = end;
    while ( cut > label && pos.x + CalcTextSize( label, cut ).x + dots > max_x ) {
        --cut;
        while ( cut > label && ( ( unsigned char )*cut & 0xC0 ) == 0x80 ) --cut;
    }
    draw->AddText( pos, col, label, cut );
    draw->AddText( ImVec2( pos.x + CalcTextSize( label, cut ).x, pos.y ), col, "..." );
    if ( IsMouseHoveringRect( pos, ImVec2( max_x, pos.y + full.y ) ) )
        SetTooltip( "%.*s", ( int )( end - label ), label );
}

bool c_gui::subtabs( const char* id, const char* const* names, int count, int* current ) {

    auto window = GetCurrentWindow( );
    if ( window->SkipItems || count <= 0 ) return false;
    auto draw = window->DrawList;
    const float h = 28.f, pad = 3.f;
    const float r = m_design == 1 ? m_rounding * 0.6f : 5.f;
    ImVec2 pos = window->DC.CursorPos;
    ImRect bb( pos, pos + ImVec2( GetContentRegionAvail( ).x, h ) );
    ItemSize( bb );
    if ( !ItemAdd( bb, 0 ) ) return false;

    draw->AddRectFilled( bb.Min, bb.Max, frame_inactive.to_im_color( ), r );
    draw->AddRect( bb.Min, bb.Max, border.to_im_color( ), r );

    // плавно едущая подложка под выбранной вкладкой
    static std::unordered_map < ImGuiID, float > slide;
    ImGuiID sid = window->GetID( id );
    const float segW = ( bb.GetWidth( ) - pad * 2 ) / count;
    auto it = slide.find( sid );
    if ( it == slide.end( ) ) it = slide.insert( { sid, ( float )*current } ).first;
    it->second = ImLerp( it->second, ( float )*current, 0.18f );
    ImVec2 sMin( bb.Min.x + pad + segW * it->second, bb.Min.y + pad ), sMax( sMin.x + segW, bb.Max.y - pad );
    draw->AddRectFilled( sMin, sMax, frame_active.to_im_color( ), r - 1.f );
    draw->AddRectFilled( ImVec2( sMin.x + segW * 0.3f, sMax.y - 2.f ), ImVec2( sMax.x - segW * 0.3f, sMax.y ), accent_color.to_im_color( ), 1.f );

    bool changed = false;
    for ( int i = 0; i < count; ++i ) {
        ImRect s( ImVec2( bb.Min.x + pad + segW * i, bb.Min.y ), ImVec2( bb.Min.x + pad + segW * ( i + 1 ), bb.Max.y ) );
        PushID( i );
        ImGuiID bid = window->GetID( id );
        PopID( );
        // каждая кнопка — отдельный элемент: без ItemAdd ImGui сбрасывает ActiveId
        // через кадр после нажатия, и клик (нажал-отпустил) не засчитывается
        ItemAdd( s, bid );
        bool hovered, held;
        if ( ButtonBehavior( s, bid, &hovered, &held ) && *current != i ) { *current = i; changed = true; }
        ImVec2 ts = CalcTextSize( names[ i ] );
        ImU32 col = ( *current == i ) ? text.to_im_color( ) : ( hovered ? text.to_im_color( 0.8f ) : text_disabled.to_im_color( ) );
        draw->AddText( ImVec2( std::floor( s.GetCenter( ).x - ts.x * 0.5f ), std::floor( s.GetCenter( ).y - ts.y * 0.5f ) ), col, names[ i ] );
    }
    return changed;
}

// ==================== MODERN: фон окна ====================
// Та же "стеклянная" подача, что у вотермарки: тёмное стекло, мягкая тень,
// акцентный отсвет слева, блик сверху, акцентная кромка с бегущим бликом под шапкой.
void c_gui::draw_modern_window( ImDrawList* draw, ImVec2 pos, ImVec2 size ) {

    const float R = m_rounding;
    const float a = m_bg_alpha;
    const float side = 170.f, head = 60.f;
    const float t = ( float )GetTime( );
    ImVec2 max = pos + size;

    // тень за пределами окна
    draw->PushClipRect( pos - ImVec2( 24, 24 ), max + ImVec2( 24, 24 ), false );
    for ( int s = 1; s <= 4; ++s ) {
        float e = s * 3.f;
        draw->AddRectFilled( pos - ImVec2( e, e - 4 ), max + ImVec2( e, e + 4 ), IM_COL32( 0, 0, 0, ( int )( a * ( 34 - s * 7 ) ) ), R + e );
    }
    draw->PopClipRect( );

    // стекло
    draw->AddRectFilled( pos, max, window_bg.to_im_color( a, false ), R );
    // сайдбар чуть светлее, шапка — тонкая полупрозрачная полоса
    draw->AddRectFilled( pos, pos + ImVec2( side, size.y ), IM_COL32( 255, 255, 255, ( int )( a * 6 ) ), R, ImDrawFlags_RoundCornersLeft );
    draw->AddRectFilled( pos + ImVec2( side, 0 ), pos + ImVec2( size.x, head ), IM_COL32( 255, 255, 255, ( int )( a * 4 ) ), R, ImDrawFlags_RoundCornersTopRight );
    if ( m_glow )
        draw->AddRectFilledMultiColor( pos + ImVec2( 2, 2 ), pos + ImVec2( side * 1.6f, size.y - 2 ), accent_a( 0.09f * a ), IM_COL32( 0, 0, 0, 0 ), IM_COL32( 0, 0, 0, 0 ), accent_a( 0.09f * a ) );

    // разделители
    draw->AddLine( pos + ImVec2( side, 10 ), pos + ImVec2( side, size.y - 10 ), IM_COL32( 255, 255, 255, 20 ), 1.f );
    draw->AddLine( pos + ImVec2( R, 0.5f ), pos + ImVec2( size.x - R, 0.5f ), IM_COL32( 255, 255, 255, ( int )( a * 20 ) ), 1.f );

    // акцентная кромка под шапкой + бегущий блик (как нижняя кромка вотермарки)
    float ly = pos.y + head - 1.f, lx0 = pos.x + side + 1.f, lx1 = max.x - 1.f;
    draw->AddRectFilled( ImVec2( lx0, ly ), ImVec2( lx1, ly + 1.5f ), accent_a( 0.30f ) );
    if ( m_shimmer ) {
        draw->PushClipRect( ImVec2( lx0, ly - 2 ), ImVec2( lx1, ly + 4 ), true );
        float span = ( lx1 - lx0 ) + 200.f;
        float sx = lx0 - 100.f + std::fmod( t * 160.f, span );
        draw->AddRectFilledMultiColor( ImVec2( sx - 80.f, ly - 0.5f ), ImVec2( sx, ly + 2.f ), accent_a( 0.f ), accent_a( 1.f ), accent_a( 1.f ), accent_a( 0.f ) );
        draw->AddRectFilledMultiColor( ImVec2( sx, ly - 0.5f ), ImVec2( sx + 80.f, ly + 2.f ), accent_a( 1.f ), accent_a( 0.f ), accent_a( 0.f ), accent_a( 1.f ) );
        draw->PopClipRect( );
    }

    // рамка
    draw->AddRect( pos, max, border.to_im_color( 1.f, false ), R, 0, 1.f );
}

void c_gui::draw_modern_logo( ImDrawList* draw, ImVec2 pos, ImFont* bold ) {

    ImFont* body = GetFont( );
    if ( !bold ) bold = body;
    const float fs = body->FontSize * GetIO( ).FontGlobalScale;
    const float bfs = bold->FontSize * GetIO( ).FontGlobalScale * 1.25f;
    const float tile = 30.f;
    const float t = ( float )GetTime( );
    const float al = GetStyle( ).Alpha;

    ImVec2 tl = pos, br = pos + ImVec2( tile, tile );
    draw->AddRectFilled( tl, br, accent_a( al ), 7.f );
    draw->AddRectFilledMultiColor( ImVec2( tl.x + 1, tl.y + tile * 0.5f ), ImVec2( br.x - 1, br.y - 1 ), IM_COL32( 0, 0, 0, 0 ), IM_COL32( 0, 0, 0, 0 ), IM_COL32( 0, 0, 0, 70 ), IM_COL32( 0, 0, 0, 70 ) );
    float pulse = 0.5f + 0.5f * std::sin( t * 2.2f );
    draw->AddRect( tl - ImVec2( 1, 1 ), br + ImVec2( 1, 1 ), accent_a( ( 0.25f + 0.25f * pulse ) * al ), 8.f, 0, 1.f );
    ImVec2 isz = body->CalcTextSizeA( fs, FLT_MAX, 0.f, ICON_FA_BOLT );
    draw->AddText( body, fs, ImVec2( std::floor( tl.x + ( tile - isz.x ) * 0.5f ), std::floor( tl.y + ( tile - isz.y ) * 0.5f ) ), IM_COL32( 255, 255, 255, ( int )( 255 * al ) ), ICON_FA_BOLT );

    ImVec2 t1 = bold->CalcTextSizeA( bfs, FLT_MAX, 0.f, "DLTB" );
    float x = br.x + 10.f, y = std::floor( tl.y + ( tile - t1.y ) * 0.5f );
    draw->AddText( bold, bfs, ImVec2( x, y ), text.to_im_color( ), "DLTB" );
    draw->AddText( bold, bfs, ImVec2( x + t1.x, y ), accent_color.to_im_color( ), ".RIP" );
}

void c_gui::draw_modern_status( ImDrawList* draw, ImVec2 pos, const char* label, const char* status, bool ok ) {

    const float t = ( float )GetTime( );
    const float al = GetStyle( ).Alpha;
    draw->AddText( pos, text.to_im_color( ), label );
    float cy = pos.y + CalcTextSize( label ).y + 9.f;
    ImVec4 dc = ok ? ImVec4( 0.28f, 0.86f, 0.50f, 1.f ) : ImVec4( 0.94f, 0.77f, 0.27f, 1.f );
    float pulse = 0.5f + 0.5f * std::sin( t * 3.f );
    draw->AddCircleFilled( ImVec2( pos.x + 4.f, cy ), 5.5f, ImColor( dc.x, dc.y, dc.z, ( 0.18f + 0.15f * pulse ) * al ), 16 );
    draw->AddCircleFilled( ImVec2( pos.x + 4.f, cy ), 3.f, ImColor( dc.x, dc.y, dc.z, al ), 12 );
    draw->AddText( ImVec2( pos.x + 14.f, cy - CalcTextSize( status ).y * 0.5f ), text_disabled.to_im_color( ), status );
}

bool c_gui::tab( const char* icon, const char* label, bool selected ) {

    auto window = GetCurrentWindow( );
    auto id = window->GetID( label );

    auto icon_size = CalcTextSize( icon );
    auto label_size = CalcTextSize( label, 0, 1 );

    auto pos = window->DC.CursorPos;
    auto draw = window->DrawList;

    ImRect bb( pos, pos + ImVec2( GetWindowWidth( ), 30 ) );
    ItemAdd( bb, id );
    ItemSize( bb, GetStyle( ).FramePadding.y );

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    static std::unordered_map < ImGuiID, float > values;
    auto value = values.find( id );
    if ( value == values.end( ) ) {

        values.insert( { id, 0.f } );
        value = values.find( id );
    }

    value->second = ImLerp( value->second, ( selected ? 1.f : 0.f ), 0.05f );

    if ( m_design == 1 ) {
        // Modern: акцентная подсветка с градиентом и полоска слева, как у плитки вотермарки
        const float v = value->second;
        if ( hovered && !selected ) draw->AddRectFilled( bb.Min, bb.Max, IM_COL32( 255, 255, 255, 8 ), 6 );
        draw->AddRectFilledMultiColor( bb.Min, bb.Max, accent_color.to_im_color( 0.20f * v ), accent_color.to_im_color( 0.f ), accent_color.to_im_color( 0.f ), accent_color.to_im_color( 0.20f * v ) );
        draw->AddRectFilled( ImVec2( bb.Min.x, bb.Min.y + 6 + 9 * ( 1.f - v ) ), ImVec2( bb.Min.x + 3, bb.Max.y - 6 - 9 * ( 1.f - v ) ), accent_color.to_im_color( v ), 2 );
        draw->AddText( ImVec2( bb.Min.x + 12, bb.GetCenter( ).y - label_size.y / 2 ), selected ? accent_color.to_im_color( ) : text_disabled.to_im_color( ), icon );
        draw->AddText( ImVec2( bb.Min.x + 37, bb.GetCenter( ).y - label_size.y / 2 ), selected ? text.to_im_color( ) : ( hovered ? text.to_im_color( 0.85f ) : text_disabled.to_im_color( ) ), label );
        return pressed;
    }

    draw->AddRectFilled( bb.Min, bb.Max, gui.frame_active.to_im_color( 0.5f * value->second ), 5 );

    draw->AddText( ImVec2( bb.Min.x + 10, bb.GetCenter( ).y - label_size.y / 2 ), gui.accent_color.to_im_color( ), icon );
    draw->AddText( ImVec2( bb.Min.x + 35, bb.GetCenter( ).y - label_size.y / 2 ), GetColorU32( ImGuiCol_Text ), label );

    return pressed;
}

bool c_gui::subtab( const char* label, bool selected, int size, ImDrawFlags flags ) {

    auto window = GetCurrentWindow( );
    auto id = window->GetID( label );

    auto label_size = CalcTextSize( label, 0, 1 );

    auto pos = window->DC.CursorPos;
    auto draw = window->DrawList;

    ImRect bb( pos, pos + ImVec2( GetWindowWidth( ) / size, GetWindowHeight( ) ) );
    ItemAdd( bb, id );
    ItemSize( bb, GetStyle( ).FramePadding.y );

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );

    static std::unordered_map < ImGuiID, float > values;
    auto value = values.find( id );
    if ( value == values.end( ) ) {

        values.insert( { id, 0.f } );
        value = values.find( id );
    }

    value->second = ImLerp( value->second, ( selected ? 1.f : 0.f ), 0.05f );

    draw->AddRectFilled( bb.Min, bb.Max, gui.frame_active.to_im_color( 0.8f * value->second ), 4, flags );

    draw->AddText( bb.GetCenter( ) - label_size / 2, selected ? gui.text.to_im_color( ) : gui.text_disabled.to_im_color( ), label );

    return pressed;
}

void c_gui::group_box( const char* name, ImVec2 size_arg ) {

    auto window = GetCurrentWindow( );
    auto pos = window->DC.CursorPos;

    BeginChild( std::string( name ).append( ".main" ).c_str( ), size_arg, false, ImGuiWindowFlags_NoScrollbar );

    if ( m_design == 1 ) {
        // Modern: стеклянная панель, блик сверху, заголовок с акцентной полоской
        const float r = m_rounding * 0.7f;
        auto draw = GetWindowDrawList( );
        ImVec2 a = pos + ImVec2( 0, 20 ), b = pos + size_arg;
        draw->AddRectFilled( a, b, gui.group_box_bg.to_im_color( ), r );
        draw->AddRectFilledMultiColor( a + ImVec2( 1, 1 ), ImVec2( b.x - 1, a.y + 26 ), IM_COL32( 255, 255, 255, ( int )( 7 * GetStyle( ).Alpha ) ), IM_COL32( 255, 255, 255, ( int )( 7 * GetStyle( ).Alpha ) ), IM_COL32( 255, 255, 255, 0 ), IM_COL32( 255, 255, 255, 0 ) );
        draw->AddLine( a + ImVec2( r, 0.5f ), ImVec2( b.x - r, a.y + 0.5f ), IM_COL32( 255, 255, 255, ( int )( 16 * GetStyle( ).Alpha ) ), 1.f );
        draw->AddRect( a, b, gui.border.to_im_color( ), r );
        draw->AddRectFilled( pos + ImVec2( 4, 3 ), pos + ImVec2( 7, 14 ), gui.accent_color.to_im_color( ), 1.5f );
        draw->AddText( pos + ImVec2( 13, 0 ), gui.text.to_im_color( 0.85f ), name );
    }
    else {
        GetWindowDrawList( )->AddRectFilled( pos + ImVec2( 0, 20 ), pos + size_arg, gui.group_box_bg.to_im_color( ), 6 );
        GetWindowDrawList( )->AddRect( pos + ImVec2( 0, 20 ), pos + size_arg, gui.border.to_im_color( ), 6 );

        GetWindowDrawList( )->AddText( pos + ImVec2( 12, 0 ), GetColorU32( ImGuiCol_Text, 0.5f ), name );
    }

    SetCursorPos( ImVec2( 12, 21 ) );
    PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 10 } );
    BeginChild( name, { size_arg.x - 24, size_arg.y - 21 }, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding );

    BeginGroup( );

    PushStyleVar( ImGuiStyleVar_ItemSpacing, { 8, 10 } );
    PushStyleVar( ImGuiStyleVar_Alpha, gui.m_anim );
}

void c_gui::end_group_box( ) {

    PopStyleVar( 3 );
    EndGroup( );
    EndChild( );
    EndChild( );
}
