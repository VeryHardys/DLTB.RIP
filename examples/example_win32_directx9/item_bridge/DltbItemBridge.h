#pragma once
// ============================================================================
//  DLTB <-> Item Extravaganza bridge: общая память между трейнером (внешний
//  процесс, меню ImGui) и рантаймом Item Extravaganza внутри игры.
//
//  Рантайм остаётся движком: все нативные действия идут через его же запросы,
//  проверки и диспетчеризацию в игровом потоке. Мост только публикует данные
//  (каталог, снимок предметов, статус) и принимает команды вместо кнопок его окна.
//
//  Синхронизация:
//   - команды: трейнер пишет cmd, затем cmdSeq++; рантайм выполняет и ставит ackSeq = cmdSeq;
//   - данные: seqlock — writeSeq нечётный во время записи; читатель копирует,
//     пока writeSeq чётный и не изменился. Тяжёлые блоки (каталог, предметы)
//     перезаписываются только при смене catalogVersion / ownedVersion.
// ============================================================================
#include <stdint.h>

#define DLTB_BRIDGE_MAGIC   0x42544C44u   // 'DLTB'
#define DLTB_BRIDGE_VERSION 1u
#define DLTB_BRIDGE_NAME_FMT L"Local\\DLTB_ItemBridge_%lu"   // %lu = PID игры

#define DLTB_MAX_CATALOG    8192
#define DLTB_MAX_OWNED      512
#define DLTB_MAX_AFFIXES    32
#define DLTB_MAX_AFFIX_META 256
#define DLTB_MAX_BUILTIN    512
#define DLTB_LOG_LINES      48

enum DltbBridgeCommandType : uint32_t {
    DLTB_CMD_NONE = 0,
    DLTB_CMD_ITEM_ACTION = 1,      // action: 1 inventory, 2 stash, 3 remove
    DLTB_CMD_OWNED_SCAN = 2,
    DLTB_CMD_OWNED_EDIT = 3,       // editKind = OwnedEditKind рантайма
    DLTB_CMD_TOGGLE_FAVORITE = 4,
    DLTB_CMD_RELOAD_CATALOG = 5,
};

#pragma pack(push, 8)
struct DltbCatalogItem {
    char name[160];
    char id[160];
    char category[64];
    char rarity[32];
    char itemType[64];
    char source[160];
    uint32_t canDrop;      // 0 не указано, 1 да, 2 нет
    uint32_t weaponLevel;  // 0 — без ранга, иначе R1..R15
    uint32_t favorite;
    uint32_t recentOrder;
};

struct DltbAffix {
    char id[128];
    float raw, factor;
    uint32_t statChange;   // 0 — тип значения неизвестен (можно только удалить)
    wchar_t shown[48];     // значение по шаблону игры ("+12%")
};

struct DltbOwnedItem {
    uint64_t instance;
    uint32_t isGear;
    uint32_t wornSlot;     // слот + 1; 0 — не надето/неизвестно
    char id[160];
    char name[160];
    char category[64];
    char rarity[32];
    uint32_t weaponLevel;
    uint32_t sameIdOrdinal;
    uint32_t affixCount;
    uint32_t pad;
    DltbAffix affixes[DLTB_MAX_AFFIXES];
};

struct DltbAffixMeta {
    char id[128];
    char name[160];
    char valueFormat[96];
    uint32_t gameLocalized;
    uint32_t pad;
};

struct DltbBuiltInStat {
    char itemId[160];
    char name[160];
    char authoredValue[32];
    uint32_t scalable;
    uint32_t pad;
};

struct DltbCommand {
    uint32_t type;
    int32_t action;
    int32_t quantity;
    int32_t editKind;
    uint64_t target;           // экземпляр предмета для OWNED_EDIT
    uint32_t affixIndex;
    float value;               // новое эффективное значение
    uint64_t donorInstance;    // Copy Owned: откуда копировать
    uint32_t donorAffixIndex;
    uint32_t confirmExtremes;  // пользователь подтвердил значения > 1000
    char itemId[160];
    char itemName[160];
    char category[64];
    char affixId[128];
};

struct DltbBridgeShared {
    uint32_t magic, version;
    uint32_t runtimePid, pad0;
    volatile long heartbeat;   // рантайм увеличивает каждые ~100 мс

    // --- команды (пишет трейнер)
    volatile long cmdSeq;
    volatile long ackSeq;
    long pad1;
    DltbCommand cmd;

    // --- состояние (пишет рантайм, seqlock)
    volatile long writeSeq;
    int32_t ready;             // нативные вызовы прошли проверку сборки игры
    int32_t gameModuleLoaded;
    int32_t actionPending;
    int32_t editPending;
    int32_t transaction;       // открыта сессия правки (Keep / Restore)
    int32_t editFaulted;
    int32_t duplicateFaulted;
    int32_t lookupProbePassed; // можно добавлять аффиксы из каталога
    int32_t extremesConfirmed;
    uint64_t transactionItem;
    char transactionItemId[160];
    wchar_t status[320];
    volatile long logCount;    // всего строк; кольцо по DLTB_LOG_LINES
    wchar_t log[DLTB_LOG_LINES][200];

    volatile long catalogVersion;
    uint32_t catalogCount, affixMetaCount, builtInCount, pad2;
    volatile long ownedVersion;
    uint32_t ownedCount, ownedScanned;
    DltbOwnedItem owned[DLTB_MAX_OWNED];
    DltbAffixMeta affixMeta[DLTB_MAX_AFFIX_META];
    DltbBuiltInStat builtIn[DLTB_MAX_BUILTIN];
    DltbCatalogItem catalog[DLTB_MAX_CATALOG];
};
#pragma pack(pop)
