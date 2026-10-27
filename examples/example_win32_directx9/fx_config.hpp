#pragma once
// Настройки хитмаркеров и килл-эффектов (одни на всех врагов: зомби и людей).
// Стиль — под Dying Light: кровь, плоть, UV-свет, никакого неона.

enum HitStyle {
    HIT_CLAW = 0,         // три рваные царапины когтями наискосок (Beast)
    HIT_BLOOD_X,          // X из сужающихся лезвий, с кончиков слетают капли крови
    HIT_CLASSIC,          // строгий крест с зазором, без лишнего
    HIT_SPLATTER,         // кровавая клякса раскрывается в точке удара
    HIT_BONE_CRACK,       // рваные трещины расходятся из центра
    HIT_UV_PULSE,         // UV-кольцо сжимается к цели, крестик в центре
    HIT_STYLE_COUNT
};

struct HitmarkerConfig {
    bool enabled = false;
    int style = HIT_BLOOD_X;
    int positionMode = 1;          // 0 = прицел (центр экрана), 1 = на враге
    float size = 11.0f;
    float thickness = 2.0f;
    float duration = 0.45f;
    bool glow = true;              // мягкий ореол вокруг маркера
    float color[4] = { 0.94f, 0.91f, 0.86f, 1.00f };      // "кость"
    float killColor[4] = { 0.82f, 0.05f, 0.04f, 1.00f };  // кровь
    bool damageNumbers = true;
    float damageColor[4] = { 0.96f, 0.80f, 0.52f, 1.00f };
    int sound = 1;                 // ChipSound::kHitNames ("Thwack")
};

struct KillFxConfig {
    bool enabled = false;
    bool bloodBurst = true;        // фонтан капель крови с физикой, капли оставляют брызги на земле
    bool goreChunks = true;        // куски плоти кувыркаются и шлёпаются на землю
    bool bloodPool = true;         // лужа крови растекается под телом
    bool bloodMist = true;         // облако кровавой взвеси в точке смерти
    bool uvBurn = false;           // тело сгорает в UV: вспышка, угли, пепел и дым
    bool screenSplatter = false;   // кровь на "объективе", если убийство вблизи
    // радужные (любые эффекты можно включать вместе)
    bool confetti = false;         // радужное конфетти: бумажки кувыркаются и планируют вниз
    bool rainbowRing = false;      // радужная волна по земле
    bool rainbowRays = false;      // радужные лучи-вспышка из груди
    bool rainbowSparkles = false;  // мерцающие звёздочки по спирали вверх
    // кристаллы
    bool crystalShatter = false;   // тело разлетается гранёными осколками с бликами
    bool crystalSpikes = false;    // из земли вырастают кристаллы и рассыпаются
    bool rainbowCrystals = false;  // осколки и шипы переливаются всеми цветами
    float crystalColor[4] = { 0.55f, 0.90f, 1.00f, 0.92f };
    float rainbowSpeed = 1.0f;     // скорость перелива радуги
    bool fireworks = false;        // салют: ракеты взлетают и взрываются радужными искрами
    bool vortex = false;           // воронка: частицы закручиваются в тело и хлопают (UV-цвет)
    bool ghostRise = false;        // полупрозрачный призрак поднимается из тела
    bool frostNova = false;        // ледяная волна по земле + падающие снежинки (цвет кристалла)
    int confettiCount = 70;
    bool killLabel = false;        // надпись над телом
    bool streakAnnouncer = true;   // DOUBLE KILL / MASSACRE / ...
    char labelText[24] = "+1";
    float primary[4] = { 0.58f, 0.02f, 0.02f, 1.00f };    // кровь
    float secondary[4] = { 0.56f, 0.36f, 1.00f, 1.00f };  // UV
    float scale = 1.0f;
    float duration = 2.6f;         // сколько живут брызги и лужа
    int particleCount = 70;        // капли крови
    int sound = 1;                 // ChipSound::kKillNames ("Splatter")
    bool pitchStreak = true;       // каждое следующее убийство в серии звучит выше
};
