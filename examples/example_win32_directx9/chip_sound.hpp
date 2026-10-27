#pragma once
// ============================================================================
//  ChipSound — синтезатор звуков попаданий/убийств.
//  Всё генерируется кодом (никаких .wav файлов): синус/треугольник/пульс с
//  экспоненциальным сдвигом высоты, белый шум через однополюсный ФНЧ/ФВЧ,
//  огибающая "атака + экспоненциальное затухание", мягкий лимитер и короткая
//  "комнатная" реверберация для убийств. 16 бит, 44.1 кГц.
//  Воспроизведение: пул из нескольких waveOut-голосов, поэтому звук убийства
//  не обрывает звук попадания и наоборот. Всё это крутится в фоновом потоке,
//  чтобы не тормозить кадр оверлея.
// ============================================================================
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <cmath>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

namespace ChipSound {
    static const int kSampleRate = 44100;

    enum Wave { W_SINE, W_TRI, W_PULSE, W_NOISE_LP, W_NOISE_HP };

    // Один слой звука. Высота скользит экспоненциально f0 -> f1 (естественный "удар"),
    // для шума f0/f1 — частота среза фильтра.
    struct Note {
        Wave wave;
        float f0, f1;            // Гц
        float duration;          // секунды
        float volume;            // 0..1
        float attack = 0.002f;   // секунды нарастания
        float decay = 4.0f;      // крутизна экспоненциального затухания (больше — короче хвост)
        float startAt = 0.0f;    // смещение начала (наслаивание слоёв)
    };

    struct Preset {
        std::vector<Note> notes;
        float reverb = 0.0f;     // 0..1 — уровень "комнаты"
    };

    // --- генерация --------------------------------------------------------
    static void RenderPreset(const Preset& pr, std::vector<int16_t>& out, float pitchMul = 1.0f) {
        float total = 0.0f;
        for (const auto& n : pr.notes) total = (std::max)(total, n.startAt + n.duration);
        const float tail = pr.reverb > 0.0f ? 0.35f : 0.0f;
        size_t len = (size_t)((total + tail) * kSampleRate) + 64;
        std::vector<float> mix(len, 0.0f);

        uint32_t rng = 0x1234567u;
        for (const auto& n : pr.notes) {
            size_t start = (size_t)(n.startAt * kSampleRate);
            size_t count = (size_t)(n.duration * kSampleRate);
            double phase = 0.0;
            float lp = 0.0f;
            const float ratio = (n.f0 > 0.0f && n.f1 > 0.0f) ? n.f1 / n.f0 : 1.0f;
            for (size_t i = 0; i < count && start + i < len; ++i) {
                float t = (float)i / (float)count;
                float sec = (float)i / kSampleRate;
                float f = n.f0 * std::pow(ratio, t) * pitchMul;
                float env = (sec < n.attack) ? sec / n.attack : std::exp(-n.decay * (sec - n.attack) / n.duration);
                if (count - i < 64) env *= (float)(count - i) / 64.0f;   // анти-щелчок в конце

                float s = 0.0f;
                if (n.wave == W_NOISE_LP || n.wave == W_NOISE_HP) {
                    rng = rng * 1664525u + 1013904223u;
                    float white = (float)(int32_t)rng / 2147483648.0f;
                    float a = 1.0f - std::exp(-6.2831853f * f / kSampleRate);  // однополюсный фильтр
                    lp += a * (white - lp);
                    s = (n.wave == W_NOISE_LP) ? lp * 1.6f : (white - lp);
                }
                else {
                    phase += (double)f / kSampleRate;
                    phase -= std::floor(phase);
                    float p = (float)phase;
                    switch (n.wave) {
                    case W_SINE:  s = std::sin(6.2831853f * p); break;
                    case W_TRI:   s = (p < 0.5f) ? (p * 4.0f - 1.0f) : (3.0f - p * 4.0f); break;
                    case W_PULSE: s = (p < 0.3f) ? 0.7f : -0.7f; break;
                    default: break;
                    }
                }
                mix[start + i] += s * env * n.volume * 0.5f;
            }
        }

        // короткая "комната": три гребенчатых задержки с затуханием
        if (pr.reverb > 0.0f) {
            const size_t taps[3] = { (size_t)(0.029f * kSampleRate), (size_t)(0.037f * kSampleRate), (size_t)(0.043f * kSampleRate) };
            std::vector<float> wet(len, 0.0f);
            for (size_t k = 0; k < 3; ++k)
                for (size_t i = taps[k]; i < len; ++i) wet[i] += (mix[i - taps[k]] + wet[i - taps[k]] * 0.55f) * 0.33f;
            for (size_t i = 0; i < len; ++i) mix[i] += wet[i] * pr.reverb;
        }

        // нормализация + мягкий лимитер: все пресеты звучат одинаково громко
        float peak = 0.0f;
        for (float v : mix) peak = (std::max)(peak, std::fabs(v));
        float gain = (peak > 1e-4f) ? (std::min)(0.9f / peak, 6.0f) : 1.0f;
        out.resize(len);
        for (size_t i = 0; i < len; ++i) out[i] = (int16_t)(std::tanh(mix[i] * gain * 1.2f) * 30000.0f);
    }

    // --- пресеты (все звуки оригинальные) ----------------------------------
    static const char* kHitNames[] = { "Off", "Thwack", "Bone Tick", "Blade Slice", "Crystal Tink", "Soft Pop", "Heavy Thump", "Clean Tick" };
    static const char* kKillNames[] = { "Off", "Splatter", "Bone Crunch", "Crystal Shatter", "Party Pop", "Deep Boom", "Bell Toll", "Kill Confirm" };
    static const int kHitCount = 8;
    static const int kKillCount = 8;

    static Preset HitPreset(int id) {
        Preset p;
        switch (id) {
        case 1: // глухой мясистый удар
            p.notes = { { W_NOISE_LP, 2200, 500, 0.07f, 0.9f, 0.001f, 5.0f }, { W_SINE, 190, 70, 0.09f, 1.0f, 0.001f, 4.0f } };
            break;
        case 2: // сухой щелчок кости
            p.notes = { { W_NOISE_HP, 4000, 4000, 0.018f, 1.0f, 0.0005f, 6.0f }, { W_SINE, 1250, 900, 0.035f, 0.6f, 0.001f, 5.0f }, { W_TRI, 320, 200, 0.05f, 0.5f, 0.001f, 4.0f } };
            break;
        case 3: // резкий взмах-разрез
            p.notes = { { W_NOISE_HP, 7000, 2500, 0.10f, 0.9f, 0.012f, 4.0f }, { W_SINE, 2600, 1800, 0.03f, 0.35f, 0.001f, 5.0f, 0.02f } };
            break;
        case 4: // звон кристалла: основной тон + квинта + октава
            p.notes = { { W_SINE, 2637, 2637, 0.22f, 0.8f, 0.001f, 5.0f }, { W_SINE, 3951, 3951, 0.15f, 0.45f, 0.001f, 6.0f }, { W_SINE, 5274, 5274, 0.10f, 0.3f, 0.001f, 6.0f } };
            p.reverb = 0.25f;
            break;
        case 5: // мягкий "поп"
            p.notes = { { W_SINE, 650, 180, 0.06f, 1.0f, 0.001f, 4.0f }, { W_NOISE_LP, 3000, 1500, 0.02f, 0.4f, 0.0005f, 5.0f } };
            break;
        case 6: // тяжёлый удар по корпусу
            p.notes = { { W_SINE, 120, 45, 0.18f, 1.0f, 0.001f, 3.5f }, { W_NOISE_LP, 900, 200, 0.09f, 0.7f, 0.001f, 4.0f } };
            break;
        case 7: // чистый тик подтверждения попадания
            p.notes = { { W_PULSE, 1900, 1900, 0.022f, 0.5f, 0.0005f, 4.0f }, { W_SINE, 3800, 3800, 0.03f, 0.6f, 0.0005f, 5.0f } };
            break;
        default: break;
        }
        return p;
    }

    static Preset KillPreset(int id) {
        Preset p;
        switch (id) {
        case 1: // сочный всплеск + низкий удар + капли
            p.notes = { { W_NOISE_LP, 3800, 350, 0.42f, 1.0f, 0.002f, 4.5f }, { W_SINE, 150, 40, 0.28f, 1.0f, 0.001f, 3.5f },
                        { W_SINE, 950, 520, 0.05f, 0.35f, 0.001f, 4.0f, 0.24f }, { W_SINE, 820, 450, 0.05f, 0.3f, 0.001f, 4.0f, 0.35f } };
            p.reverb = 0.2f;
            break;
        case 2: // хруст костей: серия щелчков + провал
            for (int i = 0; i < 4; ++i) p.notes.push_back({ W_NOISE_HP, 5000.0f - i * 600.0f, 2500.0f, 0.03f, 1.0f - i * 0.15f, 0.0005f, 5.0f, 0.04f * i });
            p.notes.push_back({ W_SINE, 170, 50, 0.28f, 0.9f, 0.001f, 3.5f });
            p.reverb = 0.15f;
            break;
        case 3: // кристалл разлетается: яркий шум + россыпь звонов
        {
            p.notes.push_back({ W_NOISE_HP, 6000, 3000, 0.25f, 0.6f, 0.001f, 4.0f });
            const float f[] = { 3136.0f, 4186.0f, 2637.0f, 5274.0f, 3520.0f, 4699.0f };
            for (int i = 0; i < 6; ++i) p.notes.push_back({ W_SINE, f[i], f[i], 0.32f, 0.5f, 0.001f, 6.0f, 0.03f * i });
            p.reverb = 0.45f;
            break;
        }
        case 4: // хлопушка + восходящие искры
        {
            p.notes.push_back({ W_NOISE_LP, 5000, 1200, 0.06f, 1.0f, 0.0005f, 4.0f });
            p.notes.push_back({ W_SINE, 420, 130, 0.07f, 0.8f, 0.001f, 4.0f });
            const float f[] = { 1568.0f, 2093.0f, 2637.0f, 3136.0f };
            for (int i = 0; i < 4; ++i) p.notes.push_back({ W_SINE, f[i], f[i], 0.10f, 0.45f, 0.002f, 4.0f, 0.06f + 0.05f * i });
            p.reverb = 0.35f;
            break;
        }
        case 5: // глубокий "бум"
            p.notes = { { W_SINE, 75, 30, 0.65f, 1.0f, 0.002f, 3.0f }, { W_NOISE_LP, 1200, 90, 0.5f, 0.8f, 0.002f, 3.5f } };
            p.reverb = 0.4f;
            break;
        case 6: // колокол: негармонические обертоны
            p.notes = { { W_SINE, 660, 660, 0.9f, 0.8f, 0.002f, 4.0f }, { W_SINE, 660 * 2.76f, 660 * 2.76f, 0.5f, 0.35f, 0.002f, 5.0f },
                        { W_SINE, 660 * 5.40f, 660 * 5.40f, 0.3f, 0.2f, 0.002f, 6.0f }, { W_NOISE_HP, 6000, 6000, 0.01f, 0.4f, 0.0005f, 5.0f } };
            p.reverb = 0.5f;
            break;
        case 7: // два чистых тона "убийство засчитано"
            p.notes = { { W_SINE, 1320, 1320, 0.08f, 0.8f, 0.001f, 3.0f }, { W_SINE, 1760, 1760, 0.14f, 0.8f, 0.001f, 4.0f, 0.07f },
                        { W_SINE, 3520, 3520, 0.10f, 0.2f, 0.001f, 5.0f, 0.07f } };
            p.reverb = 0.25f;
            break;
        default: break;
        }
        return p;
    }

    // --- вывод ---------------------------------------------------------------
    struct Voice {
        HWAVEOUT h = nullptr;
        WAVEHDR hdr{};
        bool prepared = false;
        std::vector<int16_t> buf;
    };

    static const int kVoices = 6;
    static Voice g_Voices[kVoices];
    static int g_NextVoice = 0;
    static bool g_Inited = false;
    static bool g_InitFailed = false;

    static std::vector<int16_t> g_HitCache[kHitCount];
    static bool g_HitCached[kHitCount] = {};

    static void Init() {
        if (g_Inited || g_InitFailed) return;
        WAVEFORMATEX wf{};
        wf.wFormatTag = WAVE_FORMAT_PCM;
        wf.nChannels = 1;
        wf.nSamplesPerSec = kSampleRate;
        wf.wBitsPerSample = 16;
        wf.nBlockAlign = 2;
        wf.nAvgBytesPerSec = kSampleRate * 2;
        int ok = 0;
        for (auto& v : g_Voices) {
            if (waveOutOpen(&v.h, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) ++ok;
            else v.h = nullptr;
        }
        g_Inited = ok > 0;
        g_InitFailed = !g_Inited;
    }

    static void CloseVoices() {
        for (auto& v : g_Voices) {
            if (!v.h) continue;
            waveOutReset(v.h);
            if (v.prepared) waveOutUnprepareHeader(v.h, &v.hdr, sizeof(WAVEHDR));
            waveOutClose(v.h);
            v.h = nullptr; v.prepared = false;
        }
        g_Inited = false;
    }

    static void PlayBuffer(const std::vector<int16_t>& src, float volume) {
        Init();
        if (!g_Inited || src.empty() || volume <= 0.001f) return;
        if (volume > 1.0f) volume = 1.0f;

        Voice* v = nullptr;
        for (int tries = 0; tries < kVoices; ++tries) {
            Voice& cand = g_Voices[g_NextVoice];
            g_NextVoice = (g_NextVoice + 1) % kVoices;
            if (cand.h) { v = &cand; break; }
        }
        if (!v) return;

        waveOutReset(v->h); // если голос ещё играет — обрываем самый старый
        if (v->prepared) { waveOutUnprepareHeader(v->h, &v->hdr, sizeof(WAVEHDR)); v->prepared = false; }

        v->buf.resize(src.size());
        for (size_t i = 0; i < src.size(); ++i) v->buf[i] = (int16_t)(src[i] * volume);

        v->hdr = WAVEHDR{};
        v->hdr.lpData = (LPSTR)v->buf.data();
        v->hdr.dwBufferLength = (DWORD)(v->buf.size() * sizeof(int16_t));
        if (waveOutPrepareHeader(v->h, &v->hdr, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
            v->prepared = true;
            waveOutWrite(v->h, &v->hdr, sizeof(WAVEHDR));
        }
    }

    // Кэш звуков убийства: шаг высоты в серии кратен 1.5 полутона (0..7 ступеней)
    static const int kPitchSteps = 8;
    static std::vector<int16_t> g_KillCache[kKillCount][kPitchSteps];
    static bool g_KillCached[kKillCount][kPitchSteps] = {};

    static void PlayHitNow(int id, float volume) {
        if (!g_HitCached[id]) { RenderPreset(HitPreset(id), g_HitCache[id]); g_HitCached[id] = true; }
        PlayBuffer(g_HitCache[id], volume);
    }

    static void PlayKillNow(int id, float volume, float pitchSemitones) {
        float stepF = pitchSemitones / 1.5f;
        int step = (int)std::floor(stepF + 0.5f);
        if (std::fabs(stepF - step) < 0.01f && step >= 0 && step < kPitchSteps) {
            if (!g_KillCached[id][step]) { RenderPreset(KillPreset(id), g_KillCache[id][step], std::pow(2.0f, pitchSemitones / 12.0f)); g_KillCached[id][step] = true; }
            PlayBuffer(g_KillCache[id][step], volume);
            return;
        }
        std::vector<int16_t> buf;
        RenderPreset(KillPreset(id), buf, std::pow(2.0f, pitchSemitones / 12.0f));
        PlayBuffer(buf, volume);
    }

    // --- фоновый поток ------------------------------------------------------
    // waveOutReset/PrepareHeader и синтез могут занимать несколько мс — на потоке
    // рендера это давало просадку кадра на каждом убийстве. Теперь кадр только
    // кладёт запрос в очередь, а голоса живут целиком в этом потоке.
    struct Request { bool kill; int id; float volume, pitch; };

    struct Worker {
        std::thread thread;
        std::mutex mtx;
        std::condition_variable cv;
        std::deque<Request> queue;
        bool stop = false;

        void Loop() {
            for (;;) {
                Request r;
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    cv.wait(lock, [this] { return stop || !queue.empty(); });
                    if (stop) break;
                    r = queue.front(); queue.pop_front();
                }
                if (r.kill) PlayKillNow(r.id, r.volume, r.pitch);
                else PlayHitNow(r.id, r.volume);
            }
            CloseVoices();
        }

        void Push(const Request& r) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (stop) return;
                if (!thread.joinable()) thread = std::thread([this] { Loop(); });
                if (queue.size() >= 8) queue.pop_front();   // не копим очередь при дробовике/орде
                queue.push_back(r);
            }
            cv.notify_one();
        }

        void Stop() {
            {
                std::lock_guard<std::mutex> lock(mtx);
                stop = true;
            }
            cv.notify_one();
            if (thread.joinable()) thread.join();
        }

        ~Worker() { Stop(); }
    };
    static Worker g_Worker;

    static void Shutdown() { g_Worker.Stop(); }

    static void PlayHit(int id, float volume) {
        if (id <= 0 || id >= kHitCount || volume <= 0.001f) return;
        g_Worker.Push({ false, id, volume, 0.0f });
    }

    // pitchSemitones: для серии убийств каждое следующее звучит чуть выше
    static void PlayKill(int id, float volume, float pitchSemitones = 0.0f) {
        if (id <= 0 || id >= kKillCount || volume <= 0.001f) return;
        g_Worker.Push({ true, id, volume, pitchSemitones });
    }
}
