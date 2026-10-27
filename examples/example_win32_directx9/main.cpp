#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>
#include <dwmapi.h>
#include <mmsystem.h>
#include <TlHelp32.h>
#include <cstdint>
#include <cwctype>
#include <algorithm>
#include <map>
#include <cfloat>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

#include "gui.hpp"
#include "hashes.hpp"
#include "bytes.hpp"

using namespace ImGui;

#define ALPHA    ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )
#define NO_ALPHA ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )

IDirect3DTexture9* avatar = nullptr;
IDirect3DTexture9* bg = nullptr;

static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS   g_d3dpp = {};

bool g_ShowMenu = false;

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

struct Matrix4x4 {
    float m[16] = {};
};

#include "fx_config.hpp"

namespace MenuVars {
    bool godMode = false;
    bool infStamina = false;
    bool infUVFlashlight = false;
    bool infFury = false;
    bool noFallDamage = false;
    bool noKnockdown = false;

    bool moveSpeedEnabled = false;
    float speedMultiplier = 100.0f;
    bool jumpHeightEnabled = false;
    float jumpMultiplier = 4.85f;
    bool unlimitedJumps = false;
    bool tripleJump = false;
    bool alwaysVaultKick = false;

    bool floatInAir = false;
    int floatInAirKey = 0;
    int floatInAirBindMode = 0;
    bool waitingFloatKey = false;

    int tpWaypointKey = 0;
    int tpWaypointBindMode = 0;
    bool waitingTpKey = false;

    int unloadKey = VK_END;
    int unloadBindMode = 0;
    bool waitingUnloadKey = false;

    bool unlimitedCableLength = false;
    bool disableHookPullDown = false;
    bool swimSpeedEnabled = false;
    float swimSpeedMultiplier = 2.5f;
    bool disableWaterPhysics = false;
    bool disablePlayableArea = false;

    ULONGLONG tpWaypointEndTime = 0;
    ULONGLONG tpRespawnEndTime = 0;
    ULONGLONG tempNoFallEndTime = 0;

    bool nightExpEnabled = false;
    float nightExpMultiplier = 2.0f;
    bool openWorldExpEnabled = false;
    float openWorldExpMultiplier = 2.0f;
    bool disableDeathPenalty = false;

    bool powerAttackBlocking = false;
    bool alwaysPerfectBlock = false;
    bool autoBlock = false;
    bool infDurability = false;
    bool oneHitKill = false;
    bool easyFinisher = false;
    bool instantFinisher = false;
    bool alwaysElemental = false;
    bool damageMultEnabled = false;
    float damageMultiplier = 2.0f;

    bool infAmmo = false;
    bool noReload = false;
    bool noRecoil = false;
    bool megaAccuracy = false;
    bool silencedFirearms = false;
    bool bowRapidFire = false;
    bool rapidFire = false;
    bool rapidFireAdsOnly = true;
    int rapidFireDelayMs = 65;

    // Хитмаркеры и килл-эффекты (одни настройки на всех врагов)
    HitmarkerConfig hitCfg;
    KillFxConfig killCfg;
    float sfxVolume = 0.70f;
    bool debugPosCandidates = false;          // точки-кандидаты позиции на врагах
    unsigned long long hitPosSourceKey = 0;   // 0 = авто, иначе выбранный путь к позиции врага

    // Калибровка 3D-позиции хитмаркеров (общая для зомби и людей)
    float hitProjFov = 90.0f;        // горизонтальный FOV, как в настройках игры
    float hitHeightOffset = 1.15f;   // высота точки удара над ногами врага (м)
    float hitEyeHeight = 1.65f;      // высота камеры над позицией игрока (м)
    bool hitInvertPitch = false;     // если маркер уезжает вверх при взгляде вниз — включить
    bool hitDistanceScale = true;    // уменьшать маркер на дальних врагах

    bool freeCraft = false;
    bool multiplyGatheredEnabled = false;
    int gatheredMultiplier = 10;
    int moneyToAdd = 10000;

    bool freezeTime = false;
    bool forceDaytimeEnabled = false;
    float worldTime = 12.0f;
    float timeJumpHour = 12.0f;         // разовый прыжок времени (не сохраняется в конфиг)
    ULONGLONG timeJumpUntil = 0;
    bool forceDayPhaseEnabled = false;
    int dayPhaseIndex = 1;
    bool worldSpeedEnabled = false;
    float worldSpeedValue = 1.0f;
    bool disableWorldNoise = false;

    bool removeFogOfWar = false;
    ULONGLONG discoverMapEndTime = 0;
    bool editZoneLevelEnabled = false;
    int zoneMinLevel = 1;
    int zoneMaxLevel = 1;
    bool openLockedDoors = false;
    bool easyLockpick = false;

    bool changeAIScaleEnabled = false;
    float aiScaleValue = 2.0f;
    bool aiInvisibility = false;
    bool disableZombieGrab = false;
    bool disableChaseLogic = false;
    bool alwaysStomp = false;
    bool freezeKillNearbyAI = false;
    bool burnNearbyAI = false;
    bool disableAISpawn = false;

    bool noVehicleDamage = false;
    bool noFuelUsage = false;
    bool noRepairKitNeed = false;
    bool unlimitedVehicleUV = false;
    bool instantVehicleUV = false;
    bool removeCarCamSnap = false;
    bool challengeModeVehicle = false;

    bool noMoneyCost = false;
    bool multiplySellPrice = false;
    float sellPriceMultiplier = 10.0f;
    bool multiplyBuyPrice = false;
    float buyPriceMultiplier = 0.0001f;

    bool toggleHUD = false;
    bool weaponReturnThrown = false;
    bool instantDismantle = false;
    bool disableActionEquipUse = false;
    bool rightHandLoot = false;
    bool disableHeadbob = false;

    int fontWeightPreset = 0;
    float textBrightness = 0.86f;
    float textMutedBrightness = 0.65f;
    float fontScale = 1.00f;

    bool watermarkEnabled = true;
    int watermarkDesign = 1;          // 0 = Classic (старая), 1 = Modern (новая)
    bool watermarkShowStatus = true;  // Modern: статус "in game / waiting"
    bool watermarkShimmer = true;     // Modern: бегущий блик по нижней кромке
    int watermarkStyle = 0;
    bool watermarkShowCustomText = true;
    char watermarkCustomText[64] = "";   // своё имя пользователь вводит во вкладке Interface
    bool watermarkShowTime = true;
    bool watermarkShowFPS = true;
    float watermarkBgAlpha = 0.88f;
    float watermarkOffsetX = 18.0f;
    float watermarkOffsetY = 16.0f;

    // --- тема меню
    int menuDesign = 1;               // 0 = Classic (исходный дизайн), 1 = Modern (стекло как у вотермарки)
    int menuPreset = 0;               // последний применённый пресет (только для подписи в комбо)
    float menuAccent[4] = { 0.30f, 0.49f, 1.00f, 1.00f };
    float menuBg[4] = { 0.043f, 0.051f, 0.075f, 1.00f };
    float menuPanel[4] = { 0.065f, 0.075f, 0.105f, 1.00f };
    float menuFrame[4] = { 0.085f, 0.098f, 0.135f, 1.00f };
    float menuBorder[4] = { 0.150f, 0.180f, 0.250f, 1.00f };
    float menuOpacity = 0.96f;
    float menuRounding = 10.0f;
    bool menuShimmer = true;
    bool menuGlow = true;
    bool watermarkFollowMenu = true;  // Modern-вотермарка берёт фон/рамку из темы меню
    bool itemsAutoLoad = false;       // Items: сам подгружать рантайм Item Extravaganza при входе в игру
    bool weatherControlEnabled = false; // Misc > Weather: форсировать погоду (TimeWeather::CSystem)
    int weatherPreset = 4;              // EWeather: 0 Foggy, 1 Clear, 2 Overcast, 3 Cloudy, 4 Rainy, 5 Stormy, 6..9 снег/wildcard
    bool weatherUiVisible = false;      // вкладка погоды открыта (нужно читать состояние)
}

const char* GetKeyName(int vk) {
    static char buf[32];
    if (vk <= 0) return "None";
    switch (vk) {
    case VK_LBUTTON: return "Mouse1";
    case VK_RBUTTON: return "Mouse2";
    case VK_MBUTTON: return "Mouse3";
    case VK_XBUTTON1: return "Mouse4";
    case VK_XBUTTON2: return "Mouse5";
    case VK_SPACE: return "Space";
    case VK_TAB: return "Tab";
    case VK_CAPITAL: return "CapsLock";
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: return "Shift";
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return "Ctrl";
    case VK_MENU: case VK_LMENU: case VK_RMENU: return "Alt";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_PRIOR: return "PageUp";
    case VK_NEXT: return "PageDown";
    case VK_HOME: return "Home";
    case VK_DELETE: return "Delete";
    case VK_END: return "End";
    }
    if (vk >= 'A' && vk <= 'Z') { snprintf(buf, sizeof(buf), "%c", (char)vk); return buf; }
    if (vk >= '0' && vk <= '9') { snprintf(buf, sizeof(buf), "%c", (char)vk); return buf; }
    if (vk >= VK_F1 && vk <= VK_F12) { snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1); return buf; }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { snprintf(buf, sizeof(buf), "Num%d", vk - VK_NUMPAD0); return buf; }
    snprintf(buf, sizeof(buf), "Key %d", vk);
    return buf;
}

void DrawKeybindWidget(const char* idPrefix, int& key, int& mode, bool& waitingForKey, const char* mode0Name, const char* mode1Name) {
    ImGui::PushID(idPrefix);
    char btnLabel[64];
    if (waitingForKey) snprintf(btnLabel, sizeof(btnLabel), "Press Any Key...");
    else snprintf(btnLabel, sizeof(btnLabel), "Bind: %s", GetKeyName(key));

    float halfWidth = (ImGui::GetWindowWidth() - 28.0f) * 0.5f;
    if (ImGui::Button(btnLabel, ImVec2(halfWidth, 24))) waitingForKey = !waitingForKey;

    if (waitingForKey) {
        if (GetAsyncKeyState(VK_ESCAPE) & 1) { key = 0; waitingForKey = false; }
        else {
            for (int vk = 3; vk < 255; ++vk) {
                if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_INSERT) continue;
                if (GetAsyncKeyState(vk) & 0x8000) { key = vk; waitingForKey = false; break; }
            }
        }
    }
    ImGui::SameLine();
    const char* currentModeText = (mode == 0) ? mode0Name : mode1Name;
    if (ImGui::Button(currentModeText, ImVec2(halfWidth, 24))) mode = (mode == 0) ? 1 : 0;
    ImGui::PopID();
}

// ==================== ОБЛАСТЬ ИГРЫ НА ЭКРАНЕ ====================
// Оверлей растянут на весь монитор, а игра может быть в окне / с другим разрешением.
// Все эффекты (центр прицела, проекция 3D -> 2D, уведомления) считаются от клиентской области окна игры.
struct GameViewRect {
    float x = 0.0f, y = 0.0f, w = 1920.0f, h = 1080.0f;
    ImVec2 Center() const { return ImVec2(x + w * 0.5f, y + h * 0.5f); }
};
GameViewRect g_GameView;
ImFont* g_FontBold = nullptr;

void UpdateGameViewRect(HWND gameWnd, int screenW, int screenH) {
    g_GameView.x = 0.0f; g_GameView.y = 0.0f; g_GameView.w = (float)screenW; g_GameView.h = (float)screenH;
    if (!gameWnd || !IsWindow(gameWnd)) return;
    RECT rc{};
    if (!GetClientRect(gameWnd, &rc)) return;
    POINT tl = { rc.left, rc.top };
    if (!ClientToScreen(gameWnd, &tl)) return;
    float w = (float)(rc.right - rc.left), h = (float)(rc.bottom - rc.top);
    if (w < 64.0f || h < 64.0f) return;
    g_GameView.x = (float)tl.x; g_GameView.y = (float)tl.y; g_GameView.w = w; g_GameView.h = h;
}

// ==================== СИСТЕМА ВСПЛЫВАЮЩИХ УВЕДОМЛЕНИЙ ====================
// Карточки в правом верхнем углу (под вотермаркой): иконка, заголовок, описание, таймер-полоска.
// Выезжают справа, плавно сдвигаются в стеке, уезжают обратно вправо.
namespace TopNotificationSystem {
    struct Toast {
        std::string title;
        std::string message;
        std::string icon;
        ImVec4 accentColor;
        float elapsed = 0.0f;
        float duration = 3.2f;
        float curY = -1.0f;   // анимированная позиция в стеке
    };

    std::vector<Toast> toasts;

    void Push(const char* title, const char* msg, ImVec4 color = ImVec4(0.25f, 0.88f, 0.52f, 1.0f), float duration = 3.2f, const char* icon = ICON_FA_INFO_CIRCLE) {
        Toast t;
        t.title = title ? title : "";
        t.message = msg ? msg : "";
        t.icon = icon ? icon : "";
        t.accentColor = color;
        t.elapsed = 0.0f;
        t.duration = (duration < 1.0f) ? 1.0f : duration;
        toasts.push_back(t);
        if (toasts.size() > 5) toasts.erase(toasts.begin());
    }

    static float EaseOutBack(float t) {
        const float c1 = 1.70158f, c3 = c1 + 1.0f;
        float u = t - 1.0f;
        return 1.0f + c3 * u * u * u + c1 * u * u;
    }

    static ImU32 Col(const ImVec4& c, float a) { return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * a)); }

    void Render(ImDrawList* draw, float dt, int screenWidth) {
        (void)screenWidth;
        if (dt <= 0.0f || dt > 0.25f) dt = 0.016f;
        if (toasts.empty()) return;

        ImFont* titleFont = g_FontBold ? g_FontBold : ImGui::GetFont();
        ImFont* bodyFont = ImGui::GetFont();
        float titleFs = titleFont->FontSize * ImGui::GetIO().FontGlobalScale;
        float bodyFs = bodyFont->FontSize * ImGui::GetIO().FontGlobalScale;

        const float marginR = 18.0f;
        float stackTop = g_GameView.y + 18.0f;
        if (MenuVars::watermarkEnabled && stackTop < MenuVars::watermarkOffsetY + 40.0f) stackTop = MenuVars::watermarkOffsetY + 40.0f;
        const float rightEdge = g_GameView.x + g_GameView.w - marginR;

        // Новые сверху: идём с конца, чтобы самая свежая карточка была первой в стеке.
        float targetY = stackTop;
        for (int idx = (int)toasts.size() - 1; idx >= 0; --idx) {
            Toast& t = toasts[idx];
            t.elapsed += dt;

            const float inDur = 0.38f, outDur = 0.30f;
            float inT = (t.elapsed < inDur) ? (t.elapsed / inDur) : 1.0f;
            float outT = (t.elapsed > t.duration - outDur) ? ((t.duration - t.elapsed) / outDur) : 1.0f;
            if (outT < 0.0f) outT = 0.0f;

            float slideIn = EaseOutBack(inT);
            float slideOut = 1.0f - outT * outT;               // 0 -> 1 при уходе
            float alpha = (inT < 1.0f ? (inT * inT * (3.0f - 2.0f * inT)) : 1.0f) * (outT * outT * (3.0f - 2.0f * outT));

            ImVec2 titleSz = titleFont->CalcTextSizeA(titleFs, FLT_MAX, 0.0f, t.title.c_str());
            ImVec2 msgSz = bodyFont->CalcTextSizeA(bodyFs, FLT_MAX, 0.0f, t.message.c_str());

            const float padX = 14.0f, padY = 11.0f, iconBox = 30.0f, gapIcon = 12.0f;
            float textW = (titleSz.x > msgSz.x) ? titleSz.x : msgSz.x;
            float boxW = padX + iconBox + gapIcon + textW + padX + 6.0f;
            if (boxW < 250.0f) boxW = 250.0f;
            float textH = titleSz.y + 3.0f + msgSz.y;
            float boxH = ((textH > iconBox) ? textH : iconBox) + padY * 2.0f + 3.0f;

            if (t.curY < 0.0f) t.curY = targetY;
            t.curY += (targetY - t.curY) * (1.0f - std::exp(-14.0f * dt)); // плавный сдвиг стека

            float hiddenOffset = boxW + marginR + 12.0f;
            float xOffset = (1.0f - slideIn) * hiddenOffset + slideOut * hiddenOffset;
            float xMax = rightEdge + xOffset;
            float xMin = xMax - boxW;
            float yMin = std::floor(t.curY), yMax = yMin + boxH;
            xMin = std::floor(xMin); xMax = std::floor(xMax);

            // Мягкая тень
            for (int s = 1; s <= 4; ++s) {
                float e = (float)s * 2.0f;
                draw->AddRectFilled(ImVec2(xMin - e, yMin - e + 3.0f), ImVec2(xMax + e, yMax + e + 3.0f), IM_COL32(0, 0, 0, (int)(alpha * (34 - s * 7))), 8.0f + e);
            }

            // Фон и рамка
            draw->AddRectFilled(ImVec2(xMin, yMin), ImVec2(xMax, yMax), IM_COL32(11, 14, 21, (int)(alpha * 246.0f)), 7.0f);
            draw->AddRectFilledMultiColor(ImVec2(xMin + 3.0f, yMin + 1.0f), ImVec2(xMin + boxW * 0.55f, yMax - 1.0f),
                Col(t.accentColor, alpha * 0.10f), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), Col(t.accentColor, alpha * 0.10f));
            draw->AddRect(ImVec2(xMin, yMin), ImVec2(xMax, yMax), IM_COL32(40, 50, 70, (int)(alpha * 255.0f)), 7.0f, 0, 1.0f);

            // Акцентная полоса слева
            draw->AddRectFilled(ImVec2(xMin, yMin), ImVec2(xMin + 3.0f, yMax), Col(t.accentColor, alpha), 7.0f, ImDrawFlags_RoundCornersLeft);

            // Иконка в круглом бейдже
            ImVec2 iconC(xMin + padX + iconBox * 0.5f, yMin + (boxH - 3.0f) * 0.5f);
            float pulse = (t.elapsed < 0.6f) ? (1.0f - t.elapsed / 0.6f) : 0.0f;
            if (pulse > 0.0f)
                draw->AddCircle(iconC, iconBox * 0.5f + 7.0f * (1.0f - pulse), Col(t.accentColor, alpha * pulse * 0.8f), 32, 1.5f);
            draw->AddCircleFilled(iconC, iconBox * 0.5f, Col(t.accentColor, alpha * 0.16f), 32);
            draw->AddCircle(iconC, iconBox * 0.5f, Col(t.accentColor, alpha * 0.55f), 32, 1.2f);
            if (!t.icon.empty()) {
                ImVec2 iSz = titleFont->CalcTextSizeA(titleFs, FLT_MAX, 0.0f, t.icon.c_str());
                draw->AddText(titleFont, titleFs, ImVec2(std::floor(iconC.x - iSz.x * 0.5f), std::floor(iconC.y - iSz.y * 0.5f)), Col(t.accentColor, alpha), t.icon.c_str());
            }

            // Текст
            float tx = xMin + padX + iconBox + gapIcon;
            float ty = std::floor(yMin + (boxH - 3.0f - textH) * 0.5f);
            draw->AddText(titleFont, titleFs, ImVec2(tx, ty), IM_COL32(240, 244, 250, (int)(alpha * 255.0f)), t.title.c_str());
            draw->AddText(bodyFont, bodyFs, ImVec2(tx, ty + titleSz.y + 3.0f), IM_COL32(150, 162, 184, (int)(alpha * 255.0f)), t.message.c_str());

            // Таймер: тонкая полоса снизу, убывает справа налево
            float life = 1.0f - (t.elapsed / t.duration);
            if (life < 0.0f) life = 0.0f;
            float barL = xMin + 8.0f, barR = xMax - 8.0f;
            draw->AddRectFilled(ImVec2(barL, yMax - 4.0f), ImVec2(barR, yMax - 2.0f), IM_COL32(255, 255, 255, (int)(alpha * 14.0f)), 1.0f);
            draw->AddRectFilled(ImVec2(barL, yMax - 4.0f), ImVec2(barL + (barR - barL) * life, yMax - 2.0f), Col(t.accentColor, alpha * 0.9f), 1.0f);

            // Следующая карточка ниже; уходящая карточка постепенно освобождает место
            targetY += (boxH + 8.0f) * outT;
        }

        for (size_t i = 0; i < toasts.size(); ) {
            if (toasts[i].elapsed >= toasts[i].duration) toasts.erase(toasts.begin() + i);
            else ++i;
        }
    }
}

// ==================== ХИТМАРКЕРЫ / КИЛЛ-ЭФФЕКТЫ / 8-BIT ЗВУК ====================
#include "fx_system.hpp"

namespace ConfigSystem {
    std::vector<std::string> configFiles;
    int selectedConfig = 0;
    char newConfigName[64] = "default";
    char renameConfigBuf[64] = "default";
    char statusText[64] = "Ready";

    static bool DirWritable(const std::string& dir) {
        CreateDirectoryA(dir.c_str(), nullptr);
        std::string probe = dir + "\\.write_test";
        HANDLE h = CreateFileA(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        CloseHandle(h);
        return true;
    }

    // Портативно: configs рядом с DLTB.RIP.exe. Если папка не доступна на запись
    // (например, распаковали в Program Files) — %LOCALAPPDATA%\DLTB.RIP\configs.
    std::string GetConfigDir() {
        static std::string cached;
        if (!cached.empty()) return cached;
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string exeDir = exePath;
        size_t slash = exeDir.find_last_of("\\/");
        exeDir = (slash == std::string::npos) ? "." : exeDir.substr(0, slash);
        if (DirWritable(exeDir + "\\configs")) return cached = exeDir + "\\configs";
        char* local = nullptr; size_t len = 0;
        if (_dupenv_s(&local, &len, "LOCALAPPDATA") == 0 && local) {
            std::string base = std::string(local) + "\\DLTB.RIP";
            free(local);
            CreateDirectoryA(base.c_str(), nullptr);
            if (DirWritable(base + "\\configs")) return cached = base + "\\configs";
        }
        CreateDirectoryA("configs", nullptr);
        return cached = "configs";
    }

    void MigrateOldConfigs(const std::string& targetDir) {
        // прежние места: папка на рабочем столе (старые сборки) и папки разработки
        std::string desktopDir;
        char* userProfile = nullptr; size_t len = 0;
        if (_dupenv_s(&userProfile, &len, "USERPROFILE") == 0 && userProfile) {
            desktopDir = std::string(userProfile) + "\\Desktop\\DLTB.RIP\\configs";
            free(userProfile);
        }
        const char* oldDirs[] = { desktopDir.c_str(), "configs", "..\\execute\\configs", "..\\..\\execute\\configs" };
        for (const char* oldDir : oldDirs) {
            if (!oldDir[0] || !_stricmp(oldDir, targetDir.c_str())) continue;
            WIN32_FIND_DATAA findData;
            std::string mask = std::string(oldDir) + "\\*.cfg";
            HANDLE hFind = FindFirstFileA(mask.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::string src = std::string(oldDir) + "\\" + findData.cFileName;
                        std::string dst = targetDir + "\\" + findData.cFileName;
                        CopyFileA(src.c_str(), dst.c_str(), TRUE);
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        }
    }

    void RefreshConfigs() {
        std::string dir = GetConfigDir();
        MigrateOldConfigs(dir);
        configFiles.clear();
        WIN32_FIND_DATAA findData;
        std::string searchMask = dir + "\\*.cfg";
        HANDLE hFind = FindFirstFileA(searchMask.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::string fileName = findData.cFileName;
                    if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".cfg")
                        fileName = fileName.substr(0, fileName.size() - 4);
                    configFiles.push_back(fileName);
                }
            } while (FindNextFileA(hFind, &findData));
            FindClose(hFind);
        }
        if (configFiles.empty()) configFiles.push_back("default");
        if (selectedConfig >= (int)configFiles.size()) selectedConfig = (int)configFiles.size() - 1;
        if (selectedConfig < 0) selectedConfig = 0;
        snprintf(renameConfigBuf, sizeof(renameConfigBuf), "%s", configFiles[selectedConfig].c_str());
    }

    bool Save(const std::string& name) {
        if (name.empty()) return false;
        std::string path = GetConfigDir() + "\\" + name + ".cfg";
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) return false;

#define SAVE_VAR(var) out << #var << "=" << MenuVars::var << "\n"
        SAVE_VAR(godMode); SAVE_VAR(infStamina); SAVE_VAR(infUVFlashlight);
        SAVE_VAR(infFury); SAVE_VAR(noFallDamage); SAVE_VAR(noKnockdown);
        SAVE_VAR(moveSpeedEnabled); SAVE_VAR(speedMultiplier);
        SAVE_VAR(jumpHeightEnabled); SAVE_VAR(jumpMultiplier);
        SAVE_VAR(unlimitedJumps); SAVE_VAR(tripleJump);
        SAVE_VAR(alwaysVaultKick); SAVE_VAR(floatInAir);
        SAVE_VAR(floatInAirKey); SAVE_VAR(floatInAirBindMode);
        SAVE_VAR(tpWaypointKey); SAVE_VAR(tpWaypointBindMode);
        SAVE_VAR(unloadKey); SAVE_VAR(unloadBindMode);
        SAVE_VAR(unlimitedCableLength); SAVE_VAR(disableHookPullDown);
        SAVE_VAR(swimSpeedEnabled); SAVE_VAR(swimSpeedMultiplier);
        SAVE_VAR(disableWaterPhysics); SAVE_VAR(disablePlayableArea);
        SAVE_VAR(nightExpEnabled); SAVE_VAR(nightExpMultiplier);
        SAVE_VAR(openWorldExpEnabled); SAVE_VAR(openWorldExpMultiplier);
        SAVE_VAR(disableDeathPenalty);
        SAVE_VAR(powerAttackBlocking); SAVE_VAR(alwaysPerfectBlock);
        SAVE_VAR(autoBlock); SAVE_VAR(infDurability); SAVE_VAR(oneHitKill);
        SAVE_VAR(easyFinisher); SAVE_VAR(instantFinisher);
        SAVE_VAR(alwaysElemental); SAVE_VAR(damageMultEnabled); SAVE_VAR(damageMultiplier);
        SAVE_VAR(infAmmo); SAVE_VAR(noReload); SAVE_VAR(noRecoil);
        SAVE_VAR(megaAccuracy); SAVE_VAR(silencedFirearms); SAVE_VAR(bowRapidFire);
        SAVE_VAR(rapidFire); SAVE_VAR(rapidFireAdsOnly); SAVE_VAR(rapidFireDelayMs);

        SAVE_VAR(hitCfg.enabled); SAVE_VAR(hitCfg.style); SAVE_VAR(hitCfg.positionMode);
        SAVE_VAR(hitCfg.size); SAVE_VAR(hitCfg.thickness); SAVE_VAR(hitCfg.duration); SAVE_VAR(hitCfg.glow);
        SAVE_VAR(hitCfg.color[0]); SAVE_VAR(hitCfg.color[1]); SAVE_VAR(hitCfg.color[2]); SAVE_VAR(hitCfg.color[3]);
        SAVE_VAR(hitCfg.killColor[0]); SAVE_VAR(hitCfg.killColor[1]); SAVE_VAR(hitCfg.killColor[2]); SAVE_VAR(hitCfg.killColor[3]);
        SAVE_VAR(hitCfg.damageNumbers);
        SAVE_VAR(hitCfg.damageColor[0]); SAVE_VAR(hitCfg.damageColor[1]); SAVE_VAR(hitCfg.damageColor[2]); SAVE_VAR(hitCfg.damageColor[3]);
        SAVE_VAR(hitCfg.sound);
        SAVE_VAR(killCfg.enabled); SAVE_VAR(killCfg.bloodBurst); SAVE_VAR(killCfg.goreChunks); SAVE_VAR(killCfg.bloodPool);
        SAVE_VAR(killCfg.bloodMist); SAVE_VAR(killCfg.uvBurn); SAVE_VAR(killCfg.screenSplatter);
        SAVE_VAR(killCfg.confetti); SAVE_VAR(killCfg.rainbowRing); SAVE_VAR(killCfg.rainbowRays); SAVE_VAR(killCfg.rainbowSparkles);
        SAVE_VAR(killCfg.crystalShatter); SAVE_VAR(killCfg.crystalSpikes); SAVE_VAR(killCfg.rainbowCrystals);
        SAVE_VAR(killCfg.crystalColor[0]); SAVE_VAR(killCfg.crystalColor[1]); SAVE_VAR(killCfg.crystalColor[2]); SAVE_VAR(killCfg.crystalColor[3]);
        SAVE_VAR(killCfg.rainbowSpeed); SAVE_VAR(killCfg.confettiCount);
        SAVE_VAR(killCfg.fireworks); SAVE_VAR(killCfg.vortex); SAVE_VAR(killCfg.ghostRise); SAVE_VAR(killCfg.frostNova);
        SAVE_VAR(killCfg.killLabel); SAVE_VAR(killCfg.streakAnnouncer);
        SAVE_VAR(killCfg.primary[0]); SAVE_VAR(killCfg.primary[1]); SAVE_VAR(killCfg.primary[2]); SAVE_VAR(killCfg.primary[3]);
        SAVE_VAR(killCfg.secondary[0]); SAVE_VAR(killCfg.secondary[1]); SAVE_VAR(killCfg.secondary[2]); SAVE_VAR(killCfg.secondary[3]);
        SAVE_VAR(killCfg.scale); SAVE_VAR(killCfg.duration); SAVE_VAR(killCfg.particleCount);
        SAVE_VAR(killCfg.sound); SAVE_VAR(killCfg.pitchStreak);
        out << "killCfg.labelText=" << MenuVars::killCfg.labelText << "\n";
        SAVE_VAR(sfxVolume);
        out << "hitPosSourceKey=" << MenuVars::hitPosSourceKey << "\n";
        SAVE_VAR(hitProjFov); SAVE_VAR(hitHeightOffset); SAVE_VAR(hitEyeHeight);
        SAVE_VAR(hitInvertPitch); SAVE_VAR(hitDistanceScale);

        SAVE_VAR(freeCraft); SAVE_VAR(multiplyGatheredEnabled); SAVE_VAR(gatheredMultiplier); SAVE_VAR(moneyToAdd);
        SAVE_VAR(freezeTime); SAVE_VAR(forceDaytimeEnabled); SAVE_VAR(worldTime);
        SAVE_VAR(forceDayPhaseEnabled); SAVE_VAR(dayPhaseIndex);
        SAVE_VAR(worldSpeedEnabled); SAVE_VAR(worldSpeedValue);
        SAVE_VAR(disableWorldNoise); SAVE_VAR(removeFogOfWar);
        SAVE_VAR(editZoneLevelEnabled); SAVE_VAR(zoneMinLevel); SAVE_VAR(zoneMaxLevel);
        SAVE_VAR(openLockedDoors); SAVE_VAR(easyLockpick);
        SAVE_VAR(changeAIScaleEnabled); SAVE_VAR(aiScaleValue);
        SAVE_VAR(aiInvisibility); SAVE_VAR(disableZombieGrab);
        SAVE_VAR(disableChaseLogic); SAVE_VAR(alwaysStomp);
        SAVE_VAR(freezeKillNearbyAI); SAVE_VAR(burnNearbyAI); SAVE_VAR(disableAISpawn);
        SAVE_VAR(noVehicleDamage); SAVE_VAR(noFuelUsage); SAVE_VAR(noRepairKitNeed);
        SAVE_VAR(unlimitedVehicleUV); SAVE_VAR(instantVehicleUV);
        SAVE_VAR(removeCarCamSnap); SAVE_VAR(challengeModeVehicle);
        SAVE_VAR(noMoneyCost); SAVE_VAR(multiplySellPrice); SAVE_VAR(sellPriceMultiplier);
        SAVE_VAR(multiplyBuyPrice); SAVE_VAR(buyPriceMultiplier);
        SAVE_VAR(toggleHUD); SAVE_VAR(weaponReturnThrown); SAVE_VAR(instantDismantle);
        SAVE_VAR(disableActionEquipUse); SAVE_VAR(rightHandLoot); SAVE_VAR(disableHeadbob);
        SAVE_VAR(fontWeightPreset); SAVE_VAR(textBrightness);
        SAVE_VAR(textMutedBrightness); SAVE_VAR(fontScale);
        SAVE_VAR(watermarkEnabled); SAVE_VAR(watermarkStyle);
        SAVE_VAR(watermarkDesign); SAVE_VAR(watermarkShowStatus); SAVE_VAR(watermarkShimmer);
        SAVE_VAR(watermarkShowCustomText);
        out << "watermarkCustomText=" << MenuVars::watermarkCustomText << "\n";
        SAVE_VAR(watermarkShowTime); SAVE_VAR(watermarkShowFPS);
        SAVE_VAR(watermarkBgAlpha); SAVE_VAR(watermarkOffsetX); SAVE_VAR(watermarkOffsetY);
        SAVE_VAR(menuDesign); SAVE_VAR(menuPreset);
        SAVE_VAR(menuAccent[0]); SAVE_VAR(menuAccent[1]); SAVE_VAR(menuAccent[2]); SAVE_VAR(menuAccent[3]);
        SAVE_VAR(menuBg[0]); SAVE_VAR(menuBg[1]); SAVE_VAR(menuBg[2]); SAVE_VAR(menuBg[3]);
        SAVE_VAR(menuPanel[0]); SAVE_VAR(menuPanel[1]); SAVE_VAR(menuPanel[2]); SAVE_VAR(menuPanel[3]);
        SAVE_VAR(menuFrame[0]); SAVE_VAR(menuFrame[1]); SAVE_VAR(menuFrame[2]); SAVE_VAR(menuFrame[3]);
        SAVE_VAR(menuBorder[0]); SAVE_VAR(menuBorder[1]); SAVE_VAR(menuBorder[2]); SAVE_VAR(menuBorder[3]);
        SAVE_VAR(menuOpacity); SAVE_VAR(menuRounding); SAVE_VAR(menuShimmer); SAVE_VAR(menuGlow); SAVE_VAR(watermarkFollowMenu); SAVE_VAR(itemsAutoLoad);
        SAVE_VAR(weatherControlEnabled); SAVE_VAR(weatherPreset);
#undef SAVE_VAR
        out.close();
        snprintf(statusText, sizeof(statusText), "Saved: %s.cfg", name.c_str());
        RefreshConfigs();
        for (size_t i = 0; i < configFiles.size(); ++i) {
            if (configFiles[i] == name) {
                selectedConfig = (int)i;
                snprintf(renameConfigBuf, sizeof(renameConfigBuf), "%s", name.c_str());
                break;
            }
        }
        {
            std::string toastMsg = name + ".cfg  \xC2\xB7  saved to configs";
            TopNotificationSystem::Push("Config saved", toastMsg.c_str(), ImVec4(0.22f, 0.92f, 0.52f, 1.0f), 3.2f, ICON_FA_CHECK);
        }
        return true;
    }

    bool Load(const std::string& name) {
        if (name.empty()) return false;
        std::string path = GetConfigDir() + "\\" + name + ".cfg";
        std::ifstream in(path);
        if (!in.is_open()) {
            snprintf(statusText, sizeof(statusText), "File not found: %s.cfg", name.c_str());
            TopNotificationSystem::Push("Load failed", statusText, ImVec4(1.00f, 0.35f, 0.35f, 1.0f), 3.6f, ICON_FA_EXCLAMATION_TRIANGLE);
            return false;
        }
        std::string line;
        while (std::getline(in, line)) {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "watermarkCustomText") {
                snprintf(MenuVars::watermarkCustomText, sizeof(MenuVars::watermarkCustomText), "%s", val.c_str());
                continue;
            }
            if (key == "hitPosSourceKey") { try { MenuVars::hitPosSourceKey = std::stoull(val); } catch (...) {} continue; }
            if (key == "killCfg.labelText") {
                snprintf(MenuVars::killCfg.labelText, sizeof(MenuVars::killCfg.labelText), "%s", val.c_str());
                continue;
            }
            if (val.empty()) continue;

#define LOAD_BOOL(var)  if (key == #var) { MenuVars::var = (std::stoi(val) != 0); continue; }
#define LOAD_INT(var)   if (key == #var) { MenuVars::var = std::stoi(val); continue; }
#define LOAD_FLOAT(var) if (key == #var) { MenuVars::var = std::stof(val); continue; }
            LOAD_BOOL(godMode); LOAD_BOOL(infStamina); LOAD_BOOL(infUVFlashlight);
            LOAD_BOOL(infFury); LOAD_BOOL(noFallDamage); LOAD_BOOL(noKnockdown);
            LOAD_BOOL(moveSpeedEnabled); LOAD_FLOAT(speedMultiplier);
            LOAD_BOOL(jumpHeightEnabled); LOAD_FLOAT(jumpMultiplier);
            LOAD_BOOL(unlimitedJumps); LOAD_BOOL(tripleJump);
            LOAD_BOOL(alwaysVaultKick); LOAD_BOOL(floatInAir);
            LOAD_INT(floatInAirKey); LOAD_INT(floatInAirBindMode);
            LOAD_INT(tpWaypointKey); LOAD_INT(tpWaypointBindMode);
            LOAD_INT(unloadKey); LOAD_INT(unloadBindMode);
            LOAD_BOOL(unlimitedCableLength); LOAD_BOOL(disableHookPullDown);
            LOAD_BOOL(swimSpeedEnabled); LOAD_FLOAT(swimSpeedMultiplier);
            LOAD_BOOL(disableWaterPhysics); LOAD_BOOL(disablePlayableArea);
            LOAD_BOOL(nightExpEnabled); LOAD_FLOAT(nightExpMultiplier);
            LOAD_BOOL(openWorldExpEnabled); LOAD_FLOAT(openWorldExpMultiplier);
            LOAD_BOOL(disableDeathPenalty);
            LOAD_BOOL(powerAttackBlocking); LOAD_BOOL(alwaysPerfectBlock);
            LOAD_BOOL(autoBlock); LOAD_BOOL(infDurability); LOAD_BOOL(oneHitKill);
            LOAD_BOOL(easyFinisher); LOAD_BOOL(instantFinisher);
            LOAD_BOOL(alwaysElemental); LOAD_BOOL(damageMultEnabled); LOAD_FLOAT(damageMultiplier);
            LOAD_BOOL(infAmmo); LOAD_BOOL(noReload); LOAD_BOOL(noRecoil);
            LOAD_BOOL(megaAccuracy); LOAD_BOOL(silencedFirearms); LOAD_BOOL(bowRapidFire);
            LOAD_BOOL(rapidFire); LOAD_BOOL(rapidFireAdsOnly); LOAD_INT(rapidFireDelayMs);

            LOAD_BOOL(hitCfg.enabled); LOAD_INT(hitCfg.style); LOAD_INT(hitCfg.positionMode);
            LOAD_FLOAT(hitCfg.size); LOAD_FLOAT(hitCfg.thickness); LOAD_FLOAT(hitCfg.duration); LOAD_BOOL(hitCfg.glow);
            LOAD_FLOAT(hitCfg.color[0]); LOAD_FLOAT(hitCfg.color[1]); LOAD_FLOAT(hitCfg.color[2]); LOAD_FLOAT(hitCfg.color[3]);
            LOAD_FLOAT(hitCfg.killColor[0]); LOAD_FLOAT(hitCfg.killColor[1]); LOAD_FLOAT(hitCfg.killColor[2]); LOAD_FLOAT(hitCfg.killColor[3]);
            LOAD_BOOL(hitCfg.damageNumbers);
            LOAD_FLOAT(hitCfg.damageColor[0]); LOAD_FLOAT(hitCfg.damageColor[1]); LOAD_FLOAT(hitCfg.damageColor[2]); LOAD_FLOAT(hitCfg.damageColor[3]);
            LOAD_INT(hitCfg.sound);
            LOAD_BOOL(killCfg.enabled); LOAD_BOOL(killCfg.bloodBurst); LOAD_BOOL(killCfg.goreChunks); LOAD_BOOL(killCfg.bloodPool);
            LOAD_BOOL(killCfg.bloodMist); LOAD_BOOL(killCfg.uvBurn); LOAD_BOOL(killCfg.screenSplatter);
            LOAD_BOOL(killCfg.confetti); LOAD_BOOL(killCfg.rainbowRing); LOAD_BOOL(killCfg.rainbowRays); LOAD_BOOL(killCfg.rainbowSparkles);
            LOAD_BOOL(killCfg.crystalShatter); LOAD_BOOL(killCfg.crystalSpikes); LOAD_BOOL(killCfg.rainbowCrystals);
            LOAD_FLOAT(killCfg.crystalColor[0]); LOAD_FLOAT(killCfg.crystalColor[1]); LOAD_FLOAT(killCfg.crystalColor[2]); LOAD_FLOAT(killCfg.crystalColor[3]);
            LOAD_FLOAT(killCfg.rainbowSpeed); LOAD_INT(killCfg.confettiCount);
            LOAD_BOOL(killCfg.fireworks); LOAD_BOOL(killCfg.vortex); LOAD_BOOL(killCfg.ghostRise); LOAD_BOOL(killCfg.frostNova);
            LOAD_BOOL(killCfg.killLabel); LOAD_BOOL(killCfg.streakAnnouncer);
            LOAD_FLOAT(killCfg.primary[0]); LOAD_FLOAT(killCfg.primary[1]); LOAD_FLOAT(killCfg.primary[2]); LOAD_FLOAT(killCfg.primary[3]);
            LOAD_FLOAT(killCfg.secondary[0]); LOAD_FLOAT(killCfg.secondary[1]); LOAD_FLOAT(killCfg.secondary[2]); LOAD_FLOAT(killCfg.secondary[3]);
            LOAD_FLOAT(killCfg.scale); LOAD_FLOAT(killCfg.duration); LOAD_INT(killCfg.particleCount);
            LOAD_INT(killCfg.sound); LOAD_BOOL(killCfg.pitchStreak);
            LOAD_FLOAT(sfxVolume);

            LOAD_BOOL(freeCraft); LOAD_BOOL(multiplyGatheredEnabled); LOAD_INT(gatheredMultiplier); LOAD_INT(moneyToAdd);
            LOAD_BOOL(freezeTime); LOAD_BOOL(forceDaytimeEnabled); LOAD_FLOAT(worldTime);
            LOAD_BOOL(forceDayPhaseEnabled); LOAD_INT(dayPhaseIndex);
            LOAD_BOOL(worldSpeedEnabled); LOAD_FLOAT(worldSpeedValue);
            LOAD_BOOL(disableWorldNoise); LOAD_BOOL(removeFogOfWar);
            LOAD_BOOL(editZoneLevelEnabled); LOAD_INT(zoneMinLevel); LOAD_INT(zoneMaxLevel);
            LOAD_BOOL(openLockedDoors); LOAD_BOOL(easyLockpick);
            LOAD_BOOL(changeAIScaleEnabled); LOAD_FLOAT(aiScaleValue);
            LOAD_BOOL(aiInvisibility); LOAD_BOOL(disableZombieGrab);
            LOAD_BOOL(disableChaseLogic); LOAD_BOOL(alwaysStomp);
            LOAD_BOOL(freezeKillNearbyAI); LOAD_BOOL(burnNearbyAI); LOAD_BOOL(disableAISpawn);
            LOAD_BOOL(noVehicleDamage); LOAD_BOOL(noFuelUsage); LOAD_BOOL(noRepairKitNeed);
            LOAD_BOOL(unlimitedVehicleUV); LOAD_BOOL(instantVehicleUV);
            LOAD_BOOL(removeCarCamSnap); LOAD_BOOL(challengeModeVehicle);
            LOAD_BOOL(noMoneyCost); LOAD_BOOL(multiplySellPrice); LOAD_FLOAT(sellPriceMultiplier);
            LOAD_BOOL(multiplyBuyPrice); LOAD_FLOAT(buyPriceMultiplier);
            LOAD_BOOL(toggleHUD); LOAD_BOOL(weaponReturnThrown); LOAD_BOOL(instantDismantle);
            LOAD_BOOL(disableActionEquipUse); LOAD_BOOL(rightHandLoot); LOAD_BOOL(disableHeadbob);
            LOAD_INT(fontWeightPreset); LOAD_FLOAT(textBrightness);
            LOAD_FLOAT(textMutedBrightness); LOAD_FLOAT(fontScale);
            LOAD_BOOL(watermarkEnabled); LOAD_INT(watermarkStyle);
            LOAD_INT(watermarkDesign); LOAD_BOOL(watermarkShowStatus); LOAD_BOOL(watermarkShimmer);
            LOAD_BOOL(watermarkShowCustomText);
            LOAD_BOOL(watermarkShowTime); LOAD_BOOL(watermarkShowFPS);
            LOAD_FLOAT(watermarkBgAlpha); LOAD_FLOAT(watermarkOffsetX); LOAD_FLOAT(watermarkOffsetY);
            LOAD_INT(menuDesign); LOAD_INT(menuPreset);
            LOAD_FLOAT(menuAccent[0]); LOAD_FLOAT(menuAccent[1]); LOAD_FLOAT(menuAccent[2]); LOAD_FLOAT(menuAccent[3]);
            LOAD_FLOAT(menuBg[0]); LOAD_FLOAT(menuBg[1]); LOAD_FLOAT(menuBg[2]); LOAD_FLOAT(menuBg[3]);
            LOAD_FLOAT(menuPanel[0]); LOAD_FLOAT(menuPanel[1]); LOAD_FLOAT(menuPanel[2]); LOAD_FLOAT(menuPanel[3]);
            LOAD_FLOAT(menuFrame[0]); LOAD_FLOAT(menuFrame[1]); LOAD_FLOAT(menuFrame[2]); LOAD_FLOAT(menuFrame[3]);
            LOAD_FLOAT(menuBorder[0]); LOAD_FLOAT(menuBorder[1]); LOAD_FLOAT(menuBorder[2]); LOAD_FLOAT(menuBorder[3]);
            LOAD_FLOAT(menuOpacity); LOAD_FLOAT(menuRounding); LOAD_BOOL(menuShimmer); LOAD_BOOL(menuGlow); LOAD_BOOL(watermarkFollowMenu); LOAD_BOOL(itemsAutoLoad);
            LOAD_BOOL(weatherControlEnabled); LOAD_INT(weatherPreset);
            LOAD_FLOAT(hitProjFov); LOAD_FLOAT(hitHeightOffset); LOAD_FLOAT(hitEyeHeight);
            LOAD_BOOL(hitInvertPitch); LOAD_BOOL(hitDistanceScale);
#undef LOAD_BOOL
#undef LOAD_INT
#undef LOAD_FLOAT
        }
        if (MenuVars::rapidFireDelayMs < 25) MenuVars::rapidFireDelayMs = 65;
        snprintf(statusText, sizeof(statusText), "Loaded: %s.cfg", name.c_str());
        TopNotificationSystem::Push("Config loaded", (name + ".cfg  \xC2\xB7  all settings applied").c_str(), ImVec4(0.28f, 0.78f, 1.00f, 1.0f), 3.2f, ICON_FA_FOLDER_OPEN);
        return true;
    }

    bool Delete(const std::string& name) {
        if (name.empty()) return false;
        std::string path = GetConfigDir() + "\\" + name + ".cfg";
        if (DeleteFileA(path.c_str())) {
            snprintf(statusText, sizeof(statusText), "Deleted: %s.cfg", name.c_str());
            RefreshConfigs();
            TopNotificationSystem::Push("Config deleted", (name + ".cfg").c_str(), ImVec4(1.00f, 0.35f, 0.35f, 1.0f), 3.2f, ICON_FA_TRASH);
            return true;
        }
        snprintf(statusText, sizeof(statusText), "Delete failed!");
        return false;
    }
}
struct BytePatch { uintptr_t address = 0; std::vector<uint8_t> origBytes; std::vector<uint8_t> patchBytes; bool isEnabled = false; };
struct MultiBytePatch { std::vector<uintptr_t> addresses; std::vector<std::vector<uint8_t>> origBytesList; std::vector<uint8_t> patchBytes; bool isEnabled = false; };
struct CodeCavePatch { uintptr_t targetAddress = 0; uintptr_t caveAddress = 0; uintptr_t paramAddress = 0; std::vector<uint8_t> origBytes; std::vector<uint8_t> jmpBytes; bool isEnabled = false; };

namespace GameMemory {
    DWORD processId = 0; HANDLE hProcess = nullptr; HWND gameWindow = nullptr;
    uintptr_t moduleBase = 0; DWORD moduleSize = 0; uintptr_t engineBase = 0; DWORD engineSize = 0;
    char statusMessage[64] = "Waiting Game..."; bool patternsInitialized = false;
    bool wasEverConnected = false; bool shouldAutoExit = false;
    uintptr_t cavePool = 0; size_t caveOffset = 0; uintptr_t engineCavePool = 0; size_t engineCaveOffset = 0;
    int foundCount = 0; uintptr_t gameDIPtrAddr = 0;

    BytePatch patchGodMode1, patchGodMode2, patchStamina, patchUVFlashlight, patchNoFall1, patchNoFall2, patchKnockdown, patchHitReaction;
    BytePatch patchUnlimitedJumps, patchTripleJump, patchVaultKick, patchFloatInAir, patchWaterPhysics, patchPlayableArea1, patchPlayableArea2, patchIsOutOfMap, patchTPTime;
    BytePatch patchPowerAttackBlock, patchAlwaysPerfectBlock, patchAutoBlock, patchDurability, patchUnlimitedRepairs, patchOneHitKill, patchEasyFinisher;
    BytePatch patchInstantFinisher1, patchInstantFinisher2, patchInstantFinisher3, patchInstantFinisher4, patchElemental1, patchElemental2;
    BytePatch patchAmmo, patchArrows, patchSilenced1, patchSilenced2, patchDeathPenalty1, patchSkipIsFury, patchNoMoneyCost;
    MultiBytePatch patchFreeCraftAll;
    BytePatch patchFreezeTime, patchWorldNoise1, patchWorldNoise2, patchDiscoverMap1, patchDiscoverMap2, patchDiscoverMap3, patchAllowDoorOpening, patchAutoUnlock;
    BytePatch patchInvisibility1, patchDisableAIAwareness, patchDisableZombieGrab, patchDisableChase, patchAlwaysStomp, patchFreezeKillAI, patchBurnAI;
    BytePatch patchDisableAISpawn1, patchDisableAISpawn2, patchDisableAISpawn3, patchDisableAISpawn4, patchDisableAISpawn5;
    BytePatch patchNoFuelUsage, patchNoGallonUse, patchNoRepairKit, patchUnlimitedVehicleUV, patchRemoveCarCamSnapFlag, patchChallengeVehicle;
    BytePatch patchWeaponReturn, patchInstantDismantle, patchDisableActionEquip, patchRightHandLoot;

    CodeCavePatch caveNightExp, caveOpenWorldExp, caveDeathPenalty2, caveDamageMult, caveNoReload, caveNoRecoil, caveMegaAccuracy, caveBowRapidFire;
    CodeCavePatch caveMoveSpeed, caveJumpHeight, caveUnlimitedFury, caveChangeAIScale;
    CodeCavePatch caveAICollector, caveHealthHitDetector;
    uintptr_t playerHealthCallRet = 0;
    uintptr_t healthCavePage = 0;          // отдельная страница для пещеры хитов/убийств     // адрес возврата из PlayerHealthModule -> LifeHealth::Subtract (урон по игроку)
    CodeCavePatch caveFreeBlueprints, caveMultiplyGathered, caveUnlimitedCable, caveDisableHookNerf, caveSwimSpeed, caveLogicalPlayer, caveTeleportWaypoint;
    CodeCavePatch caveForceDayTime1, caveForceDayTime2, caveForceDayPhase, caveWorldSpeed, caveRemoveFogOfWar, caveEditZoneLevel, caveRemoveDoorLock, caveEasyLockpick;
    CodeCavePatch caveNoVehicleDamage, caveInstantUVRecharge, caveNoUVCooldown, caveRemoveCarCamSnap, caveSellPrice, caveBuyPrice, caveToggleHUD, caveDisableHeadbob;

    HWND FindMainWindow(DWORD pid) {
        struct HandleData { DWORD pid; HWND hwnd; } data = { pid, nullptr };
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* pData = reinterpret_cast<HandleData*>(lParam);
            DWORD winPid = 0; GetWindowThreadProcessId(hwnd, &winPid);
            if (winPid == pData->pid && GetWindow(hwnd, GW_OWNER) == nullptr && IsWindowVisible(hwnd)) { pData->hwnd = hwnd; return FALSE; }
            return TRUE;
            }, reinterpret_cast<LPARAM>(&data));
        return data.hwnd;
    }

    bool GetModuleInfo(DWORD pid, const wchar_t* moduleName, uintptr_t& outBase, DWORD& outSize) {
        outBase = 0; outSize = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W entry = { sizeof(entry) };
            if (Module32FirstW(snapshot, &entry)) {
                do {
                    if (_wcsicmp(entry.szModule, moduleName) == 0) { outBase = (uintptr_t)entry.modBaseAddr; outSize = entry.modBaseSize; break; }
                } while (Module32NextW(snapshot, &entry));
            }
            CloseHandle(snapshot);
        }
        return (outBase != 0);
    }

    DWORD FindGameProcess(const wchar_t* targetExe, const wchar_t* targetModule, uintptr_t& outModuleBase, DWORD& outModuleSize) {
        DWORD foundPid = 0; outModuleBase = 0; outModuleSize = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W entry = { sizeof(entry) };
            if (Process32FirstW(snapshot, &entry)) {
                do {
                    if (entry.th32ProcessID <= 4) continue;
                    std::wstring exeLower = entry.szExeFile;
                    for (wchar_t& c : exeLower) c = (wchar_t)towlower(c);
                    bool nameMatch = (_wcsicmp(entry.szExeFile, targetExe) == 0) ||
                        (exeLower.find(L"dyinglight") != std::wstring::npos) ||
                        (exeLower.find(L"thebeast") != std::wstring::npos) ||
                        (exeLower.find(L"_rwdi") != std::wstring::npos);
                    if (nameMatch) {
                        uintptr_t modBase = 0; DWORD modSize = 0;
                        if (GetModuleInfo(entry.th32ProcessID, targetModule, modBase, modSize)) {
                            foundPid = entry.th32ProcessID; outModuleBase = modBase; outModuleSize = modSize; break;
                        }
                    }
                } while (Process32NextW(snapshot, &entry));
            }
            CloseHandle(snapshot);
        }
        return foundPid;
    }

    template <typename T> T Read(uintptr_t address) {
        T value{};
        if (hProcess && address != 0) ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), nullptr);
        return value;
    }

    template <typename T> void Write(uintptr_t address, T value) {
        if (hProcess && address != 0) WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), nullptr);
    }

    void WriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes) {
        if (!hProcess || address == 0 || bytes.empty()) return;
        DWORD oldProtect = 0;
        VirtualProtectEx(hProcess, (LPVOID)address, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
        WriteProcessMemory(hProcess, (LPVOID)address, bytes.data(), bytes.size(), nullptr);
        VirtualProtectEx(hProcess, (LPVOID)address, bytes.size(), oldProtect, &oldProtect);
    }

    void SetPatchState(BytePatch& patch, bool enable) {
        if (patch.address == 0 || patch.isEnabled == enable) return;
        WriteBytes(patch.address, enable ? patch.patchBytes : patch.origBytes);
        patch.isEnabled = enable;
    }

    void SetMultiPatchState(MultiBytePatch& patch, bool enable) {
        if (patch.addresses.empty() || patch.isEnabled == enable) return;
        for (size_t i = 0; i < patch.addresses.size(); ++i)
            WriteBytes(patch.addresses[i], enable ? patch.patchBytes : patch.origBytesList[i]);
        patch.isEnabled = enable;
    }

    void SetCaveState(CodeCavePatch& cave, bool enable) {
        if (cave.targetAddress == 0 || cave.caveAddress == 0 || cave.isEnabled == enable) return;
        WriteBytes(cave.targetAddress, enable ? cave.jmpBytes : cave.origBytes);
        cave.isEnabled = enable;
    }

    uintptr_t AllocateCavePoolNearModule(uintptr_t baseAddr, size_t size = 0x10000) {
        for (uintptr_t off = 0x10000; off < 0x60000000; off += 0x10000) {
            if (baseAddr > off) {
                void* p = VirtualAllocEx(hProcess, (LPVOID)(baseAddr - off), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                if (p) return (uintptr_t)p;
            }
            void* p = VirtualAllocEx(hProcess, (LPVOID)(baseAddr + off), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (p) return (uintptr_t)p;
        }
        return 0;
    }

    bool ReadModuleBuffer(uintptr_t baseAddr, DWORD modSize, std::vector<uint8_t>& outBuffer) {
        if (!hProcess || baseAddr == 0 || modSize == 0) return false;
        outBuffer.assign(modSize, 0);
        uintptr_t curr = baseAddr, end = baseAddr + modSize; size_t totalRead = 0;
        while (curr < end) {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQueryEx(hProcess, (LPCVOID)curr, &mbi, sizeof(mbi))) { curr += 0x1000; continue; }
            uintptr_t regionBase = (uintptr_t)mbi.BaseAddress;
            uintptr_t next = regionBase + mbi.RegionSize;
            if (next <= curr) next = curr + 0x1000;
            bool isExecutable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
            if (mbi.State == MEM_COMMIT && isExecutable && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
                uintptr_t readStart = (curr > regionBase) ? curr : regionBase;
                uintptr_t readEnd = (next < end) ? next : end;
                if (readEnd > readStart) {
                    SIZE_T br = 0;
                    if (ReadProcessMemory(hProcess, (LPCVOID)readStart, outBuffer.data() + (readStart - baseAddr), readEnd - readStart, &br))
                        totalRead += br;
                }
            }
            curr = next;
        }
        return (totalRead > 0);
    }

    uintptr_t FindPatternInBuffer(const std::vector<uint8_t>& buffer, uintptr_t baseAddr, const char* signature) {
        std::vector<int> pattern; std::istringstream iss(signature); std::string token;
        while (iss >> token) {
            if (token == "?" || token == "??") pattern.push_back(-1);
            else pattern.push_back((int)std::strtoul(token.c_str(), nullptr, 16));
        }
        size_t patSize = pattern.size();
        if (patSize == 0 || buffer.size() < patSize) return 0;
        const uint8_t* data = buffer.data(); size_t maxScan = buffer.size() - patSize;
        for (size_t i = 0; i <= maxScan; ++i) {
            bool found = true;
            for (size_t j = 0; j < patSize; ++j) {
                if (pattern[j] != -1 && data[i + j] != (uint8_t)pattern[j]) { found = false; break; }
            }
            if (found) return baseAddr + i;
        }
        return 0;
    }

    void SetupMultiPatchFromBuffer(const std::vector<uint8_t>& buffer, uintptr_t baseAddr, MultiBytePatch& patch, const char* signature, const std::vector<uint8_t>& newBytes) {
        patch = MultiBytePatch{}; patch.patchBytes = newBytes;
        std::vector<int> pattern; std::istringstream iss(signature); std::string token;
        while (iss >> token) {
            if (token == "?" || token == "??") pattern.push_back(-1);
            else pattern.push_back((int)std::strtoul(token.c_str(), nullptr, 16));
        }
        size_t patSize = pattern.size();
        if (patSize == 0 || buffer.size() < patSize) return;
        const uint8_t* data = buffer.data(); size_t maxScan = buffer.size() - patSize;
        for (size_t i = 0; i <= maxScan; ++i) {
            bool found = true;
            for (size_t j = 0; j < patSize; ++j) {
                if (pattern[j] != -1 && data[i + j] != (uint8_t)pattern[j]) { found = false; break; }
            }
            if (found && i + newBytes.size() <= buffer.size()) {
                patch.addresses.push_back(baseAddr + i);
                patch.origBytesList.emplace_back(buffer.begin() + i, buffer.begin() + i + newBytes.size());
            }
        }
        if (!patch.addresses.empty()) foundCount++;
    }

    void SetupPatchFromBuffer(const std::vector<uint8_t>& buffer, uintptr_t baseAddr, BytePatch& patch, const char* sig, int offset, const std::vector<uint8_t>& newBytes) {
        patch = BytePatch{};
        uintptr_t found = FindPatternInBuffer(buffer, baseAddr, sig);
        if (found != 0) {
            patch.address = found + offset; patch.patchBytes = newBytes;
            size_t rva = (patch.address - baseAddr);
            if (rva + newBytes.size() <= buffer.size()) {
                patch.origBytes.assign(buffer.begin() + rva, buffer.begin() + rva + newBytes.size());
                foundCount++;
            }
        }
    }

    bool SetupCaveEx(const std::vector<uint8_t>& buffer, uintptr_t baseAddr, uintptr_t poolBase, size_t& poolOffset, CodeCavePatch& cave, uintptr_t targetAddr, size_t patchLen, std::vector<uint8_t> shellcode, float initParamValue = 0.0f) {
        cave = CodeCavePatch{};
        if (targetAddr == 0 || poolBase == 0 || patchLen < 5) return false;
        size_t rva = targetAddr - baseAddr;
        if (rva + patchLen > buffer.size()) return false;

        cave.targetAddress = targetAddr;
        cave.caveAddress = poolBase + poolOffset;
        cave.paramAddress = cave.caveAddress + 0x80;
        poolOffset += 0x400;

        cave.origBytes.assign(buffer.begin() + rva, buffer.begin() + rva + patchLen);

        uintptr_t returnAddr = targetAddr + patchLen;
        uintptr_t jmpFrom = cave.caveAddress + shellcode.size();
        int32_t retRel = (int32_t)((int64_t)returnAddr - (int64_t)(jmpFrom + 5));
        shellcode.push_back(0xE9);
        shellcode.push_back((uint8_t)(retRel & 0xFF));
        shellcode.push_back((uint8_t)((retRel >> 8) & 0xFF));
        shellcode.push_back((uint8_t)((retRel >> 16) & 0xFF));
        shellcode.push_back((uint8_t)((retRel >> 24) & 0xFF));

        WriteBytes(cave.caveAddress, shellcode);
        Write<float>(cave.paramAddress, initParamValue);

        int32_t toCaveRel = (int32_t)((int64_t)cave.caveAddress - (int64_t)(targetAddr + 5));
        cave.jmpBytes.resize(patchLen, 0x90);
        cave.jmpBytes[0] = 0xE9;
        cave.jmpBytes[1] = (uint8_t)(toCaveRel & 0xFF);
        cave.jmpBytes[2] = (uint8_t)((toCaveRel >> 8) & 0xFF);
        cave.jmpBytes[3] = (uint8_t)((toCaveRel >> 16) & 0xFF);
        cave.jmpBytes[4] = (uint8_t)((toCaveRel >> 24) & 0xFF);

        foundCount++;
        return true;
    }

    bool SetupCaveCommon(const std::vector<uint8_t>& buffer, uintptr_t baseAddr, CodeCavePatch& cave, uintptr_t targetAddr, size_t patchLen, std::vector<uint8_t> shellcode, float initParamValue = 0.0f) {
        return SetupCaveEx(buffer, baseAddr, cavePool, caveOffset, cave, targetAddr, patchLen, shellcode, initParamValue);
    }

    void InitSignatures() {
        if (patternsInitialized || !hProcess || moduleBase == 0 || moduleSize == 0) return;
        std::vector<uint8_t> buffer;
        if (!ReadModuleBuffer(moduleBase, moduleSize, buffer)) return;

        foundCount = 0;
        if (cavePool == 0) { cavePool = AllocateCavePoolNearModule(moduleBase, 0x30000); caveOffset = 0; }

        SetupPatchFromBuffer(buffer, moduleBase, patchGodMode1, "40 53 48 83 EC ? 80 B9 ? ? ? ? ? 48 8B D9 74 ? E8 ? ? ? ? 48 85 C0 74 ? E8", 0, { 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchGodMode2, "80 B8 ?? ?? ?? ?? ?? 75 ?? 48 8B 03 48 8B CB FF 50", 6, { 0x02 });
        SetupPatchFromBuffer(buffer, moduleBase, patchStamina, "40 53 48 83 EC ?? 48 8B D9 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 80 B8", 0, { 0x30, 0xC0, 0xC3, 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchUVFlashlight, "F3 0F 5C C8 0F 2F CE F3 0F 11 8E ?? ?? ?? ?? 77 ?? C7 86", 0, { 0x90, 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchNoFall1, "40 55 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 44 0F 29 9C 24", 0, { 0x30, 0xC0, 0xC3 });
        SetupPatchFromBuffer(buffer, moduleBase, patchNoFall2, "FF 90 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? F3 0F 10 B3", 0, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchKnockdown, "0F 85 ?? ?? ?? ?? 48 89 B4 24 ?? ?? ?? ?? 44 0F 29 44 24", 0, { 0x90, 0xE9 });
        SetupPatchFromBuffer(buffer, moduleBase, patchHitReaction, "40 55 53 56 57 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B 01", 0, { 0xC3, 0x90 });

        uintptr_t addrFury = FindPatternInBuffer(buffer, moduleBase, "F3 0F 5C C6 C7 44 24 ?? ?? ?? ?? ?? 48 8D 44 24");
        if (addrFury != 0) {
            size_t rva = addrFury - moduleBase;
            std::vector<uint8_t> sc = { 0x53, 0x8B, 0x19, 0x89, 0x59, 0x08, 0x5B };
            sc.insert(sc.end(), buffer.begin() + rva + 4, buffer.begin() + rva + 12);
            SetupCaveCommon(buffer, moduleBase, caveUnlimitedFury, addrFury, 12, sc, 0.0f);
        }

        uintptr_t addrMovSpeed = FindPatternInBuffer(buffer, moduleBase, "0F 28 C3 0F 28 D3 F3 0F 59 03");
        if (addrMovSpeed != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x1D, 0x78, 0x00, 0x00, 0x00, 0x0F, 0x28, 0xC3, 0x0F, 0x28, 0xD3 };
            SetupCaveCommon(buffer, moduleBase, caveMoveSpeed, addrMovSpeed, 6, sc, MenuVars::speedMultiplier);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchSkipIsFury, "75 ?? FF 92 ?? ?? ?? ?? 48 8B C8 48 8B 10 FF 92 ?? ?? ?? ?? F3 0F 10 80 ?? ?? ?? ?? 48 83 C4 ?? 5B C3 FF 92 ?? ?? ?? ?? 48 8B C8 48 8B 10 FF 92 ?? ?? ?? ?? F3 0F 10 80 ?? ?? ?? ?? 48 83 C4 ?? 5B C3 CC", 0, { 0x70 });

        uintptr_t addrJump = FindPatternInBuffer(buffer, moduleBase, "F3 0F 10 80 ?? ?? ?? ?? 48 83 C4 ?? 5B C3 FF 92 ?? ?? ?? ?? 48 8B C8 48 8B 10 FF 92 ?? ?? ?? ?? F3 0F 10 80 ?? ?? ?? ?? 48 83 C4 ?? 5B C3 CC CC CC CC CC 40 53");
        if (addrJump != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x05, 0x78, 0x00, 0x00, 0x00 };
            SetupCaveCommon(buffer, moduleBase, caveJumpHeight, addrJump, 8, sc, MenuVars::jumpMultiplier);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchUnlimitedJumps, "74 ?? E8 ?? ?? ?? ?? 48 BA ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 49 23 CD 4C 8D 85 ?? ?? ?? ?? 48 0B CA 48 8D 55 ?? 48 89 8D ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 48 8B D0 48 8B CE E8 ?? ?? ?? ?? 48 8D 8D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 8D ?? ?? ?? ?? E8 ?? ?? ?? ?? B0", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchTripleJump, "0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 BA ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 49 23 CD", 0, { 0x0F, 0x80 });
        SetupPatchFromBuffer(buffer, moduleBase, patchVaultKick, "80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8D 8F", 7, { 0x90, 0xE9 });

        uintptr_t addrCable = FindPatternInBuffer(buffer, moduleBase, "F3 0F 10 81 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC 48 83 EC ?? FF 15");
        if (addrCable != 0) {
            size_t rva = addrCable - moduleBase;
            std::vector<uint8_t> sc = { 0xC7, 0x81, buffer[rva + 4], buffer[rva + 5], buffer[rva + 6], buffer[rva + 7], 0xCD, 0xCC, 0xCC, 0x3D };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 8);
            SetupCaveCommon(buffer, moduleBase, caveUnlimitedCable, addrCable, 8, sc, 0.1f);
        }

        uintptr_t addrHookNerf = FindPatternInBuffer(buffer, moduleBase, "F3 0F 11 86 ?? ?? ?? ?? 48 8B CE E8 ?? ?? ?? ?? 48 8B 06");
        if (addrHookNerf != 0) {
            size_t rva = addrHookNerf - moduleBase;
            std::vector<uint8_t> sc = { 0x0F, 0x57, 0xC0 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 8);
            SetupCaveCommon(buffer, moduleBase, caveDisableHookNerf, addrHookNerf, 8, sc, 0.0f);
        }

        uintptr_t addrSwim = FindPatternInBuffer(buffer, moduleBase, "48 8B 07 FF 90 ?? ?? ?? ?? 48 8B C8 48 8B 10 FF 92 ?? ?? ?? ?? F3 44 0F 10 B0");
        if (addrSwim != 0) {
            size_t rva = addrSwim - moduleBase;
            std::vector<uint8_t> sc = { 0xF3, 0x44, 0x0F, 0x10, 0x2D, 0x77, 0x00, 0x00, 0x00 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 9);
            SetupCaveCommon(buffer, moduleBase, caveSwimSpeed, addrSwim, 9, sc, MenuVars::swimSpeedMultiplier);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchWaterPhysics, "0F 84 ?? ?? ?? ?? 48 8B 41 ?? 48 83 C1 ?? 48 89 5C 24 ?? 48 89 74 24", 0, { 0x0F, 0x81 });
        SetupPatchFromBuffer(buffer, moduleBase, patchPlayableArea1, "0F 94 C0 48 8B 5C 24 ?? 48 8B 6C 24 ?? 48 8B 74 24 ?? 48 83 C4 ?? 41 5F 41 5E 5F C3 CC", 0, { 0x0F, 0x91 });
        SetupPatchFromBuffer(buffer, moduleBase, patchPlayableArea2, "75 ?? 38 05 ?? ?? ?? ?? 75 ?? E8", 0, { 0xEB, 0x08 });
        SetupPatchFromBuffer(buffer, moduleBase, patchIsOutOfMap, "74 ?? 80 3D ?? ?? ?? ?? ?? 75 ?? 32 C9", 0, { 0x71 });
        SetupPatchFromBuffer(buffer, moduleBase, patchTPTime, "76 ?? 83 BB ?? ?? ?? ?? ?? 75 ?? 48 8B 0D", 0, { 0x71 });

        uintptr_t addrLogicalPlayer = FindPatternInBuffer(buffer, moduleBase, "48 8B 01 FF 90 ?? ?? ?? ?? 48 8B 8E ?? ?? ?? ?? 48 85 C9 74 ?? E8 ?? ?? ?? ?? 48 8B 0D");
        if (addrLogicalPlayer != 0) {
            size_t rva = addrLogicalPlayer - moduleBase;
            std::vector<uint8_t> sc = { 0x48, 0x89, 0x0D, 0x79, 0x00, 0x00, 0x00 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 9);
            if (SetupCaveCommon(buffer, moduleBase, caveLogicalPlayer, addrLogicalPlayer, 9, sc, 0.0f)) {
                Write<uint64_t>(caveLogicalPlayer.paramAddress, 0);
                SetCaveState(caveLogicalPlayer, true);
            }
        }

        uintptr_t addrAICollect = FindPatternInBuffer(buffer, moduleBase, "38 81 ?? ?? ?? ?? 74 ?? 38 81 ?? ?? ?? ?? ?? ?? ??");
        if (addrAICollect != 0) {
            size_t rva = addrAICollect - moduleBase;
            // Кольцо из 32 записей: +0x08 — объект ИИ (rcx), +0x108 — rbx = компонент
            // CoAIObservableHumanAI + 0x38 (его сущность cbs::CEntity лежит в [rbx-0x28]).
            //   push rax/rdx; eax = (cnt+1) & 31; cnt = eax; ring[eax] = rcx; ring2[eax] = rbx; pop
            std::vector<uint8_t> sc = {
                0x50, 0x52,
                0x8B, 0x05, 0x78, 0x00, 0x00, 0x00,
                0xFF, 0xC0,
                0x83, 0xE0, 0x1F,
                0x89, 0x05, 0x6D, 0x00, 0x00, 0x00,
                0x48, 0x8D, 0x15, 0x6E, 0x00, 0x00, 0x00,
                0x48, 0x89, 0x0C, 0xC2,
                0x48, 0x89, 0x9C, 0xC2, 0x00, 0x01, 0x00, 0x00,
                0x5A, 0x58
            };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 6);
            if (SetupCaveCommon(buffer, moduleBase, caveAICollector, addrAICollect, 6, sc, 0.0f)) {
                Write<uint32_t>(caveAICollector.paramAddress, 0);
                std::vector<uint8_t> zeros(0x208, 0);
                WriteBytes(caveAICollector.paramAddress + 8, zeros);
            }
        }

        // Хиты и убийства: перехват lifecs::PrivHealth::LifeHealth<HealthFactors>::Subtract
        // (subss xmm0,xmm6: xmm0 = здоровье ДО удара, xmm6 = урон, r12 = объект здоровья жертвы).
        // Своя страница памяти рядом с gamedll: код с +0x000, данные с +0x200:
        //   +0x200 счётчик попаданий, +0x204 счётчик убийств, +0x208 0.0f
        //   +0x210 кольцо 16 попаданий по 0x80 байт:
        //       +0x00 health ptr (r12)   +0x08 адрес возврата   +0x10 урон   +0x14 HP до удара
        //       +0x18 8 qword из кадра функции [rsp+0xD0..0x110): сохранённые r14,r13,r12,RBX,rbp,ret,rsi,rdi
        //       +0x58 4 qword [rsp+0x1A8..0x1C8): сохранённые регистры обёртки LifeHealth (вызов через vtable)
        //     Сохранённый RBX вызывающего = HumanHealthModule*: по нему мод находит точного владельца
        //     (HumanAI = ([module+8] & mask) - 0x80, проверка: [[module+0x18]] == health ptr).
        //   +0xA10 кольцо 8 убийств по 16 байт: [health ptr][адрес возврата]
        //   Шеллкод: push rax/rdx/rcx/rsi/rdi; запись записи + 2x rep movsq; проверка убийства; pop.
        uintptr_t addrHealthSub = FindPatternInBuffer(buffer, moduleBase, "F3 0F 5C C6 0F 28 C8 E8 ?? ?? ?? ?? 49 8D 4C 24 ?? 49 03 CF E8 ?? ?? ?? ?? 83 7D ?? ?? 0F 28 F0 74 ?? 44 8B C6");
        if (addrHealthSub == 0) addrHealthSub = FindPatternInBuffer(buffer, moduleBase, "F3 0F 5C C6 0F 28 C8 E8 ?? ?? ?? ?? 49 8D 4C 24 ?? 49 03 CF");
        if (addrHealthSub != 0) {
            size_t rva = addrHealthSub - moduleBase;
            std::vector<uint8_t> sc = {
                0x50, 0x52, 0x51, 0x56, 0x57, 0x8B, 0x05, 0xF5, 0x01, 0x00, 0x00, 0x83,
                0xE0, 0x0F, 0xC1, 0xE0, 0x07, 0x48, 0x8D, 0x15, 0xF8, 0x01, 0x00, 0x00,
                0x48, 0x01, 0xC2, 0x4C, 0x89, 0x22, 0x48, 0x8B, 0x8C, 0x24, 0x20, 0x01,
                0x00, 0x00, 0x48, 0x89, 0x4A, 0x08, 0xF3, 0x0F, 0x11, 0x72, 0x10, 0xF3,
                0x0F, 0x11, 0x42, 0x14, 0x48, 0x8D, 0xB4, 0x24, 0xF8, 0x00, 0x00, 0x00,
                0x48, 0x8D, 0x7A, 0x18, 0xB9, 0x08, 0x00, 0x00, 0x00, 0xF3, 0x48, 0xA5,
                0x48, 0x8D, 0xB4, 0x24, 0xD0, 0x01, 0x00, 0x00, 0xB9, 0x04, 0x00, 0x00,
                0x00, 0xF3, 0x48, 0xA5, 0xFF, 0x05, 0xA2, 0x01, 0x00, 0x00, 0x0F, 0x2F,
                0x05, 0xA3, 0x01, 0x00, 0x00, 0x76, 0x30, 0x0F, 0x2F, 0xC6, 0x77, 0x2B,
                0x8B, 0x05, 0x92, 0x01, 0x00, 0x00, 0x83, 0xE0, 0x07, 0xC1, 0xE0, 0x04,
                0x48, 0x8D, 0x15, 0x91, 0x09, 0x00, 0x00, 0x48, 0x01, 0xC2, 0x4C, 0x89,
                0x22, 0x48, 0x8B, 0x8C, 0x24, 0x20, 0x01, 0x00, 0x00, 0x48, 0x89, 0x4A,
                0x08, 0xFF, 0x05, 0x6D, 0x01, 0x00, 0x00, 0x5F, 0x5E, 0x59, 0x5A, 0x58
            };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 7);
            if (healthCavePage == 0) healthCavePage = AllocateCavePoolNearModule(moduleBase, 0x1000);
            if (healthCavePage != 0) {
                CodeCavePatch& cv = caveHealthHitDetector;
                cv = CodeCavePatch{};
                cv.targetAddress = addrHealthSub;
                cv.caveAddress = healthCavePage;
                cv.paramAddress = healthCavePage + 0x200;
                cv.origBytes.assign(buffer.begin() + rva, buffer.begin() + rva + 7);
                int32_t back = (int32_t)((int64_t)(addrHealthSub + 7) - (int64_t)(healthCavePage + sc.size() + 5));
                sc.push_back(0xE9);
                for (int k = 0; k < 4; ++k) sc.push_back((uint8_t)((back >> (k * 8)) & 0xFF));
                WriteBytes(healthCavePage, sc);
                std::vector<uint8_t> zeros(0xA90, 0);
                WriteBytes(cv.paramAddress, zeros);
                int32_t to = (int32_t)((int64_t)healthCavePage - (int64_t)(addrHealthSub + 5));
                cv.jmpBytes = { 0xE9, (uint8_t)(to & 0xFF), (uint8_t)((to >> 8) & 0xFF), (uint8_t)((to >> 16) & 0xFF), (uint8_t)((to >> 24) & 0xFF), 0x90, 0x90 };
                foundCount++;
            }

            // Вызов из PlayerHealthModule: lea rcx,[rip+X]; mov [rsp+20],rcx; lea rcx,[rdi+10]; call Subtract
            uintptr_t addrPlayerCall = FindPatternInBuffer(buffer, moduleBase, "48 8D 0D ?? ?? ?? ?? 48 89 4C 24 20 48 8D 4F 10 E8");
            playerHealthCallRet = 0;
            if (addrPlayerCall != 0) {
                size_t crva = addrPlayerCall - moduleBase + 0x10;
                int32_t rel; std::memcpy(&rel, buffer.data() + crva + 1, 4);
                uintptr_t callee = addrPlayerCall + 0x10 + 5 + rel;
                if (callee < addrHealthSub && addrHealthSub - callee < 0x1000) playerHealthCallRet = addrPlayerCall + 0x15;
            }
        }

        if (GetModuleInfo(processId, L"engine_x64_rwdi.dll", engineBase, engineSize)) {
            std::vector<uint8_t> engBuffer;
            if (ReadModuleBuffer(engineBase, engineSize, engBuffer)) {
                if (engineCavePool == 0) { engineCavePool = AllocateCavePoolNearModule(engineBase, 0x10000); engineCaveOffset = 0; }
                SetupPatchFromBuffer(engBuffer, engineBase, patchFloatInAir, "48 8B C4 48 89 58 ?? 55 56 57 41 55 41 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 44 0F 29 48", 0, { 0x30, 0xC0, 0xC3 });

                uintptr_t addrTPWaypoint = FindPatternInBuffer(engBuffer, engineBase, "F2 0F 11 81 ?? ?? ?? ?? 8B 42 ?? B2");
                if (addrTPWaypoint != 0 && engineCavePool != 0) {
                    size_t rva = addrTPWaypoint - engineBase;
                    std::vector<uint8_t> sc = { 0xF2, 0x0F, 0x10, 0x05, 0x78, 0x00, 0x00, 0x00, 0xF2, 0x0F, 0x11, 0x02, 0x8B, 0x05, 0x76, 0x00, 0x00, 0x00, 0x89, 0x42, 0x08 };
                    sc.insert(sc.end(), engBuffer.begin() + rva, engBuffer.begin() + rva + 8);
                    SetupCaveEx(engBuffer, engineBase, engineCavePool, engineCaveOffset, caveTeleportWaypoint, addrTPWaypoint, 8, sc, 0.0f);
                }

                uintptr_t addrToggleHUD = FindPatternInBuffer(engBuffer, engineBase, "F6 86 ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 48 8B 86");
                if (addrToggleHUD != 0 && engineCavePool != 0) {
                    size_t rva = addrToggleHUD - engineBase;
                    std::vector<uint8_t> sc = { 0x50, 0x48, 0x8D, 0x86, engBuffer[rva + 2], engBuffer[rva + 3], engBuffer[rva + 4], engBuffer[rva + 5], 0x48, 0x89, 0x05, 0x71, 0x00, 0x00, 0x00, 0xC6, 0x00, 0x00, 0xF6, 0x00, 0x04, 0x58 };
                    if (SetupCaveEx(engBuffer, engineBase, engineCavePool, engineCaveOffset, caveToggleHUD, addrToggleHUD, 7, sc, 0.0f))
                        Write<uint64_t>(caveToggleHUD.paramAddress, 0);
                }
            }
        }

        uintptr_t addrNightExp = FindPatternInBuffer(buffer, moduleBase, "0F 28 74 24 ?? 48 83 C4 ?? 5B C3 CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC ?? 48 8B D9");
        if (addrNightExp != 0) {
            size_t rva = addrNightExp - moduleBase;
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x59, 0x05, 0x78, 0x00, 0x00, 0x00 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 5);
            SetupCaveCommon(buffer, moduleBase, caveNightExp, addrNightExp, 5, sc, MenuVars::nightExpMultiplier);
        }

        uintptr_t addrOpenWorldExp = FindPatternInBuffer(buffer, moduleBase, "F3 0F 59 B0 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8 48 8B 10");
        if (addrOpenWorldExp != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x59, 0x35, 0x78, 0x00, 0x00, 0x00 };
            SetupCaveCommon(buffer, moduleBase, caveOpenWorldExp, addrOpenWorldExp, 8, sc, MenuVars::openWorldExpMultiplier);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchDeathPenalty1, "0F 82 ?? ?? ?? ?? 48 8B 07 BA", 0, { 0x90, 0xE9 });
        uintptr_t addrPSCheck = FindPatternInBuffer(buffer, moduleBase, "89 B7 ?? ?? ?? ?? 85 F6");
        if (addrPSCheck != 0) {
            size_t rva = addrPSCheck - moduleBase;
            std::vector<uint8_t> sc = { 0x31, 0xF6 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 6);
            SetupCaveCommon(buffer, moduleBase, caveDeathPenalty2, addrPSCheck, 6, sc, 0.0f);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchPowerAttackBlock, "32 C0 48 83 C4 ?? 5B C3 CC CC CC CC 40 53 48 83 EC ?? 48 8B 01 48 8B D9 FF 90 ?? ?? ?? ?? 84 C0", 0, { 0xB0, 0x01 });
        SetupPatchFromBuffer(buffer, moduleBase, patchAlwaysPerfectBlock, "0F 93 C0 0F 28 7C 24 ?? 48 83 C4 ?? 5B", 0, { 0x0F, 0x91, 0xC0 });
        SetupPatchFromBuffer(buffer, moduleBase, patchAutoBlock, "32 C0 48 83 C4 ?? 5F 5E 5B C3 CC CC CC CC 40 53", 0, { 0xB0, 0x01 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDurability, "F3 0F 11 B7 ? ? ? ? 74 ? 48 8B 07 48 8B CF 48 89 5C 24", 0, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchUnlimitedRepairs, "FF 43 1C 48 8B 43 ?? 48 8D 4B", 0, { 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchOneHitKill, "74 ?? 48 8B 8E ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 09 EB ?? 49 8B CD 48 8B 01 FF 50 ?? 84 C0", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchEasyFinisher, "0F 94 C0 48 83 C4 ?? 5B C3 32 C0 48 83 C4 ?? 5B C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC ?? 48 8B D9", 0, { 0x0F, 0x91 });
        SetupPatchFromBuffer(buffer, moduleBase, patchInstantFinisher1, "0F 85 ?? ?? ?? ?? 40 38 AB ?? ?? ?? ?? 74 ?? 48 8D 8B", 0, { 0x0F, 0x80 });
        SetupPatchFromBuffer(buffer, moduleBase, patchInstantFinisher2, "74 ?? 48 8D 8B ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 15 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8D 88", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchInstantFinisher3, "74 ?? 48 8B D3 48 8B CF E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 40 38 AB", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchInstantFinisher4, "0F 84 ?? ?? ?? ?? 49 8B CD E8 ?? ?? ?? ?? 84 C0 74 ?? 48 8D 8B", 0, { 0x0F, 0x80 });
        SetupPatchFromBuffer(buffer, moduleBase, patchElemental1, "0F 44 CA 08 8B ?? ?? ?? ?? 39 BB ?? ?? ?? ?? 7C ?? 48 8B 05 ?? ?? ?? ?? 48 85 C0 75 ?? 8D 48 ?? FF 15 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B C8 E8 ?? ?? ?? ?? EB ?? 48 8B C7 48 89 05 ?? ?? ?? ?? 8B 93 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 80 A3 ?? ?? ?? ?? ?? 8B D7", 0, { 0x0F, 0x41 });
        SetupPatchFromBuffer(buffer, moduleBase, patchElemental2, "74 ?? 49 83 C4 ?? 49 8B CC", 0, { 0x70 });

        uintptr_t addrDmgMult = FindPatternInBuffer(buffer, moduleBase, "F3 44 0F 10 86 ?? ?? ?? ?? 45 0F 2F C3");
        if (addrDmgMult != 0) {
            size_t rva = addrDmgMult - moduleBase;
            std::vector<uint8_t> sc = {
                0xF3, 0x0F, 0x10, 0x3D, 0x78, 0x00, 0x00, 0x00,
                buffer[rva + 0], buffer[rva + 1], buffer[rva + 2], buffer[rva + 3],
                buffer[rva + 4], buffer[rva + 5], buffer[rva + 6], buffer[rva + 7], buffer[rva + 8]
            };
            SetupCaveCommon(buffer, moduleBase, caveDamageMult, addrDmgMult, 9, sc, MenuVars::damageMultiplier - 1.0f);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchAmmo, "48 83 EC ?? 83 C2 ?? 83 FA", 0, { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchArrows, "0F 85 ? ? ? ? 41 83 3E ? 75", 0, { 0x90, 0xE9 });

        uintptr_t addrNoReload = FindPatternInBuffer(buffer, moduleBase, "0F 57 C0 0F 2F D0 0F 28 CE");
        if (addrNoReload != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x58, 0x3D, 0x78, 0x00, 0x00, 0x00, 0x0F, 0x57, 0xC0, 0x0F, 0x2F, 0xD0 };
            SetupCaveCommon(buffer, moduleBase, caveNoReload, addrNoReload, 6, sc, 1000000000000.0f);
        }

        uintptr_t addrNoRecoil = FindPatternInBuffer(buffer, moduleBase, "F3 0F 59 B0 ?? ?? ?? ?? 48 8B 03 FF 90 ?? ?? ?? ?? 48 8B C8 48 8B 10 FF 92 ?? ?? ?? ?? 48 8B 7C 24 ?? 84 C0 74 ?? 48 8B 03 48 8B CB FF 90 ?? ?? ?? ?? 48 8B C8 48 8B 10 FF 92 ?? ?? ?? ?? F3 0F 59 B0 ?? ?? ?? ?? 48 8B 5C 24 ?? 0F 28 C6 0F 28 74 24 ?? 48 83 C4 ?? 5E C3 CC CC CC CC CC CC CC CC 48 89 5C 24 ?? 56 48 83 EC ?? 48 8B 01 8B F2 48 8B D9 FF 90 ?? ?? ?? ?? 8B 93 ?? ?? ?? ?? 48 8B C8 4C 8B 00 41 FF 90 ?? ?? ?? ?? 48 85 C0 75 ?? 0F 57 C0 48 8B 5C 24 ?? 48 83 C4 ?? 5E C3 80 3D ?? ?? ?? ?? ?? 74 ?? 83 3D ?? ?? ?? ?? ?? 75 ?? F3 0F 10 05");
        if (addrNoRecoil != 0) {
            size_t rva = addrNoRecoil - moduleBase;
            std::vector<uint8_t> sc = { 0xC7, 0x80, buffer[rva + 4], buffer[rva + 5], buffer[rva + 6], buffer[rva + 7], 0x00, 0x00, 0x00, 0x00, 0x0F, 0x57, 0xF6 };
            SetupCaveCommon(buffer, moduleBase, caveNoRecoil, addrNoRecoil, 8, sc, 0.0f);
        }

        uintptr_t addrAccScan = FindPatternInBuffer(buffer, moduleBase, "F3 0F 5C 88 ?? ?? ?? ?? 48 8B 03 F3 0F 59 F9 FF 90 ?? ?? ?? ?? 48 8B C8 48 8B 10 FF 92 ?? ?? ?? ?? 84 C0 74");
        if (addrAccScan != 0) {
            size_t rva = addrAccScan - moduleBase; uintptr_t addrMegaAcc = addrAccScan + 8;
            std::vector<uint8_t> sc = { 0xC7, 0x80, buffer[rva + 4], buffer[rva + 5], buffer[rva + 6], buffer[rva + 7], 0x00, 0x00, 0x00, 0x40 };
            sc.insert(sc.end(), buffer.begin() + rva + 8, buffer.begin() + rva + 15);
            SetupCaveCommon(buffer, moduleBase, caveMegaAccuracy, addrMegaAcc, 7, sc, 2.0f);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchSilenced1, "74 ?? 48 8B 16 48 8B CE 0F 29 74 24", 0, { 0x71 });
        SetupPatchFromBuffer(buffer, moduleBase, patchSilenced2, "E8 ?? ?? ?? ?? 44 0F 2F C7 76", 0, { 0x90, 0x90, 0x90, 0x90, 0x90 });

        uintptr_t addrBowRapid = FindPatternInBuffer(buffer, moduleBase, "F3 0F 10 44 81 ?? 48 8B 5C 24");
        if (addrBowRapid != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x05, 0x78, 0x00, 0x00, 0x00 };
            SetupCaveCommon(buffer, moduleBase, caveBowRapidFire, addrBowRapid, 6, sc, 0.1f);
        }

        SetupMultiPatchFromBuffer(buffer, moduleBase, patchFreeCraftAll, "44 8B 24 39", { 0x45, 0x31, 0xE4, 0x90 });

        uintptr_t addrFreeBP = FindPatternInBuffer(buffer, moduleBase, "8B 42 ?? 89 45 ?? 48 8B 02");
        if (addrFreeBP != 0) {
            size_t rva = addrFreeBP - moduleBase;
            std::vector<uint8_t> sc = { 0xC7, 0x42, buffer[rva + 2], 0x00, 0x00, 0x00, 0x00, 0x31, 0xC0, 0x89, 0x45, buffer[rva + 5] };
            SetupCaveCommon(buffer, moduleBase, caveFreeBlueprints, addrFreeBP, 6, sc, 0.0f);
        }

        uintptr_t addrGatherMul = FindPatternInBuffer(buffer, moduleBase, "48 89 9C 24 ?? ?? ?? ?? 48 8B CE 4C 89 B4 24 ?? ?? ?? ?? 45 33 F6");
        if (addrGatherMul != 0) {
            size_t rva = addrGatherMul - moduleBase;
            std::vector<uint8_t> sc = { 0x8B, 0x4A, 0x04, 0x83, 0xF9, 0xFF, 0x74, 0x0D, 0x8B, 0x0D, 0x70, 0x00, 0x00, 0x00, 0x8B, 0x02, 0x0F, 0xAF, 0xC1, 0x89, 0x02 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 8);
            if (SetupCaveCommon(buffer, moduleBase, caveMultiplyGathered, addrGatherMul, 8, sc, 0.0f))
                Write<int>(caveMultiplyGathered.paramAddress, MenuVars::gatheredMultiplier);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchFreezeTime, "75 ?? 48 8B CB E8 ?? ?? ?? ?? 48 8B CB F3 0F 5E F0", 0, { 0xEB });

        uintptr_t addrForceDT1 = FindPatternInBuffer(buffer, moduleBase, "F3 0F 5C D0 0F 2F D1 F3 0F 11 53");
        if (addrForceDT1 != 0) {
            std::vector<uint8_t> sc = { 0x50, 0x48, 0x8B, 0x46, 0x20, 0x48, 0x85, 0xC0, 0x74, 0x13, 0x48, 0x8B, 0x80, 0xC0, 0x38, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x74, 0x07, 0xC7, 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x58, 0xF3, 0x0F, 0x10, 0x15, 0x5A, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x5C, 0xD0, 0x0F, 0x2F, 0xD1 };
            SetupCaveCommon(buffer, moduleBase, caveForceDayTime1, addrForceDT1, 7, sc, MenuVars::worldTime / 24.0f);
        }

        uintptr_t addrForceDT2 = FindPatternInBuffer(buffer, moduleBase, "F3 0F 11 43 5C F3 0F 10 4B");
        if (addrForceDT2 != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x05, 0x78, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x11, 0x43, 0x5C };
            SetupCaveCommon(buffer, moduleBase, caveForceDayTime2, addrForceDT2, 5, sc, MenuVars::worldTime / 24.0f);
        }

        uintptr_t addrForcePhase = FindPatternInBuffer(buffer, moduleBase, "8B 4A ?? E9 ?? ?? ?? ?? B8");
        if (addrForcePhase != 0) {
            size_t rva = addrForcePhase - moduleBase;
            std::vector<uint8_t> sc = { 0x8B, 0x4A, buffer[rva + 2], 0x8B, 0x05, 0x77, 0x00, 0x00, 0x00, 0xC3 };
            if (SetupCaveCommon(buffer, moduleBase, caveForceDayPhase, addrForcePhase, 8, sc, 0.0f))
                Write<int>(caveForceDayPhase.paramAddress, 1);
        }

        uintptr_t addrWorldSpeed = FindPatternInBuffer(buffer, moduleBase, "F3 0F 5D CE 48 8B 5C 24");
        if (addrWorldSpeed != 0) {
            size_t rva = addrWorldSpeed - moduleBase;
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x0D, 0x78, 0x00, 0x00, 0x00 };
            sc.insert(sc.end(), buffer.begin() + rva + 4, buffer.begin() + rva + 9);
            SetupCaveCommon(buffer, moduleBase, caveWorldSpeed, addrWorldSpeed, 9, sc, MenuVars::worldSpeedValue);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchWorldNoise1, "0F 84 ?? ?? ?? ?? 4C 8B DC 55 57 41 56 48 81 EC", 0, { 0x0F, 0x81 });
        SetupPatchFromBuffer(buffer, moduleBase, patchWorldNoise2, "0F 84 ?? ?? ?? ?? 48 85 FF 0F 84 ?? ?? ?? ?? 49 8B 03", 0, { 0x0F, 0x81 });

        uintptr_t addrFog = FindPatternInBuffer(buffer, moduleBase, "F3 0F 59 B7 ?? ?? ?? ?? E8");
        if (addrFog != 0) {
            size_t rva = addrFog - moduleBase;
            std::vector<uint8_t> sc = { 0xC7, 0x87, buffer[rva + 4], buffer[rva + 5], buffer[rva + 6], buffer[rva + 7], 0x00, 0x00, 0xFA, 0x43 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 8);
            SetupCaveCommon(buffer, moduleBase, caveRemoveFogOfWar, addrFog, 8, sc, 500.0f);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchDiscoverMap1, "74 ?? 48 8B 4F ?? E8 ?? ?? ?? ?? 48 8D 55", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDiscoverMap2, "74 ?? 48 8B 4B ?? 48 85 C9 74 ?? 48 8B 01 FF 50 ?? EB ?? 48 8D 43 ?? 80 38 ?? 75 ?? 48 8B 8B", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDiscoverMap3, "74 ?? 48 8B 03 48 8B CB FF 90 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B D6", 0, { 0x75 });

        uintptr_t addrZoneLvl = FindPatternInBuffer(buffer, moduleBase, "48 8B 5C 24 ?? 48 8B 6C 24 ?? 48 8B 74 24 ?? 48 83 C4 ?? 41 5F 41 5E 5F C3 48 8D 8D");
        if (addrZoneLvl != 0) {
            size_t rva = addrZoneLvl - moduleBase;
            std::vector<uint8_t> sc = { 0x8B, 0x1D, 0x7A, 0x00, 0x00, 0x00, 0x41, 0x89, 0x1F, 0x8B, 0x1D, 0x75, 0x00, 0x00, 0x00, 0x41, 0x89, 0x1E };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 5);
            if (SetupCaveCommon(buffer, moduleBase, caveEditZoneLevel, addrZoneLvl, 5, sc, 0.0f)) {
                Write<int>(caveEditZoneLevel.paramAddress, MenuVars::zoneMinLevel);
                Write<int>(caveEditZoneLevel.paramAddress + 4, MenuVars::zoneMaxLevel);
            }
        }

        uintptr_t addrDoorLock = FindPatternInBuffer(buffer, moduleBase, "48 8D 83 ?? ?? ?? ?? 3B 38 0F 94 C0");
        if (addrDoorLock != 0) {
            size_t rva = addrDoorLock - moduleBase;
            std::vector<uint8_t> sc(buffer.begin() + rva, buffer.begin() + rva + 7);
            std::vector<uint8_t> setUnlocked = { 0xC7, 0x00, 0x07, 0x00, 0x00, 0x00 };
            sc.insert(sc.end(), setUnlocked.begin(), setUnlocked.end());
            SetupCaveCommon(buffer, moduleBase, caveRemoveDoorLock, addrDoorLock, 7, sc, 0.0f);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchAllowDoorOpening, "74 ?? 48 8B 01 FF 90 ?? ?? ?? ?? 84 C0 74 ?? 48 8B 47 ?? 48 8B 08", 0, { 0x71 });
        SetupPatchFromBuffer(buffer, moduleBase, patchAutoUnlock, "40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? 48 8B D9 74 ?? 48 8B 81", 0, { 0xB0, 0x01, 0xC3 });

        uintptr_t addrEasyLockpick = FindPatternInBuffer(buffer, moduleBase, "F3 0F 11 4B 58 0F");
        if (addrEasyLockpick != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x4B, 0x6C, 0xF3, 0x0F, 0x11, 0x4B, 0x58 };
            SetupCaveCommon(buffer, moduleBase, caveEasyLockpick, addrEasyLockpick, 5, sc, 0.0f);
        }

        uintptr_t addrAIScale = FindPatternInBuffer(buffer, moduleBase, "44 8B C8 F3 0F 5C F1");
        if (addrAIScale != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x35, 0x78, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x10, 0xCE, 0x44, 0x8B, 0xC8, 0xF3, 0x0F, 0x5C, 0xF1 };
            SetupCaveCommon(buffer, moduleBase, caveChangeAIScale, addrAIScale, 7, sc, MenuVars::aiScaleValue);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchInvisibility1, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D B9 ? ? ? ? 48 8B F1 48 8B 07 48 8B CF FF 90", 0, { 0xB0, 0x01, 0xC3 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableAIAwareness, "0F 94 C0 48 83 C4 ?? 5B C3 32 C0 48 83 C4 ?? 5B C3 CC CC CC CC CC CC CC CC CC 40 53 48 83 EC ?? 48 8B D9 48 8B 49", 0, { 0x0F, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableZombieGrab, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B DA 48 8B F9 E8 ?? ?? ?? ?? 48 85 DB 74 ?? 48 8B 03 48 8B CB FF 50 ?? 48 63 D8 48 03 DF 74 ?? 48 8B 13 48 8B CB FF 52 ?? 83 F8 ?? 75 ?? 0F B6 43", 0, { 0xB0, 0x01, 0xC3 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableChase, "40 53 48 83 EC ? 80 79 ? ? 48 8B D9 0F 84 ? ? ? ? 48 8B 0D", 0, { 0x30, 0xC0, 0xC3 });
        SetupPatchFromBuffer(buffer, moduleBase, patchAlwaysStomp, "32 C0 4C 8D 9C 24 ?? ?? ?? ?? 49 8B 5B ?? 49 8B 73 ?? 49 8B 7B ?? 41 0F 28 73 ?? 49 8B E3 41 5E C3 CC CC CC CC CC CC CC", 0, { 0xB0, 0x01 });
        SetupPatchFromBuffer(buffer, moduleBase, patchFreezeKillAI, "75 ?? 0F B6 C2 0F B6 4C 08", 0, { 0x71 });
        SetupPatchFromBuffer(buffer, moduleBase, patchBurnAI, "0F 95 C0 C3 CC CC 48 81 E9", 0, { 0xB0, 0x01, 0xC3 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableAISpawn1, "0F 95 C0 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3 48 8D 83", 0, { 0x0F, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableAISpawn2, "0F 95 C0 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3 48 8B 5C 24 ?? 32 C0 48 83 C4 ?? 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 4C 89 44 24", 0, { 0x0F, 0x90, 0xC0 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableAISpawn3, "B8 ?? ?? ?? ?? 48 83 C4 ?? 5F C3 CC 48 89 6C 24", 0, { 0xB8, 0x00, 0x00, 0x00, 0x00 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableAISpawn4, "74 ?? B0 ?? C3 32 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC F3 0F 10 81", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableAISpawn5, "0F 84 ?? ?? ?? ?? 48 85 FF 75 ?? 8D 47", 0, { 0x0F, 0x81 });

        uintptr_t addrVehDmg = FindPatternInBuffer(buffer, moduleBase, "80 B9 ?? ?? ?? ?? ?? 74 ?? F3 0F 10 05 ?? ?? ?? ?? 48 83 C4 ?? 5B");
        if (addrVehDmg != 0) {
            size_t rva = addrVehDmg - moduleBase;
            std::vector<uint8_t> sc = { 0x50, 0x48, 0x8D, 0x81, buffer[rva + 2], buffer[rva + 3], buffer[rva + 4], buffer[rva + 5], 0x48, 0x89, 0x05, 0x71, 0x00, 0x00, 0x00, 0xC6, 0x00, 0x01, 0x58 };
            sc.insert(sc.end(), buffer.begin() + rva, buffer.begin() + rva + 7);
            if (SetupCaveCommon(buffer, moduleBase, caveNoVehicleDamage, addrVehDmg, 7, sc, 0.0f))
                Write<uint64_t>(caveNoVehicleDamage.paramAddress, 0);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchNoFuelUsage, "F3 0F 11 4F ?? 41 0F 28 CA E8", 0, { 0x90, 0x90, 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchNoGallonUse, "FF 50 ?? 4C 8B 05 ?? ?? ?? ?? B9", 0, { 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchNoRepairKit, "8B 81 ?? ?? ?? ?? C3 CC 40 53", 0, { 0x31, 0xC0, 0xC3, 0x90, 0x90, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchUnlimitedVehicleUV, "F3 0F 5C C8 0F 2F CF F3 0F 11 4F ?? 77", 0, { 0x90, 0x90, 0x90, 0x90 });

        uintptr_t addrInstUV = FindPatternInBuffer(buffer, moduleBase, "F3 44 0F 10 88 ?? ?? ?? ?? EB ?? 44 0F 28 CE");
        if (addrInstUV != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x44, 0x0F, 0x10, 0x0D, 0x77, 0x00, 0x00, 0x00 };
            SetupCaveCommon(buffer, moduleBase, caveInstantUVRecharge, addrInstUV, 9, sc, 100.0f);
        }

        uintptr_t addrNoUVCD = FindPatternInBuffer(buffer, moduleBase, "F3 0F 10 B2 ?? ?? ?? ?? 48 8B CA");
        if (addrNoUVCD != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x35, 0x78, 0x00, 0x00, 0x00 };
            SetupCaveCommon(buffer, moduleBase, caveNoUVCooldown, addrNoUVCD, 8, sc, 0.1f);
        }

        uintptr_t addrCarCam = FindPatternInBuffer(buffer, moduleBase, "48 C7 83 ?? ?? ?? ?? ?? ?? ?? ?? C6 83 ?? ?? ?? ?? ?? 48 8B 74 24");
        if (addrCarCam != 0) {
            std::vector<uint8_t> sc = { 0x8B, 0xB3, 0xAC, 0x01, 0x00, 0x00, 0x89, 0xB3, 0xC4, 0x01, 0x00, 0x00 };
            SetupCaveCommon(buffer, moduleBase, caveRemoveCarCamSnap, addrCarCam, 11, sc, 0.0f);
            SetupPatchFromBuffer(buffer, moduleBase, patchRemoveCarCamSnapFlag, "48 C7 83 ?? ?? ?? ?? ?? ?? ?? ?? C6 83 ?? ?? ?? ?? ?? 48 8B 74 24", 0x11, { 0x00 });
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchChallengeVehicle, "32 C0 C3 CC CC 0F B6 41 ?? D0 E8", 0, { 0xB0, 0x01 });
        SetupPatchFromBuffer(buffer, moduleBase, patchNoMoneyCost, "2B C7 89 03", 0, { 0x90, 0x90 });

        uintptr_t addrSellPrice = FindPatternInBuffer(buffer, moduleBase, "48 83 C4 28 C3 F3 0F 10 05 ?? ?? ?? ?? 48 83 C4 ?? C3 48 81 C1");
        if (addrSellPrice != 0) {
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x59, 0x05, 0x78, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3 };
            SetupCaveCommon(buffer, moduleBase, caveSellPrice, addrSellPrice, 5, sc, MenuVars::sellPriceMultiplier);
        }

        uintptr_t addrBuyPrice = FindPatternInBuffer(buffer, moduleBase, "48 83 C4 ? C3 CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 48 81 C1");
        if (addrBuyPrice != 0) {
            size_t rva = addrBuyPrice - moduleBase;
            std::vector<uint8_t> sc = { 0xF3, 0x0F, 0x10, 0x05, 0x78, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, buffer[rva + 3], 0xC3 };
            SetupCaveCommon(buffer, moduleBase, caveBuyPrice, addrBuyPrice, 5, sc, MenuVars::buyPriceMultiplier);
        }

        SetupPatchFromBuffer(buffer, moduleBase, patchWeaponReturn, "75 ? 48 8B CE C7 86", 0, { 0x70 });
        SetupPatchFromBuffer(buffer, moduleBase, patchInstantDismantle, "F3 0F 10 41 FC C3", 0, { 0x0F, 0x57, 0xC0, 0xC3, 0x90 });
        SetupPatchFromBuffer(buffer, moduleBase, patchDisableActionEquip, "48 89 5C 24 ?? 57 48 83 EC ?? 4C 63 41 ?? 48 8B F9 48 8B 42 ?? 4D 03 C0 48 B9 ?? ?? ?? ?? ?? ?? ?? ?? 48 23 C1 4A 8B 5C C0 ?? 48 85 DB 74 ?? 48 8B 03 48 8B CB FF 50 ?? 48 85 C0 74 ?? 48 8B 03 48 8B CB FF 50 ?? 48 8D 57 ?? 48 8B C8 4C 8B 00 41 FF 50 ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3 CC 48 89 5C 24 ?? 57", 0, { 0xC3 });
        SetupPatchFromBuffer(buffer, moduleBase, patchRightHandLoot, "BB ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? 48 8D 0D", 1, { 0x00, 0x00, 0x00, 0x00 });

        uintptr_t addrHeadbob = FindPatternInBuffer(buffer, moduleBase, "0F BF 43 ?? F3 0F 10 05");
        if (addrHeadbob != 0) {
            size_t rva = addrHeadbob - moduleBase;
            int32_t origDisp = *(int32_t*)(&buffer[rva + 8]);
            uintptr_t targetFloatAddr = addrHeadbob + 12 + origDisp;
            uintptr_t caveAddr = cavePool + caveOffset;
            int32_t newDisp = (int32_t)((int64_t)targetFloatAddr - (int64_t)(caveAddr + 13));
            std::vector<uint8_t> sc = { 0xB8, 0x00, 0x24, 0x74, 0x49, 0xF3, 0x0F, 0x10, 0x05, (uint8_t)(newDisp & 0xFF), (uint8_t)((newDisp >> 8) & 0xFF), (uint8_t)((newDisp >> 16) & 0xFF), (uint8_t)((newDisp >> 24) & 0xFF) };
            SetupCaveCommon(buffer, moduleBase, caveDisableHeadbob, addrHeadbob, 12, sc, 0.0f);
        }

        uintptr_t getGameDI = FindPatternInBuffer(buffer, moduleBase, "48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 40 53 48 83 EC ?? 48 8B 49");
        if (getGameDI != 0) {
            int32_t relOffset = Read<int32_t>(getGameDI + 3);
            gameDIPtrAddr = getGameDI + 7 + relOffset;
        }

        snprintf(statusMessage, sizeof(statusMessage), "Connected (%d)", foundCount);
        patternsInitialized = true;
    }

    uintptr_t GetMoneyValueAddress() {
        if (!hProcess || gameDIPtrAddr == 0) return 0;
        uintptr_t gameDI = Read<uintptr_t>(gameDIPtrAddr);
        if (gameDI == 0) return 0;
        uintptr_t a = Read<uintptr_t>(gameDI + 0x8);
        if (a) a = Read<uintptr_t>(a + 0x468);
        if (a) a = Read<uintptr_t>(a + 0x20);
        if (a) a = Read<uintptr_t>(a + 0x23D0);
        if (a) a = Read<uintptr_t>(a + 0x450);
        if (a) a = Read<uintptr_t>(a + 0x38);
        if (a) return a + 0x38;
        return 0;
    }

    int ClassifyAIEntity(uintptr_t aiPtr) {
        if (!hProcess || aiPtr < 0x10000 || aiPtr > 0x00007FFFFFFFFFFFULL) return 1;

        uintptr_t pViral = Read<uintptr_t>(aiPtr + 0x2250);
        if (pViral != 0) {
            pViral &= ~(1ULL << 61);
            pViral &= ~(1ULL << 57);
            if (pViral >= 0x10000 && pViral <= 0x00007FFFFFFFFFFFULL) {
                uint64_t v8 = Read<uint64_t>(pViral);
                if ((v8 & 0x000000FFFFFFFFFFULL) == 0x6C61726976ULL) return 1;
            }
        }

        uintptr_t p = Read<uintptr_t>(aiPtr + 0x2270);
        if (p != 0) {
            p &= ~(1ULL << 61);
            p &= ~(1ULL << 57);
            if (p >= 0x10000 && p <= 0x00007FFFFFFFFFFFULL) {
                char buf[32] = {};
                if (ReadProcessMemory(hProcess, (LPCVOID)p, buf, sizeof(buf) - 1, nullptr)) {
                    buf[sizeof(buf) - 1] = '\0';
                    for (char& c : buf) c = (char)tolower((unsigned char)c);

                    if (strstr(buf, "biter") || strstr(buf, "viral") || strstr(buf, "zombie") ||
                        strstr(buf, "infected") || strstr(buf, "volatile") || strstr(buf, "goon") ||
                        strstr(buf, "demolisher") || strstr(buf, "spitter") || strstr(buf, "banshee")) {
                        return 1;
                    }

                    if ((strncmp(buf, "human", 5) == 0 && buf[5] != 'o') ||
                        strstr(buf, "bandit") || strstr(buf, "renegade") ||
                        strstr(buf, "soldier") || strstr(buf, "baron")) {
                        return 2;
                    }
                }
            }
        }
        return 1;
    }

    bool ReadEntityPosition(uintptr_t entityPtr, Vec3& outPos) {
        if (!hProcess || entityPtr < 0x10000 || entityPtr > 0x00007FFFFFFFFFFFULL) return false;
        auto isValidWorldPos = [](const Vec3& v) {
            if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) return false;
            float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
            if (lenSq < 25.0f || lenSq > 50000.0f * 50000.0f) return false;
            if (std::fabs(v.x - 1.0f) < 0.01f && std::fabs(v.y - 1.0f) < 0.01f && std::fabs(v.z - 1.0f) < 0.01f) return false;
            return (std::fabs(v.x) > 2.0f && std::fabs(v.z) > 2.0f && std::fabs(v.y) < 5000.0f);
            };
        uint8_t rawBuf[0x240] = {};
        if (ReadProcessMemory(hProcess, (LPCVOID)entityPtr, rawBuf, sizeof(rawBuf), nullptr)) {
            static const int prioOffsets[] = { 0x120, 0x130, 0x140, 0xFC, 0x100, 0x110, 0x150, 0x160, 0x180, 0x1C0 };
            for (int off : prioOffsets) {
                Vec3 p = *(const Vec3*)(rawBuf + off);
                if (isValidWorldPos(p)) { outPos = p; return true; }
            }
        }
        return false;
    }

    // ==================== ПОЗИЦИИ ВРАГОВ ЧЕРЕЗ ТРАНСФОРМ ДВИЖКА ====================
    // Объект ИИ, который ловит AI-коллектор, — это не сам трансформ. Где внутри него лежит
    // указатель на мировую матрицу, заранее неизвестно, поэтому путь ищется автоматически:
    //   kind 1: [A+off1]                -> CoWorldXform
    //   kind 2: [[A+off1]+0x8]+0x148    -> CoWorldXform (как в IControlObject::GetWorldXform)
    //   kind 3: [[A+off1]+off2]         -> CoWorldXform
    // Мировая матрица CoWorldXform (см. engine_x64_rwdi: 0x78BBC0): storage=[xf+0x20],
    // idx=[xf+0x28]&0xFFF, матрица = [storage+0x70] + idx*0x30 (mtx34, позиция в +0x0C/+0x1C/+0x2C).
    // Кандидат принимается, только если матрица ортонормальная, персонаж стоит вертикально
    // и он рядом с камерой. Путь, подтверждённый на нескольких РАЗНЫХ врагах, фиксируется.
    struct AiPosDiscovery {
        bool locked = false;
        int kind = 0; uint32_t off1 = 0, off2 = 0;
        struct Vote { int count = 0; float sumDy = 0.0f; Vec3 lastPos; uintptr_t lastEntity = 0; bool shared = false; };
        std::map<uint64_t, Vote> votes;
        std::map<uintptr_t, bool> scannedEntities;
        int scanned = 0;
        int okReads = 0, failReads = 0;
        ULONGLONG lastScan = 0;
        char status[96] = "searching (hit or look at enemies)";
    };
    // отдельный путь для каждого класса ИИ (ключ — указатель на vtable объекта)
    std::map<uintptr_t, AiPosDiscovery> aiPosByClass;
    struct AiPair { uintptr_t ai, comp; };
    AiPair lastAiPairs[32]; int lastAiPairCount = 0;
    uint32_t compPosOk = 0, compPosFail = 0;   // статистика прямого пути через компонент
    char aiPosStatus[128] = "searching (look at / hit enemies)";

    void UpdateAiPosStatus() {
        int locked = 0, total = (int)aiPosByClass.size(); const AiPosDiscovery* any = nullptr;
        for (auto& kv : aiPosByClass) if (kv.second.locked) { ++locked; if (!any) any = &kv.second; }
        if (any) snprintf(aiPosStatus, sizeof(aiPosStatus), "%d/%d AI types: %s", locked, total, any->status);
        else if (total) { int sc = 0; for (auto& kv : aiPosByClass) sc += kv.second.scanned; snprintf(aiPosStatus, sizeof(aiPosStatus), "searching... %d enemies scanned", sc); }
    }

    static bool ValidPtr(uintptr_t p) { return p >= 0x10000 && p < 0x00007FFFFFFFFFFFULL && (p & 7) == 0; }

    static bool PlausibleMatrix(const float m[12]) {
        for (int i = 0; i < 12; ++i) if (!std::isfinite(m[i])) return false;
        Vec3 ax = Fx::V(m[0], m[4], m[8]), ay = Fx::V(m[1], m[5], m[9]), az = Fx::V(m[2], m[6], m[10]);
        float lx = Fx::Len(ax), ly = Fx::Len(ay), lz = Fx::Len(az);
        if (lx < 0.2f || lx > 5.0f) return false;
        if (std::fabs(lx - ly) > lx * 0.25f || std::fabs(lx - lz) > lx * 0.25f) return false;
        if (std::fabs(Fx::Dot(ax, ay)) > 0.03f * lx * ly || std::fabs(Fx::Dot(ax, az)) > 0.03f * lx * lz || std::fabs(Fx::Dot(ay, az)) > 0.03f * ly * lz) return false;
        Vec3 t = Fx::V(m[3], m[7], m[11]);
        float tl = Fx::Len(t);
        return tl > 1.0f && tl < 100000.0f;
    }

    bool ReadXformMatrix(uintptr_t xf, float m[12]) {
        if (!ValidPtr(xf)) return false;
        // 1) CoWorldXform хранит мировую матрицу прямо в себе: +0x80 (основной поток) / +0xE0
        //    (см. engine: CoWorldXform::GetWorldXform; сущность: CEntity+0x58 -> CoWorldXform)
        if (ReadProcessMemory(hProcess, (LPCVOID)(xf + 0x80), m, 12 * sizeof(float), nullptr) && PlausibleMatrix(m)) return true;
        if (ReadProcessMemory(hProcess, (LPCVOID)(xf + 0xE0), m, 12 * sizeof(float), nullptr) && PlausibleMatrix(m)) return true;
        // 2) вариант с общим хранилищем матриц (IControlObject / модели)
        uint8_t hdr[0x30];
        if (!ReadProcessMemory(hProcess, (LPCVOID)xf, hdr, sizeof(hdr), nullptr)) return false;
        uintptr_t storage; uint32_t idx;
        std::memcpy(&storage, hdr + 0x20, 8); std::memcpy(&idx, hdr + 0x28, 4);
        idx &= 0xFFF;
        if (!ValidPtr(storage)) return false;
        uint8_t st[0x20];
        if (!ReadProcessMemory(hProcess, (LPCVOID)(storage + 0x58), st, sizeof(st), nullptr)) return false;
        uintptr_t arr70, arr58;
        std::memcpy(&arr58, st + 0x00, 8); std::memcpy(&arr70, st + 0x18, 8);
        const uintptr_t arrs[2] = { arr70, arr58 };
        for (uintptr_t arr : arrs) {
            if (!ValidPtr(arr)) continue;
            if (!ReadProcessMemory(hProcess, (LPCVOID)(arr + (uintptr_t)idx * 0x30), m, 12 * sizeof(float), nullptr)) continue;
            if (PlausibleMatrix(m)) return true;
        }
        return false;
    }

    uintptr_t ResolveAiXform(uintptr_t A, int kind, uint32_t off1, uint32_t off2) {
        uintptr_t p = Read<uintptr_t>(A + off1);
        if (!ValidPtr(p)) return 0;
        if (kind == 1) return p;
        if (kind == 2) { uintptr_t q = Read<uintptr_t>(p + 8); return ValidPtr(q) ? Read<uintptr_t>(q + 0x148) : 0; }
        return Read<uintptr_t>(p + off2);
    }

    // Один проход поиска по одному врагу (дорого, поэтому не чаще раза в 200 мс)
    void DiscoverAiPath(AiPosDiscovery& aiPos, uintptr_t A, const Vec3& camPos) {
        aiPos.scannedEntities[A] = true;
        std::vector<uint8_t> blk(0x2400, 0);
        SIZE_T got = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)A, blk.data(), blk.size(), &got)) {
            if (!ReadProcessMemory(hProcess, (LPCVOID)A, blk.data(), 0x800, &got)) return;
            blk.resize(0x800);
        }
        struct Cand { uint64_t key; Vec3 pos; };
        std::vector<Cand> cands;
        auto consider = [&](int kind, uint32_t o1, uint32_t o2, uintptr_t xf) {
            float m[12];
            if (!ReadXformMatrix(xf, m)) return;
            float ly = Fx::Len(Fx::V(m[1], m[5], m[9]));
            if (m[5] / ly < 0.85f) return;                                  // персонаж стоит вертикально
            Vec3 t = Fx::V(m[3], m[7], m[11]);
            Vec3 d = Fx::Sub(t, camPos);
            if (Fx::Len(d) > 150.0f) return;                                // рядом с камерой
            if (std::sqrt(d.x * d.x + d.z * d.z) < 0.5f) return;            // это сам игрок, а не враг
            uint64_t key = ((uint64_t)kind << 48) | ((uint64_t)o1 << 16) | (uint64_t)o2;
            for (auto& c : cands) if (c.key == key) return;
            cands.push_back({ key, t });
        };
        // kind 0: мировая матрица лежит прямо в объекте ИИ (в этой версии игры — так)
        for (uint32_t o = 0; o + 48 <= blk.size(); o += 4) {
            float m[12]; std::memcpy(m, blk.data() + o, 48);
            if (!PlausibleMatrix(m)) continue;
            float ly = Fx::Len(Fx::V(m[1], m[5], m[9]));
            if (m[5] / ly < 0.85f) continue;
            Vec3 t = Fx::V(m[3], m[7], m[11]);
            Vec3 d = Fx::Sub(t, camPos);
            if (Fx::Len(d) > 150.0f || std::sqrt(d.x * d.x + d.z * d.z) < 0.5f) continue;
            uint64_t key = ((uint64_t)0 << 48) | ((uint64_t)o << 16);
            bool dup = false; for (auto& c : cands) if (c.key == key) dup = true;
            if (!dup) cands.push_back({ key, t });
        }
        for (uint32_t o1 = 0; o1 + 8 <= blk.size(); o1 += 8) {
            uintptr_t p; std::memcpy(&p, blk.data() + o1, 8);
            if (!ValidPtr(p)) continue;
            consider(1, o1, 0, p);
            uintptr_t q = Read<uintptr_t>(p + 8);
            if (ValidPtr(q)) consider(2, o1, 0, Read<uintptr_t>(q + 0x148));
            if (o1 < 0x300) {
                uint8_t inner[0x100];
                if (ReadProcessMemory(hProcess, (LPCVOID)p, inner, sizeof(inner), nullptr))
                    for (uint32_t o2 = 0; o2 < sizeof(inner); o2 += 8) {
                        uintptr_t x; std::memcpy(&x, inner + o2, 8);
                        if (ValidPtr(x) && x != p) consider(3, o1, o2, x);
                    }
            }
        }
        if (cands.empty()) { ++aiPos.scanned; return; }
        float minY = FLT_MAX;
        for (auto& c : cands) minY = (std::min)(minY, c.pos.y);
        for (auto& c : cands) {
            auto& v = aiPos.votes[c.key];
            // один и тот же мировой трансформ у разных врагов = общий объект (цель/игрок), не тело
            if (v.lastEntity && v.lastEntity != A && Fx::Len(Fx::Sub(v.lastPos, c.pos)) < 0.05f) v.shared = true;
            v.count += 1; v.sumDy += c.pos.y - minY; v.lastPos = c.pos; v.lastEntity = A;
        }
        ++aiPos.scanned;

        // фиксируем путь: >= 3 разных врага подтвердили; при равенстве — самый "низкий" (корень у ног)
        if (aiPos.scanned >= 3) {
            uint64_t bestKey = 0; float bestScore = -FLT_MAX;
            for (auto& kv : aiPos.votes) {
                const auto& v = kv.second;
                if (v.shared || v.count < 3 || v.count * 10 < aiPos.scanned * 6) continue;
                int kind = (int)(kv.first >> 48);
                float score = v.count * 10.0f - (v.sumDy / v.count) * 4.0f + (kind == 0 ? 2.0f : (kind == 2 ? 1.5f : (kind == 1 ? 1.0f : 0.0f)));
                if (score > bestScore) { bestScore = score; bestKey = kv.first; }
            }
            if (bestKey) {
                aiPos.locked = true;
                aiPos.kind = (int)(bestKey >> 48);
                aiPos.off1 = (uint32_t)((bestKey >> 16) & 0xFFFFFFFF);
                aiPos.off2 = (uint32_t)(bestKey & 0xFFFF);
                aiPos.okReads = aiPos.failReads = 0;
                if (aiPos.kind == 0) snprintf(aiPos.status, sizeof(aiPos.status), "engine matrix [AI+0x%X]", aiPos.off1);
                else if (aiPos.kind == 3) snprintf(aiPos.status, sizeof(aiPos.status), "engine xform [[AI+0x%X]+0x%X]", aiPos.off1, aiPos.off2);
                else if (aiPos.kind == 2) snprintf(aiPos.status, sizeof(aiPos.status), "engine xform [[AI+0x%X]+8]+0x148", aiPos.off1);
                else snprintf(aiPos.status, sizeof(aiPos.status), "engine xform [AI+0x%X]", aiPos.off1);
            }
        }
        if (!aiPos.locked) snprintf(aiPos.status, sizeof(aiPos.status), "searching... %d enemies scanned", aiPos.scanned);
    }

    std::map<uintptr_t, uintptr_t> aiComponent;   // объект ИИ -> его компонент (rbx из коллектора)

    bool ReadEntityWorldPos(uintptr_t A, Vec3& out); // ниже, рядом с камерой движка

    bool ReadPosByKey(uintptr_t A, uint64_t key, Vec3& out) {
        int kind = (int)(key >> 48); uint32_t o1 = (uint32_t)((key >> 16) & 0xFFFFFFFF), o2 = (uint32_t)(key & 0xFFFF);
        float m[12];
        if (kind == 8) return ReadEntityWorldPos(A, out); // точный трансформ сущности (как в движке)
        if (kind == 7) { // сущность через компонент: [comp-0x28] -> +0x58 -> матрица
            auto it = aiComponent.find(A);
            if (it == aiComponent.end()) return false;
            uintptr_t ent = Read<uintptr_t>(it->second - 0x28);
            uintptr_t xf = ValidPtr(ent) ? Read<uintptr_t>(ent + 0x58) : 0;
            if (!xf || !ReadXformMatrix(xf, m)) return false;
            out = Fx::V(m[3], m[7], m[11]);
            return true;
        }
        bool ok = (kind == 0)
            ? (ReadProcessMemory(hProcess, (LPCVOID)(A + o1), m, sizeof(m), nullptr) && PlausibleMatrix(m))
            : ReadXformMatrix(ResolveAiXform(A, kind, o1, o2), m);
        if (!ok) return false;
        out = Fx::V(m[3], m[7], m[11]);
        return true;
    }


    // Позиция врага (ноги): точный трансформ движка, а пока путь не найден — старая эвристика
    bool ReadAIPosition(uintptr_t A, Vec3& out, const Vec3* camPos) {
        if (!ValidPtr(A)) return false;
        if (MenuVars::hitPosSourceKey) return ReadPosByKey(A, MenuVars::hitPosSourceKey, out); // выбран вручную
        uintptr_t vtbl = Read<uintptr_t>(A);
        if (!ValidPtr(vtbl)) return false;
        if (aiPosByClass.size() > 64 && aiPosByClass.find(vtbl) == aiPosByClass.end()) return ReadEntityPosition(A, out);
        AiPosDiscovery& aiPos = aiPosByClass[vtbl];
        static ULONGLONG s_lastAnyScan = 0;
        if (!aiPos.locked && camPos && GetTickCount64() - aiPos.lastScan > 200 && GetTickCount64() - s_lastAnyScan > 120 &&
            aiPos.scannedEntities.find(A) == aiPos.scannedEntities.end() && aiPos.scannedEntities.size() < 40) {
            aiPos.lastScan = s_lastAnyScan = GetTickCount64();
            DiscoverAiPath(aiPos, A, *camPos);
            UpdateAiPosStatus();
        }
        if (aiPos.locked) {
            float m[12];
            bool okRead = (aiPos.kind == 0)
                ? (ReadProcessMemory(hProcess, (LPCVOID)(A + aiPos.off1), m, sizeof(m), nullptr) && PlausibleMatrix(m))
                : ReadXformMatrix(ResolveAiXform(A, aiPos.kind, aiPos.off1, aiPos.off2), m);
            if (okRead) {
                out = Fx::V(m[3], m[7], m[11]);
                ++aiPos.okReads;
                return true;
            }
            ++aiPos.failReads;
            // путь перестал подходить (обновление игры / другой тип ИИ) — ищем заново
            if (aiPos.failReads > 200 && aiPos.failReads > aiPos.okReads * 2) {
                aiPos = AiPosDiscovery{};
                UpdateAiPosStatus();
            }
            return false; // не подставляем мусорную эвристику, когда путь известен
        }
        return ReadEntityPosition(A, out);
    }

    // ==================== КАЛИБРОВКА ИСТОЧНИКА ПОЗИЦИИ ВРАГОВ ====================
    // Все найденные варианты пути (по всем классам ИИ), отсортированные по числу подтверждений.
    struct PosCandidate { uint64_t key; int votes; };
    std::vector<PosCandidate> posCandidates;      // обновляется раз в 0.5 с

    static void DescribePosKey(uint64_t key, char* buf, size_t n) {
        int kind = (int)(key >> 48); uint32_t o1 = (uint32_t)((key >> 16) & 0xFFFFFFFF), o2 = (uint32_t)(key & 0xFFFF);
        if (kind == 0) snprintf(buf, n, "[AI+0x%X]", o1);
        else if (kind == 1) snprintf(buf, n, "[[AI+0x%X]]", o1);
        else if (kind == 2) snprintf(buf, n, "[[[AI+0x%X]+8]+0x148]", o1);
        else if (kind == 7) snprintf(buf, n, "entity via component");
        else if (kind == 8) snprintf(buf, n, "engine entity xform (exact)");
        else snprintf(buf, n, "[[[AI+0x%X]+0x%X]]", o1, o2);
    }

    void RefreshPosCandidates() {
        std::map<uint64_t, int> agg;
        for (auto& cls : aiPosByClass)
            for (auto& v : cls.second.votes)
                if (!v.second.shared) agg[v.first] += v.second.count;
        posCandidates.clear();
        posCandidates.push_back({ (uint64_t)8 << 48, 999 }); // точный путь движка — всегда первым
        for (auto& kv : agg) posCandidates.push_back({ kv.first, kv.second });
        std::sort(posCandidates.begin(), posCandidates.end(), [](const PosCandidate& a, const PosCandidate& b) { return a.votes > b.votes; });
        if (posCandidates.size() > 8) posCandidates.resize(8);
        posCandidates.push_back({ (uint64_t)7 << 48, (int)(compPosOk > 0) }); // путь через компонент CoAIObservableHumanAI
    }

    // Цветные точки с номерами на каждом враге для каждого варианта пути
    void DrawPosDebug(ImDrawList* dl, bool enabled) {
        static ULONGLONG lastRefresh = 0;
        if (GetTickCount64() - lastRefresh > 500) { lastRefresh = GetTickCount64(); RefreshPosCandidates(); }
        if (!enabled || !Fx::gameCam.valid || !hProcess) return;
        static const ImU32 cols[8] = { IM_COL32(255, 70, 70, 255), IM_COL32(70, 220, 90, 255), IM_COL32(80, 150, 255, 255), IM_COL32(255, 210, 60, 255),
                                       IM_COL32(220, 90, 255, 255), IM_COL32(60, 230, 230, 255), IM_COL32(255, 140, 40, 255), IM_COL32(255, 255, 255, 255) };
        ULONGLONG now = GetTickCount64();
        int drawn = 0;
        for (auto& kv : Fx::trackedEntities) {
            if (now - kv.second.lastSeenTick > 600 || drawn > 40) continue;
            ++drawn;
            for (size_t i = 0; i < posCandidates.size(); ++i) {
                Vec3 p;
                if (!ReadPosByKey(kv.first, posCandidates[i].key, p)) continue;
                ImVec2 sp; float depth;
                if (!Fx::Project(Fx::gameCam, p, sp, &depth, 1.2f)) continue;
                bool sel = MenuVars::hitPosSourceKey == posCandidates[i].key;
                float r = sel ? 7.0f : 5.0f;
                sp.x += (float)i * 3.0f; // чтобы совпадающие точки не закрывали друг друга полностью
                dl->AddCircleFilled(sp, r + 1.5f, IM_COL32(0, 0, 0, 200), 12);
                dl->AddCircleFilled(sp, r, cols[i], 12);
                char lbl[8]; snprintf(lbl, sizeof(lbl), "%d", (int)i + 1);
                dl->AddText(ImVec2(sp.x + r + 2, sp.y - 7), IM_COL32(0, 0, 0, 220), lbl);
                dl->AddText(ImVec2(sp.x + r + 1, sp.y - 8), cols[i], lbl);
            }
            // текущая позиция, которую реально использует мод (белое кольцо)
            if (kv.second.hasPos) {
                ImVec2 sp;
                if (Fx::Project(Fx::gameCam, kv.second.lastPos, sp, nullptr, 1.2f)) dl->AddCircle(sp, 11.0f, IM_COL32(255, 255, 255, 230), 16, 2.0f);
            }
        }
    }

    // ==================== КАМЕРА ДВИЖКА + СОБЫТИЯ ПОПАДАНИЙ/УБИЙСТВ ====================
    // Камера: engine_x64_rwdi.dll экспортирует gui::CManager::s_Instance и
    // gui::IManager::GetCameraMtx(EWindowType). Из кода GetCameraMtx достаём смещение
    // указателя на CCamera внутри менеджера (+0x4A8 на момент написания), а из экспортов
    // renderer_x64_rwdi.dll (CCamera::GetCombinedMatrix / GetPosition / GetBaseX..Z) —
    // смещения полей камеры. Так мод переживает обновления игры без правки адресов.
    struct EngineCameraInfo {
        bool resolved = false;
        ULONGLONG lastTry = 0;
        uintptr_t managerPtrAddr = 0;      // &gui::CManager::s_Instance
        uint32_t camOffsets[2] = { 0x4A8, 0x4B0 };
        int camOffsetCount = 2;
        uint32_t offPos = 0x4C, offBaseX = 0x40, offBaseY = 0x44, offBaseZ = 0x48;
        uint32_t offProj = 0x80, offComb = 0xC0;
        uintptr_t levelPtrAddr = 0;        // &CoContactSwitch::s_DeactivatableLevel (CLevel*)
        uintptr_t playerPosAddr = 0;       // &CoEnviroContext::s_LastBuildingTestPos (позиция игрока)
        uintptr_t xformRegistryAddr = 0;   // глобальная таблица хэндлов компонентов (из CEntity::GetXformComponent)
        uint32_t entXformOff = 0x58;       // CEntity + 0x58 = хэндл CoWorldXform
    } engineCam;
    char camDiag[200] = "";

    // Перехват входа в ILevel::GetActiveCamera / ILevel::GetViewCamera (экспорты движка):
    // запоминаем rcx = ILevel* (активный уровень), дальше камеру берём той же цепочкой, что и движок.
    // Пролог обеих функций: 40 53 / 48 83 EC 20 (push rbx; sub rsp,20h) — 6 байт без относительной адресации.
    //   mov [rip+0x79], rcx   ; -> параметр пещеры (+0x80)
    //   push rbx; sub rsp,20h ; оригинал
    CodeCavePatch caveLevelCam1, caveLevelCam2;
    bool levelHooksTried = false;



    uintptr_t FindRemoteExport(uintptr_t modBase, const char* name) {
        if (!hProcess || !modBase) return 0;
        IMAGE_DOS_HEADER dos = Read<IMAGE_DOS_HEADER>(modBase);
        if (dos.e_magic != IMAGE_DOS_SIGNATURE) return 0;
        IMAGE_NT_HEADERS64 nt = Read<IMAGE_NT_HEADERS64>(modBase + dos.e_lfanew);
        if (nt.Signature != IMAGE_NT_SIGNATURE) return 0;
        const IMAGE_DATA_DIRECTORY& dir = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!dir.VirtualAddress || !dir.Size) return 0;
        IMAGE_EXPORT_DIRECTORY ed = Read<IMAGE_EXPORT_DIRECTORY>(modBase + dir.VirtualAddress);
        if (!ed.NumberOfNames || ed.NumberOfNames > 200000) return 0;

        std::vector<DWORD> names(ed.NumberOfNames), funcs(ed.NumberOfFunctions);
        std::vector<WORD> ords(ed.NumberOfNames);
        ReadProcessMemory(hProcess, (LPCVOID)(modBase + ed.AddressOfNames), names.data(), names.size() * sizeof(DWORD), nullptr);
        ReadProcessMemory(hProcess, (LPCVOID)(modBase + ed.AddressOfNameOrdinals), ords.data(), ords.size() * sizeof(WORD), nullptr);
        ReadProcessMemory(hProcess, (LPCVOID)(modBase + ed.AddressOfFunctions), funcs.data(), funcs.size() * sizeof(DWORD), nullptr);

        // таблица имён отсортирована -> двоичный поиск (~16 чтений вместо 45 000)
        int lo = 0, hi = (int)ed.NumberOfNames - 1;
        char buf[256];
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            std::memset(buf, 0, sizeof(buf));
            ReadProcessMemory(hProcess, (LPCVOID)(modBase + names[mid]), buf, sizeof(buf) - 1, nullptr);
            int cmp = std::strcmp(buf, name);
            if (cmp == 0) {
                WORD o = ords[mid];
                if (o >= funcs.size()) return 0;
                return modBase + funcs[o];
            }
            if (cmp < 0) lo = mid + 1; else hi = mid - 1;
        }
        return 0;
    }

    // "48 8D 81 imm32" (lea rax,[rcx+imm]) / "8B 41 imm8" / "8B 81 imm32" -> imm
    static bool DecodeFieldOffset(const uint8_t* code, size_t len, uint32_t& out) {
        for (size_t i = 0; i + 3 <= len; ++i) {
            if (i + 7 <= len && code[i] == 0x48 && code[i + 1] == 0x8D && code[i + 2] == 0x81) { std::memcpy(&out, code + i + 3, 4); return true; }
            if (code[i] == 0x8B && code[i + 1] == 0x41) { out = code[i + 2]; return true; }
            if (i + 6 <= len && code[i] == 0x8B && code[i + 1] == 0x81) { std::memcpy(&out, code + i + 2, 4); return true; }
        }
        return false;
    }

    void ResolveEngineCamera() {
        engineCam.lastTry = GetTickCount64();
        if (engineBase == 0) GetModuleInfo(processId, L"engine_x64_rwdi.dll", engineBase, engineSize);
        if (engineBase == 0) return;

        uintptr_t sInstance = FindRemoteExport(engineBase, "?s_Instance@CManager@gui@@0PEAV12@EA");
        uintptr_t getCamMtx = FindRemoteExport(engineBase, "?GetCameraMtx@IManager@gui@@SAAEBVmtx34@@W4EWindowType@2@@Z");
        if (getCamMtx) {
            uint8_t code[0x30] = {};
            ReadProcessMemory(hProcess, (LPCVOID)getCamMtx, code, sizeof(code), nullptr);
            for (int i = 0; i + 5 <= (int)sizeof(code); ++i) {
                if (!sInstance && i + 7 <= (int)sizeof(code) && code[i] == 0x48 && code[i + 1] == 0x8B && code[i + 2] == 0x0D) {
                    int32_t rel; std::memcpy(&rel, code + i + 3, 4);
                    sInstance = getCamMtx + i + 7 + rel;
                }
                if (code[i] == 0xE8) {
                    int32_t rel; std::memcpy(&rel, code + i + 1, 4);
                    uintptr_t callee = getCamMtx + i + 5 + rel;
                    uint8_t c2[0x30] = {};
                    ReadProcessMemory(hProcess, (LPCVOID)callee, c2, sizeof(c2), nullptr);
                    uint32_t found[4]; int n = 0;
                    for (int j = 0; j + 8 <= (int)sizeof(c2) && n < 4; ++j)
                        if (c2[j] == 0x48 && c2[j + 1] == 0x8B && c2[j + 2] == 0x81 && c2[j + 7] == 0xC3) { std::memcpy(&found[n], c2 + j + 3, 4); ++n; }
                    if (n >= 2) { engineCam.camOffsets[0] = found[n - 1]; engineCam.camOffsets[1] = found[0]; engineCam.camOffsetCount = 2; }
                    else if (n == 1) { engineCam.camOffsets[0] = found[0]; engineCam.camOffsetCount = 1; }
                    break;
                }
            }
        }

        uintptr_t renBase = 0; DWORD renSize = 0;
        if (GetModuleInfo(processId, L"renderer_x64_rwdi.dll", renBase, renSize)) {
            struct { const char* name; uint32_t* dst; } fields[] = {
                { "?GetCombinedMatrix@CCamera@@UEBAAEBVmtx44@@XZ", &engineCam.offComb },
                { "?GetProjectionMatrix@CCamera@@QEBAAEBVmtx44@@XZ", &engineCam.offProj },
                { "?GetPosition@CCamera@@QEBA?BVvec3@@XZ", &engineCam.offPos },
                { "?GetBaseX@CCamera@@QEBA?BVvec3@@XZ", &engineCam.offBaseX },
                { "?GetBaseY@CCamera@@QEBA?BVvec3@@XZ", &engineCam.offBaseY },
                { "?GetBaseZ@CCamera@@QEBA?BVvec3@@XZ", &engineCam.offBaseZ },
            };
            for (auto& f : fields) {
                uintptr_t fn = FindRemoteExport(renBase, f.name);
                if (!fn) continue;
                uint8_t code[0x10] = {};
                ReadProcessMemory(hProcess, (LPCVOID)fn, code, sizeof(code), nullptr);
                uint32_t off = 0;
                if (DecodeFieldOffset(code, sizeof(code), off) && off < 0x400) *f.dst = off;
            }
        }

        engineCam.levelPtrAddr = FindRemoteExport(engineBase, "?s_DeactivatableLevel@CoContactSwitch@@0PEAVCLevel@@EA");
        engineCam.playerPosAddr = FindRemoteExport(engineBase, "?s_LastBuildingTestPos@CoEnviroContext@@0Vvec3@@A");
        // CEntity::GetXformComponent: mov rdx,[rcx+58h]; test; je; mov rcx,[rip+REG]; jmp Resolve
        if (uintptr_t gx = FindRemoteExport(engineBase, "?GetXformComponent@CEntity@cbs@@UEBAPEBVICoWorldXform@@XZ")) {
            uint8_t c[0x20] = {};
            ReadProcessMemory(hProcess, (LPCVOID)gx, c, sizeof(c), nullptr);
            if (c[0] == 0x48 && c[1] == 0x8B && c[2] == 0x51) engineCam.entXformOff = c[3];
            for (int i = 0; i + 7 <= (int)sizeof(c); ++i)
                if (c[i] == 0x48 && c[i + 1] == 0x8B && c[i + 2] == 0x0D) { int32_t rel; std::memcpy(&rel, c + i + 3, 4); engineCam.xformRegistryAddr = gx + i + 7 + rel; break; }
        }
        engineCam.managerPtrAddr = sInstance;
        engineCam.resolved = (sInstance != 0);
    }

    // Если прошлый запуск трейнера закрылся без восстановления патчей, функция уже начинается с
    // jmp в его пещеру (пещера жива — она в памяти игры). Раньше такой пролог считался "обновлением
    // игры", хук не ставился и уровень (а с ним камера и погода) не находился. Теперь, если пещера —
    // ровно наш код (mov [rip+79h],rcx + исходный пролог), берём её: параметр = пещера+0x80,
    // при выходе пишем обратно исходные 6 байт.
    bool AdoptLevelCameraHook(uintptr_t fn, const std::vector<uint8_t>& current, CodeCavePatch& cave, const uint8_t expectProlog[6]) {
        int32_t rel; std::memcpy(&rel, current.data() + 1, 4);
        const uintptr_t target = fn + 5 + (intptr_t)rel;
        uint8_t code[13] = {};
        if (!ReadProcessMemory(hProcess, (LPCVOID)target, code, sizeof(code), nullptr)) return false;
        static const uint8_t ours[7] = { 0x48, 0x89, 0x0D, 0x79, 0x00, 0x00, 0x00 };
        if (std::memcmp(code, ours, 7) != 0 || std::memcmp(code + 7, expectProlog, 6) != 0) return false;
        cave = CodeCavePatch{};
        cave.targetAddress = fn;
        cave.caveAddress = target;
        cave.paramAddress = target + 0x80;
        cave.origBytes.assign(expectProlog, expectProlog + 6);
        cave.jmpBytes = current;
        cave.isEnabled = true;
        return true;
    }

    // true -> Fx::gameCam заполнена из движка
    char levelHookDiag[160] = "not tried";
    ULONGLONG levelHooksLastTry = 0;

    void SetupLevelCameraHooks() {
        levelHooksTried = true;
        levelHooksLastTry = GetTickCount64();
        if (engineBase == 0) GetModuleInfo(processId, L"engine_x64_rwdi.dll", engineBase, engineSize);
        if (!engineBase) { snprintf(levelHookDiag, sizeof(levelHookDiag), "engine module not found"); return; }
        const char* fnNames[2] = { "?GetActiveCamera@ILevel@@UEBAPEAVIBaseCamera@@XZ", "?GetViewCamera@ILevel@@QEBAPEAVIBaseCamera@@XZ" };
        CodeCavePatch* caves[2] = { &caveLevelCam1, &caveLevelCam2 };
        const uint8_t expect[6] = { 0x40, 0x53, 0x48, 0x83, 0xEC, 0x20 };
        int len = 0;
        for (int i = 0; i < 2; ++i) {
            if (caves[i]->paramAddress) { len += snprintf(levelHookDiag + len, sizeof(levelHookDiag) - len, "h%d ok ", i); continue; }
            uintptr_t fn = FindRemoteExport(engineBase, fnNames[i]);
            if (!fn) { len += snprintf(levelHookDiag + len, sizeof(levelHookDiag) - len, "h%d no export ", i); continue; }
            std::vector<uint8_t> orig(6);
            if (!ReadProcessMemory(hProcess, (LPCVOID)fn, orig.data(), 6, nullptr)) { len += snprintf(levelHookDiag + len, sizeof(levelHookDiag) - len, "h%d read fail ", i); continue; }
            if (orig[0] == 0xE9) {   // хук прошлой сессии — подхватываем (пул пещер для этого не нужен)
                bool ok = AdoptLevelCameraHook(fn, orig, *caves[i], expect);
                len += snprintf(levelHookDiag + len, sizeof(levelHookDiag) - len, ok ? "h%d adopted " : "h%d foreign jmp ", i);
                continue;
            }
            if (std::memcmp(orig.data(), expect, 6) != 0) { len += snprintf(levelHookDiag + len, sizeof(levelHookDiag) - len, "h%d prolog %02X%02X ", i, orig[0], orig[1]); continue; } // обновление игры — не трогаем
            if (!engineCavePool) { len += snprintf(levelHookDiag + len, sizeof(levelHookDiag) - len, "h%d no cave pool ", i); continue; }
            len += snprintf(levelHookDiag + len, sizeof(levelHookDiag) - len, "h%d installed ", i);
            std::vector<uint8_t> sc = { 0x48, 0x89, 0x0D, 0x79, 0x00, 0x00, 0x00 };
            sc.insert(sc.end(), orig.begin(), orig.end());
            // буфер = 6 байт оригинала с базой = адрес функции (rva 0)
            if (SetupCaveEx(orig, fn, engineCavePool, engineCaveOffset, *caves[i], fn, 6, sc, 0.0f)) {
                Write<uint64_t>(caves[i]->paramAddress, 0);
                SetCaveState(*caves[i], true);
            }
        }
    }

    // ==================== ПОГОДА: TimeWeather::CSystem движка ====================
    // Всё взято из экспортов engine_x64_rwdi.dll (дизассемблированы, вызывать ничего не нужно):
    //   SetForcedWeather(EWeather)  : mov [rcx+68h],edx        ClearForcedWeather: mov [rcx+68h],-1
    //   GetCurrentWeather()         : p = [rcx+78h] ?: [rcx+70h]; return p ? [p+14h] : -1
    //   GetAllowedWeathers()        : lea rax,[rcx+188h]  (ttl::array<bool,10>)
    //   ILevel::GetTimeWeatherSystem: [[ILevel+38h]+B8h] -> vfunc mov rax,[rcx+3808h]  =>  [[ILevel+38h]+38C0h]
    // ILevel берётся из уже существующих перехватов камеры (caveLevelCam1/2).
    // Порядок EWeather: 0 Foggy, 1 Clear, 2 Overcast, 3 Cloudy, 4 Rainy, 5 Stormy, 6..9 снег/wildcard
    // (по таблице переходов engine+0xDD68A0; список is_clear.. — это флаги варлистов, а не порядок enum).
    static const int kWeatherCount = 10;
    struct WeatherState {
        uintptr_t sys = 0;
        int current = -1, forced = -1;
        bool allowed[kWeatherCount] = {};
        bool weOwnForced = false;            // форсированную погоду поставил трейнер — при выключении вернём игре
        float daytime = -1.0f;               // время слота текущего состояния (начало слота), часы
        // Слоты времени суток (как в engine+0xDD5D50): форсированная погода применяется,
        // только если в текущем слоте есть пресеты этой погоды — иначе движок её молча игнорирует.
        static const int kMaxScopes = 32;
        int scopeCount = 0, currentScope = -1;
        float scopeStart[kMaxScopes] = {};
        uint8_t scopePresets[kMaxScopes][kWeatherCount] = {};
    } weather;

    // Упакованный счётчик ttl::vector движка: старший байт указателя (+7) = count+1, иначе dword после указателя
    static int PackedCount(const uint8_t* entry) {
        uint8_t packed = entry[7];
        if (packed) return packed - 1;
        int32_t n; std::memcpy(&n, entry + 8, 4);
        return n;
    }

    bool WeatherAvailableNow(int w) {
        if (w < 0 || w >= kWeatherCount || weather.currentScope < 0) return false;
        return weather.scopePresets[weather.currentScope][w] != 0;
    }

    static bool ValidWeather(int v) { return v >= -1 && v < kWeatherCount; }

    char weatherDiag[256] = "";

    uintptr_t ResolveWeatherSystem() {
        // уровень нужен только через перехваты ILevel — от камеры интерфейса (engineCam.resolved) не зависим
        if (!engineCam.resolved && GetTickCount64() - engineCam.lastTry > 3000) ResolveEngineCamera();
        const bool haveHook = caveLevelCam1.paramAddress || caveLevelCam2.paramAddress;
        if (!haveHook && GetTickCount64() - levelHooksLastTry > 3000) SetupLevelCameraHooks();
        uintptr_t levels[3] = {
            caveLevelCam1.paramAddress ? Read<uintptr_t>(caveLevelCam1.paramAddress) : 0,
            caveLevelCam2.paramAddress ? Read<uintptr_t>(caveLevelCam2.paramAddress) : 0,
            engineCam.levelPtrAddr ? Read<uintptr_t>(engineCam.levelPtrAddr) : 0 };
        int len = snprintf(weatherDiag, sizeof(weatherDiag), "hooks: %s| ", levelHookDiag);
        for (int k = 0; k < 3; ++k) {
            uintptr_t lvl = levels[k];
            uintptr_t inner = ValidPtr(lvl) ? Read<uintptr_t>(lvl + 0x38) : 0;
            uintptr_t sys = ValidPtr(inner) ? Read<uintptr_t>(inner + 0x38C0) : 0;
            // У CSystem нет vtable (первое поле — упакованный указатель с тегом), поэтому проверяем
            // то, чем пользуемся: поле форсированной погоды и указатель на текущее состояние погоды.
            int forced = ValidPtr(sys) ? Read<int>(sys + 0x68) : -99;
            uintptr_t state = ValidPtr(sys) ? Read<uintptr_t>(sys + 0x78) : 0;
            if (!ValidPtr(state) && ValidPtr(sys)) state = Read<uintptr_t>(sys + 0x70);
            int cur = ValidPtr(state) ? Read<int>(state + 0x14) : -99;
            if (len < (int)sizeof(weatherDiag) - 56)
                len += snprintf(weatherDiag + len, sizeof(weatherDiag) - len, "L%d %llX>%llX>%llX f%d c%d ", k,
                                (unsigned long long)lvl, (unsigned long long)inner, (unsigned long long)sys, forced, cur);
            if (!ValidPtr(sys) || !ValidWeather(forced) || !ValidPtr(state) || cur < 0 || cur >= kWeatherCount) continue;
            return sys;
        }
        return 0;
    }

    // Каждый кадр, пока открыта вкладка погоды или включено управление
    void UpdateWeather(bool enabled, int preset) {
        uintptr_t sys = ResolveWeatherSystem();
        weather.sys = sys;
        if (!sys) { weather.current = weather.forced = -1; return; }

        uintptr_t cur = Read<uintptr_t>(sys + 0x78);
        if (!ValidPtr(cur)) cur = Read<uintptr_t>(sys + 0x70);
        weather.current = ValidPtr(cur) ? Read<int>(cur + 0x14) : -1;
        if (!ValidWeather(weather.current)) weather.current = -1;
        weather.daytime = ValidPtr(cur) ? Read<float>(cur + 0x10) : -1.0f;   // движок берёт слот по этому времени

        // таблица слотов: [sys+90h] (48 бит) — массив по 0xF8 байт; погода w — ttl::vector в слоте по w*0x18 + 0x10
        weather.scopeCount = 0; weather.currentScope = -1;
        uint8_t hdr[16] = {};
        if (ReadProcessMemory(hProcess, (LPCVOID)(sys + 0x90), hdr, sizeof(hdr), nullptr)) {
            uintptr_t scopes; std::memcpy(&scopes, hdr, 8); scopes &= 0xFFFFFFFFFFFFull;
            int count = PackedCount(hdr);
            if (ValidPtr(scopes) && count > 0 && count <= WeatherState::kMaxScopes) {
                std::vector<uint8_t> blk((size_t)count * 0xF8);
                if (ReadProcessMemory(hProcess, (LPCVOID)scopes, blk.data(), blk.size(), nullptr)) {
                    weather.scopeCount = count;
                    for (int i = 0; i < count; ++i) {
                        const uint8_t* sc = blk.data() + (size_t)i * 0xF8;
                        std::memcpy(&weather.scopeStart[i], sc, 4);
                        for (int w = 0; w < kWeatherCount; ++w) {
                            int n = PackedCount(sc + w * 0x18 + 0x10);
                            weather.scopePresets[i][w] = (uint8_t)(n > 0 ? (n > 255 ? 255 : n) : 0);
                        }
                    }
                    // текущий слот: последний, чей старт <= времени (до первого старта — последний слот, через полночь)
                    if (weather.daytime >= 0.0f) {
                        weather.currentScope = count - 1;
                        for (int i = 0; i < count; ++i) if (weather.scopeStart[i] <= weather.daytime) weather.currentScope = i;
                    }
                }
            }
        }
        weather.forced = Read<int>(sys + 0x68);
        uint8_t allowed[kWeatherCount] = {};
        ReadProcessMemory(hProcess, (LPCVOID)(sys + 0x188), allowed, sizeof(allowed), nullptr);
        for (int i = 0; i < kWeatherCount; ++i) weather.allowed[i] = allowed[i] != 0;

        if (enabled && preset >= 0 && preset < kWeatherCount) {
            if (weather.forced != preset) {
                Write<int>(sys + 0x68, preset);   // = SetForcedWeather(preset); переход движок делает сам
                weather.forced = preset;
            }
            weather.weOwnForced = true;
        }
        else if (weather.weOwnForced) {
            Write<int>(sys + 0x68, -1);           // = ClearForcedWeather(): погода снова у игры
            weather.forced = -1;
            weather.weOwnForced = false;
        }
    }

    void ReleaseWeather() {
        if (weather.sys && weather.weOwnForced) Write<int>(weather.sys + 0x68, -1);
        weather.weOwnForced = false;
    }

    // Точная мировая позиция врага, как в движке (cbs::CEntity::GetWorldPosition):
    //   HumanAI — это cbs::CComponent: сущность = [A+0x10]; хэндл трансформа = [сущность+0x58];
    //   указатель = [[REG]+0] + (uint32)(хэндл << 4), проверка [ptr+8] == хэндл (как в движке);
    //   мировая матрица mtx34 лежит в ptr+0x58 (основной поток) / ptr+0xB8 (копия рендера).
    uint32_t entPosOk = 0, entPosFail = 0, entPosViaReg = 0, entPosViaPtr = 0;
    std::map<uintptr_t, uint32_t> xformPtrOffByClass; // vtable HumanAI -> смещение прямого указателя на трансформ (0xFFFFFFFF = нет)

    static bool ReadMatrixAt(uintptr_t ptr, Vec3& out) {
        const uint32_t mats[2] = { 0x58, 0xB8 };   // основной поток / копия рендера
        for (uint32_t mo : mats) {
            float m[12];
            if (ReadProcessMemory(hProcess, (LPCVOID)(ptr + mo), m, sizeof(m), nullptr) && PlausibleMatrix(m)) {
                out = Fx::V(m[3], m[7], m[11]);
                return true;
            }
        }
        return false;
    }

    bool ReadEntityWorldPos(uintptr_t A, Vec3& out) {
        if (!ValidPtr(A)) return false;
        uintptr_t ent = Read<uintptr_t>(A + 0x10);
        if (!ValidPtr(ent)) return false;
        uint64_t h = Read<uint64_t>(ent + engineCam.entXformOff);
        if (!h) return false;

        // 1) таблица хэндлов движка
        if (engineCam.xformRegistryAddr) {
            uintptr_t R = Read<uintptr_t>(engineCam.xformRegistryAddr);
            if (ValidPtr(R)) {
                uintptr_t base = Read<uintptr_t>(R);
                uint64_t limit = Read<uint64_t>(R + 8);
                uint32_t off = (uint32_t)(h << 4);
                if (base && off < limit) {
                    uintptr_t ptr = base + off;
                    if (Read<uint64_t>(ptr + 8) == h && ReadMatrixAt(ptr, out)) { ++entPosViaReg; return true; }
                }
            }
        }

        // 2) прямой указатель на трансформ внутри HumanAI (в этой версии игры — AI+0x1D28).
        //    Смещение ищется один раз на класс: указатель P, у которого [P+8] == хэндл трансформа.
        uintptr_t vt = Read<uintptr_t>(A);
        auto it = xformPtrOffByClass.find(vt);
        if (it == xformPtrOffByClass.end()) {
            uint32_t found = 0xFFFFFFFF;
            uintptr_t quick = Read<uintptr_t>(A + 0x1D28);
            if (ValidPtr(quick) && Read<uint64_t>(quick + 8) == h) found = 0x1D28;
            else {
                std::vector<uint8_t> blk(0x2400, 0);
                if (ReadProcessMemory(hProcess, (LPCVOID)A, blk.data(), blk.size(), nullptr))
                    for (uint32_t o = 0x18; o + 8 <= blk.size(); o += 8) {
                        uintptr_t P; std::memcpy(&P, blk.data() + o, 8);
                        if (ValidPtr(P) && Read<uint64_t>(P + 8) == h) { found = o; break; }
                    }
            }
            it = xformPtrOffByClass.emplace(vt, found).first;
        }
        if (it->second != 0xFFFFFFFF) {
            uintptr_t ptr = Read<uintptr_t>(A + it->second);
            if (ValidPtr(ptr) && Read<uint64_t>(ptr + 8) == h && ReadMatrixAt(ptr, out)) { ++entPosViaPtr; return true; }
        }
        return false;
    }

    // Кандидаты в игровую камеру (все — объекты CCamera из renderer_x64_rwdi.dll):
    //   gui0 / gui1 — gui::CManager::s_Instance + 0x4A8 / 0x4B0 (окна интерфейса; 0 — это 2D-камера HUD!)
    //   lvl0 / lvl1 — как ILevel::GetViewCamera: [[[[CLevel+0x38]+0x1380|0x1390]+0x320]+0x38]
    // Камера интерфейса не повёрнута вообще (оси строго по X/Y/Z) — такие отбрасываем.
    // Из остальных берём ближайшую к игроку (позиция игрока — экспорт движка).
    bool UpdateEngineCamera(const Vec3* playerPos) {
        if (!engineCam.resolved) {
            if (GetTickCount64() - engineCam.lastTry > 3000) ResolveEngineCamera();
            if (!engineCam.resolved) return false;
        }
        uintptr_t cands[8] = {}; const char* names[8] = {}; int n = 0;
        uintptr_t mgr = Read<uintptr_t>(engineCam.managerPtrAddr);
        if (ValidPtr(mgr))
            for (int k = 0; k < engineCam.camOffsetCount && n < 6; ++k) { cands[n] = Read<uintptr_t>(mgr + engineCam.camOffsets[k]); names[n++] = k == 0 ? "gui0" : "gui1"; }
        if (!levelHooksTried) SetupLevelCameraHooks();
        uintptr_t levels[3] = {
            caveLevelCam1.paramAddress ? Read<uintptr_t>(caveLevelCam1.paramAddress) : 0,
            caveLevelCam2.paramAddress ? Read<uintptr_t>(caveLevelCam2.paramAddress) : 0,
            engineCam.levelPtrAddr ? Read<uintptr_t>(engineCam.levelPtrAddr) : 0 };
        static const char* lvlNames[3][2] = { { "act0", "act1" }, { "view0", "view1" }, { "lvl0", "lvl1" } };
        for (int L = 0; L < 3; ++L) {
            if (L == 1 && levels[1] == levels[0]) continue;
            if (L == 2 && (levels[2] == levels[0] || levels[2] == levels[1])) continue;
            uintptr_t lvl = levels[L];
            uintptr_t inner = ValidPtr(lvl) ? Read<uintptr_t>(lvl + 0x38) : 0;
            if (!ValidPtr(inner)) continue;
            const uint32_t offs[2] = { 0x1380, 0x1390 };
            for (int k = 0; k < 2 && n < 8; ++k) {
                uintptr_t x = Read<uintptr_t>(inner + offs[k]);
                uintptr_t ibc = ValidPtr(x) ? Read<uintptr_t>(x + 0x320) : 0;
                uintptr_t cc = ValidPtr(ibc) ? Read<uintptr_t>(ibc + 0x38) : 0;
                if (ValidPtr(cc)) { bool dup = false; for (int q = 0; q < n; ++q) if (cands[q] == cc) dup = true; if (!dup) { cands[n] = cc; names[n++] = lvlNames[L][k]; } }
            }
        }

        Fx::Camera best; float bestDist = FLT_MAX; bool any = false; const char* bestName = "-";
        Fx::Camera far_; float farDist = FLT_MAX; bool anyFar = false; const char* farName = "-";
        char info[200]; int il = 0; info[0] = 0;
        for (int k = 0; k < n; ++k) {
            uintptr_t cam = cands[k];
            if (!ValidPtr(cam)) continue;
            uint8_t blk[0x200] = {};
            if (!ReadProcessMemory(hProcess, (LPCVOID)cam, blk, sizeof(blk), nullptr)) continue;
            auto f = [&](uint32_t off) { float v; std::memcpy(&v, blk + (off & 0x1FF), 4); return v; };
            // поля CCamera хранятся "по столбцам" матрицы 3x4: x, затем +0x10 y, +0x20 z
            auto col = [&](uint32_t off) { return Fx::V(f(off), f(off + 0x10), f(off + 0x20)); };
            if (engineCam.offComb + 64 > sizeof(blk) || engineCam.offProj + 64 > sizeof(blk) || engineCam.offPos + 0x24 > sizeof(blk)) continue;
            Vec3 pos = col(engineCam.offPos), bx = col(engineCam.offBaseX), by = col(engineCam.offBaseY), bz = col(engineCam.offBaseZ);
            float comb[16], proj[16];
            std::memcpy(comb, blk + engineCam.offComb, sizeof(comb));
            std::memcpy(proj, blk + engineCam.offProj, sizeof(proj));
            if (!Fx::Finite(pos) || std::fabs(pos.x) > 100000.0f || std::fabs(pos.z) > 100000.0f) continue;
            bool axisAligned = std::fabs(bx.y) < 1e-4f && std::fabs(bx.z) < 1e-4f && std::fabs(by.x) < 1e-4f &&
                               std::fabs(by.z) < 1e-4f && std::fabs(bz.x) < 1e-4f && std::fabs(bz.y) < 1e-4f;
            float dist = playerPos ? Fx::Len(Fx::Sub(pos, *playerPos)) : 0.0f;
            if (il < 180) il += snprintf(info + il, sizeof(info) - il, "%s(%.0f,%.0f,%.0f)%s ", names[k], pos.x, pos.y, pos.z, axisAligned ? "UI" : "");
            if (axisAligned) continue;                      // камера интерфейса
            Fx::Camera c = Fx::gameCam;
            Fx::SetViewport(c);
            if (!Fx::SetupEngineCamera(c, comb, proj, pos, bx, by, bz)) continue;
            if (playerPos && dist > 25.0f) {                // далеко от игрока — только как запасной вариант
                if (dist < farDist) { farDist = dist; far_ = c; anyFar = true; farName = names[k]; }
                continue;
            }
            if (dist < bestDist) { bestDist = dist; best = c; any = true; bestName = names[k]; }
        }
        if (!any && anyFar) { best = far_; any = true; bestName = farName; }
        snprintf(camDiag, sizeof(camDiag), "use %s | %s", any ? bestName : "none", info);
        if (any) Fx::gameCam = best;
        return any;
    }

    // Выбор врага, по которому пришёлся удар: сначала по сохранённой связке
    // "объект здоровья -> враг", иначе ближайший к прицелу живой враг перед игроком.
    struct HealthLink { uintptr_t ai; ULONGLONG tick; };

    // Точный владелец объекта здоровья по сохранённым в стеке регистрам вызывающего:
    // кандидат M считается HumanHealthModule, если [[M+0x18]] == health; тогда HumanAI = ([M+8] & mask) - 0x80.
    uint32_t exactLinks = 0, guessedLinks = 0;
    uintptr_t ExactOwnerFromStack(uintptr_t health, const uint8_t* entry) {
        for (int i = 0; i < 12; ++i) {
            uintptr_t M; std::memcpy(&M, entry + 0x18 + i * 8, 8);
            if (!ValidPtr(M) || M == health) continue;
            uintptr_t q = Read<uintptr_t>(M + 0x18);
            if (!ValidPtr(q) || Read<uintptr_t>(q) != health) continue;
            uintptr_t raw = Read<uintptr_t>(M + 8) & 0x1FFFFFFFFFFFFFFFULL;
            if (raw < 0x10080) continue;
            uintptr_t owner = raw - 0x80;
            if (!ValidPtr(owner) || !ValidPtr(Read<uintptr_t>(owner))) continue;
            return owner;
        }
        return 0;
    }

    // Состояние каждого объекта здоровья, по которому мы попадали
    struct HealthTrack {
        float maxOld = 0.0f;            // максимум HP, виденный у цели
        float lastNew = 1e9f;           // HP после последнего удара
        uintptr_t ai = 0;               // привязанный враг
        Vec3 lastWorld; bool hasWorld = false; // где был последний хитмаркер
        bool exact = false;             // владелец найден точно (по модулю здоровья)
        ULONGLONG lastHit = 0, killTick = 0;
        bool killed = false;
    };
    std::map<uintptr_t, HealthTrack> healthTracks;

    // Диагностика для меню (видно, почему эффект не появился)
    struct FxDiag {
        uint32_t hits = 0, kills = 0, killsEngine = 0, killsLowHp = 0, killsVanished = 0, killsNoPos = 0;
        uint32_t playerHits = 0, engineKillEvents = 0;
        struct Event { float oldHp, dmg; bool player, attacking, kill; ULONGLONG tick; };
        static const int kLog = 6;
        Event log[kLog] = {};
        uint32_t logPos = 0;
    } fxDiag;
    std::map<uintptr_t, HealthLink> healthLinks;

    uintptr_t ResolveHitTarget(uintptr_t health, ULONGLONG now, const Vec3* playerPos, bool allowDead) {
        auto link = healthLinks.find(health);
        if (link != healthLinks.end() && now - link->second.tick < 30000) {
            auto tr = Fx::trackedEntities.find(link->second.ai);
            if (tr != Fx::trackedEntities.end() && tr->second.hasPos && (allowDead || !tr->second.dead)) {
                link->second.tick = now;
                return link->second.ai;
            }
        }

        uintptr_t best = 0; float bestScore = FLT_MAX;
        const Fx::Camera& cam = Fx::gameCam;
        Vec3 origin = cam.valid ? cam.pos : (playerPos ? *playerPos : Fx::V(0, 0, 0));
        bool hasOrigin = cam.valid || playerPos != nullptr;
        ImVec2 center = g_GameView.Center();
        float halfH = g_GameView.h * 0.5f;

        for (auto& kv : Fx::trackedEntities) {
            const Fx::TrackedAIState& st = kv.second;
            if (st.dead || !st.hasPos || now - st.lastSeenTick > 1500) continue;
            bool linked = false;
            for (auto& l : healthLinks) if (l.second.ai == kv.first && l.first != health && now - l.second.tick < 30000) { linked = true; break; }
            if (!hasOrigin) { if (!best) best = kv.first; continue; }
            float dist = Fx::Len(Fx::Sub(st.lastPos, origin));
            if (dist > 80.0f) continue;
            float score;
            ImVec2 sp;
            Vec3 chest = Fx::V(st.lastPos.x, st.lastPos.y + MenuVars::hitHeightOffset, st.lastPos.z);
            if (cam.valid && Fx::Project(cam, chest, sp, nullptr, 1.0f)) {
                float dx = (sp.x - center.x) / halfH, dy = (sp.y - center.y) / halfH;
                score = std::sqrt(dx * dx + dy * dy) * 10.0f + dist * 0.12f;
            }
            else if (cam.valid) {
                score = 40.0f + dist; // за кадром: только если ничего лучше нет (удар ногой в упор и т.п.)
            }
            else {
                score = dist; // камеры нет вообще — просто ближайший
            }
            if (linked) score += 6.0f; // этот враг уже "занят" другим объектом здоровья
            if (score < bestScore) { bestScore = score; best = kv.first; }
        }
        if (best) healthLinks[health] = { best, now };
        return best;
    }

    void ProcessHitmarkerEvents(int screenW, int screenH) {
        (void)screenW; (void)screenH;
        if (!hProcess || !patternsInitialized) return;
        const HitmarkerConfig& hcfg = MenuVars::hitCfg;
        const KillFxConfig& kcfg = MenuVars::killCfg;
        if (!hcfg.enabled && !kcfg.enabled) return;

        ULONGLONG now = GetTickCount64();

        bool isLmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        bool isKickOrActionDown =
            ((GetAsyncKeyState('E') & 0x8000) != 0) || ((GetAsyncKeyState('F') & 0x8000) != 0) ||
            ((GetAsyncKeyState('V') & 0x8000) != 0) || ((GetAsyncKeyState('G') & 0x8000) != 0) ||
            ((GetAsyncKeyState('Q') & 0x8000) != 0) || ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) ||
            ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0) || ((GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0) ||
            ((GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0);
        if (isLmbDown || isKickOrActionDown) Fx::lastAttackInputTick = now;

        // --- игрок
        uintptr_t lp = 0;
        Vec3 playerPos = { 0.0f, 0.0f, 0.0f };
        Vec3 camForward = { 0.0f, 0.0f, 1.0f };
        bool hasPlayerPos = false;
        if (caveLogicalPlayer.paramAddress != 0) {
            lp = Read<uintptr_t>(caveLogicalPlayer.paramAddress);
            if (lp != 0) {
                hasPlayerPos = ReadEntityPosition(lp, playerPos);
                Vec3 fwdCandidate = Read<Vec3>(lp + 0x100);
                float fLen = std::sqrt(fwdCandidate.x * fwdCandidate.x + fwdCandidate.z * fwdCandidate.z);
                if (std::isfinite(fLen) && fLen > 0.2f && fLen < 1.5f) camForward = fwdCandidate;
            }
        }

        // позиция игрока из движка (надёжнее эвристики по смещениям)
        if (engineCam.playerPosAddr) {
            Vec3 pp = Read<Vec3>(engineCam.playerPosAddr);
            if (Fx::Finite(pp) && Fx::Len(pp) > 1.0f && Fx::Len(pp) < 100000.0f) { playerPos = pp; hasPlayerPos = true; }
        }

        // --- камера: движок, иначе эвристика
        Fx::SetViewport(Fx::gameCam);
        if (UpdateEngineCamera(hasPlayerPos ? &playerPos : nullptr)) {
            Fx::cameraSource = 1;
        }
        else if (hasPlayerPos) {
            Fx::SetupPinhole(Fx::gameCam, Fx::V(playerPos.x, playerPos.y + MenuVars::hitEyeHeight, playerPos.z), camForward, MenuVars::hitProjFov, MenuVars::hitInvertPitch);
            Fx::cameraSource = Fx::gameCam.valid ? 2 : 0;
        }
        else { Fx::gameCam.valid = false; Fx::cameraSource = 0; }

        // --- враги рядом (из AI-коллектора)
        if (caveAICollector.paramAddress != 0) {
            uint64_t rawPtrs[64] = {};   // [0..31] объекты ИИ, [32..63] компоненты (rbx)
            ReadProcessMemory(hProcess, (LPCVOID)(caveAICollector.paramAddress + 8), rawPtrs, sizeof(rawPtrs), nullptr);
            uint64_t zeros[64] = {};
            WriteProcessMemory(hProcess, (LPVOID)(caveAICollector.paramAddress + 8), zeros, sizeof(zeros), nullptr);
            int pairsThisFrame = 0;
            for (int i = 0; i < 32; ++i) {
                uintptr_t aiPtr = (uintptr_t)rawPtrs[i];
                uintptr_t compPtr = (uintptr_t)rawPtrs[32 + i];
                if (ValidPtr(aiPtr)) {
                    bool dup = false;
                    for (int k = 0; k < pairsThisFrame; ++k) if (lastAiPairs[k].ai == aiPtr) dup = true;
                    if (!dup && pairsThisFrame < 32) { if (pairsThisFrame == 0) lastAiPairCount = 0; lastAiPairs[pairsThisFrame++] = { aiPtr, ValidPtr(compPtr) ? compPtr : 0 }; lastAiPairCount = pairsThisFrame; }
                }
                if (aiPtr < 0x10000 || aiPtr > 0x00007FFFFFFFFFFFULL) continue;
                if (lp != 0 && aiPtr == lp) continue;
                auto& state = Fx::trackedEntities[aiPtr];
                if (!state.initialized) { state.initialized = true; state.type = ClassifyAIEntity(aiPtr); }
                state.lastSeenTick = now;
                Vec3 pos{};
                const Vec3* ref = hasPlayerPos ? &playerPos : (Fx::gameCam.valid ? &Fx::gameCam.pos : nullptr);
                if (ValidPtr(compPtr)) aiComponent[aiPtr] = compPtr;
                // 1) точный путь движка: компонент -> CEntity ([comp+0x10]) -> CoWorldXform ([+0x58]) -> матрица (+0x80)
                bool got = false;
                if (MenuVars::hitPosSourceKey) got = ReadPosByKey(aiPtr, MenuVars::hitPosSourceKey, pos);
                else { got = ReadEntityWorldPos(aiPtr, pos); if (got) ++entPosOk; else ++entPosFail; } // точный путь движка
                if (!got && !MenuVars::hitPosSourceKey && ValidPtr(compPtr)) {
                    uintptr_t ent = Read<uintptr_t>(compPtr - 0x28);
                    uintptr_t xf = ValidPtr(ent) ? Read<uintptr_t>(ent + 0x58) : 0;
                    float m[12];
                    if (xf && ReadXformMatrix(xf, m)) {
                        Vec3 t = Fx::V(m[3], m[7], m[11]);
                        float upY = m[5] / (Fx::Len(Fx::V(m[1], m[5], m[9])) + 1e-6f);
                        if (upY > 0.7f && (!ref || Fx::Len(Fx::Sub(t, *ref)) < 400.0f)) { pos = t; got = true; ++compPosOk; }
                    }
                    if (!got) ++compPosFail;
                }
                // 2) запасной: автопоиск пути внутри объекта ИИ
                if (!got && !MenuVars::hitPosSourceKey) got = ReadAIPosition(aiPtr, pos, ref);
                if (got) { state.lastPos = pos; state.hasPos = true; }
            }
        }

        // --- кольцевые буферы попаданий / убийств (см. шеллкод в InitSignatures)
        // Убийство определяется ТРЕМЯ независимыми способами (что сработает первым):
        //   1) движок: HP до удара > 0 и урон >= HP (кольцо убийств в пещере);
        //   2) "низкое HP": после удара осталось <= max(1, 1.5% от максимума, виденного у этой цели)
        //      — на случай, если игра режет смертельный урон или держит минимальный порог HP;
        //   3) "исчез": недобитый враг (<= 35% HP) после нашего удара пропал из AI-апдейта.
        // Повторное срабатывание для одного объекта здоровья подавляется на 3 с.
        if (caveHealthHitDetector.paramAddress != 0 && caveHealthHitDetector.isEnabled) {
            static uint32_t lastHits = 0, lastKills = 0;
            static uintptr_t syncedCave = 0;
            uint8_t blk[0xA90] = {};
            if (ReadProcessMemory(hProcess, (LPCVOID)caveHealthHitDetector.paramAddress, blk, sizeof(blk), nullptr)) {
                uint32_t hits, kills;
                std::memcpy(&hits, blk + 0x00, 4);
                std::memcpy(&kills, blk + 0x04, 4);
                if (syncedCave != caveHealthHitDetector.paramAddress || hits < lastHits || kills < lastKills) {
                    syncedCave = caveHealthHitDetector.paramAddress; lastHits = hits; lastKills = kills;
                }
                bool attacking = (now - Fx::lastAttackInputTick) <= 900;
                bool attackingLoose = (now - Fx::lastAttackInputTick) <= 1600; // DoT / стрелы долетают позже
                bool anyHit = false;
                int killStreakForSound = 0;

                auto doKill = [&](uintptr_t health, int source) {
                    HealthTrack& tr = healthTracks[health];
                    if (tr.killed && now - tr.killTick < 3000) return;
                    tr.killed = true; tr.killTick = now;
                    uintptr_t ai = tr.ai ? tr.ai : ResolveHitTarget(health, now, hasPlayerPos ? &playerPos : nullptr, true);
                    Fx::MarkKilled(Fx::game, health);
                    Vec3 feet; bool hasFeet = false;
                    if (ai) {
                        auto it = Fx::trackedEntities.find(ai);
                        if (it != Fx::trackedEntities.end()) {
                            it->second.dead = true;
                            if (it->second.hasPos) { feet = it->second.lastPos; hasFeet = true; }
                        }
                    }
                    if (hasFeet && !tr.exact) {
                        // цель угадана — показываем эффект, только если она близко и около прицела
                        ImVec2 sp; float depth = 0.0f;
                        Vec3 chestG = Fx::V(feet.x, feet.y + MenuVars::hitHeightOffset, feet.z);
                        bool nearCross = Fx::Project(Fx::gameCam, chestG, sp, &depth, 1.0f) &&
                            std::fabs(sp.x - g_GameView.Center().x) < g_GameView.h * 0.18f && std::fabs(sp.y - g_GameView.Center().y) < g_GameView.h * 0.22f && depth < 8.0f;
                        if (!nearCross) hasFeet = false;
                    }
                    if (!hasFeet && tr.hasWorld && tr.exact) { // позиция последнего хитмаркера по этой цели
                        feet = Fx::V(tr.lastWorld.x, tr.lastWorld.y - MenuVars::hitHeightOffset, tr.lastWorld.z);
                        hasFeet = true;
                    }
                    if (!hasFeet) ++fxDiag.killsNoPos;
                    if (source == 0) ++fxDiag.killsEngine; else if (source == 1) ++fxDiag.killsLowHp; else ++fxDiag.killsVanished;
                    ++fxDiag.kills;
                    killStreakForSound = Fx::SpawnKill(Fx::game, kcfg, hasFeet ? &feet : nullptr);
                };

                uint32_t newHits = hits - lastHits; if (newHits > 16) newHits = 16;
                for (uint32_t n = hits - newHits; n != hits; ++n) {
                    const uint8_t* e = blk + 0x10 + (n & 15) * 0x80;
                    uintptr_t health, ret; float dmg, oldHp;
                    std::memcpy(&health, e, 8); std::memcpy(&ret, e + 8, 8);
                    std::memcpy(&dmg, e + 16, 4); std::memcpy(&oldHp, e + 20, 4);
                    bool isPlayer = playerHealthCallRet && ret == playerHealthCallRet;
                    float newHp = oldHp - dmg;

                    // журнал для диагностики в меню
                    FxDiag::Event& ev = fxDiag.log[fxDiag.logPos++ % FxDiag::kLog];
                    ev = { oldHp, dmg, isPlayer, attacking, false, now };
                    if (isPlayer) { ++fxDiag.playerHits; continue; } // урон по самому игроку
                    if (!std::isfinite(oldHp) || !std::isfinite(dmg)) continue;

                    HealthTrack& tr = healthTracks[health];
                    if (tr.killed && now - tr.killTick > 3000 && oldHp > tr.lastNew + 1.0f) tr.killed = false; // объект переиспользован
                    if (oldHp > tr.maxOld) tr.maxOld = oldHp;

                    // точный владелец (по модулю здоровья из стека); если нашёлся — запоминаем связь
                    uintptr_t exactAi = ExactOwnerFromStack(health, e);
                    if (exactAi) {
                        healthLinks[health] = { exactAi, now };
                        tr.ai = exactAi; tr.exact = true;
                        auto& st = Fx::trackedEntities[exactAi];
                        if (!st.initialized) { st.initialized = true; st.type = ClassifyAIEntity(exactAi); }
                        Vec3 ep; const Vec3* ref = hasPlayerPos ? &playerPos : (Fx::gameCam.valid ? &Fx::gameCam.pos : nullptr);
                        bool okp = MenuVars::hitPosSourceKey ? ReadPosByKey(exactAi, MenuVars::hitPosSourceKey, ep) : (ReadEntityWorldPos(exactAi, ep) || ReadAIPosition(exactAi, ep, ref));
                        if (okp) { st.lastPos = ep; st.hasPos = true; st.lastSeenTick = now; }
                    }

                    if (dmg >= 3.5f && oldHp > 0.0f && attacking) {
                        ++fxDiag.hits;
                        uintptr_t ai = exactAi ? exactAi : ResolveHitTarget(health, now, hasPlayerPos ? &playerPos : nullptr, false);
                        if (exactAi) ++exactLinks; else ++guessedLinks;
                        Vec3 chest; bool hasChest = false;
                        if (ai) {
                            auto& st = Fx::trackedEntities[ai];
                            if (st.hasPos) { chest = Fx::V(st.lastPos.x, st.lastPos.y + MenuVars::hitHeightOffset, st.lastPos.z); hasChest = true; }
                            tr.ai = ai;
                        }
                        if (hasChest) { tr.lastWorld = chest; tr.hasWorld = true; }
                        float shown = (dmg < oldHp) ? dmg : oldHp; // не показываем "оверкилл" больше остатка HP
                        // без точной привязки маркер ставим в прицел, а не на угаданного врага
                        bool anchor = hasChest && exactAi != 0;
                        Fx::SpawnHit(Fx::game, hcfg, anchor ? ai : 0, health, anchor ? &chest : nullptr, g_GameView.Center(), shown);
                        anyHit = true;
                        tr.lastHit = now;
                    }
                    tr.lastNew = newHp;

                    float lowThreshold = (std::max)(1.0f, tr.maxOld * 0.015f);
                    if (oldHp > 0.0f && newHp <= lowThreshold && attackingLoose && tr.lastHit != 0 && now - tr.lastHit < 2000) {
                        ev.kill = true;
                        doKill(health, 1);
                    }
                }
                lastHits = hits;

                uint32_t newKills = kills - lastKills; if (newKills > 8) newKills = 8;
                for (uint32_t n = kills - newKills; n != kills; ++n) {
                    const uint8_t* e = blk + 0x810 + (n & 7) * 16;
                    uintptr_t health, ret;
                    std::memcpy(&health, e, 8); std::memcpy(&ret, e + 8, 8);
                    if (playerHealthCallRet && ret == playerHealthCallRet) continue; // смерть игрока — не "килл"
                    ++fxDiag.engineKillEvents;
                    if (!attackingLoose) continue;
                    doKill(health, 0);
                }
                lastKills = kills;

                // 3) "исчез": враг после нашего удара был жив (виден), а потом пропал из AI-апдейта
                for (auto& kv : healthTracks) {
                    HealthTrack& tr = kv.second;
                    if (tr.killed || !tr.ai || tr.lastHit == 0 || now - tr.lastHit > 4000) continue;
                    if (!(tr.maxOld > 0.0f) || tr.lastNew > tr.maxOld * 0.35f) continue;
                    auto it = Fx::trackedEntities.find(tr.ai);
                    if (it == Fx::trackedEntities.end()) continue;
                    if (now - it->second.lastSeenTick > 700 && it->second.lastSeenTick + 100 >= tr.lastHit) doKill(kv.first, 2);
                }

                // один звук на кадр: убийство важнее попадания
                if (killStreakForSound > 0 && kcfg.enabled && kcfg.sound > 0) Fx::PlayKillSound(kcfg, killStreakForSound);
                else if (anyHit && hcfg.enabled) Fx::PlayHitSound(hcfg);
            }
        }

        for (auto it = Fx::trackedEntities.begin(); it != Fx::trackedEntities.end(); ) {
            if (now - it->second.lastSeenTick > 6000) { aiComponent.erase(it->first); it = Fx::trackedEntities.erase(it); }
            else ++it;
        }
        for (auto it = healthLinks.begin(); it != healthLinks.end(); ) {
            if (now - it->second.tick > 30000) it = healthLinks.erase(it);
            else ++it;
        }
        for (auto it = healthTracks.begin(); it != healthTracks.end(); ) {
            ULONGLONG last = (std::max)(it->second.lastHit, it->second.killTick);
            if (now - last > 30000) it = healthTracks.erase(it);
            else ++it;
        }
    }

    // ==================== ОТЛАДОЧНЫЙ ДАМП (для поиска пути к позиции врага) ====================
    // Формат: "DLTBDMP2", затем float[3] позиция камеры, float[16] матрица камеры, u32 число пар,
    // пары (ai, comp) u64, затем блоки: u32 тег, u64 адрес, u32 размер, байты.
    // Теги: 1 = объект ИИ, 2 = компонент (comp-0x38), 3 = уровень 1, 4 = уровень 2, 5 = объект здоровья.

    bool SaveAiDebugDump(std::string& outPath) {
        if (!hProcess) return false;
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string dir(exePath);
        size_t slash = dir.find_last_of("\\/");
        dir = (slash == std::string::npos) ? std::string(".") : dir.substr(0, slash);
        outPath = dir + "\\ai_debug_dump.bin";
        FILE* f = nullptr;
        if (fopen_s(&f, outPath.c_str(), "wb") != 0 || !f) return false;

        fwrite("DLTBDMP2", 1, 8, f);
        float camData[19] = { Fx::gameCam.pos.x, Fx::gameCam.pos.y, Fx::gameCam.pos.z };
        std::memcpy(camData + 3, Fx::gameCam.m, sizeof(float) * 16);
        fwrite(camData, sizeof(camData), 1, f);
        uint32_t np = (uint32_t)lastAiPairCount;
        fwrite(&np, 4, 1, f);
        for (int i = 0; i < lastAiPairCount; ++i) { uint64_t a = lastAiPairs[i].ai, c = lastAiPairs[i].comp; fwrite(&a, 8, 1, f); fwrite(&c, 8, 1, f); }

        std::map<uintptr_t, bool> done;
        size_t totalBytes = 0;
        auto block = [&](uint32_t tag, uintptr_t addr, uint32_t size, std::vector<uint8_t>& buf) -> bool {
            buf.clear();
            if (addr < 0x10000 || addr > 0x00007FFFFFFFFFFFULL || done.count(addr) || totalBytes > 48u * 1024 * 1024) return false;
            done[addr] = true;
            buf.resize(size);
            SIZE_T got = 0;
            if (!ReadProcessMemory(hProcess, (LPCVOID)addr, buf.data(), size, &got) || got == 0) {
                // частичное чтение на границе страницы
                uint32_t part = (uint32_t)(0x1000 - (addr & 0xFFF));
                if (part >= size || !ReadProcessMemory(hProcess, (LPCVOID)addr, buf.data(), part, &got) || got == 0) { buf.clear(); return false; }
                buf.resize(part);
            }
            uint64_t a64 = addr; uint32_t sz = (uint32_t)buf.size();
            fwrite(&tag, 4, 1, f); fwrite(&a64, 8, 1, f); fwrite(&sz, 4, 1, f); fwrite(buf.data(), 1, sz, f);
            totalBytes += sz + 16;
            return true;
        };
        auto ptrsIn = [](const std::vector<uint8_t>& b, size_t limit) {
            std::vector<uintptr_t> out;
            for (size_t o = 0; o + 8 <= b.size() && o < limit; o += 8) {
                uintptr_t p; std::memcpy(&p, b.data() + o, 8);
                if (p >= 0x10000 && p < 0x00007FFFFFFFFFFFULL && (p & 7) == 0) out.push_back(p);
            }
            return out;
        };

        int roots = 0;
        std::vector<uint8_t> b0, b1, b2;
        for (int i = 0; i < lastAiPairCount && roots < 5; ++i) {
            ++roots;
            std::vector<std::pair<uint32_t, uintptr_t>> starts = { { 1u, lastAiPairs[i].ai } };
            if (lastAiPairs[i].comp) starts.push_back({ 2u, lastAiPairs[i].comp - 0x38 });
            for (auto& st : starts) {
                if (!block(st.first, st.second, st.first == 1 ? 0x2400 : 0x400, b0)) continue;
                for (uintptr_t p1 : ptrsIn(b0, b0.size())) {
                    if (!block(3, p1, 0x200, b1)) continue;
                    for (uintptr_t p2 : ptrsIn(b1, 0x100)) block(4, p2, 0x140, b2);
                }
            }
        }
        // объекты здоровья последних целей — чтобы связать здоровье с сущностью
        int hcount = 0;
        for (auto& kv : healthTracks) {
            if (hcount++ >= 6) break;
            if (!block(5, kv.first, 0x200, b0)) continue;
            for (uintptr_t p1 : ptrsIn(b0, 0x200)) block(3, p1, 0x200, b1);
        }
        fclose(f);
        return true;
    }

    bool TriggerWaypointTeleport() {
        if (!hProcess || caveLogicalPlayer.paramAddress == 0 || caveTeleportWaypoint.paramAddress == 0) return false;
        uintptr_t logicalPlayer = Read<uintptr_t>(caveLogicalPlayer.paramAddress);
        if (logicalPlayer == 0 || Read<uint8_t>(logicalPlayer + 0x854) == 0) return false;
        Write<float>(caveTeleportWaypoint.paramAddress + 0, Read<float>(logicalPlayer + 0x850));
        Write<float>(caveTeleportWaypoint.paramAddress + 4, Read<float>(logicalPlayer + 0x854) + 20.0f);
        Write<float>(caveTeleportWaypoint.paramAddress + 8, Read<float>(logicalPlayer + 0x858));
        ULONGLONG now = GetTickCount64();
        MenuVars::tpWaypointEndTime = now + 350;
        MenuVars::tempNoFallEndTime = now + 5000;
        return true;
    }

    void RestoreAllPatches() {
        if (!hProcess || !patternsInitialized) return;
        SetPatchState(patchGodMode1, false); SetPatchState(patchGodMode2, false);
        SetPatchState(patchStamina, false); SetPatchState(patchUVFlashlight, false);
        SetPatchState(patchNoFall1, false); SetPatchState(patchNoFall2, false);
        SetPatchState(patchKnockdown, false); SetPatchState(patchHitReaction, false);
        SetPatchState(patchUnlimitedJumps, false); SetPatchState(patchTripleJump, false);
        SetPatchState(patchVaultKick, false); SetPatchState(patchFloatInAir, false);
        SetPatchState(patchWaterPhysics, false); SetPatchState(patchPlayableArea1, false);
        SetPatchState(patchPlayableArea2, false); SetPatchState(patchIsOutOfMap, false);
        SetPatchState(patchTPTime, false); SetPatchState(patchPowerAttackBlock, false);
        SetPatchState(patchAlwaysPerfectBlock, false); SetPatchState(patchAutoBlock, false);
        SetPatchState(patchDurability, false); SetPatchState(patchUnlimitedRepairs, false);
        SetPatchState(patchOneHitKill, false); SetPatchState(patchEasyFinisher, false);
        SetPatchState(patchInstantFinisher1, false); SetPatchState(patchInstantFinisher2, false);
        SetPatchState(patchInstantFinisher3, false); SetPatchState(patchInstantFinisher4, false);
        SetPatchState(patchElemental1, false); SetPatchState(patchElemental2, false);
        SetPatchState(patchAmmo, false); SetPatchState(patchArrows, false);
        SetPatchState(patchSilenced1, false); SetPatchState(patchSilenced2, false);
        SetPatchState(patchDeathPenalty1, false); SetPatchState(patchSkipIsFury, false);
        SetPatchState(patchNoMoneyCost, false); SetMultiPatchState(patchFreeCraftAll, false);
        SetPatchState(patchFreezeTime, false); SetPatchState(patchWorldNoise1, false);
        SetPatchState(patchWorldNoise2, false); SetPatchState(patchDiscoverMap1, false);
        SetPatchState(patchDiscoverMap2, false); SetPatchState(patchDiscoverMap3, false);
        SetPatchState(patchAllowDoorOpening, false); SetPatchState(patchAutoUnlock, false);
        SetPatchState(patchInvisibility1, false); SetPatchState(patchDisableAIAwareness, false);
        SetPatchState(patchDisableZombieGrab, false); SetPatchState(patchDisableChase, false);
        SetPatchState(patchAlwaysStomp, false); SetPatchState(patchFreezeKillAI, false);
        SetPatchState(patchBurnAI, false); SetPatchState(patchDisableAISpawn1, false);
        SetPatchState(patchDisableAISpawn2, false); SetPatchState(patchDisableAISpawn3, false);
        SetPatchState(patchDisableAISpawn4, false); SetPatchState(patchDisableAISpawn5, false);
        SetPatchState(patchNoFuelUsage, false); SetPatchState(patchNoGallonUse, false);
        SetPatchState(patchNoRepairKit, false); SetPatchState(patchUnlimitedVehicleUV, false);
        SetPatchState(patchRemoveCarCamSnapFlag, false); SetPatchState(patchChallengeVehicle, false);
        SetPatchState(patchWeaponReturn, false); SetPatchState(patchInstantDismantle, false);
        SetPatchState(patchDisableActionEquip, false); SetPatchState(patchRightHandLoot, false);

        if (caveNoVehicleDamage.isEnabled && caveNoVehicleDamage.paramAddress) {
            uintptr_t dmgFlagPtr = Read<uintptr_t>(caveNoVehicleDamage.paramAddress);
            if (dmgFlagPtr) Write<uint8_t>(dmgFlagPtr, 0);
        }
        SetCaveState(caveNoVehicleDamage, false);

        if (caveToggleHUD.isEnabled && caveToggleHUD.paramAddress) {
            uintptr_t hudFlagPtr = Read<uintptr_t>(caveToggleHUD.paramAddress);
            if (hudFlagPtr) Write<uint8_t>(hudFlagPtr, 4);
        }
        SetCaveState(caveToggleHUD, false);

        SetCaveState(caveInstantUVRecharge, false); SetCaveState(caveNoUVCooldown, false);
        SetCaveState(caveRemoveCarCamSnap, false); SetCaveState(caveSellPrice, false);
        SetCaveState(caveBuyPrice, false); SetCaveState(caveDisableHeadbob, false);
        SetCaveState(caveAICollector, false); SetCaveState(caveHealthHitDetector, false);
        ReleaseWeather();   // форсированную погоду возвращаем игре до снятия перехватов уровня
        SetCaveState(caveLevelCam1, false); SetCaveState(caveLevelCam2, false);
        SetCaveState(caveFreeBlueprints, false); SetCaveState(caveMultiplyGathered, false);
        SetCaveState(caveNightExp, false); SetCaveState(caveOpenWorldExp, false);
        SetCaveState(caveDeathPenalty2, false); SetCaveState(caveDamageMult, false);
        SetCaveState(caveNoReload, false); SetCaveState(caveNoRecoil, false);
        SetCaveState(caveMegaAccuracy, false); SetCaveState(caveBowRapidFire, false);
        SetCaveState(caveMoveSpeed, false); SetCaveState(caveJumpHeight, false);
        SetCaveState(caveUnlimitedFury, false); SetCaveState(caveChangeAIScale, false);
        SetCaveState(caveUnlimitedCable, false); SetCaveState(caveDisableHookNerf, false);
        SetCaveState(caveSwimSpeed, false); SetCaveState(caveLogicalPlayer, false);
        SetCaveState(caveTeleportWaypoint, false); SetCaveState(caveForceDayTime1, false);
        SetCaveState(caveForceDayTime2, false); SetCaveState(caveForceDayPhase, false);
        SetCaveState(caveWorldSpeed, false); SetCaveState(caveRemoveFogOfWar, false);
        SetCaveState(caveEditZoneLevel, false); SetCaveState(caveRemoveDoorLock, false);
        SetCaveState(caveEasyLockpick, false);

        if (cavePool != 0) { VirtualFreeEx(hProcess, (LPVOID)cavePool, 0, MEM_RELEASE); cavePool = 0; }
        if (healthCavePage != 0) { VirtualFreeEx(hProcess, (LPVOID)healthCavePage, 0, MEM_RELEASE); healthCavePage = 0; }
        if (engineCavePool != 0) { VirtualFreeEx(hProcess, (LPVOID)engineCavePool, 0, MEM_RELEASE); engineCavePool = 0; }
    }

    bool AttachToGame() {
        if (hProcess && WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT) {
            if (!patternsInitialized) InitSignatures();
            if (!gameWindow || !IsWindow(gameWindow)) gameWindow = FindMainWindow(processId);
            wasEverConnected = true;
            return true;
        }
        if (hProcess) {
            CloseHandle(hProcess); hProcess = nullptr; gameWindow = nullptr;
            cavePool = 0; engineCavePool = 0; patternsInitialized = false;
            if (wasEverConnected) {
                shouldAutoExit = true;
                return false;
            }
        }
        static ULONGLONG lastCheck = 0; ULONGLONG now = GetTickCount64();
        if (now - lastCheck < 1000) return false;
        lastCheck = now;

        uintptr_t foundModBase = 0; DWORD foundModSize = 0;
        processId = FindGameProcess(L"DyingLightGame_x64_rwdi.exe", L"gamedll_ph_x64_rwdi.dll", foundModBase, foundModSize);
        if (processId == 0) { snprintf(statusMessage, sizeof(statusMessage), "Waiting Game..."); return false; }

        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess) hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (!hProcess) { snprintf(statusMessage, sizeof(statusMessage), "Run as Admin!"); return false; }

        moduleBase = foundModBase; moduleSize = foundModSize;
        gameWindow = FindMainWindow(processId);
        InitSignatures();
        wasEverConnected = true;
        return true;
    }
}

void ForceWindowFocus(HWND targetWindow) {
    HWND fgWindow = GetForegroundWindow();
    if (fgWindow != targetWindow) {
        DWORD fgThread = GetWindowThreadProcessId(fgWindow, nullptr);
        DWORD curThread = GetCurrentThreadId();
        if (fgThread != curThread) {
            AttachThreadInput(fgThread, curThread, TRUE);
            BringWindowToTop(targetWindow); SetForegroundWindow(targetWindow);
            SetFocus(targetWindow); SetActiveWindow(targetWindow);
            AttachThreadInput(fgThread, curThread, FALSE);
        }
        else { BringWindowToTop(targetWindow); SetForegroundWindow(targetWindow); }
    }
    ClipCursor(nullptr);
}

#include "item_menu.hpp"

// ==================== ТЕМА МЕНЮ ====================
// Classic — исходные цвета без изменений. Modern — стекло в стиле вотермарки,
// вся палитра виджетов выводится из 5 цветов: акцент, фон, панели, виджеты, рамки.
struct MenuPreset { const char* name; float accent[3], bg[3], panel[3], frame[3], border[3]; };
static const MenuPreset kMenuPresets[] = {
    { "Midnight (watermark)", { 0.30f, 0.49f, 1.00f }, { 0.043f, 0.051f, 0.075f }, { 0.065f, 0.075f, 0.105f }, { 0.085f, 0.098f, 0.135f }, { 0.150f, 0.180f, 0.250f } },
    { "Crimson Night",        { 0.86f, 0.12f, 0.16f }, { 0.055f, 0.040f, 0.045f }, { 0.085f, 0.060f, 0.066f }, { 0.110f, 0.078f, 0.085f }, { 0.240f, 0.140f, 0.150f } },
    { "UV Violet",            { 0.62f, 0.42f, 1.00f }, { 0.050f, 0.042f, 0.075f }, { 0.078f, 0.066f, 0.110f }, { 0.100f, 0.086f, 0.140f }, { 0.200f, 0.170f, 0.280f } },
    { "Toxic Green",          { 0.36f, 0.88f, 0.40f }, { 0.038f, 0.052f, 0.044f }, { 0.058f, 0.078f, 0.066f }, { 0.076f, 0.100f, 0.086f }, { 0.140f, 0.220f, 0.170f } },
    { "Ember Amber",          { 1.00f, 0.62f, 0.20f }, { 0.058f, 0.048f, 0.040f }, { 0.088f, 0.072f, 0.060f }, { 0.112f, 0.094f, 0.078f }, { 0.250f, 0.200f, 0.150f } },
    { "Mono Steel",           { 0.86f, 0.88f, 0.92f }, { 0.045f, 0.047f, 0.052f }, { 0.070f, 0.072f, 0.080f }, { 0.092f, 0.095f, 0.105f }, { 0.180f, 0.185f, 0.200f } },
};
static const int kMenuPresetCount = IM_ARRAYSIZE(kMenuPresets);

static void ApplyMenuPreset(int i) {
    if (i < 0 || i >= kMenuPresetCount) return;
    const MenuPreset& p = kMenuPresets[i];
    auto Set = [](float* dst, const float* src) { dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = 1.0f; };
    Set(MenuVars::menuAccent, p.accent); Set(MenuVars::menuBg, p.bg); Set(MenuVars::menuPanel, p.panel);
    Set(MenuVars::menuFrame, p.frame); Set(MenuVars::menuBorder, p.border);
    MenuVars::menuPreset = i;
}

// Вызывается каждый кадр до отрисовки вотермарки и меню
static void ApplyMenuTheme() {
    ImVec4* c = ImGui::GetStyle().Colors;
    gui.m_design = (MenuVars::menuDesign == 1) ? 1 : 0;
    if (gui.m_design == 0) {
        gui.apply_palette(c_gui::classic_palette());
        c[ImGuiCol_WindowBg] = ImColor(2, 2, 2);
    }
    else {
        auto C = [](const float* v, float a) { return color_t{ v[0], v[1], v[2], v[3] * a }; };
        auto Mix = [](const float* a, const float* b, float t) { return color_t{ a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t, 1.0f }; };
        static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        c_gui::palette_t p;
        p.accent = C(MenuVars::menuAccent, 1.0f);
        p.frame_inactive = C(MenuVars::menuFrame, 1.0f);
        p.frame_active = Mix(MenuVars::menuFrame, MenuVars::menuAccent, 0.28f);
        p.button = C(MenuVars::menuFrame, 1.0f);
        p.button_hovered = Mix(MenuVars::menuFrame, white, 0.05f);
        p.button_active = Mix(MenuVars::menuFrame, MenuVars::menuAccent, 0.22f);
        p.group_box_bg = C(MenuVars::menuPanel, 1.0f);
        p.border = C(MenuVars::menuBorder, 0.55f);
        gui.apply_palette(p);
        gui.window_bg = C(MenuVars::menuBg, 1.0f);
        gui.m_bg_alpha = MenuVars::menuOpacity;
        gui.m_rounding = MenuVars::menuRounding;
        gui.m_shimmer = MenuVars::menuShimmer;
        gui.m_glow = MenuVars::menuGlow;
        c[ImGuiCol_WindowBg] = ImVec4(MenuVars::menuBg[0], MenuVars::menuBg[1], MenuVars::menuBg[2], 1.0f);
    }
    c[ImGuiCol_ScrollbarGrab] = gui.accent_color.to_vec4(0.8f, false);
    c[ImGuiCol_ScrollbarGrabHovered] = gui.accent_color.to_vec4(0.9f, false);
    c[ImGuiCol_ScrollbarGrabActive] = gui.accent_color.to_vec4(1.0f, false);
    c[ImGuiCol_TextSelectedBg] = gui.accent_color.to_vec4(0.8f, false);
}

// ==================== ВОТЕРМАРКА: CLASSIC (старая) ====================
static void RenderWatermarkClassic(int screenWidth) {
    ImGuiIO& io = ImGui::GetIO(); ImDrawList* bgDraw = ImGui::GetBackgroundDrawList();
    std::time_t now = std::time(nullptr); struct tm localTime {}; localtime_s(&localTime, &now);
    char timeBuf[32]; snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
    int fps = (int)(io.Framerate + 0.5f); char fpsBuf[32]; snprintf(fpsBuf, sizeof(fpsBuf), "%d FPS", fps);

    const char* cheatTitle = "DLTB.RIP";
    const char* sep = "  |  ";
    bool hasCustom = MenuVars::watermarkShowCustomText && (std::strlen(MenuVars::watermarkCustomText) > 0);

    ImVec2 titleSize = CalcTextSize(cheatTitle);
    ImVec2 sepSize = CalcTextSize(sep);
    ImVec2 customSize = hasCustom ? CalcTextSize(MenuVars::watermarkCustomText) : ImVec2(0, 0);
    ImVec2 timeSize = CalcTextSize(timeBuf);
    ImVec2 fpsSize = CalcTextSize(fpsBuf);

    float totalTextWidth = titleSize.x;
    if (hasCustom)                     totalTextWidth += sepSize.x + customSize.x;
    if (MenuVars::watermarkShowTime) totalTextWidth += sepSize.x + timeSize.x;
    if (MenuVars::watermarkShowFPS)  totalTextWidth += sepSize.x + fpsSize.x;

    float padX = 12.0f, padY = 6.0f;
    float boxWidth = totalTextWidth + padX * 2.0f, boxHeight = titleSize.y + padY * 2.0f;
    float xMax = (float)screenWidth - MenuVars::watermarkOffsetX, xMin = xMax - boxWidth;
    float yMin = MenuVars::watermarkOffsetY, yMax = yMin + boxHeight;
    int bgAlpha = (int)(MenuVars::watermarkBgAlpha * 255.0f);
    ImU32 bgCol = IM_COL32(13, 17, 24, bgAlpha), borderCol = IM_COL32(30, 38, 52, bgAlpha), accentCol = gui.accent_color.to_im_color();
    int txtB = (int)(MenuVars::textBrightness * 255.0f); ImU32 textCol = IM_COL32(txtB, txtB, txtB, 255);

    bgDraw->AddRectFilled(ImVec2(xMin, yMin), ImVec2(xMax, yMax), bgCol, 3.0f);
    bgDraw->AddRect(ImVec2(xMin, yMin), ImVec2(xMax, yMax), borderCol, 3.0f, 0, 1.0f);
    if (MenuVars::watermarkStyle == 0) bgDraw->AddRectFilled(ImVec2(xMin, yMin), ImVec2(xMax, yMin + 2.5f), accentCol);
    else if (MenuVars::watermarkStyle == 1) bgDraw->AddRectFilled(ImVec2(xMin, yMin), ImVec2(xMin + 3.0f, yMax), accentCol);
    else if (MenuVars::watermarkStyle == 2) bgDraw->AddRect(ImVec2(xMin, yMin), ImVec2(xMax, yMax), accentCol, 3.0f, 0, 1.5f);

    float curX = xMin + padX, curY = yMin + padY;
    bgDraw->AddText(ImVec2(curX + 1, curY + 1), IM_COL32(0, 0, 0, 180), cheatTitle);
    bgDraw->AddText(ImVec2(curX, curY), accentCol, cheatTitle);
    curX += titleSize.x;

    if (hasCustom) {
        bgDraw->AddText(ImVec2(curX, curY), accentCol, sep);
        curX += sepSize.x;
        bgDraw->AddText(ImVec2(curX + 1, curY + 1), IM_COL32(0, 0, 0, 180), MenuVars::watermarkCustomText);
        bgDraw->AddText(ImVec2(curX, curY), textCol, MenuVars::watermarkCustomText);
        curX += customSize.x;
    }

    if (MenuVars::watermarkShowTime) {
        bgDraw->AddText(ImVec2(curX, curY), accentCol, sep);
        curX += sepSize.x;
        bgDraw->AddText(ImVec2(curX, curY), textCol, timeBuf);
        curX += timeSize.x;
    }

    if (MenuVars::watermarkShowFPS) {
        bgDraw->AddText(ImVec2(curX, curY), accentCol, sep);
        curX += sepSize.x;
        bgDraw->AddText(ImVec2(curX, curY), textCol, fpsBuf);
    }
}

// ==================== ВОТЕРМАРКА: MODERN (новая) ====================
// Стеклянная "пилюля": логотип-плитка с акцентным градиентом, DLTB.RIP, сегменты
// [пользователь] [FPS с цветной точкой] [время] [статус игры]. Ширина анимируется,
// по нижней кромке бежит блик акцентного цвета.
static void RenderWatermarkModern(int screenWidth) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImFont* body = ImGui::GetFont();
    ImFont* bold = g_FontBold ? g_FontBold : body;
    const float fs = body->FontSize * io.FontGlobalScale;
    const float bfs = bold->FontSize * io.FontGlobalScale;
    const float t = (float)ImGui::GetTime();

    std::time_t now = std::time(nullptr); struct tm lt {}; localtime_s(&lt, &now);
    char timeBuf[16]; snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", lt.tm_hour, lt.tm_min);
    int fps = (int)(io.Framerate + 0.5f);
    char fpsBuf[16]; snprintf(fpsBuf, sizeof(fpsBuf), "%d fps", fps);
    bool inGame = GameMemory::hProcess != nullptr && GameMemory::patternsInitialized;
    const char* statusTxt = inGame ? "in game" : "waiting";
    bool hasUser = MenuVars::watermarkShowCustomText && MenuVars::watermarkCustomText[0];

    ImVec4 acc(gui.accent_color.r, gui.accent_color.g, gui.accent_color.b, 1.0f);
    auto A = [&](float a) { return ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, acc.w * a)); };
    const float bgA = MenuVars::watermarkBgAlpha;
    int tb = (int)(MenuVars::textBrightness * 255.0f);
    ImU32 textCol = IM_COL32(tb, tb, tb, 255);
    ImU32 mutedCol = IM_COL32(120, 132, 156, 255);

    // --- сегменты: иконка + текст
    struct Seg { const char* icon; const char* text; ImU32 iconCol; bool dot; };
    Seg segs[4]; int n = 0;
    if (hasUser) segs[n++] = { ICON_FA_USER, MenuVars::watermarkCustomText, mutedCol, false };
    if (MenuVars::watermarkShowFPS) {
        ImU32 fc = fps >= 60 ? IM_COL32(72, 220, 128, 255) : (fps >= 30 ? IM_COL32(240, 196, 70, 255) : IM_COL32(240, 80, 80, 255));
        segs[n++] = { nullptr, fpsBuf, fc, true };
    }
    if (MenuVars::watermarkShowTime) segs[n++] = { ICON_FA_CLOCK, timeBuf, mutedCol, false };
    if (MenuVars::watermarkShowStatus) segs[n++] = { nullptr, statusTxt, inGame ? IM_COL32(72, 220, 128, 255) : IM_COL32(240, 196, 70, 255), true };

    const float H = 30.0f, padL = 5.0f, padR = 12.0f, gap = 11.0f, iconGap = 6.0f, tile = H - 10.0f;
    ImVec2 t1 = bold->CalcTextSizeA(bfs, FLT_MAX, 0.0f, "DLTB");
    ImVec2 t2 = bold->CalcTextSizeA(bfs, FLT_MAX, 0.0f, ".RIP");
    float w = padL + tile + 8.0f + t1.x + t2.x;
    float segW[4];
    for (int i = 0; i < n; ++i) {
        float iw = segs[i].dot ? 7.0f : (segs[i].icon ? body->CalcTextSizeA(fs, FLT_MAX, 0.0f, segs[i].icon).x : 0.0f);
        segW[i] = iw + iconGap + body->CalcTextSizeA(fs, FLT_MAX, 0.0f, segs[i].text).x;
        w += gap * 2.0f + 1.0f + segW[i];
    }
    w += padR;

    static float animW = 0.0f;
    if (animW <= 0.0f) animW = w;
    animW += (w - animW) * (1.0f - std::exp(-io.DeltaTime * 14.0f));
    float W = std::floor(animW + 0.5f);

    float xMax = std::floor((float)screenWidth - MenuVars::watermarkOffsetX), xMin = xMax - W;
    float yMin = std::floor(MenuVars::watermarkOffsetY), yMax = yMin + H;
    const float R = 9.0f;

    // тень
    for (int s = 1; s <= 4; ++s) {
        float e = s * 2.0f;
        dl->AddRectFilled(ImVec2(xMin - e, yMin - e + 3), ImVec2(xMax + e, yMax + e + 3), IM_COL32(0, 0, 0, (int)(bgA * (30 - s * 6))), R + e);
    }
    // стеклянный фон + лёгкий акцентный отсвет слева + блик сверху
    const bool themed = MenuVars::watermarkFollowMenu && MenuVars::menuDesign == 1;
    ImU32 glassCol = themed ? gui.window_bg.to_im_color(bgA * 0.96f, false) : IM_COL32(11, 13, 19, (int)(bgA * 245));
    ImU32 rimCol = themed ? gui.border.to_im_color(bgA, false) : IM_COL32(38, 46, 64, (int)(bgA * 255));
    dl->AddRectFilled(ImVec2(xMin, yMin), ImVec2(xMax, yMax), glassCol, R);
    dl->AddRectFilledMultiColor(ImVec2(xMin + 4, yMin + 1), ImVec2(xMin + W * 0.45f, yMax - 1), A(0.10f * bgA), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), A(0.10f * bgA));
    dl->AddLine(ImVec2(xMin + R, yMin + 0.5f), ImVec2(xMax - R, yMin + 0.5f), IM_COL32(255, 255, 255, (int)(bgA * 18)), 1.0f);
    dl->AddRect(ImVec2(xMin, yMin), ImVec2(xMax, yMax), rimCol, R, 0, 1.0f);

    dl->PushClipRect(ImVec2(xMin, yMin), ImVec2(xMax, yMax), true);

    // логотип-плитка
    ImVec2 tl(xMin + padL, yMin + 5.0f), br(tl.x + tile, tl.y + tile);
    dl->AddRectFilled(tl, br, A(1.0f), 6.0f);
    dl->AddRectFilledMultiColor(ImVec2(tl.x + 1, tl.y + tile * 0.5f), ImVec2(br.x - 1, br.y - 1), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 70), IM_COL32(0, 0, 0, 70));
    {
        float pulse = 0.5f + 0.5f * std::sin(t * 2.2f);
        dl->AddRect(ImVec2(tl.x - 1, tl.y - 1), ImVec2(br.x + 1, br.y + 1), A(0.25f + 0.25f * pulse), 7.0f, 0, 1.0f);
        ImVec2 isz = body->CalcTextSizeA(fs, FLT_MAX, 0.0f, ICON_FA_BOLT);
        dl->AddText(body, fs, ImVec2(std::floor(tl.x + (tile - isz.x) * 0.5f), std::floor(tl.y + (tile - isz.y) * 0.5f)), IM_COL32(255, 255, 255, 255), ICON_FA_BOLT);
    }
    float x = br.x + 8.0f;
    float ty = std::floor(yMin + (H - t1.y) * 0.5f);
    dl->AddText(bold, bfs, ImVec2(x, ty), IM_COL32(244, 246, 250, 255), "DLTB"); x += t1.x;
    dl->AddText(bold, bfs, ImVec2(x, ty), A(1.0f), ".RIP"); x += t2.x;

    // сегменты
    for (int i = 0; i < n; ++i) {
        x += gap;
        dl->AddLine(ImVec2(x, yMin + 8.0f), ImVec2(x, yMax - 8.0f), IM_COL32(255, 255, 255, 22), 1.0f);
        x += 1.0f + gap;
        float cy = yMin + H * 0.5f;
        if (segs[i].dot) {
            float pulse = 0.5f + 0.5f * std::sin(t * 3.0f + i);
            ImVec4 dc = ImGui::ColorConvertU32ToFloat4(segs[i].iconCol);
            dl->AddCircleFilled(ImVec2(x + 3.5f, cy), 5.5f, ImGui::ColorConvertFloat4ToU32(ImVec4(dc.x, dc.y, dc.z, 0.18f + 0.15f * pulse)), 16);
            dl->AddCircleFilled(ImVec2(x + 3.5f, cy), 3.0f, segs[i].iconCol, 12);
            x += 7.0f + iconGap;
        }
        else if (segs[i].icon) {
            ImVec2 isz = body->CalcTextSizeA(fs, FLT_MAX, 0.0f, segs[i].icon);
            dl->AddText(body, fs, ImVec2(x, std::floor(cy - isz.y * 0.5f)), A(0.95f), segs[i].icon);
            x += isz.x + iconGap;
        }
        ImVec2 tsz = body->CalcTextSizeA(fs, FLT_MAX, 0.0f, segs[i].text);
        dl->AddText(body, fs, ImVec2(x, std::floor(cy - tsz.y * 0.5f)), textCol, segs[i].text);
        x += tsz.x;
    }

    // нижняя акцентная кромка с бегущим бликом
    float ly = yMax - 2.0f;
    dl->AddRectFilled(ImVec2(xMin + R, ly), ImVec2(xMax - R, ly + 1.5f), A(0.35f));
    if (MenuVars::watermarkShimmer) {
        float span = W + 160.0f;
        float sx = xMin - 80.0f + std::fmod(t * 140.0f, span);
        dl->AddRectFilledMultiColor(ImVec2(sx - 60.0f, ly - 0.5f), ImVec2(sx, ly + 2.0f), A(0.0f), A(1.0f), A(1.0f), A(0.0f));
        dl->AddRectFilledMultiColor(ImVec2(sx, ly - 0.5f), ImVec2(sx + 60.0f, ly + 2.0f), A(1.0f), A(0.0f), A(0.0f), A(1.0f));
    }
    dl->PopClipRect();
}

void RenderWatermark(int screenWidth) {
    if (!MenuVars::watermarkEnabled) return;
    if (MenuVars::watermarkDesign == 1) { RenderWatermarkModern(screenWidth); return; }
    RenderWatermarkClassic(screenWidth);
}

bool IsGameCursorVisible() {
    CURSORINFO ci = { sizeof(CURSORINFO) };
    if (GetCursorInfo(&ci)) return (ci.flags & CURSOR_SHOWING) != 0;
    return false;
}

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**)
{
    // Один экземпляр. Повторный запуск = перезапуск: старый трейнер закрывается штатно
    // (WM_CLOSE -> выход из цикла -> RestoreAllPatches), а не убивается — иначе в игре
    // остаются перехваты без хозяина.
    HANDLE instanceMutex = CreateMutexW(nullptr, TRUE, L"Local\\DLTB.RIP.Trainer");
    if (instanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND old = FindWindowW(L"ImGui Overlay", L"DLTB.RIP Overlay")) PostMessageW(old, WM_CLOSE, 0, 0);
        const DWORD w = WaitForSingleObject(instanceMutex, 10000);
        if (w != WAIT_OBJECT_0 && w != WAIT_ABANDONED) {
            MessageBoxW(nullptr, L"Another copy of DLTB.RIP is still running and did not close.\nClose it (Unload key) and try again.", L"DLTB.RIP", MB_ICONWARNING);
            return 1;
        }
    }

    srand((unsigned int)time(nullptr));
    int screenWidth = GetSystemMetrics(SM_CXSCREEN), screenHeight = GetSystemMetrics(SM_CYSCREEN);
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"ImGui Overlay", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, wc.lpszClassName, L"DLTB.RIP Overlay", WS_POPUP, 0, 0, screenWidth, screenHeight, nullptr, nullptr, wc.hInstance, nullptr);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margins = { -1, -1, -1, -1 }; DwmExtendFrameIntoClientArea(hwnd, &margins);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); ::UnregisterClassW(wc.lpszClassName, wc.hInstance); return 1; }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT); ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX9_Init(g_pd3dDevice);

    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    auto AddCustomFontWithIcons = [&](const void* fontData, size_t dataSize, float fontSize, float rasterMult) -> ImFont* {
        ImFontConfig cfg; cfg.FontDataOwnedByAtlas = false; cfg.RasterizerMultiply = rasterMult; cfg.OversampleH = 3; cfg.OversampleV = 2;
        ImFont* f = io.Fonts->AddFontFromMemoryTTF((void*)fontData, (int)dataSize, fontSize, &cfg);
        ImFontConfig icons_cfg; icons_cfg.FontDataOwnedByAtlas = false; icons_cfg.MergeMode = true; icons_cfg.PixelSnapH = true; icons_cfg.RasterizerMultiply = rasterMult;
        io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, 13.5f, &icons_cfg, icon_ranges);
        return f;
        };

    ImFont* fontMedium = AddCustomFontWithIcons(font_medium_binary, sizeof font_medium_binary, 14.5f, 1.32f);
    ImFontConfig logo_cfg; logo_cfg.FontDataOwnedByAtlas = false; logo_cfg.RasterizerMultiply = 1.25f;
    ImFont* fontLogo = io.Fonts->AddFontFromMemoryTTF(font_heavy_binary, sizeof font_heavy_binary, 28.0f, &logo_cfg);
    ImFont* fontRegular = AddCustomFontWithIcons(font_medium_binary, sizeof font_medium_binary, 14.0f, 1.05f);
    ImFont* fontBold = AddCustomFontWithIcons(font_heavy_binary, sizeof font_heavy_binary, 14.5f, 1.15f);
    g_FontBold = fontBold;

    static const char* dayPhaseNames[] = { "Morning", "Day", "Evening", "Night Start", "Night Attack", "Night", "Night Final" };
    static const int dayPhaseValues[] = { 0, 1, 2, 4, 5, 6, 7 };
    static const char* fontWeightNames[] = { "Balanced Medium (Default)", "Soft Regular (Thin)", "Ultra Bold (Heavy)" };
    static const char* watermarkStyleNames[] = { "Top Accent Line", "Left Accent Bar", "Full Accent Outline" };
    static const char* watermarkDesignNames[] = { "Classic (old)", "Modern (new)" };
    static const char* menuDesignNames[] = { "Classic (current design)", "Modern (watermark glass)" };
    static const char* hitCrossStyleNames[] = { "Classic X-Cross", "Tactical Corners", "Circle + Cross", "Biohazard Tri-Star", "Diamond Reticle" };
    static const char* hitSoundNames[] = { "None (Muted)", "Crisp Tick", "Heavy Thud", "High Bell", "Double Tap" };
    static const char* hitPosModeNames[] = { "Center Crosshair (2D)", "3D Enemy Position" };

    ConfigSystem::RefreshConfigs();

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg); ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        bool isGameConnected = GameMemory::AttachToGame();
        ItemBridge::Update(isGameConnected ? GameMemory::processId : 0);
        {   // Items: автозагрузка рантайма — одна попытка на процесс игры
            static DWORD s_autoLoadPid = 0;
            if (MenuVars::itemsAutoLoad && isGameConnected && GameMemory::patternsInitialized && !ItemBridge::view && s_autoLoadPid != GameMemory::processId) {
                s_autoLoadPid = GameMemory::processId;
                if (ItemBridge::RuntimeFilesPresent() && !ItemBridge::Inject(GameMemory::hProcess))
                    TopNotificationSystem::Push("Item Extravaganza", ItemBridge::injectError.c_str(), ImVec4(1.0f, 0.4f, 0.35f, 1.0f), 4.0f, ICON_FA_EXCLAMATION_TRIANGLE);
            }
        }
        UpdateGameViewRect(isGameConnected ? GameMemory::gameWindow : nullptr, screenWidth, screenHeight);
        if (GameMemory::shouldAutoExit) {
            done = true;
            break;
        }

        HWND fgWnd = GetForegroundWindow();
        bool isOverlayAllowedToShow = isGameConnected && GameMemory::gameWindow != nullptr && !IsIconic(GameMemory::gameWindow) && (fgWnd == GameMemory::gameWindow || fgWnd == hwnd);

        if (isOverlayAllowedToShow && (GetAsyncKeyState(VK_INSERT) & 1)) {
            g_ShowMenu = !g_ShowMenu;
            if (!g_ShowMenu && GameMemory::gameWindow && IsWindow(GameMemory::gameWindow)) ForceWindowFocus(GameMemory::gameWindow);
        }

        // Состояние окна оверлея выставляется в одном месте каждый кадр. Раньше после
        // сворачивания/Alt+Tab с открытым меню окно оставалось "прозрачным для кликов":
        // меню видно, но мышь уходит в игру, а оверлей при этом отбирает фокус.
        {
            static bool s_wasInteractive = false, s_wasAllowed = false;
            bool interactive = isOverlayAllowedToShow && g_ShowMenu;
            // игра развернулась обратно — она могла перекрыть оверлей, поднимаем его наверх
            if (isOverlayAllowedToShow && !s_wasAllowed)
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            s_wasAllowed = isOverlayAllowedToShow;
            LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
            LONG wanted = interactive ? (exStyle & ~WS_EX_TRANSPARENT) : (exStyle | WS_EX_TRANSPARENT);
            if (wanted != exStyle) SetWindowLongW(hwnd, GWL_EXSTYLE, wanted);
            if (interactive && !s_wasInteractive) ForceWindowFocus(hwnd);
            if (!interactive && s_wasInteractive && ImGui::GetCurrentContext()) {
                // меню скрылось посреди клика/перетаскивания — сбрасываем, иначе слайдер "залипает"
                ImGuiIO& rio = ImGui::GetIO();
                for (bool& b : rio.MouseDown) b = false;
                rio.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
                ImGui::ClearActiveID();
            }
            s_wasInteractive = interactive;
        }

        if (isOverlayAllowedToShow && !MenuVars::waitingUnloadKey && MenuVars::unloadKey > 0 && (GetAsyncKeyState(MenuVars::unloadKey) & 1))
            done = true;

        if (isOverlayAllowedToShow && (GetAsyncKeyState(VK_F8) & 1))
            MenuVars::toggleHUD = !MenuVars::toggleHUD;

        if (isOverlayAllowedToShow && MenuVars::rapidFire && !g_ShowMenu) {
            bool isGameFocused = (fgWnd == GameMemory::gameWindow);
            bool isGamePlaying = !IsGameCursorVisible();
            bool isLmbHeld = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            bool isAdsOk = !MenuVars::rapidFireAdsOnly || ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
            if (isGameFocused && isGamePlaying && isLmbHeld && isAdsOk) {
                static ULONGLONG lastShotTick = 0; ULONGLONG currentTick = GetTickCount64();
                int safeDelay = (MenuVars::rapidFireDelayMs < 25) ? 25 : MenuVars::rapidFireDelayMs;
                if (currentTick - lastShotTick >= (ULONGLONG)safeDelay) {
                    lastShotTick = currentTick;
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                }
            }
        }

        if (isOverlayAllowedToShow && !MenuVars::waitingFloatKey && MenuVars::floatInAirKey > 0) {
            if (MenuVars::floatInAirBindMode == 0) { if (GetAsyncKeyState(MenuVars::floatInAirKey) & 1) MenuVars::floatInAir = !MenuVars::floatInAir; }
            else MenuVars::floatInAir = (GetAsyncKeyState(MenuVars::floatInAirKey) & 0x8000) != 0;
        }

        if (isOverlayAllowedToShow && !MenuVars::waitingTpKey && MenuVars::tpWaypointKey > 0) {
            if (MenuVars::tpWaypointBindMode == 0) { if (GetAsyncKeyState(MenuVars::tpWaypointKey) & 1) GameMemory::TriggerWaypointTeleport(); }
            else { if ((GetAsyncKeyState(MenuVars::tpWaypointKey) & 0x8000) != 0) GameMemory::TriggerWaypointTeleport(); }
        }

        if (isOverlayAllowedToShow && g_ShowMenu) {
            ClipCursor(nullptr);
            if (GetForegroundWindow() != hwnd) ForceWindowFocus(hwnd);
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            g_d3dpp.BackBufferWidth = g_ResizeWidth; g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0; ResetDevice();
        }

        if (isGameConnected && GameMemory::patternsInitialized)
        {
            ULONGLONG now = GetTickCount64();
            bool activeNoFall = MenuVars::noFallDamage || (now < MenuVars::tempNoFallEndTime);
            bool activeTPWaypoint = (now < MenuVars::tpWaypointEndTime);
            bool activeTPRespawn = (now < MenuVars::tpRespawnEndTime);
            bool activeDiscoverMap = (now < MenuVars::discoverMapEndTime);

            GameMemory::SetPatchState(GameMemory::patchGodMode1, MenuVars::godMode);
            GameMemory::SetPatchState(GameMemory::patchGodMode2, MenuVars::godMode);
            GameMemory::SetPatchState(GameMemory::patchStamina, MenuVars::infStamina);
            GameMemory::SetPatchState(GameMemory::patchUVFlashlight, MenuVars::infUVFlashlight);
            GameMemory::SetCaveState(GameMemory::caveUnlimitedFury, MenuVars::infFury);
            GameMemory::SetPatchState(GameMemory::patchNoFall1, activeNoFall);
            GameMemory::SetPatchState(GameMemory::patchNoFall2, activeNoFall);
            GameMemory::SetPatchState(GameMemory::patchKnockdown, MenuVars::noKnockdown);
            GameMemory::SetPatchState(GameMemory::patchHitReaction, MenuVars::noKnockdown);

            GameMemory::SetCaveState(GameMemory::caveMoveSpeed, MenuVars::moveSpeedEnabled);
            if (MenuVars::moveSpeedEnabled && GameMemory::caveMoveSpeed.paramAddress)
                GameMemory::Write<float>(GameMemory::caveMoveSpeed.paramAddress, MenuVars::speedMultiplier);

            GameMemory::SetPatchState(GameMemory::patchSkipIsFury, MenuVars::jumpHeightEnabled);
            GameMemory::SetCaveState(GameMemory::caveJumpHeight, MenuVars::jumpHeightEnabled);
            if (MenuVars::jumpHeightEnabled && GameMemory::caveJumpHeight.paramAddress)
                GameMemory::Write<float>(GameMemory::caveJumpHeight.paramAddress, MenuVars::jumpMultiplier);

            GameMemory::SetPatchState(GameMemory::patchUnlimitedJumps, MenuVars::unlimitedJumps);
            GameMemory::SetPatchState(GameMemory::patchTripleJump, MenuVars::tripleJump);
            GameMemory::SetPatchState(GameMemory::patchVaultKick, MenuVars::alwaysVaultKick);
            GameMemory::SetPatchState(GameMemory::patchFloatInAir, MenuVars::floatInAir);
            GameMemory::SetCaveState(GameMemory::caveUnlimitedCable, MenuVars::unlimitedCableLength);
            GameMemory::SetCaveState(GameMemory::caveDisableHookNerf, MenuVars::disableHookPullDown);

            GameMemory::SetCaveState(GameMemory::caveSwimSpeed, MenuVars::swimSpeedEnabled);
            if (MenuVars::swimSpeedEnabled && GameMemory::caveSwimSpeed.paramAddress)
                GameMemory::Write<float>(GameMemory::caveSwimSpeed.paramAddress, MenuVars::swimSpeedMultiplier);

            GameMemory::SetPatchState(GameMemory::patchWaterPhysics, MenuVars::disableWaterPhysics);
            GameMemory::SetPatchState(GameMemory::patchPlayableArea1, MenuVars::disablePlayableArea);
            GameMemory::SetPatchState(GameMemory::patchPlayableArea2, MenuVars::disablePlayableArea);
            GameMemory::SetCaveState(GameMemory::caveTeleportWaypoint, activeTPWaypoint);
            GameMemory::SetPatchState(GameMemory::patchIsOutOfMap, activeTPRespawn);
            GameMemory::SetPatchState(GameMemory::patchTPTime, activeTPRespawn);

            GameMemory::SetCaveState(GameMemory::caveNightExp, MenuVars::nightExpEnabled);
            if (MenuVars::nightExpEnabled && GameMemory::caveNightExp.paramAddress)
                GameMemory::Write<float>(GameMemory::caveNightExp.paramAddress, MenuVars::nightExpMultiplier);

            GameMemory::SetCaveState(GameMemory::caveOpenWorldExp, MenuVars::openWorldExpEnabled);
            if (MenuVars::openWorldExpEnabled && GameMemory::caveOpenWorldExp.paramAddress)
                GameMemory::Write<float>(GameMemory::caveOpenWorldExp.paramAddress, MenuVars::openWorldExpMultiplier);

            GameMemory::SetPatchState(GameMemory::patchDeathPenalty1, MenuVars::disableDeathPenalty);
            GameMemory::SetCaveState(GameMemory::caveDeathPenalty2, MenuVars::disableDeathPenalty);

            GameMemory::SetPatchState(GameMemory::patchPowerAttackBlock, MenuVars::powerAttackBlocking);
            GameMemory::SetPatchState(GameMemory::patchAlwaysPerfectBlock, MenuVars::alwaysPerfectBlock);
            GameMemory::SetPatchState(GameMemory::patchAutoBlock, MenuVars::autoBlock);
            GameMemory::SetPatchState(GameMemory::patchDurability, MenuVars::infDurability);
            GameMemory::SetPatchState(GameMemory::patchUnlimitedRepairs, MenuVars::infDurability);
            GameMemory::SetPatchState(GameMemory::patchOneHitKill, MenuVars::oneHitKill);
            GameMemory::SetPatchState(GameMemory::patchEasyFinisher, MenuVars::easyFinisher);
            GameMemory::SetPatchState(GameMemory::patchInstantFinisher1, MenuVars::instantFinisher);
            GameMemory::SetPatchState(GameMemory::patchInstantFinisher2, MenuVars::instantFinisher);
            GameMemory::SetPatchState(GameMemory::patchInstantFinisher3, MenuVars::instantFinisher);
            GameMemory::SetPatchState(GameMemory::patchInstantFinisher4, MenuVars::instantFinisher);
            GameMemory::SetPatchState(GameMemory::patchElemental1, MenuVars::alwaysElemental);
            GameMemory::SetPatchState(GameMemory::patchElemental2, MenuVars::alwaysElemental);

            bool anyHitmarkerActive = MenuVars::hitCfg.enabled || MenuVars::killCfg.enabled;

            GameMemory::SetCaveState(GameMemory::caveDamageMult, MenuVars::damageMultEnabled);
            if (MenuVars::damageMultEnabled && GameMemory::caveDamageMult.paramAddress) {
                GameMemory::Write<float>(GameMemory::caveDamageMult.paramAddress, MenuVars::damageMultiplier - 1.0f);
            }

            GameMemory::SetPatchState(GameMemory::patchAmmo, MenuVars::infAmmo);
            GameMemory::SetPatchState(GameMemory::patchArrows, MenuVars::infAmmo);
            GameMemory::SetCaveState(GameMemory::caveNoReload, MenuVars::noReload);
            GameMemory::SetCaveState(GameMemory::caveNoRecoil, MenuVars::noRecoil);
            GameMemory::SetCaveState(GameMemory::caveMegaAccuracy, MenuVars::megaAccuracy);
            GameMemory::SetPatchState(GameMemory::patchSilenced1, MenuVars::silencedFirearms);
            GameMemory::SetPatchState(GameMemory::patchSilenced2, MenuVars::silencedFirearms);
            GameMemory::SetCaveState(GameMemory::caveBowRapidFire, MenuVars::bowRapidFire);

            GameMemory::SetCaveState(GameMemory::caveAICollector, anyHitmarkerActive);
            GameMemory::SetCaveState(GameMemory::caveHealthHitDetector, anyHitmarkerActive);
            if (anyHitmarkerActive) {
                GameMemory::ProcessHitmarkerEvents(screenWidth, screenHeight);
            }
            else {
                Fx::trackedEntities.clear();
                Fx::cameraSource = 0;
            }

            GameMemory::SetMultiPatchState(GameMemory::patchFreeCraftAll, MenuVars::freeCraft);
            GameMemory::SetCaveState(GameMemory::caveFreeBlueprints, MenuVars::freeCraft);
            GameMemory::SetCaveState(GameMemory::caveMultiplyGathered, MenuVars::multiplyGatheredEnabled);
            if (MenuVars::multiplyGatheredEnabled && GameMemory::caveMultiplyGathered.paramAddress)
                GameMemory::Write<int>(GameMemory::caveMultiplyGathered.paramAddress, MenuVars::gatheredMultiplier);

            GameMemory::SetPatchState(GameMemory::patchFreezeTime, MenuVars::freezeTime);
            // Разовый прыжок времени (Misc > Weather, погода "в скобках"): те же перехваты, что у
            // Force Daytime, держим ~1.5 с — они записывают час в часы игры, — затем снимаем, и время идёт дальше.
            const bool timeJumping = !MenuVars::forceDaytimeEnabled && GetTickCount64() < MenuVars::timeJumpUntil;
            const float forcedHour = timeJumping ? MenuVars::timeJumpHour : MenuVars::worldTime;
            float normTime = (forcedHour <= 0.001f) ? 0.0001f : (forcedHour / 24.0f);
            GameMemory::SetCaveState(GameMemory::caveForceDayTime1, MenuVars::forceDaytimeEnabled || timeJumping);
            GameMemory::SetCaveState(GameMemory::caveForceDayTime2, MenuVars::forceDaytimeEnabled || timeJumping);
            if (MenuVars::forceDaytimeEnabled || timeJumping) {
                if (GameMemory::caveForceDayTime1.paramAddress) GameMemory::Write<float>(GameMemory::caveForceDayTime1.paramAddress, normTime);
                if (GameMemory::caveForceDayTime2.paramAddress) GameMemory::Write<float>(GameMemory::caveForceDayTime2.paramAddress, normTime);
            }

            GameMemory::SetCaveState(GameMemory::caveForceDayPhase, MenuVars::forceDayPhaseEnabled);
            if (MenuVars::forceDayPhaseEnabled && GameMemory::caveForceDayPhase.paramAddress)
                GameMemory::Write<int>(GameMemory::caveForceDayPhase.paramAddress, dayPhaseValues[MenuVars::dayPhaseIndex]);

            GameMemory::SetCaveState(GameMemory::caveWorldSpeed, MenuVars::worldSpeedEnabled);
            if (MenuVars::worldSpeedEnabled && GameMemory::caveWorldSpeed.paramAddress)
                GameMemory::Write<float>(GameMemory::caveWorldSpeed.paramAddress, MenuVars::worldSpeedValue);

            GameMemory::SetPatchState(GameMemory::patchWorldNoise1, MenuVars::disableWorldNoise);
            GameMemory::SetPatchState(GameMemory::patchWorldNoise2, MenuVars::disableWorldNoise);
            GameMemory::SetCaveState(GameMemory::caveRemoveFogOfWar, MenuVars::removeFogOfWar);
            GameMemory::SetPatchState(GameMemory::patchDiscoverMap1, activeDiscoverMap);
            GameMemory::SetPatchState(GameMemory::patchDiscoverMap2, activeDiscoverMap);
            GameMemory::SetPatchState(GameMemory::patchDiscoverMap3, activeDiscoverMap);

            GameMemory::SetCaveState(GameMemory::caveEditZoneLevel, MenuVars::editZoneLevelEnabled);
            if (MenuVars::editZoneLevelEnabled && GameMemory::caveEditZoneLevel.paramAddress) {
                GameMemory::Write<int>(GameMemory::caveEditZoneLevel.paramAddress, MenuVars::zoneMinLevel);
                GameMemory::Write<int>(GameMemory::caveEditZoneLevel.paramAddress + 4, MenuVars::zoneMaxLevel);
            }

            GameMemory::SetCaveState(GameMemory::caveRemoveDoorLock, MenuVars::openLockedDoors);
            GameMemory::SetPatchState(GameMemory::patchAllowDoorOpening, MenuVars::openLockedDoors);
            GameMemory::SetPatchState(GameMemory::patchAutoUnlock, MenuVars::easyLockpick);
            GameMemory::SetCaveState(GameMemory::caveEasyLockpick, MenuVars::easyLockpick);

            GameMemory::SetCaveState(GameMemory::caveChangeAIScale, MenuVars::changeAIScaleEnabled);
            if (MenuVars::changeAIScaleEnabled && GameMemory::caveChangeAIScale.paramAddress)
                GameMemory::Write<float>(GameMemory::caveChangeAIScale.paramAddress, MenuVars::aiScaleValue);

            GameMemory::SetPatchState(GameMemory::patchInvisibility1, MenuVars::aiInvisibility);
            GameMemory::SetPatchState(GameMemory::patchDisableAIAwareness, MenuVars::aiInvisibility);
            GameMemory::SetPatchState(GameMemory::patchDisableZombieGrab, MenuVars::disableZombieGrab);
            GameMemory::SetPatchState(GameMemory::patchDisableChase, MenuVars::disableChaseLogic);
            GameMemory::SetPatchState(GameMemory::patchAlwaysStomp, MenuVars::alwaysStomp);
            GameMemory::SetPatchState(GameMemory::patchFreezeKillAI, MenuVars::freezeKillNearbyAI);
            GameMemory::SetPatchState(GameMemory::patchBurnAI, MenuVars::burnNearbyAI);
            GameMemory::SetPatchState(GameMemory::patchDisableAISpawn1, MenuVars::disableAISpawn);
            GameMemory::SetPatchState(GameMemory::patchDisableAISpawn2, MenuVars::disableAISpawn);
            GameMemory::SetPatchState(GameMemory::patchDisableAISpawn3, MenuVars::disableAISpawn);
            GameMemory::SetPatchState(GameMemory::patchDisableAISpawn4, MenuVars::disableAISpawn);
            GameMemory::SetPatchState(GameMemory::patchDisableAISpawn5, MenuVars::disableAISpawn);

            bool prevVehDmg = GameMemory::caveNoVehicleDamage.isEnabled;
            GameMemory::SetCaveState(GameMemory::caveNoVehicleDamage, MenuVars::noVehicleDamage);
            if (prevVehDmg && !MenuVars::noVehicleDamage && GameMemory::caveNoVehicleDamage.paramAddress) {
                uintptr_t dmgFlagPtr = GameMemory::Read<uintptr_t>(GameMemory::caveNoVehicleDamage.paramAddress);
                if (dmgFlagPtr) GameMemory::Write<uint8_t>(dmgFlagPtr, 0);
            }

            GameMemory::SetPatchState(GameMemory::patchNoFuelUsage, MenuVars::noFuelUsage);
            GameMemory::SetPatchState(GameMemory::patchNoGallonUse, MenuVars::noFuelUsage);
            GameMemory::SetPatchState(GameMemory::patchNoRepairKit, MenuVars::noRepairKitNeed);
            GameMemory::SetPatchState(GameMemory::patchUnlimitedVehicleUV, MenuVars::unlimitedVehicleUV);
            GameMemory::SetCaveState(GameMemory::caveInstantUVRecharge, MenuVars::instantVehicleUV);
            GameMemory::SetCaveState(GameMemory::caveNoUVCooldown, MenuVars::instantVehicleUV);
            GameMemory::SetCaveState(GameMemory::caveRemoveCarCamSnap, MenuVars::removeCarCamSnap);
            GameMemory::SetPatchState(GameMemory::patchRemoveCarCamSnapFlag, MenuVars::removeCarCamSnap);
            GameMemory::SetPatchState(GameMemory::patchChallengeVehicle, MenuVars::challengeModeVehicle);
            GameMemory::SetPatchState(GameMemory::patchNoMoneyCost, MenuVars::noMoneyCost);

            GameMemory::SetCaveState(GameMemory::caveSellPrice, MenuVars::multiplySellPrice);
            if (MenuVars::multiplySellPrice && GameMemory::caveSellPrice.paramAddress)
                GameMemory::Write<float>(GameMemory::caveSellPrice.paramAddress, MenuVars::sellPriceMultiplier);

            GameMemory::SetCaveState(GameMemory::caveBuyPrice, MenuVars::multiplyBuyPrice);
            if (MenuVars::multiplyBuyPrice && GameMemory::caveBuyPrice.paramAddress)
                GameMemory::Write<float>(GameMemory::caveBuyPrice.paramAddress, MenuVars::buyPriceMultiplier);

            bool prevHUD = GameMemory::caveToggleHUD.isEnabled;
            GameMemory::SetCaveState(GameMemory::caveToggleHUD, MenuVars::toggleHUD);
            if (prevHUD && !MenuVars::toggleHUD && GameMemory::caveToggleHUD.paramAddress) {
                uintptr_t hudFlagPtr = GameMemory::Read<uintptr_t>(GameMemory::caveToggleHUD.paramAddress);
                if (hudFlagPtr) GameMemory::Write<uint8_t>(hudFlagPtr, 4);
            }

            GameMemory::SetPatchState(GameMemory::patchWeaponReturn, MenuVars::weaponReturnThrown);
            GameMemory::SetPatchState(GameMemory::patchInstantDismantle, MenuVars::instantDismantle);
            GameMemory::SetPatchState(GameMemory::patchDisableActionEquip, MenuVars::disableActionEquipUse);
            GameMemory::SetPatchState(GameMemory::patchRightHandLoot, MenuVars::rightHandLoot);
            GameMemory::SetCaveState(GameMemory::caveDisableHeadbob, MenuVars::disableHeadbob);

            if (MenuVars::weatherControlEnabled || MenuVars::weatherUiVisible || GameMemory::weather.weOwnForced)
                GameMemory::UpdateWeather(MenuVars::weatherControlEnabled, MenuVars::weatherPreset);
            MenuVars::weatherUiVisible = false;   // вкладка погоды выставит снова, пока открыта
        }

        if (MenuVars::fontWeightPreset == 1 && fontRegular) io.FontDefault = fontRegular;
        else if (MenuVars::fontWeightPreset == 2 && fontBold) io.FontDefault = fontBold;
        else if (fontMedium) io.FontDefault = fontMedium;
        io.FontGlobalScale = MenuVars::fontScale;

        ImGui_ImplDX9_NewFrame(); ImGui_ImplWin32_NewFrame();
        if (isOverlayAllowedToShow && g_ShowMenu) {
            POINT p; if (GetCursorPos(&p)) io.MousePos = ImVec2((float)p.x, (float)p.y);
            io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        }
        ImGui::NewFrame();
        ApplyMenuTheme();

        if (isOverlayAllowedToShow) {
            Fx::RenderGame(ImGui::GetBackgroundDrawList(), io.DeltaTime);
            if (MenuVars::hitCfg.enabled || MenuVars::killCfg.enabled) GameMemory::DrawPosDebug(ImGui::GetForegroundDrawList(), MenuVars::debugPosCandidates);
            RenderWatermark(screenWidth);
            TopNotificationSystem::Render(ImGui::GetForegroundDrawList(), io.DeltaTime, screenWidth);

            if (g_ShowMenu) {
                float mb = MenuVars::textBrightness, db = MenuVars::textMutedBrightness;
                float dbG = (db + 0.03f > 1.0f) ? 1.0f : (db + 0.03f), dbB = (db + 0.07f > 1.0f) ? 1.0f : (db + 0.07f);
                gui.text = { mb, mb, mb, 1.0f }; gui.text_disabled = { db, dbG, dbB, 1.0f };
                ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(mb, mb, mb, 1.00f);
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] = ImVec4(db, dbG, dbB, 1.00f);

                PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                ImGui::Begin("Menu Window", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground); {
                    auto window = GetCurrentWindow(); auto draw = window->DrawList; auto pos = window->Pos; auto size = window->Size;
                    gui.m_anim = ImLerp(gui.m_anim, 1.f, 0.045f);
                    SetWindowSize(ImVec2(760, 580));

                    if (gui.m_design == 1) {
                        gui.draw_modern_logo(draw, pos + ImVec2(18, 16), fontBold);
                        draw->AddLine(pos + ImVec2(14, size.y - 52), pos + ImVec2(156, size.y - 52), IM_COL32(255, 255, 255, 18));
                        gui.draw_modern_status(draw, pos + ImVec2(16, size.y - 44), "Profile", GameMemory::statusMessage, GameMemory::hProcess != nullptr && GameMemory::patternsInitialized);
                    }
                    else {
                        if (fontLogo) {
                            draw->AddText(fontLogo, fontLogo->FontSize, pos + ImVec2(170 / 2 - fontLogo->CalcTextSizeA(fontLogo->FontSize, FLT_MAX, 0, "DLTB.RIP").x / 2 + 1, 20), gui.accent_color.to_im_color(), "DLTB.RIP");
                            draw->AddText(fontLogo, fontLogo->FontSize, pos + ImVec2(170 / 2 - fontLogo->CalcTextSizeA(fontLogo->FontSize, FLT_MAX, 0, "DLTB.RIP").x / 2, 20), GetColorU32(ImGuiCol_Text), "DLTB.RIP");
                        }
                        draw->AddLine(pos + ImVec2(0, size.y - 50), pos + ImVec2(170, size.y - 50), GetColorU32(ImGuiCol_WindowBg, 0.5f));
                        draw->AddText(pos + ImVec2(15, size.y - 40), gui.text.to_im_color(), "Profile");
                        draw->AddText(pos + ImVec2(15, size.y - 25), gui.text_disabled.to_im_color(), "Status:");
                        draw->AddText(pos + ImVec2(15 + CalcTextSize("Status: ").x, size.y - 25), gui.accent_color.to_im_color(), GameMemory::statusMessage);
                    }

                    SetCursorPos(ImVec2(10, 70));
                    BeginChild("##tabs", ImVec2(150, size.y - 120));
                    auto Tab = [&](const char* icon, const char* name, int idx) {
                        if (gui.tab(icon, name, gui.m_tab == idx) && gui.m_tab != idx) gui.m_tab = idx, gui.m_anim = 0.f;
                    };
                    gui.group_title("Gameplay");
                    Tab(ICON_FA_USER, "Player", 0);
                    Tab(ICON_FA_CROSSHAIRS, "Combat", 1);
                    Spacing();
                    gui.group_title("Visuals");
                    Tab(ICON_FA_MAGIC, "Hit & Kill FX", 7);
                    Spacing();
                    gui.group_title("World");
                    Tab(ICON_FA_GLOBE, "Misc", 3);
                    Tab(ICON_FA_PALLET, "Items", 2);
                    Spacing();
                    gui.group_title("Settings");
                    Tab(ICON_FA_PALETTE, "Interface", 5);
                    Tab(ICON_FA_SAVE, "Configs", 6);
                    EndChild();

                    PushStyleVar(ImGuiStyleVar_Alpha, gui.m_anim);
                    PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
                    SetCursorPos(ImVec2(185, 81 - (5 * gui.m_anim)));
                    BeginChild("##childs", ImVec2(size.x - 200, size.y - 96));

                    switch (gui.m_tab) {
                    case 0: {
                        static int playerSubTab = 0;
                        static const char* playerSubNames[] = { "Survival", "Movement", "XP" };
                        gui.subtabs("##playersub", playerSubNames, 3, &playerSubTab);
                        if (playerSubTab == 0) {
                            gui.group_box(ICON_FA_FEATHER " Player Stats", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 310)); {
                                Checkbox("God Mode / Immortal", &MenuVars::godMode); Separator();
                                Checkbox("Unlimited Stamina", &MenuVars::infStamina); Separator();
                                Checkbox("Unlimited UV & Flashlight", &MenuVars::infUVFlashlight); Separator();
                                Checkbox("Unlimited Fury (Beast)", &MenuVars::infFury); Separator();
                                Checkbox("No Fall Damage", &MenuVars::noFallDamage); Separator();
                                Checkbox("No Knockdown & Knockback", &MenuVars::noKnockdown);
                            } gui.end_group_box();
                        }
                        else if (playerSubTab == 1) {
                            gui.group_box(ICON_FA_CROWN " Speed, Jumps & Parkour", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 435)); {
                                Checkbox("Increase Movement Speed", &MenuVars::moveSpeedEnabled);
                                if (MenuVars::moveSpeedEnabled) SliderFloat("Speed", &MenuVars::speedMultiplier, 50.0f, 300.0f, "%.0f");
                                Separator(); Checkbox("Jump Height Multiplier", &MenuVars::jumpHeightEnabled);
                                if (MenuVars::jumpHeightEnabled) SliderFloat("Jump", &MenuVars::jumpMultiplier, 4.85f, 30.0f, "%.2f");
                                Separator(); Checkbox("Unlimited Jumps (Air Jump)", &MenuVars::unlimitedJumps);
                                Separator(); Checkbox("Triple Jump Easter Egg", &MenuVars::tripleJump);
                                Separator(); Checkbox("Always Enable Vault Kick", &MenuVars::alwaysVaultKick);
                                Separator(); Checkbox("Float In Air", &MenuVars::floatInAir);
                                Text("Float In Air Bind (ESC = Clear):");
                                DrawKeybindWidget("FloatAir", MenuVars::floatInAirKey, MenuVars::floatInAirBindMode, MenuVars::waitingFloatKey, "Mode: Toggle", "Mode: Hold");
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_BOLT " Hook, Water & Teleport", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 435)); {
                                Checkbox("Unlimited Cable Length", &MenuVars::unlimitedCableLength); Separator();
                                Checkbox("Disable Hook Pull-Down", &MenuVars::disableHookPullDown); Separator();
                                Checkbox("Swim Speed Modifier", &MenuVars::swimSpeedEnabled);
                                if (MenuVars::swimSpeedEnabled) SliderFloat("Swim Mult", &MenuVars::swimSpeedMultiplier, 1.0f, 15.0f, "%.1fx");
                                Separator(); Checkbox("Disable Water Physics", &MenuVars::disableWaterPhysics);
                                Separator(); Checkbox("Disable Playable Area Check", &MenuVars::disablePlayableArea);
                                Separator();
                                if (Button("Teleport To Waypoint", ImVec2(GetWindowWidth(), 24)) && isGameConnected) GameMemory::TriggerWaypointTeleport();
                                Text("Waypoint TP Bind (ESC = Clear):");
                                DrawKeybindWidget("TPWaypoint", MenuVars::tpWaypointKey, MenuVars::tpWaypointBindMode, MenuVars::waitingTpKey, "Mode: Press", "Mode: Hold");
                                Separator();
                                if (Button("Teleport To Nearest Respawn", ImVec2(GetWindowWidth(), 24)) && isGameConnected) MenuVars::tpRespawnEndTime = GetTickCount64() + 350;
                            } gui.end_group_box();
                        }
                        else if (playerSubTab == 2) {
                            gui.group_box(ICON_FA_STAR " Experience (XP)", ImVec2(GetWindowWidth(), 165)); {
                                Checkbox("Night Exp Multiplier", &MenuVars::nightExpEnabled); Separator();
                                Checkbox("Open World Exp Multiplier", &MenuVars::openWorldExpEnabled); Separator();
                                Checkbox("Disable Death EXP Penalty", &MenuVars::disableDeathPenalty);
                            } gui.end_group_box();
                            if (MenuVars::nightExpEnabled || MenuVars::openWorldExpEnabled) {
                                Spacing();
                                gui.group_box(ICON_FA_USERS " Multiplayer", ImVec2(GetWindowWidth(), 185)); {
                                    if (MenuVars::nightExpEnabled) {
                                        SliderFloat("Night Exp Multiplier", &MenuVars::nightExpMultiplier, 1.0f, 100.0f, "%.1fx");
                                        if (MenuVars::nightExpMultiplier < 0.1f) MenuVars::nightExpMultiplier = 0.1f; Separator();
                                    }
                                    if (MenuVars::openWorldExpEnabled) {
                                        SliderFloat("Open World Exp Multiplier", &MenuVars::openWorldExpMultiplier, 1.0f, 100.0f, "%.1fx");
                                        if (MenuVars::openWorldExpMultiplier < 0.1f) MenuVars::openWorldExpMultiplier = 0.1f;
                                    }
                                } gui.end_group_box();
                            }
                        }
                        break;
                    }
                    case 1: {
                        static int combatSubTab = 0;
                        static const char* combatSubNames[] = { "Melee", "Firearms" };
                        gui.subtabs("##combatsub", combatSubNames, 2, &combatSubTab);
                        if (combatSubTab == 0) {
                            gui.group_box(ICON_FA_FIST_RAISED " Block & Durability", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 320)); {
                                Checkbox("Enable Power Attack Blocking", &MenuVars::powerAttackBlocking); Separator();
                                if (Checkbox("Always Perfect Block / Parry", &MenuVars::alwaysPerfectBlock) && MenuVars::alwaysPerfectBlock) MenuVars::autoBlock = false;
                                Separator();
                                if (Checkbox("Auto Block", &MenuVars::autoBlock) && MenuVars::autoBlock) MenuVars::alwaysPerfectBlock = false;
                                Separator(); Checkbox("Freeze Weapon Durability", &MenuVars::infDurability);
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_FIRE " Damage & Finishers", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 320)); {
                                Checkbox("One Hit Kill", &MenuVars::oneHitKill); Separator();
                                Checkbox("Damage Multiplier", &MenuVars::damageMultEnabled);
                                if (MenuVars::damageMultEnabled) SliderFloat("Dmg Mult", &MenuVars::damageMultiplier, 1.0f, 50.0f, "%.1fx");
                                Separator(); Checkbox("Instant Finisher", &MenuVars::instantFinisher);
                                Separator(); Checkbox("Easy Finisher", &MenuVars::easyFinisher);
                                Separator(); Checkbox("Always Elemental Effect", &MenuVars::alwaysElemental);
                            } gui.end_group_box();
                        }
                        else if (combatSubTab == 1) {
                            gui.group_box(ICON_FA_CROSSHAIRS " Firearm Mechanics", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 360)); {
                                Checkbox("Infinity Ammo & Arrows", &MenuVars::infAmmo); Separator();
                                Checkbox("No Reload", &MenuVars::noReload); Separator();
                                Checkbox("No Recoil", &MenuVars::noRecoil); Separator();
                                Checkbox("Rapid Fire", &MenuVars::rapidFire);
                                if (MenuVars::rapidFire) {
                                    Checkbox("Only When Aiming (RMB)", &MenuVars::rapidFireAdsOnly);
                                    SliderInt("Shot Delay", &MenuVars::rapidFireDelayMs, 25, 250, "%d ms");
                                }
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_BOLT " Firearm Enhancements", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 360)); {
                                Checkbox("Mega Accuracy", &MenuVars::megaAccuracy); Separator();
                                Checkbox("Silenced Firearms", &MenuVars::silencedFirearms); Separator();
                                Checkbox("Bow Rapid Fire", &MenuVars::bowRapidFire);
                            } gui.end_group_box();
                        }
                        break;
                    }
                    case 2: {
                        static int itemsSubTab = 0;
                        static const char* itemsSubNames[] = { "Catalog", "Crafting & Money" };
                        gui.subtabs("##itemssub", itemsSubNames, IM_ARRAYSIZE(itemsSubNames), &itemsSubTab);
                        if (itemsSubTab == 0) {
                            // Item Extravaganza (движок — его рантайм в игре, см. item_menu.hpp)
                            if (ItemMenu::ConnectionPanel(isGameConnected && GameMemory::patternsInitialized)) ItemMenu::DrawCatalog();
                        }
                        else {
                        gui.group_box(ICON_FA_PALLET " Crafting & Loot", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 290)); {
                            Checkbox("Free Craft & Blueprints Upgrade", &MenuVars::freeCraft); Separator();
                            Checkbox("Multiply Gathered Resources", &MenuVars::multiplyGatheredEnabled);
                            if (MenuVars::multiplyGatheredEnabled) SliderInt("Loot Mult", &MenuVars::gatheredMultiplier, 2, 100, "%dx");
                        } gui.end_group_box();
                        SameLine();
                        gui.group_box(ICON_FA_CODE " Money Editor", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 290)); {
                            SliderInt("Add Money", &MenuVars::moneyToAdd, 1000, 100000, "%d");
                            if (Button("Apply Value", ImVec2(GetWindowWidth(), 25)) && isGameConnected) {
                                uintptr_t moneyAddr = GameMemory::GetMoneyValueAddress();
                                if (moneyAddr != 0) GameMemory::Write<int>(moneyAddr, GameMemory::Read<int>(moneyAddr) + MenuVars::moneyToAdd);
                            }
                        } gui.end_group_box();
                        }
                        break;
                    }
                    case 3: {
                        static int worldSubTab = 0;
                        static const char* worldSubNames[] = { "World", "Weather", "AI", "Vehicle", "Vendor", "QoL" };
                        gui.subtabs("##worldsub", worldSubNames, 6, &worldSubTab);
                        if (worldSubTab == 0) {
                        gui.group_box(ICON_FA_HAMMER " Time & World Speed", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 390)); {
                            Checkbox("Pause World Time", &MenuVars::freezeTime); Separator();
                            Checkbox("Force Daytime (Exact Hour)", &MenuVars::forceDaytimeEnabled);
                            if (MenuVars::forceDaytimeEnabled) {
                                SliderFloat("World Hour", &MenuVars::worldTime, 0.0f, 23.9f, "%.1f h");
                                if (MenuVars::worldTime < 0.0f) MenuVars::worldTime = 0.0f;
                                if (MenuVars::worldTime > 23.9f) MenuVars::worldTime = 23.9f;
                            }
                            Separator(); Checkbox("Force Day Phase", &MenuVars::forceDayPhaseEnabled);
                            if (MenuVars::forceDayPhaseEnabled) Combo("Phase", &MenuVars::dayPhaseIndex, dayPhaseNames, IM_ARRAYSIZE(dayPhaseNames));
                            Separator(); Checkbox("Change World Speed", &MenuVars::worldSpeedEnabled);
                            if (MenuVars::worldSpeedEnabled) SliderFloat("Speed Mult", &MenuVars::worldSpeedValue, 0.5f, 5.0f, "%.1fx");
                            Separator(); Checkbox("Disable World Noise (No Attract)", &MenuVars::disableWorldNoise);
                        } gui.end_group_box();
                        SameLine();
                        gui.group_box(ICON_FA_PALLET " Map & World Locks", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 390)); {
                            Checkbox("Remove Fog Of War (Walk 1 Step)", &MenuVars::removeFogOfWar); Separator();
                            ULONGLONG now = GetTickCount64();
                            if (now < MenuVars::discoverMapEndTime) {
                                char btnLabel[64]; snprintf(btnLabel, sizeof(btnLabel), "Unlocking Map... (%ds)", (int)((MenuVars::discoverMapEndTime - now) / 1000) + 1);
                                Button(btnLabel, ImVec2(GetWindowWidth(), 25));
                            }
                            else if (Button("Discover All Map Locations (10s)", ImVec2(GetWindowWidth(), 25)) && isGameConnected) {
                                MenuVars::discoverMapEndTime = now + 10000;
                            }
                            Separator(); Checkbox("Edit Zone Level", &MenuVars::editZoneLevelEnabled);
                            if (MenuVars::editZoneLevelEnabled) { SliderInt("Min Level", &MenuVars::zoneMinLevel, 1, 30); SliderInt("Max Level", &MenuVars::zoneMaxLevel, 1, 30); }
                            Separator(); Checkbox("Open Locked Doors (Other Side)", &MenuVars::openLockedDoors);
                            Separator(); Checkbox("Auto Unlock & Easy Lockpick", &MenuVars::easyLockpick);
                        } gui.end_group_box();
                        }
                        else if (worldSubTab == 1) {
                            // Погода: поля TimeWeather::CSystem движка (см. GameMemory::UpdateWeather)
                            // Порядок EWeather — по таблице переходов движка (engine+0xDD68A0, выбор аудио-свича):
                            // 0 fog, 1 clear (по умолчанию), 2 overcast, 3 cloudy, 4 rain, 5 storm, 6..9 — снег/wildcard (аудио storm)
                            static const char* weatherNames[GameMemory::kWeatherCount] = { "Foggy", "Clear", "Overcast", "Cloudy", "Rainy", "Stormy", "Light Snow", "Snow", "Blizzard", "Wildcard" };
                            static const int weatherOrder[GameMemory::kWeatherCount] = { 1, 3, 0, 2, 4, 5, 6, 7, 8, 9 };   // кнопки: Clear, Cloudy, Foggy, Overcast, ...
                            MenuVars::weatherUiVisible = true;
                            const auto& ws = GameMemory::weather;
                            const float wHalf = GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2;
                            auto NameOf = [&](int v) { return (v >= 0 && v < GameMemory::kWeatherCount) ? weatherNames[v] : "Game default"; };
                            auto HourText = [](float h, char* buf, size_t n) { if (h < 0.0f) { snprintf(buf, n, "--:--"); return; } int m = (int)(h * 60.0f + 0.5f) % (24 * 60); snprintf(buf, n, "%02d:%02d", m / 60, m % 60); };
                            // ближайший слот (по кругу суток), где у погоды есть пресеты
                            auto NextHour = [&](int w) -> float {
                                if (ws.scopeCount <= 0 || ws.currentScope < 0) return -1.0f;
                                for (int k = 1; k <= ws.scopeCount; ++k) { int i = (ws.currentScope + k) % ws.scopeCount; if (ws.scopePresets[i][w]) return ws.scopeStart[i]; }
                                return -1.0f;
                            };
                            if (MenuVars::weatherPreset < 0 || MenuVars::weatherPreset >= GameMemory::kWeatherCount) MenuVars::weatherPreset = 4;
                            gui.group_box(ICON_FA_GLOBE " Weather Control", ImVec2(wHalf, 330)); {
                                Checkbox("Force Weather", &MenuVars::weatherControlEnabled); Separator();
                                TextDisabled("Click a weather to apply it:");
                                const float bw = GetWindowWidth() / 2 - 4;
                                for (int slot = 0; slot < GameMemory::kWeatherCount; ++slot) {
                                    const int i = weatherOrder[slot];
                                    char label[64], at[16];
                                    const bool active = MenuVars::weatherControlEnabled && MenuVars::weatherPreset == i;
                                    const bool now = !ws.sys || GameMemory::WeatherAvailableNow(i);
                                    if (now) snprintf(label, sizeof(label), active ? ICON_FA_CHECK " %s##w%d" : "%s##w%d", weatherNames[i], i);
                                    else { HourText(NextHour(i), at, sizeof(at)); snprintf(label, sizeof(label), "%s (%s)##w%d", weatherNames[i], at, i); }
                                    if (slot & 1) SameLine();
                                    if (Button(label, ImVec2(bw, 26))) {
                                        MenuVars::weatherPreset = i; MenuVars::weatherControlEnabled = true;
                                        const float h = now ? -1.0f : NextHour(i);
                                        if (h >= 0.0f) {
                                            // на 6 минут вглубь слота, чтобы движок точно выбрал его
                                            float target = h + 0.1f; if (target >= 24.0f) target -= 24.0f;
                                            if (!GameMemory::caveForceDayTime1.paramAddress && !GameMemory::caveForceDayTime2.paramAddress)
                                                TopNotificationSystem::Push("Weather", "Time control isn't available on this game build", ImVec4(1.0f, 0.55f, 0.35f, 1.0f), 3.0f, ICON_FA_EXCLAMATION_TRIANGLE);
                                            else {
                                                if (MenuVars::forceDaytimeEnabled) MenuVars::worldTime = target;   // время уже зафиксировано — двигаем фиксацию
                                                else { MenuVars::timeJumpHour = target; MenuVars::timeJumpUntil = GetTickCount64() + 1500; }
                                                char jumpMsg[64]; HourText(target, at, sizeof(at)); snprintf(jumpMsg, sizeof(jumpMsg), "Time moved to %s for %s", at, weatherNames[i]);
                                                TopNotificationSystem::Push("Weather", jumpMsg, ImVec4(0.3f, 0.85f, 0.5f, 1.0f), 3.0f, ICON_FA_GLOBE);
                                            }
                                        }
                                    }
                                }
                                Spacing();
                                if (Button("Return Weather To Game", ImVec2(GetWindowWidth(), 25))) MenuVars::weatherControlEnabled = false;
                                PushTextWrapPos(GetWindowWidth() - 4.0f);
                                TextDisabled("A time in brackets = that weather only exists from that hour; clicking it moves the game clock there.");
                                PopTextWrapPos();
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_STAR " Weather Status", ImVec2(wHalf, 330)); {
                                if (!isGameConnected || !GameMemory::patternsInitialized) TextDisabled("Load into the game first.");
                                else if (!ws.sys) {
                                    TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Weather system not found yet.");
                                    PushTextWrapPos(GetWindowWidth() - 4.0f);
                                    TextDisabled("It is picked up from the level once you are in the open world; give it a second after loading.");
                                    Separator();
                                    TextDisabled("Diagnostics (send a screenshot of this):");
                                    TextDisabled("%s", GameMemory::weatherDiag);
                                    PopTextWrapPos();
                                }
                                else {
                                    const ImVec4 green(0.3f, 0.85f, 0.5f, 1.0f), warn(1.0f, 0.7f, 0.3f, 1.0f);
                                    char t[16]; HourText(ws.daytime, t, sizeof(t));
                                    Text("Time slot:"); SameLine(); TextColored(ImVec4(gui.accent_color.r, gui.accent_color.g, gui.accent_color.b, 1.0f), "%s", t);
                                    Text("Current:"); SameLine(); TextColored(ImVec4(gui.accent_color.r, gui.accent_color.g, gui.accent_color.b, 1.0f), "%s", NameOf(ws.current));
                                    Text("Forced:"); SameLine(); TextColored(ws.forced >= 0 ? green : gui.text_disabled.to_vec4(1.0f, false), "%s", ws.forced >= 0 ? NameOf(ws.forced) : "None (game decides)");
                                    Separator();
                                    PushTextWrapPos(GetWindowWidth() - 4.0f);
                                    if (ws.forced >= 0 && !GameMemory::WeatherAvailableNow(ws.forced)) {
                                        char at[16]; HourText(NextHour(ws.forced), at, sizeof(at));
                                        TextColored(warn, "%s has no presets at this hour, so the engine keeps the current weather. It switches at %s.", NameOf(ws.forced), at);
                                    }
                                    else if (ws.forced >= 0 && ws.current != ws.forced) TextColored(warn, "Switching to %s...", NameOf(ws.forced));
                                    else if (ws.forced >= 0) TextColored(green, "%s is active.", NameOf(ws.forced));
                                    else TextDisabled("Pick a weather on the left to force it.");
                                    PopTextWrapPos();
                                    Separator();
                                    TextDisabled("Available right now:");
                                    for (int slot = 0; slot < GameMemory::kWeatherCount; ++slot) {
                                        const int i = weatherOrder[slot];
                                        const bool ok = GameMemory::WeatherAvailableNow(i);
                                        TextColored(ok ? green : gui.text_disabled.to_vec4(1.0f, false), "%s %s", ok ? ICON_FA_CHECK : ICON_FA_TIMES, weatherNames[i]);
                                        if (!(slot & 1)) SameLine(GetWindowWidth() / 2);
                                    }
                                }
                            } gui.end_group_box();
                        }
                        else if (worldSubTab == 2) {
                            gui.group_box(ICON_FA_USER " AI Scale & Behavior", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 330)); {
                                Checkbox("Change AI Scale", &MenuVars::changeAIScaleEnabled);
                                if (MenuVars::changeAIScaleEnabled) {
                                    SliderFloat("AI Scale", &MenuVars::aiScaleValue, 0.1f, 10.0f, "%.1fx");
                                    if (MenuVars::aiScaleValue < 0.05f) MenuVars::aiScaleValue = 0.05f;
                                }
                                Separator(); Checkbox("Invisibility (Ignore Player)", &MenuVars::aiInvisibility);
                                Separator(); Checkbox("Disable Zombie Grab", &MenuVars::disableZombieGrab);
                                Separator(); Checkbox("Disable Chase Logic", &MenuVars::disableChaseLogic);
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_BOLT " AI Actions & Control", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 330)); {
                                Checkbox("Stomp Anywhere", &MenuVars::alwaysStomp); Separator();
                                Checkbox("Freeze & Kill Nearby AIs", &MenuVars::freezeKillNearbyAI); Separator();
                                Checkbox("Burn Nearby AIs", &MenuVars::burnNearbyAI); Separator();
                                Checkbox("Disable AI Spawn", &MenuVars::disableAISpawn);
                            } gui.end_group_box();
                        }
                        else if (worldSubTab == 3) {
                            gui.group_box(ICON_FA_CROWN " Vehicle Durability & Fuel", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 320)); {
                                Checkbox("No Vehicle Damage (God Car)", &MenuVars::noVehicleDamage); Separator();
                                Checkbox("No Fuel & Canister Usage", &MenuVars::noFuelUsage); Separator();
                                Checkbox("No Repair Kit Need", &MenuVars::noRepairKitNeed); Separator();
                                Checkbox("Enable Challenge Mode Vehicle", &MenuVars::challengeModeVehicle);
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_BOLT " Vehicle UV & Camera", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 320)); {
                                Checkbox("Unlimited UV Light Active Time", &MenuVars::unlimitedVehicleUV); Separator();
                                Checkbox("Instant UV Recharge & No Cooldown", &MenuVars::instantVehicleUV); Separator();
                                Checkbox("Remove Car Camera Snap", &MenuVars::removeCarCamSnap);
                            } gui.end_group_box();
                        }
                        else if (worldSubTab == 4) {
                            gui.group_box(ICON_FA_PALLET " Vendor & Shop", ImVec2(GetWindowWidth(), 310)); {
                                Checkbox("No Money Cost (Free Purchases)", &MenuVars::noMoneyCost); Separator();
                                Checkbox("Multiply Sell Price", &MenuVars::multiplySellPrice);
                                if (MenuVars::multiplySellPrice) {
                                    SliderFloat("Sell Price Multiplier", &MenuVars::sellPriceMultiplier, 1.0f, 100.0f, "%.1fx");
                                }
                                Separator(); Checkbox("Multiply Buy Price", &MenuVars::multiplyBuyPrice);
                                if (MenuVars::multiplyBuyPrice) {
                                    SliderFloat("Buy Price Multiplier", &MenuVars::buyPriceMultiplier, 0.0001f, 2.0f, "%.4fx");
                                }
                            } gui.end_group_box();
                        }
                        else {
                            gui.group_box(ICON_FA_COG " QoL & Camera", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 320)); {
                                Checkbox("Toggle HUD [Bind: F8]", &MenuVars::toggleHUD); Separator();
                                Checkbox("Weapon Return When Thrown", &MenuVars::weaponReturnThrown); Separator();
                                Checkbox("Disable Headbob", &MenuVars::disableHeadbob);
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_FEATHER " Fast Actions", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 320)); {
                                Checkbox("Instant Dismantle", &MenuVars::instantDismantle); Separator();
                                Checkbox("Disable Action Equipment Use", &MenuVars::disableActionEquipUse); Separator();
                                Checkbox("Enable Right Hand Item Pickup", &MenuVars::rightHandLoot);
                            } gui.end_group_box();
                        }
                        break;
                    }
                    case 7: {
                        static const char* hitStyleNames[] = { "Claw Slash", "Blood X", "Classic Cross", "Blood Splatter", "Bone Crack", "UV Pulse" };
                        static const char* hitPosNames[] = { "Crosshair (screen center)", "On Enemy (3D)" };
                        static const char* fxSubNames[] = { "Hitmarkers", "Kill Effects", "Kill Style", "Tools" };
                        static int fxSubTab = 0;
                        HitmarkerConfig& hc = MenuVars::hitCfg;
                        KillFxConfig& kc = MenuVars::killCfg;
                        gui.subtabs("##fxsub", fxSubNames, 4, &fxSubTab);
                        const float halfW = GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2;
                        auto LivePreview = [&]() {
                            gui.group_box(ICON_FA_STAR " Live Preview", ImVec2(halfW, 440)); {
                                ImVec2 cp = GetCursorScreenPos();
                                ImVec2 cs(GetWindowWidth() - 4.0f, 410.0f);
                                ImDrawList* pdl = GetWindowDrawList();
                                Fx::RenderPreview(pdl, cp, ImVec2(cp.x + cs.x, cp.y + cs.y), io.DeltaTime, hc, kc);
                                pdl->AddRect(cp, ImVec2(cp.x + cs.x, cp.y + cs.y), IM_COL32(35, 45, 65, 255), 4.0f);
                                Dummy(cs);
                            } gui.end_group_box();
                        };

                        if (fxSubTab == 0) {
                            gui.group_box(ICON_FA_CROSSHAIRS " Hitmarkers", ImVec2(halfW, 440)); {
                                Checkbox("Enable Hitmarkers", &hc.enabled);
                                Separator();
                                Combo("Style", &hc.style, hitStyleNames, IM_ARRAYSIZE(hitStyleNames));
                                Combo("Position", &hc.positionMode, hitPosNames, IM_ARRAYSIZE(hitPosNames));
                                Text("Hit Color"); SameLine(GetWindowWidth() - 45);
                                PushID("hitCol"); ColorEdit4("", hc.color, ALPHA); PopID();
                                Text("Kill Confirm Color"); SameLine(GetWindowWidth() - 45);
                                PushID("hitKillCol"); ColorEdit4("", hc.killColor, ALPHA); PopID();
                                SliderFloat("Size", &hc.size, 5.0f, 28.0f, "%.0f px");
                                SliderFloat("Thickness", &hc.thickness, 1.0f, 5.0f, "%.1f px");
                                SliderFloat("Duration", &hc.duration, 0.20f, 1.20f, "%.2f s");
                                Checkbox("Glow", &hc.glow);
                                Separator();
                                Checkbox("Damage Numbers", &hc.damageNumbers); SameLine(GetWindowWidth() - 45);
                                PushID("dmgCol"); ColorEdit4("", hc.damageColor, ALPHA); PopID();
                                Separator();
                                if (hc.sound < 0 || hc.sound >= ChipSound::kHitCount) hc.sound = 1; // старые конфиги: прежние номера звуков
                                Combo("Hit Sound", &hc.sound, ChipSound::kHitNames, ChipSound::kHitCount);
                                if (Button(ICON_FA_PLAY " Play Hit Sound", ImVec2(GetWindowWidth(), 22))) ChipSound::PlayHit(hc.sound, MenuVars::sfxVolume);
                            } gui.end_group_box();
                            SameLine();
                            LivePreview();
                        }
                        else if (fxSubTab == 1) {
                            gui.group_box(ICON_FA_BOLT " Kill Effects (mix any)", ImVec2(halfW, 440)); {
                                PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
                                Checkbox("Enable Kill Effects", &kc.enabled);
                                static const char* fxPresetNames[] = { "Custom", "Gore", "Rainbow Party", "Crystal", "Crystal + Rainbow", "Everything", "Magic" };
                                static int fxPreset = 0;
                                if (Combo("Quick Preset", &fxPreset, fxPresetNames, IM_ARRAYSIZE(fxPresetNames))) Fx::ApplyKillPreset(kc, fxPreset);
                                Separator();
                                // две колонки переключателей — все эффекты сочетаются друг с другом
                                const float colW = GetWindowWidth() / 2 - 4;
                                BeginChild("##kfxColA", ImVec2(colW, 300), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
                                TextDisabled(ICON_FA_SKULL "  Horror");
                                Checkbox("Blood Burst", &kc.bloodBurst);
                                Checkbox("Gore Chunks", &kc.goreChunks);
                                Checkbox("Blood Pool", &kc.bloodPool);
                                Checkbox("Blood Mist", &kc.bloodMist);
                                Checkbox("Ghost Rise", &kc.ghostRise);
                                TextDisabled(ICON_FA_GEM "  Crystal");
                                Checkbox("Shatter", &kc.crystalShatter);
                                Checkbox("Spikes", &kc.crystalSpikes);
                                Checkbox("Frost Nova", &kc.frostNova);
                                EndChild();
                                SameLine();
                                BeginChild("##kfxColB", ImVec2(colW, 300), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
                                TextDisabled(ICON_FA_MAGIC "  Rainbow");
                                Checkbox("Confetti", &kc.confetti);
                                Checkbox("Shockwave", &kc.rainbowRing);
                                Checkbox("Rays", &kc.rainbowRays);
                                Checkbox("Sparkles", &kc.rainbowSparkles);
                                Checkbox("Fireworks", &kc.fireworks);
                                TextDisabled(ICON_FA_BOLT "  Magic");
                                Checkbox("UV Burn", &kc.uvBurn);
                                Checkbox("Vortex", &kc.vortex);
                                EndChild();
                                PopStyleVar();
                            } gui.end_group_box();
                            SameLine();
                            LivePreview();
                        }
                        else if (fxSubTab == 2) {
                            gui.group_box(ICON_FA_PALETTE " Colors & Size", ImVec2(halfW, 440)); {
                                Text("Blood Color"); SameLine(GetWindowWidth() - 45);
                                PushID("kPrim"); ColorEdit4("", kc.primary, ALPHA); PopID();
                                Text("UV Color"); SameLine(GetWindowWidth() - 45);
                                PushID("kSec"); ColorEdit4("", kc.secondary, ALPHA); PopID();
                                Text("Crystal Color"); SameLine(GetWindowWidth() - 45);
                                PushID("kCry"); ColorEdit4("", kc.crystalColor, ALPHA); PopID();
                                Checkbox("Rainbow Crystals", &kc.rainbowCrystals);
                                SliderFloat("Rainbow Speed", &kc.rainbowSpeed, 0.0f, 3.0f, "%.1fx");
                                Separator();
                                SliderFloat("Effect Scale", &kc.scale, 0.5f, 2.0f, "%.2fx");
                                SliderFloat("Effect Duration", &kc.duration, 1.0f, 6.0f, "%.1f s");
                                if (kc.bloodBurst) SliderInt("Blood Drops", &kc.particleCount, 12, 160, "%d");
                                if (kc.confetti) SliderInt("Confetti Pieces", &kc.confettiCount, 20, 160, "%d");
                            } gui.end_group_box();
                            SameLine();
                            gui.group_box(ICON_FA_COG " Screen, Text & Sound", ImVec2(halfW, 440)); {
                                Checkbox("Screen Blood (close kills)", &kc.screenSplatter);
                                Checkbox("Kill Label", &kc.killLabel);
                                if (kc.killLabel) InputText("Label Text", kc.labelText, sizeof(kc.labelText));
                                Checkbox("Kill Streak Announcer", &kc.streakAnnouncer);
                                Separator();
                                if (kc.sound < 0 || kc.sound >= ChipSound::kKillCount) kc.sound = 1;
                                Combo("Kill Sound", &kc.sound, ChipSound::kKillNames, ChipSound::kKillCount);
                                Checkbox("Pitch Up On Kill Streak", &kc.pitchStreak);
                                if (Button(ICON_FA_PLAY " Play Kill Sound", ImVec2(GetWindowWidth(), 22))) ChipSound::PlayKill(kc.sound, MenuVars::sfxVolume);
                            } gui.end_group_box();
                        }
                        else {
                            gui.group_box(ICON_FA_COG " Sound, Camera & Diagnostics", ImVec2(halfW, 440)); {
                                SliderFloat("SFX Volume", &MenuVars::sfxVolume, 0.0f, 1.0f, "%.2f");
                                Separator();
                                if (Button("Test Hit In-Game", ImVec2(GetWindowWidth() / 2 - 4, 24))) {
                                    if (Fx::gameCam.valid) {
                                        Vec3 pnt = Fx::Add(Fx::gameCam.pos, Fx::Mul(Fx::gameCam.fwd, 4.0f));
                                        HitmarkerConfig t = hc; t.enabled = true;
                                        Fx::SpawnHit(Fx::game, t, 0, 0x7E57, &pnt, g_GameView.Center(), 27.0f);
                                    }
                                    else { HitmarkerConfig t = hc; t.enabled = true; Fx::SpawnHit(Fx::game, t, 0, 0x7E57, nullptr, g_GameView.Center(), 27.0f); }
                                    Fx::PlayHitSound(hc);
                                }
                                SameLine();
                                if (Button("Test Kill In-Game", ImVec2(GetWindowWidth() / 2 - 4, 24))) {
                                    KillFxConfig t = kc; t.enabled = true;
                                    if (Fx::gameCam.valid) {
                                        Vec3 f = Fx::Norm(Fx::V(Fx::gameCam.fwd.x, 0.0f, Fx::gameCam.fwd.z));
                                        Vec3 feet = Fx::Add(Fx::gameCam.pos, Fx::Mul(f, 5.0f)); feet.y -= 1.65f;
                                        int st = Fx::SpawnKill(Fx::game, t, &feet);
                                        Fx::PlayKillSound(kc, st);
                                    }
                                    else TopNotificationSystem::Push("Camera not ready", "Load into the game world first", ImVec4(1.0f, 0.65f, 0.25f, 1.0f), 3.0f, ICON_FA_EXCLAMATION_TRIANGLE);
                                }
                                Separator();
                                const char* camSrc = (Fx::cameraSource == 1) ? "Engine camera (exact)" : (Fx::cameraSource == 2 ? "Fallback (estimated)" : "Not available");
                                ImVec4 camCol = (Fx::cameraSource == 1) ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : (Fx::cameraSource == 2 ? ImVec4(1.0f, 0.75f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.65f, 1.0f));
                                Text("Camera:"); SameLine(); TextColored(camCol, "%s", camSrc);
                                TextDisabled("%s", GameMemory::camDiag);
                                {
                                    const auto& dg = GameMemory::fxDiag;
                                    TextDisabled("Target: exact %u / guessed %u", GameMemory::exactLinks, GameMemory::guessedLinks);
                                    TextDisabled("Enemy xform: ok %u / fail %u", GameMemory::entPosOk, GameMemory::entPosFail);
                                    TextDisabled("  via table %u / via ptr %u", GameMemory::entPosViaReg, GameMemory::entPosViaPtr);
                                    TextDisabled("Hits %u | Kills %u (engine %u, low HP %u, vanished %u)", dg.hits, dg.kills, dg.killsEngine, dg.killsLowHp, dg.killsVanished);
                                    if (GameMemory::compPosOk > 0) TextDisabled("Enemy positions: engine entity xform (ok %u / fail %u)", GameMemory::compPosOk, GameMemory::compPosFail);
                                    else { TextDisabled("Enemy pos: %s", GameMemory::aiPosStatus); TextDisabled("Entity path: ok %u / fail %u | pairs %d", GameMemory::compPosOk, GameMemory::compPosFail, GameMemory::lastAiPairCount); }
                                    TextDisabled("Raw engine kills %u | no position %u | hits on you %u | targets %d", dg.engineKillEvents, dg.killsNoPos, dg.playerHits, (int)Fx::trackedEntities.size());
                                    Checkbox("Debug: Show Position Candidates", &MenuVars::debugPosCandidates);
                                    if (MenuVars::debugPosCandidates) {
                                        TextDisabled("Pick the number whose dot sits on the zombie's feet:");
                                        if (RadioButton("Auto", MenuVars::hitPosSourceKey == 0)) MenuVars::hitPosSourceKey = 0;
                                        for (size_t ci = 0; ci < GameMemory::posCandidates.size(); ++ci) {
                                            char desc[64], lbl[128];
                                            GameMemory::DescribePosKey(GameMemory::posCandidates[ci].key, desc, sizeof(desc));
                                            snprintf(lbl, sizeof(lbl), "#%d %s (votes %d)", (int)ci + 1, desc, GameMemory::posCandidates[ci].votes);
                                            if (RadioButton(lbl, MenuVars::hitPosSourceKey == GameMemory::posCandidates[ci].key)) MenuVars::hitPosSourceKey = GameMemory::posCandidates[ci].key;
                                        }
                                    }
                                    if (Button("Save AI Debug Dump", ImVec2(GetWindowWidth(), 22))) {
                                        std::string dumpPath;
                                        if (GameMemory::SaveAiDebugDump(dumpPath)) TopNotificationSystem::Push("Debug dump saved", "ai_debug_dump.bin next to the .exe", ImVec4(0.28f, 0.78f, 1.0f, 1.0f), 4.0f, ICON_FA_SAVE);
                                        else TopNotificationSystem::Push("Dump failed", "Game not attached?", ImVec4(1.0f, 0.35f, 0.35f, 1.0f), 4.0f, ICON_FA_EXCLAMATION_TRIANGLE);
                                    }
                                    ULONGLONG nowT = GetTickCount64();
                                    for (int i = 0; i < GameMemory::FxDiag::kLog; ++i) {
                                        const auto& e = dg.log[(dg.logPos + GameMemory::FxDiag::kLog - 1 - i) % GameMemory::FxDiag::kLog];
                                        if (e.tick == 0 || nowT - e.tick > 15000) continue;
                                        TextDisabled("  HP %.1f -> %.1f  (dmg %.1f)%s%s%s", e.oldHp, e.oldHp - e.dmg, e.dmg,
                                            e.player ? "  [you]" : "", e.attacking ? "" : "  [no input]", e.kill ? "  [KILL]" : "");
                                    }
                                }
                                SliderFloat("Hit Point Height", &MenuVars::hitHeightOffset, 0.4f, 2.0f, "%.2f m");
                                Checkbox("Scale Hitmarker By Distance", &MenuVars::hitDistanceScale);
                                if (Fx::cameraSource != 1) {
                                    Separator();
                                    TextDisabled("Fallback camera calibration:");
                                    SliderFloat("Game FOV (horizontal)", &MenuVars::hitProjFov, 50.0f, 130.0f, "%.0f deg");
                                    SliderFloat("Camera Eye Height", &MenuVars::hitEyeHeight, 1.0f, 2.2f, "%.2f m");
                                    Checkbox("Invert Camera Pitch", &MenuVars::hitInvertPitch);
                                }
                            } gui.end_group_box();
                            SameLine();
                            LivePreview();
                        }
                        break;
                    }
                    case 5: {
                        static int uiSubTab = 0;
                        static const char* uiSubNames[] = { "Menu Theme", "Watermark & Text" };
                        gui.subtabs("##uisub", uiSubNames, 2, &uiSubTab);
                        if (uiSubTab == 0) {
                        const float themeW = GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2;
                        const bool modern = MenuVars::menuDesign == 1;
                        gui.group_box(ICON_FA_PALLET " Menu Design", ImVec2(themeW, 330)); {
                            Combo("Design", &MenuVars::menuDesign, menuDesignNames, IM_ARRAYSIZE(menuDesignNames)); Separator();
                            if (modern) {
                                static const char* presetNames[kMenuPresetCount];
                                for (int i = 0; i < kMenuPresetCount; ++i) presetNames[i] = kMenuPresets[i].name;
                                if (Combo("Color Preset", &MenuVars::menuPreset, presetNames, kMenuPresetCount)) ApplyMenuPreset(MenuVars::menuPreset);
                                Separator();
                                SliderFloat("Glass Opacity", &MenuVars::menuOpacity, 0.60f, 1.00f, "%.2f");
                                SliderFloat("Corner Rounding", &MenuVars::menuRounding, 0.0f, 16.0f, "%.0f px");
                                Separator();
                                Checkbox("Accent Shimmer", &MenuVars::menuShimmer);
                                Checkbox("Accent Glow", &MenuVars::menuGlow);
                                Checkbox("Watermark Uses Menu Colors", &MenuVars::watermarkFollowMenu);
                            }
                            else {
                                TextDisabled("Classic keeps the original design.");
                                TextDisabled("Switch to Modern to customize colors.");
                            }
                        } gui.end_group_box();
                        SameLine();
                        gui.group_box(ICON_FA_STAR " Theme Colors", ImVec2(themeW, 330)); {
                            if (modern) {
                                auto ColorRow = [](const char* label, const char* id, float* col) {
                                    Text("%s", label); SameLine(GetWindowWidth() - 45);
                                    PushID(id); ColorEdit4("", col, NO_ALPHA); PopID();
                                };
                                ColorRow("Accent", "mAcc", MenuVars::menuAccent);
                                ColorRow("Background", "mBg", MenuVars::menuBg);
                                ColorRow("Panels", "mPanel", MenuVars::menuPanel);
                                ColorRow("Widgets", "mFrame", MenuVars::menuFrame);
                                ColorRow("Borders", "mBorder", MenuVars::menuBorder);
                                Separator();
                                if (Button("Reset To Preset", ImVec2(GetWindowWidth(), 25))) ApplyMenuPreset(MenuVars::menuPreset);
                            }
                            else TextDisabled("Available in the Modern design.");
                        } gui.end_group_box();
                        }
                        else {
                        gui.group_box(ICON_FA_STAR " Watermark", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 410)); {
                            Checkbox("Enable Watermark", &MenuVars::watermarkEnabled); Separator();
                            Combo("Design", &MenuVars::watermarkDesign, watermarkDesignNames, IM_ARRAYSIZE(watermarkDesignNames)); Separator();
                            if (MenuVars::watermarkDesign == 0) { Combo("Style", &MenuVars::watermarkStyle, watermarkStyleNames, IM_ARRAYSIZE(watermarkStyleNames)); Separator(); }
                            else {
                                Checkbox("Show Game Status", &MenuVars::watermarkShowStatus);
                                Checkbox("Accent Shimmer##wm", &MenuVars::watermarkShimmer); Separator();
                            }
                            Checkbox("Show Custom Text", &MenuVars::watermarkShowCustomText);
                            if (MenuVars::watermarkShowCustomText) {
                                InputText("Custom Text", MenuVars::watermarkCustomText, sizeof(MenuVars::watermarkCustomText));
                            }
                            Separator();
                            Checkbox("Show Time", &MenuVars::watermarkShowTime); Separator();
                            Checkbox("Show FPS", &MenuVars::watermarkShowFPS); Separator();
                            SliderFloat("Bg Opacity", &MenuVars::watermarkBgAlpha, 0.20f, 1.00f, "%.2f"); Separator();
                            SliderFloat("Offset X (Right)", &MenuVars::watermarkOffsetX, 5.0f, 150.0f, "%.0f px");
                            SliderFloat("Offset Y (Top)", &MenuVars::watermarkOffsetY, 5.0f, 150.0f, "%.0f px");
                        } gui.end_group_box();
                        SameLine();
                        gui.group_box(ICON_FA_FEATHER " Font & Text Settings", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 410)); {
                            Combo("Font Weight", &MenuVars::fontWeightPreset, fontWeightNames, IM_ARRAYSIZE(fontWeightNames)); Separator();
                            SliderFloat("Text Brightness", &MenuVars::textBrightness, 0.45f, 1.00f, "%.2f"); Separator();
                            SliderFloat("Muted Text Brightness", &MenuVars::textMutedBrightness, 0.30f, 0.90f, "%.2f"); Separator();
                            SliderFloat("Font Scale", &MenuVars::fontScale, 0.85f, 1.25f, "%.2fx"); Separator();
                            if (Button("Reset Font Defaults", ImVec2(GetWindowWidth(), 25))) {
                                MenuVars::fontWeightPreset = 0; MenuVars::textBrightness = 0.86f; MenuVars::textMutedBrightness = 0.65f; MenuVars::fontScale = 1.00f;
                            }
                        } gui.end_group_box();
                        }
                        break;
                    }
                    case 6: {
                        gui.group_box(ICON_FA_PALLET " Config List", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
                            BeginChild("##cfg_list", ImVec2(GetWindowWidth(), 240), true);
                            for (int i = 0; i < (int)ConfigSystem::configFiles.size(); ++i) {
                                if (Selectable(ConfigSystem::configFiles[i].c_str(), ConfigSystem::selectedConfig == i)) {
                                    ConfigSystem::selectedConfig = i;
                                    snprintf(ConfigSystem::renameConfigBuf, sizeof(ConfigSystem::renameConfigBuf), "%s", ConfigSystem::configFiles[i].c_str());
                                }
                            }
                            EndChild();
                            if (Button("Refresh List", ImVec2(GetWindowWidth(), 25))) ConfigSystem::RefreshConfigs();
                        } gui.end_group_box();
                        SameLine();
                        gui.group_box(ICON_FA_SAVE " Config Manager", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
                            InputText("Create New Config", ConfigSystem::newConfigName, sizeof(ConfigSystem::newConfigName));
                            if (Button("Create & Save", ImVec2(GetWindowWidth(), 25)) && strlen(ConfigSystem::newConfigName) > 0)
                                ConfigSystem::Save(ConfigSystem::newConfigName);
                            Separator();
                            Text("Unload Cheat Key:");
                            DrawKeybindWidget("UnloadKeyCfg", MenuVars::unloadKey, MenuVars::unloadBindMode, MenuVars::waitingUnloadKey, "Mode: Press", "Mode: Hold");
                            Separator();
                            if (Button("Load Selected", ImVec2(GetWindowWidth() / 2 - 4, 26)) && !ConfigSystem::configFiles.empty())
                                ConfigSystem::Load(ConfigSystem::configFiles[ConfigSystem::selectedConfig]);
                            SameLine();
                            if (Button("Save Selected", ImVec2(GetWindowWidth() / 2 - 4, 26)) && !ConfigSystem::configFiles.empty())
                                ConfigSystem::Save(ConfigSystem::configFiles[ConfigSystem::selectedConfig]);
                            if (Button("Delete Selected", ImVec2(GetWindowWidth(), 25)) && !ConfigSystem::configFiles.empty())
                                ConfigSystem::Delete(ConfigSystem::configFiles[ConfigSystem::selectedConfig]);
                        } gui.end_group_box();
                        break;
                    }
                    }
                    EndChild();
                    PopStyleVar(2);
                } ImGui::End();
                PopStyleVar();

                ImDrawList* fgDraw = ImGui::GetForegroundDrawList(); ImVec2 mp = io.MousePos;
                fgDraw->AddTriangleFilled(mp, ImVec2(mp.x + 12, mp.y + 12), ImVec2(mp.x, mp.y + 17), IM_COL32(255, 255, 255, 255));
                fgDraw->AddTriangle(mp, ImVec2(mp.x + 12, mp.y + 12), ImVec2(mp.x, mp.y + 17), IM_COL32(0, 0, 0, 255), 1.5f);
            }
        }

        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui::Render(); ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData()); g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) ResetDevice();
    }

    GameMemory::RestoreAllPatches();
    ChipSound::Shutdown();
    if (GameMemory::hProcess) CloseHandle(GameMemory::hProcess);
    ImGui_ImplDX9_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); ::DestroyWindow(hwnd); ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr) return false;
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE; g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    g_d3dpp.EnableAutoDepthStencil = TRUE; g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16; g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) return false;
    return true;
}

void CleanupDeviceD3D() {
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL) IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_ShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); g_ResizeHeight = (UINT)HIWORD(lParam); return 0;
    case WM_NCHITTEST: return g_ShowMenu ? HTCLIENT : HTTRANSPARENT;
    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY: ::PostQuitMessage(0); return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
