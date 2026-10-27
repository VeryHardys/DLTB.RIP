#pragma once
// ============================================================================
//  Fx — хитмаркеры, цифры урона и килл-эффекты (кровь, плоть, UV), которые живут В МИРЕ ИГРЫ.
//
//  Всё, что привязано к врагу, хранится в мировых координатах и каждый кадр
//  проецируется через камеру. Камера берётся из движка (матрица вид*проекция
//  CCamera, см. GameMemory::UpdateEngineCamera), а если движок недоступен —
//  из прежней эвристики (позиция игрока + направление взгляда + FOV).
//
//  Требует (объявлены в main.cpp до подключения): Vec3, MenuVars (hitCfg,
//  killCfg, sfxVolume, hitHeightOffset, hitDistanceScale), g_GameView, g_FontBold.
// ============================================================================
#include "imgui.h"
#include "imgui_internal.h"   // ImDrawListSharedData::TexUvWhitePixel для градиентных квадов
#include "chip_sound.hpp"
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cfloat>

namespace Fx {
    // ------------------------------------------------------------------ math
    static inline Vec3 V(float x, float y, float z) { Vec3 r; r.x = x; r.y = y; r.z = z; return r; }
    static inline Vec3 Add(const Vec3& a, const Vec3& b) { return V(a.x + b.x, a.y + b.y, a.z + b.z); }
    static inline Vec3 Sub(const Vec3& a, const Vec3& b) { return V(a.x - b.x, a.y - b.y, a.z - b.z); }
    static inline Vec3 Mul(const Vec3& a, float s) { return V(a.x * s, a.y * s, a.z * s); }
    static inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static inline float Len(const Vec3& a) { return std::sqrt(Dot(a, a)); }
    static inline Vec3 Norm(const Vec3& a) { float l = Len(a); return (l > 1e-6f) ? Mul(a, 1.0f / l) : V(0, 0, 0); }
    static inline Vec3 Cross(const Vec3& a, const Vec3& b) { return V(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
    static inline bool Finite(const Vec3& a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }

    static inline float Clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
    static inline float Smooth(float x) { x = Clamp01(x); return x * x * (3.0f - 2.0f * x); }
    static inline float EaseOutCubic(float x) { x = Clamp01(x); float u = 1.0f - x; return 1.0f - u * u * u; }
    static inline float EaseOutBack(float x) { x = Clamp01(x); const float c1 = 1.70158f, c3 = c1 + 1.0f; float u = x - 1.0f; return 1.0f + c3 * u * u * u + c1 * u * u; }
    static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    static inline float Rand01(uint32_t& s) { s = s * 1664525u + 1013904223u; return (float)(s >> 8) * (1.0f / 16777216.0f); }
    static inline float RandRange(uint32_t& s, float a, float b) { return a + (b - a) * Rand01(s); }
    static inline uint32_t Hash(uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }

    static inline ImU32 Col(const float c[4], float a) { return ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3] * Clamp01(a))); }
    static inline ImU32 ColMix(const float a[4], const float b[4], float t, float alpha) {
        float c[4] = { Lerp(a[0], b[0], t), Lerp(a[1], b[1], t), Lerp(a[2], b[2], t), Lerp(a[3], b[3], t) };
        return Col(c, alpha);
    }
    static inline ImU32 Shadow(float a) { return IM_COL32(0, 0, 0, (int)(Clamp01(a) * 210.0f)); }
    static inline ImVec2 Snap(ImVec2 p) { return ImVec2(std::floor(p.x) + 0.5f, std::floor(p.y) + 0.5f); }

    // Заливка выпуклого многоугольника с правильным обходом (нужен для AA-кромки ImGui)
    static void FillConvex(ImDrawList* dl, ImVec2* pts, int n, ImU32 col) {
        float area = 0.0f;
        for (int i = 0; i < n; ++i) { const ImVec2& a = pts[i]; const ImVec2& b = pts[(i + 1) % n]; area += a.x * b.y - b.x * a.y; }
        if (area < 0.0f) { for (int i = 0; i < n / 2; ++i) { ImVec2 t = pts[i]; pts[i] = pts[n - 1 - i]; pts[n - 1 - i] = t; } }
        dl->AddConvexPolyFilled(pts, n, col);
    }

    // ---------------------------------------------------------------- camera
    struct Camera {
        bool valid = false;
        bool matrixMode = false;   // true = проекция через матрицу движка
        bool flipY = false;
        int conv = 0;              // 0: clip = M * p (строки), 1: clip = p * M (столбцы)
        float m[16] = {};
        Vec3 pos, right, up, fwd;
        float focalX = 1.0f, focalY = 1.0f; // cot(fov/2) по осям
        float vx = 0.0f, vy = 0.0f, vw = 1920.0f, vh = 1080.0f;
    };

    Camera gameCam;
    int cameraSource = 0;          // 0 = нет, 1 = движок, 2 = эвристика (fallback)

    static bool ProjectRaw(const Camera& c, const Vec3& p, float& nx, float& ny, float& w) {
        if (c.matrixMode) {
            const float* m = c.m;
            float cx, cy, cw;
            if (c.conv == 0) {
                cx = m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3];
                cy = m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7];
                cw = m[12] * p.x + m[13] * p.y + m[14] * p.z + m[15];
            }
            else {
                cx = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
                cy = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
                cw = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
            }
            if (!(cw > 0.05f)) return false;
            nx = cx / cw; ny = cy / cw; w = cw;
            if (c.flipY) ny = -ny;
            return std::isfinite(nx) && std::isfinite(ny);
        }
        Vec3 d = Sub(p, c.pos);
        float z = Dot(d, c.fwd);
        if (z < 0.05f) return false;
        nx = Dot(d, c.right) * c.focalX / z;
        ny = Dot(d, c.up) * c.focalY / z;
        w = z;
        return true;
    }

    static bool Project(const Camera& c, const Vec3& p, ImVec2& out, float* depth = nullptr, float tol = 1.35f) {
        if (!c.valid) return false;
        float nx, ny, w;
        if (!ProjectRaw(c, p, nx, ny, w)) return false;
        if (std::fabs(nx) > tol || std::fabs(ny) > tol) return false;
        out = ImVec2(c.vx + (nx + 1.0f) * 0.5f * c.vw, c.vy + (1.0f - ny) * 0.5f * c.vh);
        if (depth) *depth = w;
        return true;
    }

    static float PixelsPerMeter(const Camera& c, float depth) {
        return c.focalY * c.vh * 0.5f / (depth > 0.1f ? depth : 0.1f);
    }

    static void SetViewport(Camera& c) { c.vx = g_GameView.x; c.vy = g_GameView.y; c.vw = g_GameView.w; c.vh = g_GameView.h; }

    // Простая "камера-обскура": позиция + направление + горизонтальный FOV.
    static void SetupPinhole(Camera& c, const Vec3& pos, const Vec3& fwdIn, float fovHdeg, bool invertPitch = false) {
        Vec3 f = Norm(fwdIn);
        if (invertPitch) f.y = -f.y;
        f = Norm(f);
        float rl = std::sqrt(f.x * f.x + f.z * f.z);
        c.valid = (rl > 1e-4f) && Finite(f);
        if (!c.valid) return;
        c.matrixMode = false;
        c.pos = pos;
        c.fwd = f;
        c.right = V(f.z / rl, 0.0f, -f.x / rl);
        c.up = Cross(f, c.right);
        if (fovHdeg < 30.0f) fovHdeg = 30.0f;
        if (fovHdeg > 150.0f) fovHdeg = 150.0f;
        float aspect = (c.vh > 1.0f) ? c.vw / c.vh : 16.0f / 9.0f;
        float tanH = std::tan(fovHdeg * 0.5f * 3.14159265f / 180.0f);
        c.focalX = 1.0f / tanH;
        c.focalY = aspect / tanH;
    }

    // Камера движка. Конвенцию матрицы (строки/столбцы), знак "вперёд" и
    // направление оси Y определяем автоматически: точка в 10 м перед камерой
    // должна попасть в центр экрана, точка выше — выше центра.
    static bool SetupEngineCamera(Camera& c, const float comb[16], const float proj[16], const Vec3& pos,
                                  const Vec3& bx, const Vec3& by, const Vec3& bz) {
        if (!Finite(pos) || !Finite(bx) || !Finite(by) || !Finite(bz)) return false;
        for (int i = 0; i < 16; ++i) if (!std::isfinite(comb[i])) return false;
        Vec3 zN = Norm(bz);
        if (Len(zN) < 0.5f) return false;

        static bool s_lastFlip = false;
        Camera t = c;
        t.valid = true;
        t.matrixMode = true;
        std::memcpy(t.m, comb, sizeof(t.m));
        t.pos = pos;

        for (int conv = 0; conv < 2; ++conv) {
            for (int sgn = 0; sgn < 2; ++sgn) {
                t.conv = conv; t.flipY = false;
                Vec3 f = Mul(zN, sgn == 0 ? 1.0f : -1.0f);
                Vec3 target = Add(pos, Mul(f, 10.0f));
                float nx, ny, w;
                if (!ProjectRaw(t, target, nx, ny, w)) continue;
                if (std::fabs(nx) > 0.06f || std::fabs(ny) > 0.06f || w < 1.0f || w > 100.0f) continue;

                // ось Y экрана
                if (std::fabs(f.y) < 0.92f) {
                    float ux, uy, uw;
                    if (ProjectRaw(t, Add(target, V(0.0f, 1.0f, 0.0f)), ux, uy, uw)) t.flipY = (uy < ny);
                    s_lastFlip = t.flipY;
                }
                else t.flipY = s_lastFlip;
                if (t.flipY) ny = -ny;

                // базис камеры (для билбордов и fallback-оценок)
                t.fwd = f;
                t.right = Norm(bx);
                t.up = Norm(by);
                float rx, ry, rw;
                if (ProjectRaw(t, Add(target, t.right), rx, ry, rw) && rx < nx) t.right = Mul(t.right, -1.0f);
                if (ProjectRaw(t, Add(target, t.up), rx, ry, rw) && ry < ny) t.up = Mul(t.up, -1.0f);

                float fx = std::fabs(proj[0]), fy = std::fabs(proj[5]);
                if (!(fy > 0.2f && fy < 8.0f)) {
                    // проекция не прочиталась — оценим фокус по самой матрице
                    float ax, ay, aw;
                    if (ProjectRaw(t, Add(target, t.up), ax, ay, aw)) fy = std::fabs(ay - ny) * 10.0f;
                    if (ProjectRaw(t, Add(target, t.right), ax, ay, aw)) fx = std::fabs(ax - nx) * 10.0f;
                }
                if (!(fy > 0.2f && fy < 8.0f)) continue;
                t.focalY = fy;
                t.focalX = (fx > 0.1f && fx < 8.0f) ? fx : fy * (t.vh / (t.vw > 1.0f ? t.vw : 1.0f));
                c = t;
                return true;
            }
        }
        return false;
    }

    // ------------------------------------------------------- tracked enemies
    struct TrackedAIState {
        int type = 1;               // 1 = зомби, 2 = человек (для будущих фильтров)
        Vec3 lastPos = { 0.0f, 0.0f, 0.0f };
        bool hasPos = false;
        bool initialized = false;
        bool dead = false;
        ULONGLONG lastSeenTick = 0;
    };
    std::map<uintptr_t, TrackedAIState> trackedEntities;
    ULONGLONG lastAttackInputTick = 0;

    // ----------------------------------------------------------------- types
    struct HitMarker {
        uintptr_t ai = 0, health = 0;
        bool anchored = false;
        Vec3 world;
        ImVec2 screen;
        float t = 0.0f, dur = 0.45f;
        float killT = -1.0f;
        float distScale = 1.0f;
        uint32_t seed = 0;
        HitmarkerConfig cfg;
    };

    struct DamageNumber {
        uintptr_t ai = 0, health = 0;
        bool anchored = false;
        Vec3 world;
        ImVec2 screen;
        float value = 0.0f;
        float t = 0.0f, dur = 0.95f;
        float bump = 0.0f;
        bool killed = false;
        float drift = 0.0f;
        int stack = 0;
        float col[4], killCol[4];
    };

    struct Drop { Vec3 p, v; float size; int shade; bool dead; };                    // капля крови
    struct Chunk { Vec3 p, v; float size, rot, spin; int verts; uint32_t shape; bool resting, splatted; }; // кусок плоти
    struct Splat { Vec3 c; float radius, born; uint32_t seed; bool pool; };          // пятно на земле
    struct Puff { Vec3 p, v; float size, born, life; };                              // взвесь / дым
    struct Ember { float angle, radius, speed, phase, size; bool ash; };             // угли и пепел (UV), радужные искры
    struct Confetti { Vec3 p, v; float size, rot, spin, flip, flipSpeed, hue, sway; bool landed; }; // бумажка конфетти
    struct Shard { Vec3 p, v; float size, rot, spin, hue; bool resting; };           // кристальный осколок
    struct Spike { float angle, dist, height, width, delay, tilt, hue; };            // кристалл, растущий из земли

    struct KillEffect {
        Vec3 feet;
        float t = 0.0f, dur = 2.6f;
        uint32_t seed = 1;
        int streak = 1;
        bool screenChecked = false;
        KillFxConfig cfg;
        std::vector<Drop> drops;
        std::vector<Chunk> chunks;
        std::vector<Splat> splats;
        std::vector<Puff> mist;
        std::vector<Puff> smoke;
        std::vector<Ember> embers;
        std::vector<Confetti> confetti;
        std::vector<Ember> sparkles;
        std::vector<Shard> shards;
        std::vector<Spike> spikes;
        int rockets = 0, vortexCount = 0, flakeCount = 0;   // эти эффекты считаются аналитически по seed
    };

    // Кровь на "объективе": координаты в долях вьюпорта, радиус в долях высоты
    struct ScreenSplat {
        float x = 0.5f, y = 0.5f, r = 0.05f;
        float t = 0.0f, dur = 2.8f;
        uint32_t seed = 1;
        float col[4];
    };

    struct Banner {
        bool active = false;
        std::string text;
        float t = 0.0f, dur = 1.9f;
        int level = 2;
        float primary[4], secondary[4];
    };

    struct World {
        std::vector<HitMarker> hits;
        std::vector<DamageNumber> numbers;
        std::vector<KillEffect> kills;
        std::vector<ScreenSplat> screen;
        Banner banner;
        int streak = 0;
        float sinceLastKill = 999.0f;
        bool useTracking = false;     // true для игры: маркеры следуют за живым врагом
        float screenSplatRange = 4.5f; // кровь на экран только с убийств ближе этого (м)
    };

    World game, preview;

    static const float kBone[4] = { 0.94f, 0.91f, 0.86f, 1.0f };
    static const size_t kMaxKills = 8;      // одновременно живущих килл-эффектов
    static const size_t kMaxSplats = 40;    // пятен на земле у одного эффекта

    // Оттенок от базового цвета: mul — яркость, add — подсветка (в основном в красный)
    static inline ImU32 Tint(const float c[4], float mul, float add, float a) {
        float t[4] = { Clamp01(c[0] * mul + add), Clamp01(c[1] * mul + add * 0.35f), Clamp01(c[2] * mul + add * 0.35f), c[3] };
        return Col(t, a);
    }
    static inline float HashF(uint32_t x) { return (float)(Hash(x) & 0xffff) / 65535.0f; }

    // Цвет радуги по оттенку (оттенок зациклен 0..1)
    static inline ImU32 Hue(float h, float s, float v, float a) {
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(h - std::floor(h), s, v, r, g, b);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, Clamp01(a)));
    }

    // Цвет кристалла: заданный или радужный; mul/add — затемнение/высветление грани
    static ImU32 CrystalCol(const KillFxConfig& cfg, float hue, float mul, float add, float a) {
        float c[4];
        if (cfg.rainbowCrystals) { ImGui::ColorConvertHSVtoRGB(hue - std::floor(hue), 0.55f, 1.0f, c[0], c[1], c[2]); c[3] = cfg.crystalColor[3]; }
        else std::memcpy(c, cfg.crystalColor, sizeof(c));
        float t[4] = { Clamp01(c[0] * mul + add), Clamp01(c[1] * mul + add), Clamp01(c[2] * mul + add), c[3] };
        return Col(t, a);
    }

    // Быстрые наборы эффектов для меню (0 = Custom — ничего не трогаем)
    static void ApplyKillPreset(KillFxConfig& c, int preset) {
        if (preset <= 0) return;
        // 1 Gore, 2 Rainbow Party, 3 Crystal, 4 Crystal + Rainbow, 5 Everything, 6 Magic
        const bool all = preset == 5;
        const bool gore = preset == 1 || all;
        const bool rainbow = preset == 2 || all;
        const bool crystal = preset == 3 || preset == 4 || all;
        const bool magic = preset == 6 || all;
        c.enabled = true;
        c.bloodBurst = c.goreChunks = c.bloodPool = c.bloodMist = gore;
        c.uvBurn = magic;
        c.confetti = rainbow || preset == 4;
        c.rainbowRing = c.rainbowRays = rainbow;
        c.rainbowSparkles = rainbow || preset == 4 || magic;
        c.fireworks = rainbow;
        c.crystalShatter = c.crystalSpikes = c.frostNova = crystal;
        c.rainbowCrystals = preset == 4 || all;
        c.vortex = magic;
        c.ghostRise = magic;
    }

    // ============================================================== spawning
    static ULONGLONG s_lastHitSound = 0;

    static void PlayHitSound(const HitmarkerConfig& cfg) {
        ULONGLONG now = GetTickCount64();
        if (now - s_lastHitSound < 45) return; // не "пулемётим" звуком при дробовике
        s_lastHitSound = now;
        ChipSound::PlayHit(cfg.sound, MenuVars::sfxVolume);
    }

    static void PlayKillSound(const KillFxConfig& cfg, int streak) {
        float pitch = cfg.pitchStreak ? (float)((streak - 1 > 7) ? 7 : (streak - 1)) * 1.5f : 0.0f;
        ChipSound::PlayKill(cfg.sound, MenuVars::sfxVolume, pitch);
    }

    // world == nullptr -> маркер в центре прицела
    static void SpawnHit(World& w, const HitmarkerConfig& cfg, uintptr_t ai, uintptr_t health, const Vec3* world, ImVec2 fallbackScreen, float damage) {
        // несколько попаданий по одной цели за 80 мс (дробь, стихийный урон) = один маркер
        bool merged = false;
        if (health != 0) for (auto& hm : w.hits) if (hm.health == health && hm.t < 0.08f) { merged = true; break; }
        if (cfg.enabled && !merged) {
            HitMarker h;
            h.ai = ai; h.health = health;
            h.cfg = cfg;
            h.dur = (cfg.duration < 0.15f) ? 0.15f : cfg.duration;
            h.screen = fallbackScreen;
            h.anchored = (world != nullptr) && cfg.positionMode == 1;
            if (h.anchored) h.world = *world;
            h.seed = Hash((uint32_t)(GetTickCount64() & 0xffffffff) ^ (uint32_t)w.hits.size() * 7919u);
            w.hits.push_back(h);
            if (w.hits.size() > 32) w.hits.erase(w.hits.begin());
        }

        if (cfg.enabled && cfg.damageNumbers && damage > 0.0f) {
            // склеиваем быстрые попадания по одной цели в одну "тикающую" цифру
            for (auto& n : w.numbers) {
                if (n.health == health && health != 0 && n.t < 0.45f && !n.killed) {
                    n.value += damage; n.bump = 1.0f; n.t = (n.t > 0.08f) ? 0.08f : n.t;
                    if (world) { n.world = *world; n.anchored = true; }
                    return;
                }
            }
            DamageNumber n;
            n.ai = ai; n.health = health;
            n.anchored = (world != nullptr);
            if (world) n.world = *world;
            n.screen = fallbackScreen;
            n.value = damage;
            n.bump = 1.0f;
            uint32_t s = Hash((uint32_t)w.numbers.size() * 2654435761u ^ (uint32_t)GetTickCount64());
            n.drift = RandRange(s, 10.0f, 26.0f); // цифры уходят вверх-вправо и не закрывают цель
            int stack = 0;
            for (auto& o : w.numbers) if (o.health == health && o.t < 0.6f) ++stack;
            n.stack = stack;
            std::memcpy(n.col, cfg.damageColor, sizeof(n.col));
            std::memcpy(n.killCol, cfg.killColor, sizeof(n.killCol));
            w.numbers.push_back(n);
            if (w.numbers.size() > 24) w.numbers.erase(w.numbers.begin());
        }
    }

    static void MarkKilled(World& w, uintptr_t health) {
        if (health == 0) return;
        for (auto& h : w.hits) if (h.health == health && h.t < 0.35f && h.killT < 0.0f) h.killT = h.t;
        for (auto& n : w.numbers) if (n.health == health && n.t < 0.6f) { n.killed = true; n.bump = 1.0f; }
    }

    static const char* StreakName(int s) {
        switch (s) {
        case 2: return "DOUBLE KILL";
        case 3: return "TRIPLE KILL";
        case 4: return "MASSACRE";
        case 5: return "CARNAGE";
        default: return (s >= 6) ? "BLOODBATH" : "";
        }
    }

    // Возвращает номер убийства в серии (для звука)
    static int SpawnKill(World& w, const KillFxConfig& cfg, const Vec3* feet) {
        w.streak = (w.sinceLastKill < 4.0f) ? w.streak + 1 : 1;
        w.sinceLastKill = 0.0f;
        if (!cfg.enabled) return w.streak;

        if (cfg.streakAnnouncer && w.streak >= 2) {
            w.banner.active = true;
            w.banner.text = StreakName(w.streak);
            w.banner.t = 0.0f;
            w.banner.level = w.streak;
            std::memcpy(w.banner.primary, cfg.primary, sizeof(w.banner.primary));
            std::memcpy(w.banner.secondary, cfg.secondary, sizeof(w.banner.secondary));
        }
        if (!feet) return w.streak;

        KillEffect k;
        k.feet = *feet;
        k.cfg = cfg;
        k.dur = (cfg.duration < 0.6f) ? 0.6f : cfg.duration;
        k.streak = w.streak;
        k.seed = Hash((uint32_t)GetTickCount64() ^ (uint32_t)(feet->x * 131.0f) ^ (uint32_t)(feet->z * 977.0f)) | 1u;
        uint32_t s = k.seed;
        const float S = (cfg.scale < 0.3f) ? 0.3f : cfg.scale;
        const Vec3 chest = V(feet->x, feet->y + 1.25f * S, feet->z);
        // бюджет: чем больше эффектов уже на экране, тем меньше частиц у нового — орда не просаживает FPS
        const float budget = w.kills.size() >= 5 ? 0.35f : (w.kills.size() >= 2 ? 0.6f : 1.0f);
        auto Budget = [&](int n, int minimum) { int r = (int)(n * budget + 0.5f); return r < minimum ? minimum : r; };
        // кровь летит преимущественно в одну сторону — как от удара, а не "салютом"
        float biasA = RandRange(s, 0.0f, 6.2831853f);
        Vec3 bias = V(std::cos(biasA), 0.0f, std::sin(biasA));

        if (cfg.bloodBurst) {
            int count = Budget(cfg.particleCount < 10 ? 10 : (cfg.particleCount > 160 ? 160 : cfg.particleCount), 8);
            k.drops.reserve(count);
            for (int i = 0; i < count; ++i) {
                Drop d;
                float a = RandRange(s, 0.0f, 6.2831853f);
                Vec3 dir = Norm(Add(V(std::cos(a), 0.0f, std::sin(a)), Mul(bias, 0.65f)));
                float out = RandRange(s, 0.8f, 4.2f) * S;
                float off = RandRange(s, 0.0f, 0.15f) * S;
                d.p = V(chest.x + dir.x * off, chest.y + RandRange(s, -0.35f, 0.35f) * S, chest.z + dir.z * off);
                d.v = V(dir.x * out, RandRange(s, 0.5f, 4.5f) * S, dir.z * out);
                d.size = RandRange(s, 0.018f, 0.05f) * S;
                d.shade = (int)(Rand01(s) * 3.0f);
                d.dead = false;
                k.drops.push_back(d);
            }
        }
        if (cfg.goreChunks) {
            int count = Budget(7 + (int)(Rand01(s) * 5.0f), 3);
            for (int i = 0; i < count; ++i) {
                Chunk c;
                float a = RandRange(s, 0.0f, 6.2831853f);
                Vec3 dir = Norm(Add(V(std::cos(a), 0.0f, std::sin(a)), Mul(bias, 0.4f)));
                float out = RandRange(s, 1.0f, 3.2f) * S;
                c.p = V(chest.x, chest.y + RandRange(s, -0.25f, 0.3f) * S, chest.z);
                c.v = V(dir.x * out, RandRange(s, 2.0f, 5.0f) * S, dir.z * out);
                c.size = RandRange(s, 0.05f, 0.12f) * S;
                c.rot = RandRange(s, 0.0f, 6.2831853f);
                c.spin = RandRange(s, -14.0f, 14.0f);
                c.verts = 5 + (int)(Rand01(s) * 3.0f);
                c.shape = Hash(s);
                c.resting = false; c.splatted = false;
                k.chunks.push_back(c);
            }
        }
        if (cfg.bloodPool) {
            // основная лужа + несколько "языков" вокруг; растут с задержкой, как будто натекает
            Splat m;
            m.c = *feet; m.radius = RandRange(s, 0.55f, 0.8f) * S; m.born = 0.15f; m.seed = Hash(s); m.pool = true;
            k.splats.push_back(m);
            for (int i = 0, n = Budget(5, 2); i < n; ++i) {
                Splat l;
                float a = RandRange(s, 0.0f, 6.2831853f), d = RandRange(s, 0.3f, 0.55f) * S;
                l.c = V(feet->x + std::cos(a) * d, feet->y, feet->z + std::sin(a) * d);
                l.radius = RandRange(s, 0.18f, 0.35f) * S;
                l.born = 0.25f + i * 0.08f;
                l.seed = Hash(s);
                l.pool = true;
                k.splats.push_back(l);
            }
        }
        if (cfg.bloodMist) {
            for (int i = 0, n = Budget(10, 4); i < n; ++i) {
                Puff pf;
                float a = RandRange(s, 0.0f, 6.2831853f), sp = RandRange(s, 0.4f, 1.6f);
                pf.p = chest;
                pf.v = Mul(Add(V(std::cos(a) * sp, RandRange(s, -0.2f, 0.9f), std::sin(a) * sp), Mul(bias, 0.5f)), S);
                pf.size = RandRange(s, 0.16f, 0.32f) * S;
                pf.born = RandRange(s, 0.0f, 0.06f);
                pf.life = RandRange(s, 0.6f, 1.1f);
                k.mist.push_back(pf);
            }
        }
        if (cfg.uvBurn) {
            for (int i = 0, n = Budget(24, 8); i < n; ++i) {
                Ember e;
                e.angle = RandRange(s, 0.0f, 6.2831853f);
                e.radius = RandRange(s, 0.05f, 0.35f) * S;
                e.speed = RandRange(s, 0.8f, 1.8f);
                e.phase = RandRange(s, 0.0f, 1.0f);
                e.size = RandRange(s, 0.015f, 0.035f) * S;
                e.ash = (i % 3) == 0;
                k.embers.push_back(e);
            }
            for (int i = 0, n = Budget(9, 3); i < n; ++i) {
                Puff pf;
                float a = RandRange(s, 0.0f, 6.2831853f), r = RandRange(s, 0.0f, 0.25f) * S;
                pf.p = V(feet->x + std::cos(a) * r, feet->y + RandRange(s, 0.3f, 1.5f) * S, feet->z + std::sin(a) * r);
                pf.v = Mul(V(RandRange(s, -0.15f, 0.15f), RandRange(s, 0.5f, 1.1f), RandRange(s, -0.15f, 0.15f)), S);
                pf.size = RandRange(s, 0.2f, 0.35f) * S;
                pf.born = RandRange(s, 0.05f, 0.5f);
                pf.life = RandRange(s, 1.2f, 1.8f);
                k.smoke.push_back(pf);
            }
        }
        if (cfg.confetti) {
            int n = Budget(cfg.confettiCount < 10 ? 10 : (cfg.confettiCount > 200 ? 200 : cfg.confettiCount), 12);
            k.confetti.reserve(n);
            for (int i = 0; i < n; ++i) {
                Confetti c;
                float a = RandRange(s, 0.0f, 6.2831853f), out = RandRange(s, 0.6f, 3.0f) * S;
                c.p = V(chest.x + std::cos(a) * 0.1f * S, chest.y + RandRange(s, -0.2f, 0.3f) * S, chest.z + std::sin(a) * 0.1f * S);
                c.v = V(std::cos(a) * out, RandRange(s, 3.0f, 6.5f) * S, std::sin(a) * out);
                c.size = RandRange(s, 0.035f, 0.07f) * S;
                c.rot = RandRange(s, 0.0f, 6.2831853f);
                c.spin = RandRange(s, -9.0f, 9.0f);
                c.flip = RandRange(s, 0.0f, 6.2831853f);
                c.flipSpeed = RandRange(s, 6.0f, 14.0f);
                c.hue = Rand01(s);
                c.sway = RandRange(s, 0.6f, 1.4f);
                c.landed = false;
                k.confetti.push_back(c);
            }
        }
        if (cfg.rainbowSparkles) {
            for (int i = 0, n = Budget(28, 10); i < n; ++i) {
                Ember e;
                e.angle = RandRange(s, 0.0f, 6.2831853f);
                e.radius = RandRange(s, 0.15f, 0.6f) * S;
                e.speed = RandRange(s, 0.7f, 1.5f);
                e.phase = RandRange(s, 0.0f, 1.0f);
                e.size = RandRange(s, 0.025f, 0.05f) * S;
                e.ash = false;
                k.sparkles.push_back(e);
            }
        }
        if (cfg.crystalShatter) {
            for (int i = 0, n = Budget(26, 10); i < n; ++i) {
                Shard sh;
                float a = RandRange(s, 0.0f, 6.2831853f), r = RandRange(s, 0.0f, 0.25f) * S;
                Vec3 dir = Norm(Add(V(std::cos(a), 0.0f, std::sin(a)), Mul(bias, 0.3f)));
                float out = RandRange(s, 1.0f, 3.6f) * S;
                sh.p = V(chest.x + dir.x * r, chest.y + RandRange(s, -0.9f, 0.6f) * S, chest.z + dir.z * r);
                sh.v = V(dir.x * out, RandRange(s, 1.5f, 4.5f) * S, dir.z * out);
                sh.size = RandRange(s, 0.05f, 0.13f) * S;
                sh.rot = RandRange(s, 0.0f, 6.2831853f);
                sh.spin = RandRange(s, -10.0f, 10.0f);
                sh.hue = Rand01(s);
                sh.resting = false;
                k.shards.push_back(sh);
            }
        }
        if (cfg.crystalSpikes) {
            int n = Budget(9, 5);
            for (int i = 0; i < n; ++i) {
                Spike sp;
                sp.angle = i * 6.2831853f / n + RandRange(s, -0.3f, 0.3f);
                sp.dist = RandRange(s, 0.15f, 0.7f) * S;
                sp.height = RandRange(s, 0.5f, 1.3f) * S;
                sp.width = RandRange(s, 0.08f, 0.16f) * S;
                sp.delay = i * 0.03f + RandRange(s, 0.0f, 0.08f);
                sp.tilt = RandRange(s, 0.1f, 0.45f);
                sp.hue = Rand01(s);
                k.spikes.push_back(sp);
            }
        }
        k.rockets = cfg.fireworks ? Budget(3, 1) : 0;
        k.vortexCount = cfg.vortex ? Budget(36, 12) : 0;
        k.flakeCount = cfg.frostNova ? Budget(40, 12) : 0;
        w.kills.push_back(std::move(k));
        if (w.kills.size() > kMaxKills) w.kills.erase(w.kills.begin());
        return w.streak;
    }

    // ============================================================ hit styles
    // Лезвие: четырёхугольник от r0 до r1 вдоль dir, ширина w0 у основания и w1 у кончика.
    static void Blade(ImDrawList* dl, ImVec2 c, float ang, float r0, float r1, float w0, float w1, ImU32 col) {
        float dx = std::cos(ang), dy = std::sin(ang), px = -dy, py = dx;
        ImVec2 q[4] = {
            ImVec2(c.x + dx * r0 + px * w0 * 0.5f, c.y + dy * r0 + py * w0 * 0.5f),
            ImVec2(c.x + dx * r1 + px * w1 * 0.5f, c.y + dy * r1 + py * w1 * 0.5f),
            ImVec2(c.x + dx * r1 - px * w1 * 0.5f, c.y + dy * r1 - py * w1 * 0.5f),
            ImVec2(c.x + dx * r0 - px * w0 * 0.5f, c.y + dy * r0 - py * w0 * 0.5f),
        };
        FillConvex(dl, q, 4, col);
    }

    static void Diamond(ImDrawList* dl, ImVec2 c, float d, ImU32 col, ImU32 shadow) {
        ImVec2 dmo[4] = { ImVec2(c.x, c.y - d - 1.5f), ImVec2(c.x + d + 1.5f, c.y), ImVec2(c.x, c.y + d + 1.5f), ImVec2(c.x - d - 1.5f, c.y) };
        ImVec2 dm[4] = { ImVec2(c.x, c.y - d), ImVec2(c.x + d, c.y), ImVec2(c.x, c.y + d), ImVec2(c.x - d, c.y) };
        FillConvex(dl, dmo, 4, shadow);
        FillConvex(dl, dm, 4, col);
    }

    // age — секунды с момента удара, p — прогресс 0..1, s — общий масштаб
    static void DrawHitStyle(ImDrawList* dl, ImVec2 c, const HitmarkerConfig& cfg, float age, float p, float s,
                             ImU32 col, ImU32 glowCol, ImU32 shadow, uint32_t seed, float alpha) {
        const float size = cfg.size * s;
        const float th = (cfg.thickness < 1.0f ? 1.0f : cfg.thickness) * (0.75f + 0.25f * s);
        const float snap = Smooth(age / 0.10f);              // 0 -> 1 за 100 мс ("защёлкивание")
        const float expand = EaseOutCubic(p);

        switch (cfg.style) {
        default:
        case HIT_CLAW: {
            // три царапины наискосок (сверху-справа вниз-влево), прорезаются по очереди
            const float dx = -0.58f, dy = 0.81f;    // направление взмаха
            const float px = dy, py = -dx;          // поперёк
            for (int i = 0; i < 3; ++i) {
                float reveal = Smooth((age - i * 0.022f) / 0.07f);
                if (reveal <= 0.01f) continue;
                float off = (i - 1) * size * (0.62f + 0.18f * expand);
                float L = size * ((i == 1) ? 2.5f : 2.0f);
                float shift = (i - 1) * size * 0.18f;   // крайние смещены, как следы пальцев
                ImVec2 st(c.x + px * off - dx * (L * 0.5f - shift), c.y + py * off - dy * (L * 0.5f - shift));
                float cur = L * reveal;
                ImVec2 en(st.x + dx * cur, st.y + dy * cur);
                ImVec2 md(st.x + dx * cur * 0.42f, st.y + dy * cur * 0.42f);
                float w = th * 1.7f * (0.6f + 0.4f * reveal);
                auto slash = [&](float grow, float halfW, ImU32 cc) {
                    ImVec2 q[4] = {
                        ImVec2(st.x - dx * grow, st.y - dy * grow),
                        ImVec2(md.x + px * halfW, md.y + py * halfW),
                        ImVec2(en.x + dx * grow, en.y + dy * grow),
                        ImVec2(md.x - px * halfW, md.y - py * halfW) };
                    FillConvex(dl, q, 4, cc);
                };
                if (cfg.glow) slash(4.0f, w * 2.4f, glowCol);
                slash(1.5f, w + 1.5f, shadow);
                slash(0.0f, w, col);
            }
            break;
        }
        case HIT_BLOOD_X: {
            float r0 = size * (0.40f + 0.25f * expand);
            float r1 = r0 + size * (0.95f + 0.45f * (1.0f - snap));
            ImU32 blood = Col(cfg.killColor, alpha);
            for (int i = 0; i < 4; ++i) {
                float a = 0.7853982f + i * 1.5707963f;
                if (cfg.glow) Blade(dl, c, a, r0 - 2.0f, r1 + 3.0f, th * 4.2f, th * 1.8f, glowCol);
                Blade(dl, c, a, r0 - 1.0f, r1 + 1.0f, th * 2.0f + 2.0f, 2.0f, shadow);
                Blade(dl, c, a, r0, r1, th * 2.0f, 0.6f, col);
                // с кончика срываются капли и падают вниз
                if (age < 0.02f) continue;
                for (int k = 0; k < 2; ++k) {
                    uint32_t h = Hash(seed + i * 31u + k * 7u);
                    float sp = 0.6f + (float)(h & 255) / 255.0f;
                    float da = a + ((float)((h >> 8) & 255) / 255.0f - 0.5f) * 0.6f;
                    float dist = r1 + size * 1.6f * sp * EaseOutCubic(age / 0.35f);
                    float fall = 140.0f * s * age * age;
                    ImVec2 dp(c.x + std::cos(da) * dist, c.y + std::sin(da) * dist + fall);
                    float rad = th * (0.7f + 0.5f * sp) * (1.0f - 0.6f * p);
                    dl->AddCircleFilled(dp, rad + 1.0f, shadow, 8);
                    dl->AddCircleFilled(dp, rad, blood, 8);
                }
            }
            break;
        }
        case HIT_CLASSIC: {
            float gap = size * (0.45f + 0.25f * expand);
            float len = size * (0.85f + 0.3f * (1.0f - snap));
            for (int pass = 0; pass < 3; ++pass) {
                if (pass == 0 && !cfg.glow) continue;
                ImU32 pc = pass == 0 ? glowCol : (pass == 1 ? shadow : col);
                float pt = pass == 0 ? th * 3.0f : (pass == 1 ? th + 2.0f : th);
                float e = (pass == 1) ? 1.0f : 0.0f;
                for (int i = 0; i < 4; ++i) {
                    float a = 0.7853982f + i * 1.5707963f, dx = std::cos(a), dy = std::sin(a);
                    dl->AddLine(ImVec2(c.x + dx * (gap - e), c.y + dy * (gap - e)), ImVec2(c.x + dx * (gap + len + e), c.y + dy * (gap + len + e)), pc, pt);
                }
            }
            break;
        }
        case HIT_SPLATTER: {
            // клякса всегда цвета крови (Kill Confirm Color), центр — цвет хита
            ImU32 blood = Col(cfg.killColor, alpha);
            float dk[4] = { cfg.killColor[0] * 0.55f, cfg.killColor[1] * 0.55f, cfg.killColor[2] * 0.55f, cfg.killColor[3] };
            ImU32 bloodDark = Col(dk, alpha);
            float open = EaseOutBack(snap);
            for (int i = 0; i < 6; ++i) {
                uint32_t h = Hash(seed + i * 53u);
                float a = i * 1.0471976f + ((float)(h & 255) / 255.0f - 0.5f) * 0.8f;
                float L = size * (0.7f + 0.8f * (float)((h >> 8) & 255) / 255.0f) * open * (1.0f + 0.25f * expand);
                Blade(dl, c, a, 0.0f, L + 1.5f, th * 2.6f + 2.0f, 1.5f, shadow);
                Blade(dl, c, a, 0.0f, L, th * 2.6f, 0.8f, bloodDark);
                float dr = th * (0.8f + 0.6f * (float)((h >> 16) & 255) / 255.0f) * open;
                float td = L + dr * 2.2f + size * 0.5f * expand;
                ImVec2 tip(c.x + std::cos(a) * td, c.y + std::sin(a) * td);
                dl->AddCircleFilled(tip, dr + 1.0f, shadow, 10);
                dl->AddCircleFilled(tip, dr, blood, 10);
            }
            float br = size * 0.42f * open;
            dl->AddCircleFilled(c, br + 1.5f, shadow, 18);
            dl->AddCircleFilled(c, br, blood, 18);
            dl->AddCircleFilled(ImVec2(c.x - br * 0.3f, c.y - br * 0.3f), br * 0.28f, IM_COL32(255, 255, 255, (int)(alpha * 55)), 10); // мокрый блик
            dl->AddCircleFilled(c, th * 0.9f, col, 10);
            break;
        }
        case HIT_BONE_CRACK: {
            const int n = 5;
            float grow = EaseOutCubic(age / 0.09f);
            float gap = size * (0.25f + 0.2f * expand);
            float segL = size * 0.36f;
            for (int i = 0; i < n; ++i) {
                uint32_t h = Hash(seed + i * 97u);
                float a = i * 6.2831853f / n + ((float)(h & 255) / 255.0f - 0.5f) * 0.7f;
                ImVec2 pts[5]; int cnt = 0;
                ImVec2 cur(c.x + std::cos(a) * gap, c.y + std::sin(a) * gap);
                pts[cnt++] = cur;
                float total = 4.0f * grow;
                for (int k = 0; k < 4 && (float)k < total; ++k) {
                    uint32_t hk = Hash(h + k * 13u);
                    float da = a + ((float)(hk & 255) / 255.0f - 0.5f) * 1.0f;
                    float L = segL * (0.7f + 0.6f * (float)((hk >> 8) & 255) / 255.0f) * Clamp01(total - k);
                    cur = ImVec2(cur.x + std::cos(da) * L, cur.y + std::sin(da) * L);
                    pts[cnt++] = cur;
                }
                if (cnt < 2) continue;
                if (cfg.glow) dl->AddPolyline(pts, cnt, glowCol, 0, th * 3.0f);
                dl->AddPolyline(pts, cnt, shadow, 0, th + 2.0f);
                // трещина сужается к концу
                for (int k = 0; k + 1 < cnt; ++k) dl->AddLine(pts[k], pts[k + 1], col, th * (1.0f - 0.18f * k));
                if (((h >> 20) & 1) && cnt >= 3) {   // короткая ветка от второго излома
                    float ba = a + (((h >> 21) & 1) ? 0.9f : -0.9f);
                    ImVec2 be(pts[2].x + std::cos(ba) * segL * 0.6f * grow, pts[2].y + std::sin(ba) * segL * 0.6f * grow);
                    dl->AddLine(pts[2], be, shadow, th * 0.7f + 2.0f);
                    dl->AddLine(pts[2], be, col, th * 0.7f);
                }
            }
            Diamond(dl, c, th * 1.1f, col, shadow);
            break;
        }
        case HIT_UV_PULSE: {
            float r = size * (2.3f - 1.25f * EaseOutCubic(age / 0.14f)) + expand * size * 0.3f;
            ImU32 uvGlow = IM_COL32(150, 95, 255, (int)(alpha * 70));
            ImU32 uvCore = IM_COL32(165, 115, 255, (int)(alpha * 220));
            if (cfg.glow) dl->AddCircle(c, r, uvGlow, 40, th * 4.0f);
            dl->AddCircle(c, r, shadow, 40, th + 2.0f);
            dl->AddCircle(c, r, uvCore, 40, th);
            dl->AddCircle(c, r, col, 40, th * 0.45f);   // раскалённая сердцевина кольца
            for (int i = 0; i < 4; ++i) {
                float a = i * 1.5707963f, dx = std::cos(a), dy = std::sin(a);
                ImVec2 o(c.x + dx * (r + th), c.y + dy * (r + th)), in(c.x + dx * (r - size * 0.45f), c.y + dy * (r - size * 0.45f));
                dl->AddLine(o, in, shadow, th + 2.0f);
                dl->AddLine(o, in, col, th);
            }
            float d = size * 0.28f;
            dl->AddLine(ImVec2(c.x - d - 1, c.y), ImVec2(c.x + d + 1, c.y), shadow, th + 2.0f);
            dl->AddLine(ImVec2(c.x, c.y - d - 1), ImVec2(c.x, c.y + d + 1), shadow, th + 2.0f);
            dl->AddLine(ImVec2(c.x - d, c.y), ImVec2(c.x + d, c.y), uvCore, th);
            dl->AddLine(ImVec2(c.x, c.y - d), ImVec2(c.x, c.y + d), uvCore, th);
            break;
        }
        }
    }

    // ============================================================= rendering
    static bool ResolveScreen(const Camera& cam, bool anchored, const Vec3& world, ImVec2& out, float& depth) {
        if (!anchored) return true;
        return Project(cam, world, out, &depth, 1.6f);
    }

    static void UpdateAnchor(World& w, uintptr_t ai, bool& anchored, Vec3& world, float height) {
        if (!w.useTracking || !anchored || ai == 0) return;
        auto it = trackedEntities.find(ai);
        if (it == trackedEntities.end() || !it->second.hasPos) return;
        if (GetTickCount64() - it->second.lastSeenTick > 600) return;
        Vec3 target = V(it->second.lastPos.x, it->second.lastPos.y + height, it->second.lastPos.z);
        // мягко следуем за врагом (позиция из памяти обновляется не каждый кадр)
        world.x += (target.x - world.x) * 0.5f;
        world.y += (target.y - world.y) * 0.5f;
        world.z += (target.z - world.z) * 0.5f;
    }

    static void RenderHits(World& w, const Camera& cam, ImDrawList* dl, float dt) {
        for (size_t i = 0; i < w.hits.size(); ) {
            HitMarker& h = w.hits[i];
            h.t += dt;
            float total = h.dur + ((h.killT >= 0.0f) ? 0.18f : 0.0f);
            if (h.t >= total) { w.hits.erase(w.hits.begin() + i); continue; }
            ++i;

            UpdateAnchor(w, h.ai, h.anchored, h.world, MenuVars::hitHeightOffset);
            ImVec2 pos = h.screen; float depth = 6.0f;
            if (!ResolveScreen(cam, h.anchored, h.world, pos, depth)) continue;
            if (h.anchored && MenuVars::hitDistanceScale) {
                float target = 7.0f / (depth > 0.5f ? depth : 0.5f);
                target = target < 0.6f ? 0.6f : (target > 1.2f ? 1.2f : target);
                h.distScale += (target - h.distScale) * Clamp01(dt * 12.0f);
            }
            pos = Snap(pos);

            bool killed = h.killT >= 0.0f;
            float p = Clamp01(h.t / total);
            float alpha = Smooth(h.t / 0.04f) * (1.0f - Smooth((p - 0.55f) / 0.45f));
            float pop = 1.0f + 0.35f * (1.0f - Smooth(h.t / 0.09f));
            if (killed) pop += 0.30f * (1.0f - Smooth((h.t - h.killT) / 0.16f)) + 0.15f;
            float s = h.distScale * pop;

            const float* base = killed ? h.cfg.killColor : h.cfg.color;
            ImU32 col = Col(base, alpha);
            ImU32 glowCol = Col(base, alpha * 0.20f);
            ImU32 shadow = Shadow(alpha * 0.85f);
            DrawHitStyle(dl, pos, h.cfg, h.t, p, s, col, glowCol, shadow, h.seed, alpha);

            if (killed) {
                // подтверждение убийства: кровавые брызги разлетаются от маркера
                float kt = Clamp01((h.t - h.killT) / 0.35f);
                float ka = (1.0f - kt) * alpha;
                for (int r = 0; r < 8; ++r) {
                    uint32_t hh = Hash(h.seed + r * 211u);
                    float a = r * 0.7853982f + ((float)(hh & 255) / 255.0f - 0.5f) * 0.5f;
                    float r0 = h.cfg.size * s * (1.2f + 1.6f * EaseOutCubic(kt));
                    float L = h.cfg.size * s * (0.5f + 0.6f * (float)((hh >> 8) & 255) / 255.0f) * (1.0f - kt);
                    Blade(dl, pos, a, r0 - 1.0f, r0 + L + 1.0f, 3.5f * (1.0f - kt) + 3.0f, 1.5f, Shadow(ka * 0.7f));
                    Blade(dl, pos, a, r0, r0 + L, 3.5f * (1.0f - kt) + 1.0f, 0.5f, Col(h.cfg.killColor, ka));
                }
            }
        }
    }

    static void RenderNumbers(World& w, const Camera& cam, ImDrawList* dl, float dt) {
        ImFont* font = g_FontBold ? g_FontBold : ImGui::GetFont();
        float baseSize = font->FontSize * ImGui::GetIO().FontGlobalScale;
        for (size_t i = 0; i < w.numbers.size(); ) {
            DamageNumber& n = w.numbers[i];
            n.t += dt;
            n.bump = (n.bump > 0.0f) ? (n.bump - dt * 6.0f) : 0.0f;
            if (n.t >= n.dur) { w.numbers.erase(w.numbers.begin() + i); continue; }
            ++i;

            UpdateAnchor(w, n.ai, n.anchored, n.world, MenuVars::hitHeightOffset + 0.25f);
            ImVec2 pos = n.screen; float depth = 6.0f;
            if (!ResolveScreen(cam, n.anchored, n.world, pos, depth)) continue;

            float p = n.t / n.dur;
            float alpha = Smooth(n.t / 0.05f) * (1.0f - Smooth((p - 0.6f) / 0.4f));
            float rise = 26.0f * EaseOutCubic(p) + 16.0f + n.stack * 14.0f;
            float sz = baseSize * (1.05f + 0.45f * Clamp01(n.bump)) * (n.killed ? 1.25f : 1.0f);
            if (n.anchored) {
                float ds = 6.0f / (depth > 0.5f ? depth : 0.5f);
                sz *= (ds < 0.75f ? 0.75f : (ds > 1.15f ? 1.15f : ds));
            }

            char buf[32];
            if (n.value < 9.95f) snprintf(buf, sizeof(buf), "%.1f", n.value);
            else snprintf(buf, sizeof(buf), "%d", (int)(n.value + 0.5f));
            ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, buf);
            ImVec2 tp(std::floor(pos.x + 14.0f + n.drift * EaseOutCubic(p)), std::floor(pos.y - rise - ts.y * 0.5f));

            ImU32 c = Col(n.killed ? n.killCol : n.col, alpha);
            ImU32 o = Shadow(alpha);
            dl->AddText(font, sz, ImVec2(tp.x + 2, tp.y + 2), o, buf);
            dl->AddText(font, sz, ImVec2(tp.x - 1, tp.y), o, buf);
            dl->AddText(font, sz, ImVec2(tp.x + 1, tp.y), o, buf);
            dl->AddText(font, sz, ImVec2(tp.x, tp.y - 1), o, buf);
            dl->AddText(font, sz, ImVec2(tp.x, tp.y + 1), o, buf);
            dl->AddText(font, sz, tp, c, buf);
        }
    }

    // Вертикальный квад с градиентом (для UV-свечения тела)
    static void GradientQuad(ImDrawList* dl, ImVec2 b0, ImVec2 b1, ImVec2 t1, ImVec2 t0, ImU32 cb, ImU32 ct) {
        dl->PrimReserve(6, 4);
        ImVec2 uv = dl->_Data->TexUvWhitePixel;
        ImDrawIdx idx = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteIdx(idx); dl->PrimWriteIdx((ImDrawIdx)(idx + 1)); dl->PrimWriteIdx((ImDrawIdx)(idx + 2));
        dl->PrimWriteIdx(idx); dl->PrimWriteIdx((ImDrawIdx)(idx + 2)); dl->PrimWriteIdx((ImDrawIdx)(idx + 3));
        dl->PrimWriteVtx(b0, uv, cb); dl->PrimWriteVtx(b1, uv, cb);
        dl->PrimWriteVtx(t1, uv, ct); dl->PrimWriteVtx(t0, uv, ct);
    }

    // Плоское пятно на земле: проецируем окружность с "рваным" краем и заливаем.
    static bool GroundBlob(ImDrawList* dl, const Camera& cam, const Vec3& c, float r, uint32_t seed, float jitter, ImU32 col, int N = 16) {
        ImVec2 pts[16];
        N = N > 16 ? 16 : (N < 6 ? 6 : N);
        for (int i = 0; i < N; ++i) {
            float ang = i * 6.2831853f / N;
            float rr = r * (1.0f - jitter * HashF(seed + i * 37u));
            if (!Project(cam, V(c.x + std::cos(ang) * rr, c.y + 0.02f, c.z + std::sin(ang) * rr), pts[i], nullptr, 3.0f)) return false;
        }
        FillConvex(dl, pts, N, col);
        return true;
    }

    // LOD по размеру на экране: далёкие/мелкие пятна — точка или грубый полигон без AA-кромки
    // (AA удваивает вершины), детали (брызги вокруг, блик) — только у крупных.
    static void DrawSplat(ImDrawList* dl, const Camera& cam, const Splat& sp, float r, const float base[4], float a) {
        ImVec2 cs; float d;
        if (!Project(cam, sp.c, cs, &d, 2.0f)) return;
        float rpx = r * PixelsPerMeter(cam, d);
        ImU32 body = Tint(base, sp.pool ? 0.62f : 0.85f, 0.0f, a * 0.92f);
        if (rpx < 2.5f) { dl->AddRectFilled(ImVec2(cs.x - 1.0f, cs.y - 0.7f), ImVec2(cs.x + 1.0f, cs.y + 0.7f), body); return; }
        ImU32 rim = Tint(base, sp.pool ? 0.38f : 0.5f, 0.0f, a * 0.95f);
        const int N = rpx < 8.0f ? 7 : (rpx < 22.0f ? 11 : 16);
        const ImDrawListFlags savedFlags = dl->Flags;
        if (rpx < 14.0f) dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;
        bool ok = (rpx < 6.0f) || GroundBlob(dl, cam, sp.c, r * 1.06f, sp.seed, 0.22f, rim, N);
        if (ok && GroundBlob(dl, cam, sp.c, r * 0.9f, sp.seed, 0.22f, body, N) && rpx >= 10.0f) {
            dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;
            for (int i = 0; i < 3; ++i) {
                uint32_t h = Hash(sp.seed + i * 71u);
                float ang = HashF(h) * 6.2831853f;
                float dist = r * (1.15f + 0.45f * HashF(h + 1u));
                float sr = r * (0.10f + 0.14f * HashF(h + 2u));
                GroundBlob(dl, cam, V(sp.c.x + std::cos(ang) * dist, sp.c.y, sp.c.z + std::sin(ang) * dist), sr, h, 0.1f, body, 7);
            }
            if (sp.pool && rpx > 24.0f)   // мокрый блик на луже
                GroundBlob(dl, cam, V(sp.c.x - r * 0.22f, sp.c.y, sp.c.z - r * 0.18f), r * 0.32f, sp.seed ^ 0xABCu, 0.3f, IM_COL32(255, 235, 235, (int)(a * 16)), 10);
        }
        dl->Flags = savedFlags;
    }

    // Кольцо по земле (вспышка UV)
    // hue >= 0 — радужное кольцо: оттенок меняется по окружности
    static void GroundRing(ImDrawList* dl, const Camera& cam, const Vec3& c, float radius, const float col[4], float a, float S, float hue = -1.0f) {
        const int N = 48;
        ImVec2 pts[N]; bool ok[N];
        float avgDepth = 0.0f; int cnt = 0;
        for (int i = 0; i < N; ++i) {
            float ang = i * 6.2831853f / N, d;
            ok[i] = Project(cam, V(c.x + std::cos(ang) * radius, c.y + 0.04f, c.z + std::sin(ang) * radius), pts[i], &d, 3.0f);
            if (ok[i]) { avgDepth += d; ++cnt; }
        }
        if (cnt < 3) return;
        float wpx = 0.06f * S * PixelsPerMeter(cam, avgDepth / cnt);
        wpx = wpx < 1.5f ? 1.5f : (wpx > 5.0f ? 5.0f : wpx);
        for (int pass = 0; pass < 2; ++pass) {
            float pa = pass == 0 ? a * 0.30f : a;
            ImU32 cc = Col(col, pa);
            float th = pass == 0 ? wpx * 3.0f : wpx;
            for (int i = 0; i < N; ++i) {
                int j = (i + 1) % N;
                if (hue >= 0.0f) cc = Hue(hue + (float)i / N, 0.85f, 1.0f, pa);
                if (ok[i] && ok[j]) dl->AddLine(pts[i], pts[j], cc, th);
            }
        }
    }

    // 4-лучевая звёздочка-блик
    static void Star4(ImDrawList* dl, ImVec2 c, float r, ImU32 col, ImU32 glow) {
        dl->AddCircleFilled(c, r * 1.1f, glow, 10);
        for (int i = 0; i < 4; ++i) Blade(dl, c, i * 1.5707963f, 0.0f, r * 1.8f, r * 0.55f, 0.4f, col);
    }

    static void RenderKill(World& w, KillEffect& k, const Camera& cam, ImDrawList* dl, float dt) {
        const KillFxConfig& cfg = k.cfg;
        const float S = (cfg.scale < 0.3f) ? 0.3f : cfg.scale;
        const float T = k.t;
        const float p = Clamp01(T / k.dur);
        const float fadeAll = 1.0f - Smooth((p - 0.72f) / 0.28f);
        ImVec2 feetS; float feetDepth = 6.0f;
        bool feetVisible = Project(cam, k.feet, feetS, &feetDepth, 2.5f);
        const Vec3 chest = V(k.feet.x, k.feet.y + 1.25f * S, k.feet.z);

        // --- 0. Кровь на экране (один раз, если убийство совсем рядом)
        if (!k.screenChecked) {
            k.screenChecked = true;
            if (cfg.screenSplatter && feetVisible && feetDepth < w.screenSplatRange) {
                uint32_t s = Hash(k.seed ^ 0x5157u) | 1u;
                int n = 3 + (int)(Rand01(s) * 3.0f);
                for (int i = 0; i < n; ++i) {
                    ScreenSplat ss;
                    float a = RandRange(s, 0.0f, 6.2831853f), rr = RandRange(s, 0.30f, 0.46f);
                    ss.x = 0.5f + std::cos(a) * rr;         // ближе к краям, центр экрана свободен
                    ss.y = 0.5f + std::sin(a) * rr * 0.9f;
                    ss.r = RandRange(s, 0.03f, 0.075f) * (S > 1.5f ? 1.5f : S);
                    ss.dur = RandRange(s, 2.4f, 3.4f);
                    ss.seed = Hash(s);
                    std::memcpy(ss.col, cfg.primary, sizeof(ss.col));
                    w.screen.push_back(ss);
                }
                while (w.screen.size() > 14) w.screen.erase(w.screen.begin());
            }
        }

        // --- 1. Лужа и брызги на земле (под всем остальным)
        for (size_t i = 0; i < k.splats.size(); ++i) {
            const Splat& sp = k.splats[i];
            float age = T - sp.born;
            if (age <= 0.0f) continue;
            float grow = sp.pool ? EaseOutCubic(age / 1.3f) : EaseOutBack(age / 0.12f);
            float r = sp.radius * grow;
            if (r < 0.01f) continue;
            DrawSplat(dl, cam, sp, r, cfg.primary, fadeAll * Smooth(age / 0.05f));
        }

        const float hueT = T * 0.35f * cfg.rainbowSpeed;   // общий перелив радуги

        // --- 1b. Радужная волна по земле (три кольца друг за другом)
        if (cfg.rainbowRing) {
            for (int ring = 0; ring < 3; ++ring) {
                float lt = (T - ring * 0.12f) / 0.8f;
                if (lt <= 0.0f || lt >= 1.0f) continue;
                GroundRing(dl, cam, k.feet, (0.25f + 2.6f * EaseOutCubic(lt)) * S, kBone, std::pow(1.0f - lt, 1.3f) * fadeAll, S, hueT + ring * 0.15f);
            }
        }

        // --- 1c. Кристаллы растут из земли, в конце осыпаются (уходят в землю и гаснут)
        for (const Spike& sp : k.spikes) {
            float g = EaseOutBack((T - sp.delay) / 0.18f);
            if (g <= 0.01f) continue;
            float sink = Smooth((p - 0.7f) / 0.3f);
            float h = sp.height * g * (1.0f - 0.6f * sink);
            float ca = std::cos(sp.angle), sa = std::sin(sp.angle);
            Vec3 base = V(k.feet.x + ca * sp.dist, k.feet.y, k.feet.z + sa * sp.dist);
            Vec3 tip = V(base.x + ca * sp.tilt * h, base.y + h, base.z + sa * sp.tilt * h);
            Vec3 side = Mul(cam.right, sp.width * 0.5f);
            ImVec2 bl, br, bm, tp; float d;
            if (!Project(cam, Sub(base, side), bl, nullptr, 2.0f) || !Project(cam, Add(base, side), br, nullptr, 2.0f) ||
                !Project(cam, Sub(base, Mul(cam.fwd, sp.width * 0.4f)), bm, &d, 2.0f) || !Project(cam, tip, tp, nullptr, 2.0f)) continue;
            float a = fadeAll * (1.0f - sink * 0.8f);
            float hue = sp.hue + hueT;
            ImVec2 f1[3] = { bl, bm, tp }, f2[3] = { bm, br, tp };
            FillConvex(dl, f1, 3, CrystalCol(cfg, hue, 0.55f, 0.0f, a));    // теневая грань
            FillConvex(dl, f2, 3, CrystalCol(cfg, hue, 1.0f, 0.12f, a));    // освещённая грань
            dl->AddLine(bm, tp, IM_COL32(255, 255, 255, (int)(a * 150)), 1.2f); // ребро
            float tw = 0.5f + 0.5f * std::sin(T * 9.0f + sp.hue * 20.0f);
            if (tw > 0.6f) Star4(dl, tp, 2.0f + 3.0f * tw, IM_COL32(255, 255, 255, (int)(a * 230)), CrystalCol(cfg, hue, 1.0f, 0.3f, a * 0.25f));
        }

        // --- 2. UV: кольцо по земле и свечение тела
        if (cfg.uvBurn) {
            float lt = T / 0.6f;
            if (lt < 1.0f) GroundRing(dl, cam, k.feet, (0.2f + 1.8f * EaseOutCubic(lt)) * S, cfg.secondary, std::pow(1.0f - lt, 1.4f), S);
            float ga = Smooth(T / 0.08f) * (1.0f - Smooth(T / 1.2f));
            ImVec2 b, t; float db, dtp;
            if (ga > 0.01f && Project(cam, k.feet, b, &db, 3.0f) && Project(cam, V(k.feet.x, k.feet.y + 1.9f * S, k.feet.z), t, &dtp, 3.0f)) {
                float wb = 0.34f * S * PixelsPerMeter(cam, db), wt = 0.22f * S * PixelsPerMeter(cam, dtp);
                float dx = t.x - b.x, dy = t.y - b.y, l = std::sqrt(dx * dx + dy * dy);
                if (l > 1.0f) {
                    float px = -dy / l, py = dx / l;
                    GradientQuad(dl, ImVec2(b.x - px * wb, b.y - py * wb), ImVec2(b.x + px * wb, b.y + py * wb),
                        ImVec2(t.x + px * wt, t.y + py * wt), ImVec2(t.x - px * wt, t.y - py * wt),
                        Col(cfg.secondary, ga * 0.30f), Col(cfg.secondary, 0.0f));
                    GradientQuad(dl, ImVec2(b.x - px * wb * 0.35f, b.y - py * wb * 0.35f), ImVec2(b.x + px * wb * 0.35f, b.y + py * wb * 0.35f),
                        ImVec2(t.x + px * wt * 0.35f, t.y + py * wt * 0.35f), ImVec2(t.x - px * wt * 0.35f, t.y - py * wt * 0.35f),
                        IM_COL32(255, 240, 255, (int)(ga * 80)), IM_COL32(255, 240, 255, 0));
                }
            }
            // дым поднимается от тела
            for (auto& pf : k.smoke) {
                float age = T - pf.born, lt2 = age / pf.life;
                if (lt2 <= 0.0f || lt2 >= 1.0f) continue;
                ImVec2 sp; float d;
                if (!Project(cam, Add(pf.p, Mul(pf.v, age)), sp, &d, 2.0f)) continue;
                float rad = pf.size * (0.6f + 1.4f * lt2) * PixelsPerMeter(cam, d);
                rad = rad > 110.0f ? 110.0f : rad;
                float a = 0.30f * std::sin(lt2 * 3.14159f) * fadeAll;
                dl->AddCircleFilled(sp, rad, IM_COL32(30, 27, 32, (int)(a * 255)), 14);
                dl->AddCircleFilled(sp, rad * 0.6f, IM_COL32(20, 18, 22, (int)(a * 200)), 10);
            }
        }

        // --- 3. Куски плоти (физика + кровавый след в полёте)
        if (!k.chunks.empty()) {
            float fleshBase[4] = { Lerp(cfg.primary[0], 0.80f, 0.45f), Lerp(cfg.primary[1], 0.42f, 0.45f), Lerp(cfg.primary[2], 0.36f, 0.45f), cfg.primary[3] };
            for (auto& c : k.chunks) {
                if (!c.resting) {
                    c.v.y -= 9.8f * dt;
                    c.p = Add(c.p, Mul(c.v, dt));
                    c.rot += c.spin * dt;
                    float floorY = k.feet.y + c.size * 0.35f;
                    if (c.p.y < floorY) {
                        if (!c.splatted && k.splats.size() < kMaxSplats) {
                            Splat sp;
                            sp.c = V(c.p.x, k.feet.y, c.p.z); sp.radius = c.size * (2.0f + 1.2f * HashF(c.shape)); sp.born = T; sp.seed = Hash(c.shape ^ 0x77u); sp.pool = false;
                            k.splats.push_back(sp);
                            c.splatted = true;
                        }
                        c.p.y = floorY;
                        c.v.y = -c.v.y * 0.22f;
                        c.v.x *= 0.45f; c.v.z *= 0.45f; c.spin *= 0.5f;
                        if (std::fabs(c.v.y) < 0.4f) { c.v = V(0, 0, 0); c.resting = true; }
                    }
                }
                ImVec2 sp; float d;
                if (!Project(cam, c.p, sp, &d, 1.5f)) continue;
                float rad = c.size * PixelsPerMeter(cam, d);
                rad = rad < 2.0f ? 2.0f : (rad > 34.0f ? 34.0f : rad);
                float a = fadeAll;
                if (!c.resting) {
                    ImVec2 tail;
                    if (Project(cam, Sub(c.p, Mul(c.v, 0.06f)), tail, nullptr, 2.0f))
                        dl->AddLine(tail, sp, Tint(cfg.primary, 0.7f, 0.0f, a * 0.85f), rad * 0.55f);
                }
                ImVec2 poly[8], sh[8], hi[8];
                int nv = c.verts > 8 ? 8 : c.verts;
                for (int v = 0; v < nv; ++v) {
                    float ang = c.rot + v * 6.2831853f / nv;
                    float j = 0.72f + 0.28f * HashF(c.shape + v * 11u);
                    float cx = std::cos(ang), cy = std::sin(ang);
                    poly[v] = ImVec2(sp.x + cx * rad * j, sp.y + cy * rad * j);
                    sh[v] = ImVec2(sp.x + cx * (rad * j + 1.5f), sp.y + cy * (rad * j + 1.5f));
                    hi[v] = ImVec2(sp.x - rad * 0.15f + cx * rad * j * 0.5f, sp.y - rad * 0.15f + cy * rad * j * 0.5f);
                }
                FillConvex(dl, sh, nv, Shadow(a * 0.8f));
                FillConvex(dl, poly, nv, Tint(cfg.primary, 0.45f, 0.0f, a));
                FillConvex(dl, hi, nv, Col(fleshBase, a * 0.85f));
            }
        }

        // --- 4. Капли крови (вытянуты по скорости, на земле оставляют брызги)
        for (size_t i = 0; i < k.drops.size(); ++i) {
            Drop& d = k.drops[i];
            if (d.dead) continue;
            d.v.y -= 9.8f * dt;
            d.v = Mul(d.v, 1.0f - 0.6f * dt);   // сопротивление воздуха
            d.p = Add(d.p, Mul(d.v, dt));
            if (d.p.y <= k.feet.y) {
                d.dead = true;
                uint32_t h = Hash(k.seed + (uint32_t)i * 2654435761u);
                if (k.splats.size() < kMaxSplats && (h % 3u) == 0) {
                    Splat sp;
                    sp.c = V(d.p.x, k.feet.y, d.p.z); sp.radius = d.size * (2.2f + 1.8f * HashF(h)); sp.born = T; sp.seed = h; sp.pool = false;
                    k.splats.push_back(sp);
                }
                continue;
            }
            ImVec2 head, tail; float dh;
            if (!Project(cam, d.p, head, &dh, 1.5f)) continue;
            if (!Project(cam, Sub(d.p, Mul(d.v, 0.035f)), tail, nullptr, 2.0f)) tail = head;
            float th = d.size * PixelsPerMeter(cam, dh);
            th = th < 1.2f ? 1.2f : (th > 7.0f ? 7.0f : th);
            ImU32 c = (d.shade == 0) ? Tint(cfg.primary, 1.0f, 0.0f, fadeAll)
                    : (d.shade == 1) ? Tint(cfg.primary, 0.6f, 0.0f, fadeAll)
                    : Tint(cfg.primary, 1.35f, 0.08f, fadeAll);
            dl->AddLine(tail, head, c, th);
            if (th >= 2.5f) dl->AddCircleFilled(head, th * 0.6f, c, 6);
        }

        // --- 4b. Кристальные осколки: гранёные ромбы с бликом, звенят о землю и лежат
        for (Shard& sh : k.shards) {
            if (!sh.resting) {
                sh.v.y -= 9.8f * dt;
                sh.p = Add(sh.p, Mul(sh.v, dt));
                sh.rot += sh.spin * dt;
                float floorY = k.feet.y + sh.size * 0.2f;
                if (sh.p.y < floorY) {
                    sh.p.y = floorY;
                    sh.v.y = -sh.v.y * 0.3f;
                    sh.v.x *= 0.5f; sh.v.z *= 0.5f; sh.spin *= 0.4f;
                    if (std::fabs(sh.v.y) < 0.4f) { sh.v = V(0, 0, 0); sh.resting = true; }
                }
            }
            ImVec2 sp; float d;
            if (!Project(cam, sh.p, sp, &d, 1.5f)) continue;
            float rad = sh.size * PixelsPerMeter(cam, d);
            rad = rad < 2.0f ? 2.0f : (rad > 30.0f ? 30.0f : rad);
            float cr = std::cos(sh.rot), sr = std::sin(sh.rot);
            auto P = [&](float x, float y) { return ImVec2(sp.x + (x * cr - y * sr) * rad, sp.y + (x * sr + y * cr) * rad); };
            ImVec2 top = P(0.0f, -1.4f), right = P(0.55f, 0.0f), bottom = P(0.0f, 1.0f), left = P(-0.55f, 0.0f);
            float hue = sh.hue + hueT;
            ImVec2 fa[3] = { top, right, bottom }, fb[3] = { top, bottom, left };
            FillConvex(dl, fa, 3, CrystalCol(cfg, hue, 1.0f, 0.10f, fadeAll));
            FillConvex(dl, fb, 3, CrystalCol(cfg, hue, 0.6f, 0.0f, fadeAll));
            float glint = std::sin(sh.rot * 2.0f + T * 6.0f);
            if (glint > 0.55f) {
                dl->AddLine(top, bottom, IM_COL32(255, 255, 255, (int)(fadeAll * 200 * glint)), 1.0f);
                if (rad > 6.0f) Star4(dl, top, rad * 0.25f, IM_COL32(255, 255, 255, (int)(fadeAll * 220)), CrystalCol(cfg, hue, 1.0f, 0.3f, fadeAll * 0.2f));
            }
        }

        // --- 4c. Радужное конфетти: летит вверх, потом кувыркается и планирует вниз
        if (!k.confetti.empty()) {
            const ImDrawListFlags savedFlags = dl->Flags;
            dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;   // мелкие квады — AA не нужен
            for (Confetti& c : k.confetti) {
                if (!c.landed) {
                    c.v.y -= 9.8f * 0.45f * dt;
                    float drag = 1.0f - 2.2f * dt;
                    c.v.x *= drag; c.v.z *= drag;
                    if (c.v.y < -1.1f * S) c.v.y = -1.1f * S;   // бумажка планирует, а не падает камнем
                    c.p = Add(c.p, Mul(c.v, dt));
                    c.p.x += std::sin(T * 3.0f * c.sway + c.flip) * 0.45f * S * dt;
                    c.rot += c.spin * dt;
                    c.flip += c.flipSpeed * dt;
                    if (c.p.y <= k.feet.y + 0.01f) { c.p.y = k.feet.y + 0.01f; c.landed = true; }
                }
                ImVec2 sp; float d;
                if (!Project(cam, c.p, sp, &d, 1.4f)) continue;
                float ppm = PixelsPerMeter(cam, d);
                float wx = c.size * ppm * 0.6f * (c.landed ? 1.0f : std::fabs(std::cos(c.flip))) + 0.6f;
                float wy = c.size * ppm * 0.35f + 0.6f;
                if (wy > 9.0f) { wx *= 9.0f / wy; wy = 9.0f; }
                float cr = std::cos(c.rot), sr = std::sin(c.rot);
                ImVec2 ax(cr * wx, sr * wx), ay(-sr * wy, cr * wy);
                bool back = !c.landed && std::cos(c.flip) < 0.0f;  // изнанка бумажки темнее
                ImU32 col = Hue(c.hue + hueT * 0.3f, 0.75f, back ? 0.7f : 1.0f, fadeAll);
                dl->AddQuadFilled(ImVec2(sp.x - ax.x - ay.x, sp.y - ax.y - ay.y), ImVec2(sp.x + ax.x - ay.x, sp.y + ax.y - ay.y),
                                  ImVec2(sp.x + ax.x + ay.x, sp.y + ax.y + ay.y), ImVec2(sp.x - ax.x + ay.x, sp.y - ax.y + ay.y), col);
            }
            dl->Flags = savedFlags;
        }

        // --- 5. Кровавая взвесь
        if (!k.mist.empty()) {
            for (auto& pf : k.mist) {
                float age = T - pf.born, lt = age / pf.life;
                if (lt <= 0.0f || lt >= 1.0f) continue;
                Vec3 pos = Add(pf.p, Mul(pf.v, (1.0f - std::exp(-2.5f * age)) / 2.5f));   // быстро тормозит
                ImVec2 sp; float d;
                if (!Project(cam, pos, sp, &d, 2.0f)) continue;
                float rad = pf.size * (0.5f + 1.2f * EaseOutCubic(lt)) * PixelsPerMeter(cam, d);
                rad = rad > 110.0f ? 110.0f : rad;
                float a = 0.35f * std::pow(1.0f - lt, 1.5f) * Smooth(lt / 0.08f);
                dl->AddCircleFilled(sp, rad, Tint(cfg.primary, 1.15f, 0.04f, a * 0.5f), 14);
                dl->AddCircleFilled(sp, rad * 0.6f, Tint(cfg.primary, 1.15f, 0.04f, a), 10);
            }
        }

        // --- 6. UV: угли, пепел и вспышка в груди
        if (cfg.uvBurn) {
            for (auto& e : k.embers) {
                float cycle = std::fmod(T * e.speed * 0.45f + e.phase, 1.0f);
                float y = cycle * (e.ash ? 2.8f : 2.2f) * S;
                float ang = e.angle + T * (e.ash ? 0.8f : 1.6f);
                float r = e.radius + std::sin(T * 4.0f + e.phase * 12.0f) * 0.04f * S;
                ImVec2 sp; float d;
                if (!Project(cam, V(k.feet.x + std::cos(ang) * r, k.feet.y + 0.15f * S + y, k.feet.z + std::sin(ang) * r), sp, &d, 2.0f)) continue;
                float rad = e.size * PixelsPerMeter(cam, d);
                rad = rad < 1.2f ? 1.2f : (rad > 6.0f ? 6.0f : rad);
                float a = fadeAll * std::sin(cycle * 3.14159f) * Smooth(T / 0.15f);
                if (e.ash) {
                    float q = std::floor(rad) + 1.0f;
                    dl->AddRectFilled(ImVec2(sp.x - q, sp.y - q * 0.6f), ImVec2(sp.x + q, sp.y + q * 0.6f), IM_COL32(42, 38, 38, (int)(a * 210)));
                }
                else {
                    float flick = 0.55f + 0.45f * std::sin(T * 30.0f + e.phase * 20.0f);
                    dl->AddCircleFilled(sp, rad * 3.0f, IM_COL32(255, 120, 40, (int)(a * 45)), 12);
                    dl->AddCircleFilled(sp, rad, IM_COL32(255, 170, 60, (int)(a * flick * 255)), 10);
                    dl->AddCircleFilled(sp, rad * 0.45f, IM_COL32(255, 240, 200, (int)(a * flick * 255)), 8);
                }
            }
            if (T < 0.4f) {
                ImVec2 cs; float dc;
                if (Project(cam, chest, cs, &dc, 2.5f)) {
                    float r = 0.9f * S * PixelsPerMeter(cam, dc) * (0.4f + EaseOutCubic(T / 0.4f));
                    float fa = 1.0f - T / 0.4f;
                    dl->AddCircleFilled(cs, r * 1.5f, Col(cfg.secondary, fa * 0.10f), 40);
                    dl->AddCircleFilled(cs, r, Col(cfg.secondary, fa * 0.22f), 32);
                    dl->AddCircleFilled(cs, r * 0.35f, IM_COL32(255, 245, 255, (int)(fa * 170)), 24);
                }
            }
        }

        // --- 6b. Радужные лучи-вспышка из груди
        if (cfg.rainbowRays && T < 0.6f) {
            ImVec2 cs; float dc;
            if (Project(cam, chest, cs, &dc, 2.0f)) {
                float ppm = PixelsPerMeter(cam, dc);
                float ra = 1.0f - Smooth(T / 0.6f);
                float grow = EaseOutCubic(T / 0.25f);
                const int n = 14;
                for (int i = 0; i < n; ++i) {
                    float hv = HashF(k.seed + i * 131u);
                    float ang = i * 6.2831853f / n + (hv - 0.5f) * 0.3f + T * 0.8f;
                    float r0 = ppm * 0.12f * S, r1 = r0 + ppm * S * (0.6f + 0.6f * hv) * grow;
                    float wpx = ppm * 0.09f * S; wpx = wpx < 2.0f ? 2.0f : (wpx > 14.0f ? 14.0f : wpx);
                    float hue = (float)i / n + hueT;
                    Blade(dl, cs, ang, r0, r1, wpx * 2.4f, wpx * 0.8f, Hue(hue, 0.8f, 1.0f, ra * 0.25f));
                    Blade(dl, cs, ang, r0, r1, wpx, 0.6f, Hue(hue, 0.7f, 1.0f, ra));
                }
                dl->AddCircleFilled(cs, ppm * 0.18f * S * (0.6f + grow), IM_COL32(255, 255, 255, (int)(ra * 170)), 20);
            }
        }

        // --- 6c. Радужные искорки по спирали вверх
        for (const Ember& e : k.sparkles) {
            float cycle = std::fmod(T * e.speed * 0.5f + e.phase, 1.0f);
            float ang = e.angle + T * 2.2f + cycle * 4.0f;
            float r = e.radius * (1.0f - 0.4f * cycle);
            ImVec2 sp; float d;
            if (!Project(cam, V(k.feet.x + std::cos(ang) * r, k.feet.y + 0.2f * S + cycle * 2.4f * S, k.feet.z + std::sin(ang) * r), sp, &d, 2.0f)) continue;
            float rad = e.size * PixelsPerMeter(cam, d);
            rad = rad < 1.5f ? 1.5f : (rad > 7.0f ? 7.0f : rad);
            float tw = 0.45f + 0.55f * std::fabs(std::sin(T * 12.0f + e.phase * 20.0f));
            float a = fadeAll * std::sin(cycle * 3.14159f) * Smooth(T / 0.1f);
            float hue = e.phase + hueT;
            Star4(dl, sp, rad * tw, Hue(hue, 0.35f, 1.0f, a), Hue(hue, 0.85f, 1.0f, a * 0.3f));
        }

        // --- 6d. Ледяная волна: тонкий наст на земле, кольцо инея и падающие снежинки
        if (cfg.frostNova) {
            float it = T / 0.4f;
            GroundBlob(dl, cam, k.feet, 0.95f * S * EaseOutCubic(it), k.seed ^ 0xF205u, 0.25f, CrystalCol(cfg, hueT, 1.0f, 0.25f, 0.16f * fadeAll), 14);
            float lt = T / 0.7f;
            if (lt < 1.0f) GroundRing(dl, cam, k.feet, (0.3f + 2.4f * EaseOutCubic(lt)) * S, cfg.crystalColor, (1.0f - lt) * fadeAll, S);
            for (int i = 0; i < k.flakeCount; ++i) {
                uint32_t h = Hash(k.seed + 7000u + i * 53u);
                float a = HashF(h) * 6.2831853f, r = HashF(h + 1u) * 1.5f * S;
                float y = (1.5f + 2.0f * HashF(h + 2u)) * S - (0.35f + 0.25f * HashF(h + 3u)) * S * T;
                if (y < 0.02f) continue;   // растаяла на земле
                float sway = std::sin(T * 2.0f + HashF(h + 4u) * 6.0f) * 0.15f * S;
                ImVec2 sp; float d;
                if (!Project(cam, V(k.feet.x + std::cos(a) * r + sway, k.feet.y + y, k.feet.z + std::sin(a) * r), sp, &d, 1.6f)) continue;
                float len = 0.05f * S * PixelsPerMeter(cam, d);
                len = len < 2.5f ? 2.5f : (len > 9.0f ? 9.0f : len);
                float rot = HashF(h + 5u) * 6.2831853f + T;
                ImU32 c = CrystalCol(cfg, HashF(h) + hueT, 1.0f, 0.45f, fadeAll * Smooth(T / 0.2f));
                for (int arm = 0; arm < 3; ++arm) {   // 6-лучевая снежинка: три линии через центр
                    float ra = rot + arm * 1.0471976f;
                    dl->AddLine(ImVec2(sp.x - std::cos(ra) * len, sp.y - std::sin(ra) * len), ImVec2(sp.x + std::cos(ra) * len, sp.y + std::sin(ra) * len), c, 1.2f);
                }
            }
        }

        // --- 6e. Салют: ракеты взлетают от тела и рассыпаются радужными искрами
        for (int r = 0; r < k.rockets; ++r) {
            uint32_t h = Hash(k.seed + r * 977u);
            const float launch = r * 0.12f, flight = 0.55f;
            Vec3 base = V(k.feet.x + (HashF(h) - 0.5f) * 0.6f * S, k.feet.y + 1.0f * S, k.feet.z + (HashF(h + 1u) - 0.5f) * 0.6f * S);
            Vec3 apex = V(base.x + (HashF(h + 2u) - 0.5f) * 1.4f * S, base.y + (2.6f + 1.4f * HashF(h + 3u)) * S, base.z + (HashF(h + 4u) - 0.5f) * 1.4f * S);
            float hue = HashF(h + 5u) + hueT * 0.2f;
            float tr = (T - launch) / flight;
            if (tr < 0.0f) continue;
            if (tr < 1.0f) {
                ImVec2 a, b;
                Vec3 pos = Add(base, Mul(Sub(apex, base), EaseOutCubic(tr)));
                Vec3 tail = Add(base, Mul(Sub(apex, base), EaseOutCubic(tr > 0.2f ? tr - 0.2f : 0.0f)));
                if (Project(cam, pos, a, nullptr, 2.0f) && Project(cam, tail, b, nullptr, 2.0f)) {
                    dl->AddLine(b, a, Hue(hue, 0.5f, 1.0f, 0.5f * fadeAll), 2.0f);
                    dl->AddCircleFilled(a, 3.0f, IM_COL32(255, 250, 230, (int)(255 * fadeAll)), 8);
                }
                continue;
            }
            float age = T - launch - flight;
            if (age > 1.4f) continue;
            float fa = std::pow(1.0f - age / 1.4f, 1.2f) * fadeAll;
            ImVec2 ap; float da;
            if (age < 0.12f && Project(cam, apex, ap, &da, 2.0f)) {
                float fr = 0.5f * S * PixelsPerMeter(cam, da) * (0.4f + age / 0.12f);
                dl->AddCircleFilled(ap, fr, Hue(hue, 0.4f, 1.0f, (1.0f - age / 0.12f) * 0.35f), 20);
            }
            for (int i = 0; i < 22; ++i) {
                uint32_t hs = Hash(h + 100u + i * 31u);
                float th = HashF(hs) * 6.2831853f, ph = std::acos(1.0f - 2.0f * HashF(hs + 1u));
                Vec3 dir = V(std::sin(ph) * std::cos(th), std::cos(ph), std::sin(ph) * std::sin(th));
                float speed = (2.0f + 1.2f * HashF(hs + 2u)) * S;
                auto At = [&](float t) {
                    float disp = speed * (1.0f - std::exp(-2.5f * t)) / 2.5f;
                    return V(apex.x + dir.x * disp, apex.y + dir.y * disp - 1.6f * S * t * t, apex.z + dir.z * disp);
                };
                if (age > 0.9f && (Hash(hs + (uint32_t)(T * 20.0f)) & 1)) continue;   // мерцание в конце
                ImVec2 a, b;
                if (!Project(cam, At(age), a, nullptr, 2.0f) || !Project(cam, At(age > 0.06f ? age - 0.06f : 0.0f), b, nullptr, 2.0f)) continue;
                ImU32 c = Hue(hue + (i % 3) * 0.06f, 0.75f, 1.0f, fa);
                dl->AddLine(b, a, c, 2.0f);
                dl->AddCircleFilled(a, 1.8f, IM_COL32(255, 255, 255, (int)(fa * 220)), 6);
            }
        }

        // --- 6f. Воронка: частицы закручиваются в грудь, тёмное ядро, затем хлопок
        if (k.vortexCount > 0) {
            const float pull = 0.55f;
            ImVec2 cs; float dc;
            bool chestOk = Project(cam, chest, cs, &dc, 2.0f);
            float ppm = chestOk ? PixelsPerMeter(cam, dc) : 0.0f;
            if (T < pull) {
                float e = T / pull; e = e * e;
                for (int i = 0; i < k.vortexCount; ++i) {
                    uint32_t h = Hash(k.seed + 9000u + i * 17u);
                    float a0 = HashF(h) * 6.2831853f, r0 = (0.9f + 0.9f * HashF(h + 1u)) * S, y0 = (0.1f + 1.8f * HashF(h + 2u)) * S;
                    auto At = [&](float ee) {
                        float r = r0 * (1.0f - ee), ang = a0 + ee * 6.0f;
                        return V(k.feet.x + std::cos(ang) * r, k.feet.y + y0 + (1.25f * S - y0) * ee, k.feet.z + std::sin(ang) * r);
                    };
                    ImVec2 a, b;
                    if (!Project(cam, At(e), a, nullptr, 2.0f) || !Project(cam, At(e > 0.06f ? e - 0.06f : 0.0f), b, nullptr, 2.0f)) continue;
                    float va = Smooth(T / 0.08f) * fadeAll;
                    dl->AddLine(b, a, Col(cfg.secondary, va * 0.7f), 2.0f);
                    dl->AddCircleFilled(a, 2.2f, Tint(cfg.secondary, 1.3f, 0.3f, va), 6);
                }
                if (chestOk) dl->AddCircleFilled(cs, ppm * 0.14f * S * e + 1.0f, IM_COL32(12, 0, 24, (int)(200 * fadeAll)), 20);
            }
            else if (T < pull + 0.45f && chestOk) {
                float lt = (T - pull) / 0.45f;
                float rr = ppm * S * (0.2f + 1.4f * EaseOutCubic(lt));
                dl->AddCircle(cs, rr, Col(cfg.secondary, (1.0f - lt) * fadeAll), 40, 4.0f * (1.0f - lt) + 1.0f);
                dl->AddCircle(cs, rr * 0.7f, IM_COL32(255, 255, 255, (int)((1.0f - lt) * 120 * fadeAll)), 32, 1.5f);
                if (lt < 0.3f) dl->AddCircleFilled(cs, ppm * 0.35f * S * (1.0f - lt / 0.3f), Col(cfg.secondary, 0.4f * fadeAll), 24);
            }
        }

        // --- 6g. Призрак: полупрозрачная фигура поднимается из тела и тает
        if (cfg.ghostRise) {
            float gt = (T - 0.15f) / 1.6f;
            ImVec2 sp; float d;
            Vec3 gp = Add(chest, Add(Mul(cam.right, std::sin(T * 3.0f) * 0.12f * S), V(0.0f, EaseOutCubic(gt) * 1.6f * S + 0.2f * S, 0.0f)));
            if (gt > 0.0f && gt < 1.0f && Project(cam, gp, sp, &d, 1.6f)) {
                float sz = 0.32f * S * PixelsPerMeter(cam, d);
                sz = sz > 90.0f ? 90.0f : sz;
                float a = Smooth(gt / 0.15f) * (1.0f - Smooth((gt - 0.6f) / 0.4f)) * 0.55f * fadeAll;
                ImU32 body = IM_COL32(215, 232, 255, (int)(a * 255));
                ImVec2 head(sp.x, sp.y - sz * 0.6f);
                dl->AddCircleFilled(head, sz * 2.2f, IM_COL32(180, 210, 255, (int)(a * 40)), 24);
                ImVec2 q[4] = { ImVec2(head.x - sz, head.y), ImVec2(head.x + sz, head.y), ImVec2(sp.x + sz * 0.8f, sp.y + sz * 1.6f), ImVec2(sp.x - sz * 0.8f, sp.y + sz * 1.6f) };
                FillConvex(dl, q, 4, body);
                dl->AddCircleFilled(head, sz, body, 24);
                for (int i = 0; i < 3; ++i)   // волнистый "хвост"
                    dl->AddCircleFilled(ImVec2(sp.x + (i - 1) * sz * 0.55f + std::sin(T * 6.0f + i) * sz * 0.12f, sp.y + sz * 1.6f), sz * 0.3f, body, 12);
                ImU32 eye = IM_COL32(10, 14, 30, (int)(a * 2.0f > 1.0f ? 255 : a * 2.0f * 255));
                dl->AddCircleFilled(ImVec2(head.x - sz * 0.35f, head.y - sz * 0.05f), sz * 0.16f, eye, 10);
                dl->AddCircleFilled(ImVec2(head.x + sz * 0.35f, head.y - sz * 0.05f), sz * 0.16f, eye, 10);
                dl->AddCircleFilled(ImVec2(head.x, head.y + sz * 0.42f), sz * 0.12f, eye, 10);
            }
        }

        // --- 7. Надпись над телом
        if (cfg.killLabel && cfg.labelText[0]) {
            Vec3 lp = V(k.feet.x, k.feet.y + 2.15f * S + 0.35f * EaseOutCubic(p), k.feet.z);
            ImVec2 c; float d;
            if (Project(cam, lp, c, &d, 1.3f)) {
                ImFont* font = g_FontBold ? g_FontBold : ImGui::GetFont();
                float sz = font->FontSize * ImGui::GetIO().FontGlobalScale * 1.35f * (0.8f + 0.4f * EaseOutBack(T / 0.3f));
                float ds = 6.0f / (d > 0.5f ? d : 0.5f);
                sz *= (ds < 0.7f ? 0.7f : (ds > 1.2f ? 1.2f : ds));
                char text[48];
                if (k.streak >= 2) snprintf(text, sizeof(text), "%s  x%d", cfg.labelText, k.streak);
                else snprintf(text, sizeof(text), "%s", cfg.labelText);
                ImVec2 ts = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, text);
                ImVec2 tp(std::floor(c.x - ts.x * 0.5f), std::floor(c.y - ts.y * 0.5f));
                float a = fadeAll * Smooth(T / 0.1f);
                ImU32 rim = Tint(cfg.primary, 0.6f, 0.0f, a);
                dl->AddText(font, sz, ImVec2(tp.x + 2, tp.y + 2), Shadow(a), text);
                dl->AddText(font, sz, ImVec2(tp.x - 1, tp.y), rim, text);
                dl->AddText(font, sz, ImVec2(tp.x + 1, tp.y), rim, text);
                dl->AddText(font, sz, ImVec2(tp.x, tp.y + 1), rim, text);
                dl->AddText(font, sz, tp, Col(kBone, a), text);
            }
        }
    }

    // Кровь на "объективе" поверх мира: рваное пятно, брызги и потёки вниз
    static void RenderScreen(World& w, const Camera& cam, ImDrawList* dl, float dt) {
        for (size_t i = 0; i < w.screen.size(); ) {
            ScreenSplat& s = w.screen[i];
            s.t += dt;
            if (s.t >= s.dur) { w.screen.erase(w.screen.begin() + i); continue; }
            ++i;
            float p = s.t / s.dur;
            float a = Smooth(s.t / 0.04f) * (1.0f - Smooth((p - 0.55f) / 0.45f));
            ImVec2 c(cam.vx + s.x * cam.vw, cam.vy + s.y * cam.vh);
            float R = s.r * cam.vh * (0.8f + 0.2f * EaseOutBack(s.t / 0.12f));
            ImU32 rim = Tint(s.col, 0.45f, 0.0f, a * 0.92f);
            ImU32 body = Tint(s.col, 0.8f, 0.0f, a * 0.85f);
            for (int d = 0; d < 3; ++d) {
                uint32_t h = Hash(s.seed + d * 17u);
                if (d > 0 && (h & 1)) continue;
                float ox = (HashF(h) - 0.5f) * R * 1.1f;
                float L = R * (0.6f + 1.8f * HashF(h + 1u)) * EaseOutCubic(s.t / 1.8f);
                float wd = R * (0.10f + 0.06f * HashF(h + 2u));
                ImVec2 top(c.x + ox, c.y + R * 0.4f), bot(top.x, top.y + L);
                dl->AddLine(top, bot, body, wd * 2.0f);
                dl->AddCircleFilled(bot, wd * 1.25f, body, 12);
            }
            const int N = 18;
            ImVec2 pts[N], ptsIn[N];
            for (int k = 0; k < N; ++k) {
                float ang = k * 6.2831853f / N;
                float j = 0.78f + 0.22f * HashF(s.seed + k * 29u);
                pts[k] = ImVec2(c.x + std::cos(ang) * R * j * 1.06f, c.y + std::sin(ang) * R * j * 1.06f);
                ptsIn[k] = ImVec2(c.x + std::cos(ang) * R * j * 0.9f, c.y + std::sin(ang) * R * j * 0.9f);
            }
            FillConvex(dl, pts, N, rim);
            FillConvex(dl, ptsIn, N, body);
            for (int k = 0; k < 5; ++k) {
                uint32_t h = Hash(s.seed + 1000u + k * 31u);
                float ang = HashF(h) * 6.2831853f, dist = R * (1.15f + 0.85f * HashF(h + 1u)), sr = R * (0.06f + 0.14f * HashF(h + 2u));
                ImVec2 q(c.x + std::cos(ang) * dist, c.y + std::sin(ang) * dist);
                dl->AddCircleFilled(q, sr * 1.15f, rim, 12);
                dl->AddCircleFilled(q, sr, body, 12);
            }
            dl->AddCircleFilled(ImVec2(c.x - R * 0.3f, c.y - R * 0.32f), R * 0.22f, IM_COL32(255, 220, 220, (int)(a * 40)), 14); // мокрый блик
        }
    }

    static void RenderBanner(World& w, ImDrawList* dl, float dt, float cx, float y) {
        Banner& b = w.banner;
        if (!b.active) return;
        b.t += dt;
        if (b.t >= b.dur) { b.active = false; return; }
        ImFont* font = g_FontBold ? g_FontBold : ImGui::GetFont();
        float sz = font->FontSize * ImGui::GetIO().FontGlobalScale * (1.75f + 0.08f * (b.level > 6 ? 6 : b.level));
        float a = 1.0f - Smooth((b.t - (b.dur - 0.3f)) / 0.3f);
        const char* txt = b.text.c_str();
        int n = (int)b.text.size();
        float total = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, txt).x + (n - 1) * 2.0f;
        float x = std::floor(cx - total * 0.5f);
        for (int i = 0; i < n; ++i) {
            float ct = b.t - i * 0.03f;
            if (ct <= 0.0f) break;
            char ch[2] = { txt[i], 0 };
            float cw = font->CalcTextSizeA(sz, FLT_MAX, 0.0f, ch).x;
            // буква "падает" сверху и вздрагивает от удара
            float pop = EaseOutBack(ct / 0.16f);
            float shake = (1.0f - Clamp01(ct / 0.2f)) * 3.0f * std::sin(ct * 90.0f + i);
            float yy = std::floor(y - (1.0f - pop) * 18.0f + shake);
            float ca = a * Clamp01(ct / 0.08f);
            // кровь стекает с букв
            uint32_t h = Hash((uint32_t)i * 7919u + (uint32_t)b.level);
            if (ch[0] != ' ' && (h & 3) != 0) {
                float dx = x + cw * (0.25f + 0.5f * HashF(h + 1u));
                float L = sz * (0.25f + 0.6f * HashF(h + 2u)) * EaseOutCubic((ct - 0.15f) / 1.0f);
                float wd = 2.0f + ((h >> 12) & 1) * 1.5f;
                if (L > 0.5f) {
                    float top = yy + sz * 0.78f;
                    dl->AddRectFilled(ImVec2(dx - wd * 0.5f, top), ImVec2(dx + wd * 0.5f, top + L), Tint(b.primary, 0.85f, 0.0f, ca));
                    dl->AddCircleFilled(ImVec2(dx, top + L), wd * 0.95f, Tint(b.primary, 0.85f, 0.0f, ca), 10);
                }
            }
            dl->AddText(font, sz, ImVec2(x + 3, yy + 3), Shadow(ca), ch);
            dl->AddText(font, sz, ImVec2(x - 1, yy), Tint(b.primary, 0.45f, 0.0f, ca), ch);
            dl->AddText(font, sz, ImVec2(x + 1, yy), Tint(b.primary, 0.45f, 0.0f, ca), ch);
            dl->AddText(font, sz, ImVec2(x, yy), Tint(b.primary, 1.55f, 0.06f, ca), ch);
            x += cw + 2.0f;
        }
        // мазок кистью под надписью (сужается к краям)
        float lw = total * EaseOutCubic(b.t / 0.35f);
        float ly = y + sz + 8.0f;
        if (lw > 4.0f) {
            ImVec2 q[4] = { ImVec2(cx - lw * 0.5f, ly), ImVec2(cx - lw * 0.1f, ly - 3.0f), ImVec2(cx + lw * 0.5f, ly + 1.0f), ImVec2(cx - lw * 0.05f, ly + 3.5f) };
            FillConvex(dl, q, 4, Tint(b.primary, 0.9f, 0.0f, a * 0.9f));
        }
    }

    static void Render(World& w, const Camera& cam, ImDrawList* dl, float dt, float bannerX, float bannerY) {
        if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;
        w.sinceLastKill += dt;
        if (cam.valid) {
            for (size_t i = 0; i < w.kills.size(); ) {
                w.kills[i].t += dt;
                if (w.kills[i].t >= w.kills[i].dur) { w.kills.erase(w.kills.begin() + i); continue; }
                RenderKill(w, w.kills[i], cam, dl, dt);
                ++i;
            }
        }
        else w.kills.clear();
        RenderScreen(w, cam, dl, dt);
        RenderHits(w, cam, dl, dt);
        RenderNumbers(w, cam, dl, dt);
        RenderBanner(w, dl, dt, bannerX, bannerY);
    }

    static void RenderGame(ImDrawList* dl, float dt) {
        game.useTracking = true;
        SetViewport(gameCam);
        ImVec2 c = g_GameView.Center();
        Render(game, gameCam, dl, dt, c.x, g_GameView.y + g_GameView.h * 0.22f);
    }

    // ============================================================== preview
    // Мини-сцена в меню: виртуальная камера смотрит на "манекен-зомби" в 6.5 м.
    struct PreviewState { float hitTimer = 0.3f, killTimer = 0.9f; int hitsOnDummy = 0; };
    PreviewState previewState;

    static void RenderPreview(ImDrawList* dl, ImVec2 a, ImVec2 b, float dt, const HitmarkerConfig& hcfg, const KillFxConfig& kcfg) {
        if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;
        Camera cam;
        cam.vx = a.x; cam.vy = a.y; cam.vw = b.x - a.x; cam.vh = b.y - a.y;
        SetupPinhole(cam, V(0.0f, 1.7f, 0.0f), V(0.0f, -0.10f, 1.0f), 80.0f);
        const Vec3 feet = V(0.0f, 0.0f, 6.5f);

        dl->PushClipRect(a, b, true);
        // фон: ночной тёмный градиент + сетка пола
        dl->AddRectFilledMultiColor(a, b, IM_COL32(18, 16, 20, 255), IM_COL32(18, 16, 20, 255), IM_COL32(9, 7, 8, 255), IM_COL32(9, 7, 8, 255));
        for (int i = -6; i <= 6; ++i) {
            ImVec2 p0, p1;
            if (Project(cam, V((float)i, 0.0f, 1.2f), p0, nullptr, 20.0f) && Project(cam, V((float)i, 0.0f, 14.0f), p1, nullptr, 20.0f))
                dl->AddLine(p0, p1, IM_COL32(58, 46, 48, 110), 1.0f);
        }
        for (int z = 2; z <= 14; ++z) {
            ImVec2 p0, p1;
            if (Project(cam, V(-6.0f, 0.0f, (float)z), p0, nullptr, 20.0f) && Project(cam, V(6.0f, 0.0f, (float)z), p1, nullptr, 20.0f))
                dl->AddLine(p0, p1, IM_COL32(58, 46, 48, 110), 1.0f);
        }
        // манекен (исчезает на время килл-эффекта)
        bool dummyDead = !preview.kills.empty() && preview.kills.back().t < 1.0f;
        if (!dummyDead) {
            ImVec2 f, hd, sh; float d;
            if (Project(cam, feet, f, &d) && Project(cam, V(feet.x, 1.72f, feet.z), hd) && Project(cam, V(feet.x, 1.4f, feet.z), sh)) {
                float ppm = PixelsPerMeter(cam, d);
                dl->AddLine(ImVec2(f.x, f.y), sh, IM_COL32(74, 82, 70, 255), 0.42f * ppm);
                dl->AddCircleFilled(hd, 0.16f * ppm, IM_COL32(88, 96, 80, 255), 20);
                dl->AddLine(ImVec2(sh.x - 0.28f * ppm, sh.y + 0.05f * ppm), ImVec2(sh.x + 0.28f * ppm, sh.y + 0.05f * ppm), IM_COL32(74, 82, 70, 255), 0.14f * ppm);
            }
        }

        // зацикленная демонстрация: 3 попадания -> убийство
        PreviewState& st = previewState;
        st.hitTimer -= dt;
        if (st.hitTimer <= 0.0f && !dummyDead) {
            st.hitTimer = 0.55f;
            Vec3 chest = V(feet.x, MenuVars::hitHeightOffset, feet.z);
            ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
            HitmarkerConfig hc = hcfg; hc.enabled = true;
            float dmg = 18.0f + (float)((st.hitsOnDummy * 37) % 23);
            SpawnHit(preview, hc, 0xD0D0, 0xD0D0, &chest, center, dmg);
            ++st.hitsOnDummy;
            if (st.hitsOnDummy >= 3) {
                st.hitsOnDummy = 0;
                MarkKilled(preview, 0xD0D0);
                KillFxConfig kc = kcfg; kc.enabled = true;
                preview.sinceLastKill = 999.0f;
                SpawnKill(preview, kc, &feet);
                st.hitTimer = (kc.duration > 1.2f ? kc.duration : 1.2f) + 0.4f;
            }
        }
        preview.useTracking = false;
        preview.screenSplatRange = 99.0f;   // в превью камера далеко — показываем всегда
        Render(preview, cam, dl, dt, (a.x + b.x) * 0.5f, a.y + 16.0f);
        dl->PopClipRect();
    }
}
