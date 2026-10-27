#include "imgui.h"
#include "imgui_internal.h"
#include <cstring>

static bool DummyFontBuilder_Build(ImFontAtlas* atlas) {
    // Чтобы меню не падало при обращении к Fonts[0] и Fonts[1],
    // инициализируем метрики каждого добавленного шрифта
    for (int i = 0; i < atlas->ConfigData.Size; i++) {
        ImFontConfig& cfg = atlas->ConfigData[i];
        ImFont* font = cfg.DstFont;
        if (!font) continue;

        font->FontSize = cfg.SizePixels > 0.0f ? cfg.SizePixels : 14.0f;
        font->Ascent = font->FontSize;
        font->Descent = 0.0f;
        font->ContainerAtlas = atlas;
        if (font->ConfigData == nullptr) {
            font->ConfigData = &cfg;
            font->ConfigDataCount = 1;
        }
        // Добавляем базовый глиф, чтобы CalcTextSizeA не падал
        font->AddGlyph(&cfg, ' ', 0, 0, 4, font->FontSize, 4.0f, 0, 0, 0, 0);
        font->BuildLookupTable();
    }

    // Создаем пустую белую текстуру 64x64 для рендера рамок и фигур DirectX 9
    atlas->TexWidth = 64;
    atlas->TexHeight = 64;
    atlas->TexUvScale = ImVec2(1.0f / 64.0f, 1.0f / 64.0f);
    atlas->TexUvWhitePixel = ImVec2(0.5f, 0.5f);

    atlas->TexPixelsAlpha8 = (unsigned char*)IM_ALLOC(64 * 64);
    memset(atlas->TexPixelsAlpha8, 0xFF, 64 * 64);

    return true;
}

namespace ImGuiFreeType {
    const ImFontBuilderIO* GetBuilderForFreeType() {
        static ImFontBuilderIO io;
        io.FontBuilder_Build = DummyFontBuilder_Build;
        return &io;
    }
}
