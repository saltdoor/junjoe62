#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>

#define PI 3.14159265358979323846f
#define NUM_VOICES 6
#define NUM_PARAMS 20
#define NUM_PRESETS 16

struct AEffect;
typedef intptr_t (*audioMasterCallback)(struct AEffect*, int32_t, int32_t, intptr_t, void*, float);

typedef struct AEffect {
    int32_t magic;
    intptr_t (*dispatcher)(struct AEffect*, int32_t, int32_t, intptr_t, void*, float);
    void (*process)(struct AEffect*, float**, float**, int32_t);
    void (*setParameter)(struct AEffect*, int32_t, float);
    float (*getParameter)(struct AEffect*, int32_t);
    int32_t numPrograms, numParams, numInputs, numOutputs, flags;
    intptr_t resvd1, resvd2;
    int32_t initialDelay, realQualities, offQualities;
    float ioRatio;
    void* object;
    void* user;
    int32_t uniqueID, version;
    void (*processReplacing)(struct AEffect*, float**, float**, int32_t);
    void* processDoubleReplacing;
    char future[56];
} AEffect;

typedef struct { int16_t top, left, bottom, right; } ERect;

typedef struct {
    int32_t type, byteSize, deltaFrames, flags, noteLength, noteOffset;
    char midiData[4], detune, noteOffVelocity, reserved1, reserved2;
} VstMidiEvent;

typedef struct {
    int32_t numEvents;
    intptr_t reserved;
    void* events[2];
} VstEvents;

typedef struct {
    uint8_t note;
    int active;
    int held;
    uint32_t age;
    float freq, target_freq;
    float phase;
    int sub_flip;
    float env_val;
    int env_stage;
    float f_env_val;
    float y[4];
    float cutoff_slop;
    float env_slop;
} Voice;

typedef struct {
    const char* name;
    float p[NUM_PARAMS];
} Preset;

static const char* param_names[NUM_PARAMS] = {
    "Saw Level", "Pulse Level", "Sub Level", "Noise Level",
    "Pulse Width", "PWM Depth", "LFO Rate", "Glide", "Mono Mode", "HPF / Boost",
    "Cutoff", "Resonance", "Env Mod", "Filt Decay", "Drive",
    "Attack", "Decay", "Sustain", "Release", "Chorus Mode"
};

static const Preset factory_presets[NUM_PRESETS] = {
    {"Juno Brass 62",  {0.85f,0.40f,0.35f,0.02f, 0.50f,0.25f,0.25f,0.05f,0.0f,0.25f, 0.42f,0.25f,0.55f,0.45f,0.20f, 0.08f,0.45f,0.65f,0.35f, 0.50f}},
    {"Funky Bass",     {0.90f,0.50f,0.80f,0.00f, 0.50f,0.15f,0.20f,0.08f,1.0f,0.00f, 0.22f,0.55f,0.75f,0.20f,0.45f, 0.00f,0.25f,0.15f,0.15f, 0.45f}},
    {"Wahuh Bass",     {0.80f,0.70f,0.75f,0.02f, 0.35f,0.40f,0.30f,0.12f,1.0f,0.00f, 0.18f,0.72f,0.85f,0.30f,0.50f, 0.02f,0.35f,0.20f,0.20f, 0.50f}},
    {"PW Bass",        {0.00f,1.00f,0.85f,0.00f, 0.25f,0.65f,0.35f,0.05f,1.0f,0.00f, 0.28f,0.40f,0.65f,0.22f,0.35f, 0.00f,0.30f,0.25f,0.15f, 0.45f}},
    {"LFO Bass",       {0.85f,0.60f,0.90f,0.05f, 0.50f,0.55f,0.60f,0.10f,1.0f,0.00f, 0.25f,0.65f,0.70f,0.25f,0.40f, 0.00f,0.28f,0.30f,0.20f, 0.80f}},
    {"Acid 303 Line",  {1.00f,0.00f,0.20f,0.00f, 0.50f,0.00f,0.20f,0.22f,1.0f,0.00f, 0.16f,0.88f,0.90f,0.14f,0.75f, 0.00f,0.20f,0.00f,0.10f, 0.00f}},
    {"Arp Lead",       {0.80f,0.80f,0.40f,0.03f, 0.45f,0.35f,0.30f,0.02f,0.0f,0.25f, 0.50f,0.35f,0.60f,0.25f,0.25f, 0.00f,0.25f,0.40f,0.25f, 0.80f}},
    {"Organ Lead",     {0.50f,0.90f,0.95f,0.02f, 0.50f,0.10f,0.45f,0.00f,0.0f,0.15f, 0.58f,0.60f,0.20f,0.50f,0.20f, 0.00f,0.10f,0.90f,0.10f, 0.95f}},
    {"Funky Lead",     {0.95f,0.45f,0.30f,0.00f, 0.40f,0.30f,0.35f,0.18f,1.0f,0.25f, 0.45f,0.55f,0.70f,0.35f,0.40f, 0.01f,0.40f,0.50f,0.25f, 0.50f}},
    {"Self Osc Lead",  {0.60f,0.30f,0.50f,0.05f, 0.50f,0.20f,0.25f,0.15f,1.0f,0.25f, 0.35f,0.96f,0.65f,0.40f,0.35f, 0.01f,0.45f,0.60f,0.30f, 0.80f}},
    {"E Piano",        {0.20f,0.85f,0.45f,0.01f, 0.18f,0.15f,0.25f,0.00f,0.0f,0.35f, 0.38f,0.30f,0.55f,0.32f,0.15f, 0.00f,0.45f,0.25f,0.45f, 0.50f}},
    {"Warm Pad",       {0.85f,0.75f,0.50f,0.02f, 0.50f,0.60f,0.18f,0.05f,0.0f,0.10f, 0.36f,0.25f,0.40f,0.75f,0.15f, 0.45f,0.70f,0.80f,0.65f, 0.95f}},
    {"Power Pad",      {1.00f,0.90f,0.75f,0.04f, 0.40f,0.70f,0.28f,0.08f,0.0f,0.00f, 0.48f,0.35f,0.50f,0.65f,0.35f, 0.30f,0.65f,0.85f,0.60f, 0.95f}},
    {"Synth Heaven",   {0.90f,0.80f,0.30f,0.06f, 0.50f,0.65f,0.20f,0.00f,0.0f,0.40f, 0.55f,0.40f,0.45f,0.80f,0.15f, 0.55f,0.80f,0.75f,0.75f, 0.95f}},
    {"PLugged Pluck",  {0.80f,0.60f,0.40f,0.03f, 0.30f,0.25f,0.25f,0.00f,0.0f,0.25f, 0.25f,0.45f,0.80f,0.15f,0.25f, 0.00f,0.18f,0.00f,0.22f, 0.50f}},
    {"Reso Kick",      {0.00f,0.00f,0.80f,0.10f, 0.50f,0.00f,0.10f,0.00f,1.0f,0.00f, 0.08f,0.98f,0.85f,0.12f,0.65f, 0.00f,0.18f,0.00f,0.15f, 0.00f}}
};

typedef struct {
    AEffect* effect;
    audioMasterCallback host;
    float sample_rate;
    int current_program;
    float params[NUM_PARAMS];
    char program_names[NUM_PRESETS][24];
    float preset_bank[NUM_PRESETS][NUM_PARAMS];
    Voice voices[NUM_VOICES];
    uint32_t age_counter;
    uint32_t rng_state;
    float lfo_phase;
    float hpf_z1, bass_z1;
    float chorus_buf[4410];
    int chorus_idx;
    float chorus_p1, chorus_p2;
    float chorus_lp_l, chorus_lp_r;
    float scope_buf[256];
    int scope_idx;
    HWND hwnd;
    ERect rect;
    int drag_param;
} Junjoe62Synth;

static inline float poly_blep(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return 2.0f * t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + 2.0f * t + 1.0f;
    }
    return 0.0f;
}

static inline float fast_tanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static inline float white_noise(Junjoe62Synth* s) {
    s->rng_state = s->rng_state * 214013u + 2531011u;
    return ((float)((s->rng_state >> 16) & 0x7FFF) / 16384.0f) - 1.0f;
}

static void load_program(Junjoe62Synth* s, int idx) {
    if (idx < 0 || idx >= NUM_PRESETS) return;
    s->current_program = idx;
    for (int i = 0; i < NUM_PARAMS; i++) {
        s->params[i] = s->preset_bank[idx][i];
    }
    if (s->hwnd) InvalidateRect(s->hwnd, NULL, FALSE);
}

static void note_on(Junjoe62Synth* s, uint8_t note) {
    s->age_counter++;
    int is_mono = s->params[8] >= 0.5f;
    float new_freq = 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);

    if (is_mono) {
        for (int i = 1; i < NUM_VOICES; i++) s->voices[i].active = 0;
        Voice* target = &s->voices[0];
        int legato = target->active && target->held;
        if (!target->active || s->params[7] < 0.01f) target->freq = new_freq;
        target->target_freq = new_freq;
        target->note = note;
        target->active = 1;
        target->held = 1;
        target->age = s->age_counter;
        if (!legato) {
            target->env_stage = 1;
            target->f_env_val = 1.0f;
        }
        return;
    }

    Voice* target = NULL;
    for (int i = 0; i < NUM_VOICES; i++) {
        if (!s->voices[i].active) { target = &s->voices[i]; break; }
    }
    if (!target) {
        target = &s->voices[0];
        for (int i = 1; i < NUM_VOICES; i++) {
            if (s->voices[i].age < target->age) target = &s->voices[i];
        }
    }
    if (!target->active || s->params[7] < 0.01f) target->freq = new_freq;
    target->target_freq = new_freq;
    target->note = note;
    target->active = 1;
    target->held = 1;
    target->age = s->age_counter;
    target->env_stage = 1;
    target->f_env_val = 1.0f;
}

static void note_off(Junjoe62Synth* s, uint8_t note) {
    for (int i = 0; i < NUM_VOICES; i++) {
        if (s->voices[i].active && s->voices[i].note == note) {
            s->voices[i].held = 0;
            s->voices[i].env_stage = 4;
        }
    }
}

static void process_replacing(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames) {
    (void)inputs;
    Junjoe62Synth* s = (Junjoe62Synth*)effect->user;
    float* out_l = outputs[0];
    float* out_r = outputs[1];

    float saw_lvl   = s->params[0];
    float pulse_lvl = s->params[1];
    float sub_lvl   = s->params[2];
    float noise_lvl = s->params[3] * 0.35f;
    float pw_base   = 0.1f + s->params[4] * 0.8f;
    float pwm_depth = s->params[5] * 0.38f;
    float lfo_rate  = 0.1f + s->params[6] * 14.0f;
    float glide_cf  = 1.0f - expf(-1.0f / (fmaxf(s->params[7] * 0.5f, 0.001f) * s->sample_rate));
    int is_mono     = s->params[8] >= 0.5f;
    float hpf_param = s->params[9];

    float cutoff_b  = 25.0f * powf(650.0f, s->params[10]);
    float res       = s->params[11] * 3.92f;
    float env_mod   = s->params[12] * 7500.0f;
    float f_dec_mul = expf(-1.0f / (fmaxf(s->params[13] * 1.8f, 0.012f) * s->sample_rate));
    float drive     = 1.0f + s->params[14] * 3.5f;

    float atk_inc = 1.0f / (fmaxf(s->params[15] * 2.5f, 0.0015f) * s->sample_rate);
    float dec_dec = 1.0f / (fmaxf(s->params[16] * 2.5f, 0.005f) * s->sample_rate);
    float sus_lvl = s->params[17];
    float rel_dec = 1.0f / (fmaxf(s->params[18] * 3.5f, 0.008f) * s->sample_rate);

    float c_mode = s->params[19];
    int max_v = is_mono ? 1 : NUM_VOICES;

    float bass_g = 1.0f - expf(-2.0f * PI * 110.0f / s->sample_rate);
    float hpf_hz = (hpf_param > 0.3f) ? (20.0f + (hpf_param - 0.3f) * 850.0f) : 15.0f;
    float hpf_g  = 1.0f - expf(-2.0f * PI * hpf_hz / s->sample_rate);
    float bbd_lp_g = 1.0f - expf(-2.0f * PI * 9000.0f / s->sample_rate);

    for (int i = 0; i < sampleFrames; i++) {
        s->lfo_phase = fmodf(s->lfo_phase + lfo_rate / s->sample_rate, 1.0f);
        float lfo = 1.0f - 4.0f * fabsf(s->lfo_phase - 0.5f);
        float pw = fminf(fmaxf(pw_base + lfo * pwm_depth, 0.05f), 0.95f);

        float mix = 0.0f;
        for (int v = 0; v < max_v; v++) {
            Voice* vc = &s->voices[v];
            if (!vc->active) continue;

            if (vc->env_stage == 1) {
                vc->env_val += atk_inc * vc->env_slop;
                if (vc->env_val >= 1.0f) { vc->env_val = 1.0f; vc->env_stage = 2; }
            } else if (vc->env_stage == 2) {
                vc->env_val -= dec_dec * vc->env_slop;
                if (vc->env_val <= sus_lvl) { vc->env_val = sus_lvl; vc->env_stage = 3; }
            } else if (vc->env_stage == 3) {
                vc->env_val = sus_lvl;
            } else if (vc->env_stage == 4) {
                vc->env_val -= rel_dec * vc->env_slop;
                if (vc->env_val <= 0.0f) { vc->env_val = 0.0f; vc->active = 0; continue; }
            }

            vc->f_env_val *= f_dec_mul;
            vc->freq += (vc->target_freq - vc->freq) * glide_cf;

            float dt = vc->freq / s->sample_rate;
            float saw = (2.0f * vc->phase - 1.0f) - poly_blep(vc->phase, dt);
            float pulse = (vc->phase < pw ? 1.0f : -1.0f)
                        + poly_blep(vc->phase, dt)
                        - poly_blep(fmodf(vc->phase + (1.0f - pw), 1.0f), dt);

            vc->phase += dt;
            if (vc->phase >= 1.0f) {
                vc->phase -= 1.0f;
                vc->sub_flip = !vc->sub_flip;
            }
            float sub = vc->sub_flip ? 1.0f : -1.0f;
            float nz = white_noise(s);

            float voice_sig = (saw * saw_lvl + pulse * pulse_lvl + sub * sub_lvl + nz * noise_lvl) * drive;
            float cut_hz = (cutoff_b + vc->f_env_val * env_mod) * vc->cutoff_slop;
            float f_ov = fminf(fmaxf(cut_hz / (s->sample_rate * 4.0f), 0.0005f), 0.22f);
            float g = 1.0f - expf(-2.0f * PI * f_ov);

            for (int ov = 0; ov < 4; ov++) {
                float fb = fast_tanh(voice_sig - res * vc->y[3]);
                vc->y[0] += g * (fb - vc->y[0]);
                vc->y[1] += g * (vc->y[0] - vc->y[1]);
                vc->y[2] += g * (vc->y[1] - vc->y[2]);
                vc->y[3] += g * (vc->y[2] - vc->y[3]);
            }

            mix += vc->y[3] * vc->env_val;
        }

        s->hpf_z1 += hpf_g * (mix - s->hpf_z1);
        float post_hpf = mix - s->hpf_z1;
        s->bass_z1 += bass_g * (post_hpf - s->bass_z1);
        if (hpf_param < 0.2f) {
            post_hpf += s->bass_z1 * ((0.2f - hpf_param) * 4.0f);
        }

        mix = fast_tanh(post_hpf * (is_mono ? 0.55f : 0.35f));
        s->scope_buf[s->scope_idx] = mix;
        s->scope_idx = (s->scope_idx + 1) & 255;

        s->chorus_buf[s->chorus_idx] = mix;
        s->chorus_p1 = fmodf(s->chorus_p1 + 0.51f / s->sample_rate, 1.0f);
        s->chorus_p2 = fmodf(s->chorus_p2 + 0.86f / s->sample_rate, 1.0f);

        if (c_mode >= 0.2f) {
            float m1 = sinf(s->chorus_p1 * 2.0f * PI);
            float m2 = sinf(s->chorus_p2 * 2.0f * PI);
            float mod_l = 0.0f, mod_r = 0.0f;

            if (c_mode < 0.55f) {
                mod_l = 0.0016f * m1;
                mod_r = -0.0016f * m1;
            } else if (c_mode < 0.85f) {
                mod_l = 0.0024f * m2;
                mod_r = -0.0024f * m2;
            } else {
                mod_l = 0.0014f * m1 + 0.0014f * m2;
                mod_r = -0.0014f * m1 - 0.0014f * m2;
            }

            float d_l = (0.0052f + mod_l) * s->sample_rate;
            float d_r = (0.0052f + mod_r) * s->sample_rate;

            float rpos_l = (float)s->chorus_idx - d_l + 4410.0f;
            float rpos_r = (float)s->chorus_idx - d_r + 4410.0f;
            int idx_l0 = ((int)rpos_l) % 4410, idx_l1 = (idx_l0 + 1) % 4410;
            int idx_r0 = ((int)rpos_r) % 4410, idx_r1 = (idx_r0 + 1) % 4410;
            float fl = rpos_l - floorf(rpos_l);
            float fr = rpos_r - floorf(rpos_r);

            float wet_l = s->chorus_buf[idx_l0] * (1.0f - fl) + s->chorus_buf[idx_l1] * fl;
            float wet_r = s->chorus_buf[idx_r0] * (1.0f - fr) + s->chorus_buf[idx_r1] * fr;

            s->chorus_lp_l += bbd_lp_g * (wet_l - s->chorus_lp_l);
            s->chorus_lp_r += bbd_lp_g * (wet_r - s->chorus_lp_r);

            out_l[i] = mix * 0.65f + s->chorus_lp_l * 0.55f;
            out_r[i] = mix * 0.65f + s->chorus_lp_r * 0.55f;
        } else {
            out_l[i] = mix;
            out_r[i] = mix;
        }
        s->chorus_idx = (s->chorus_idx + 1) % 4410;
    }
}

static void set_parameter(AEffect* effect, int32_t index, float value) {
    Junjoe62Synth* s = (Junjoe62Synth*)effect->user;
    if (index >= 0 && index < NUM_PARAMS) {
        float v = fminf(fmaxf(value, 0.0f), 1.0f);
        s->params[index] = v;
        s->preset_bank[s->current_program][index] = v;
        if (s->hwnd) InvalidateRect(s->hwnd, NULL, FALSE);
    }
}

static float get_parameter(AEffect* effect, int32_t index) {
    Junjoe62Synth* s = (Junjoe62Synth*)effect->user;
    return (index >= 0 && index < NUM_PARAMS) ? s->params[index] : 0.0f;
}

static void update_param_from_mouse(Junjoe62Synth* s, int x) {
    if (s->drag_param < 0 || s->drag_param >= NUM_PARAMS) return;
    int col = s->drag_param / 10;
    int bx = (col == 0) ? 115 : 375;
    float val = (float)(x - bx) / 120.0f;
    set_parameter(s->effect, s->drag_param, val);
}

static LRESULT CALLBACK vst_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Junjoe62Synth* s = (Junjoe62Synth*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if (!s) return DefWindowProcA(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_TIMER:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        case WM_LBUTTONDOWN: {
            int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
            if (y >= 10 && y <= 34) {
                if (x >= 330 && x <= 375) {
                    load_program(s, (s->current_program + NUM_PRESETS - 1) % NUM_PRESETS);
                    return 0;
                }
                if (x >= 460 && x <= 505) {
                    load_program(s, (s->current_program + 1) % NUM_PRESETS);
                    return 0;
                }
            }
            for (int i = 0; i < NUM_PARAMS; i++) {
                int col = i / 10, row = i % 10;
                int bx = (col == 0) ? 115 : 375;
                int by = 115 + row * 28;
                if (x >= bx && x <= bx + 120 && y >= by && y <= by + 18) {
                    if (i == 8) {
                        set_parameter(s->effect, 8, s->params[8] >= 0.5f ? 0.0f : 1.0f);
                    } else if (i == 19) {
                        float cm = s->params[19];
                        float next = (cm < 0.2f) ? 0.4f : (cm < 0.55f ? 0.7f : (cm < 0.85f ? 1.0f : 0.0f));
                        set_parameter(s->effect, 19, next);
                    } else {
                        s->drag_param = i;
                        SetCapture(hwnd);
                        update_param_from_mouse(s, x);
                    }
                    break;
                }
            }
            return 0;
        }
        case WM_MOUSEMOVE:
            if (s->drag_param != -1) update_param_from_mouse(s, (short)LOWORD(lParam));
            return 0;
        case WM_LBUTTONUP:
            if (s->drag_param != -1) { s->drag_param = -1; ReleaseCapture(); }
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBM);

            HBRUSH bg = CreateSolidBrush(RGB(22, 24, 30));
            FillRect(memDC, &rc, bg); DeleteObject(bg);

            SetBkMode(memDC, TRANSPARENT);
            HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SelectObject(memDC, font);

            SetTextColor(memDC, RGB(255, 120, 50));
            TextOutA(memDC, 16, 14, "JUNJOE-62 // ANALOG JUNO-60/106 SYNTH", 37);

            RECT prv = { 330, 10, 370, 30 }, nxt = { 465, 10, 505, 30 };
            HBRUSH pbr = CreateSolidBrush(RGB(45, 50, 62));
            FillRect(memDC, &prv, pbr); FillRect(memDC, &nxt, pbr); DeleteObject(pbr);
            SetTextColor(memDC, RGB(230, 235, 240));
            TextOutA(memDC, 340, 13, "< PRV", 5);
            TextOutA(memDC, 376, 13, s->program_names[s->current_program], (int)strlen(s->program_names[s->current_program]));
            TextOutA(memDC, 472, 13, "NXT >", 5);

            RECT scope_rc = { 15, 36, 505, 102 };
            HBRUSH sbg = CreateSolidBrush(RGB(10, 12, 16));
            FillRect(memDC, &scope_rc, sbg); DeleteObject(sbg);

            HPEN pen = CreatePen(PS_SOLID, 2, RGB(56, 189, 248));
            SelectObject(memDC, pen);
            for (int i = 0; i < 255; i++) {
                int idx = (s->scope_idx + i) & 255;
                int px = 15 + (i * 490) / 255;
                int py = 69 - (int)(s->scope_buf[idx] * 28.0f);
                if (py < 37) py = 37;
                if (py > 101) py = 101;
                if (i == 0) MoveToEx(memDC, px, py, NULL);
                else LineTo(memDC, px, py);
            }
            DeleteObject(pen);

            for (int i = 0; i < NUM_PARAMS; i++) {
                int col = i / 10, row = i % 10;
                int lx = (col == 0) ? 20 : 280;
                int bx = (col == 0) ? 115 : 375;
                int by = 115 + row * 28;

                SetTextColor(memDC, RGB(210, 215, 225));
                TextOutA(memDC, lx, by + 2, param_names[i], (int)strlen(param_names[i]));

                RECT trk = { bx, by, bx + 120, by + 16 };
                HBRUSH tbr = CreateSolidBrush(RGB(42, 46, 56));
                FillRect(memDC, &trk, tbr); DeleteObject(tbr);

                if (i == 8) {
                    int is_m = s->params[8] >= 0.5f;
                    RECT btn = { bx + (is_m ? 60 : 0), by, bx + (is_m ? 120 : 60), by + 16 };
                    HBRUSH bbr = CreateSolidBrush(is_m ? RGB(255, 110, 40) : RGB(56, 189, 248));
                    FillRect(memDC, &btn, bbr); DeleteObject(bbr);
                    const char* lbl = is_m ? "MONO (Acid)" : "POLY (6-Vc)";
                    SetTextColor(memDC, RGB(255, 255, 255));
                    TextOutA(memDC, bx + 24, by + 1, lbl, (int)strlen(lbl));
                } else if (i == 19) {
                    float cm = s->params[19];
                    const char* clbl = (cm < 0.2f) ? "OFF" : (cm < 0.55f ? "CHORUS I" : (cm < 0.85f ? "CHORUS II" : "CHORUS I+II"));
                    HBRUSH cbr = CreateSolidBrush(cm < 0.2f ? RGB(65, 70, 82) : RGB(230, 90, 40));
                    FillRect(memDC, &trk, cbr); DeleteObject(cbr);
                    SetTextColor(memDC, RGB(255, 255, 255));
                    TextOutA(memDC, bx + 26, by + 1, clbl, (int)strlen(clbl));
                } else {
                    RECT fill = { bx, by, bx + (int)(s->params[i] * 120.0f), by + 16 };
                    HBRUSH fbr = CreateSolidBrush(col == 0 ? RGB(56, 189, 248) : RGB(255, 110, 40));
                    FillRect(memDC, &fill, fbr); DeleteObject(fbr);
                }
            }

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBM); DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static intptr_t dispatcher(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
    Junjoe62Synth* s = (Junjoe62Synth*)effect->user;
    switch (opcode) {
        case 1:
            if (s) {
                if (s->hwnd) { KillTimer(s->hwnd, 1); DestroyWindow(s->hwnd); }
                free(s);
            }
            free(effect);
            return 0;
        case 2:
            load_program(s, (int)value);
            return 0;
        case 3:
            return s->current_program;
        case 4:
            if (ptr) strncpy(s->program_names[s->current_program], (const char*)ptr, 23);
            return 0;
        case 5:
            if (ptr) strcpy((char*)ptr, s->program_names[s->current_program]);
            return 0;
        case 6:
            if (index == 10) strcpy((char*)ptr, "Hz");
            else if (index == 8 || index == 19) strcpy((char*)ptr, "");
            else strcpy((char*)ptr, "%");
            return 0;
        case 7:
            if (index == 8) strcpy((char*)ptr, s->params[8] >= 0.5f ? "Mono" : "Poly");
            else if (index == 9) {
                if (s->params[9] < 0.2f) strcpy((char*)ptr, "Bass+3dB");
                else sprintf((char*)ptr, "%.0f%%", s->params[9] * 100.0f);
            }
            else if (index == 10) sprintf((char*)ptr, "%.0f", 25.0f * powf(650.0f, s->params[10]));
            else if (index == 19) {
                float cm = s->params[19];
                strcpy((char*)ptr, cm < 0.2f ? "Off" : (cm < 0.55f ? "Mode I" : (cm < 0.85f ? "Mode II" : "Mode I+II")));
            }
            else sprintf((char*)ptr, "%.0f", s->params[index] * 100.0f);
            return 0;
        case 8:
            if (index >= 0 && index < NUM_PARAMS) strcpy((char*)ptr, param_names[index]);
            return 0;
        case 10:
            s->sample_rate = opt > 8000.0f ? opt : 44100.0f;
            return 0;
        case 13:
            *(ERect**)ptr = &s->rect;
            return 1;
        case 14: {
            WNDCLASSA wc = {0};
            wc.lpfnWndProc = vst_wnd_proc;
            wc.hInstance = GetModuleHandleA(NULL);
            wc.lpszClassName = "Junjoe62VSTGui";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            RegisterClassA(&wc);
            s->hwnd = CreateWindowExA(0, "Junjoe62VSTGui", "Junjoe62", WS_CHILD | WS_VISIBLE,
                                      0, 0, 520, 405, (HWND)ptr, NULL, wc.hInstance, NULL);
            SetWindowLongPtrA(s->hwnd, GWLP_USERDATA, (LONG_PTR)s);
            SetTimer(s->hwnd, 1, 30, NULL);
            return 1;
        }
        case 15:
            if (s->hwnd) { KillTimer(s->hwnd, 1); DestroyWindow(s->hwnd); s->hwnd = NULL; }
            return 0;
        case 25: {
            VstEvents* evs = (VstEvents*)ptr;
            for (int i = 0; i < evs->numEvents; i++) {
                VstMidiEvent* m = (VstMidiEvent*)evs->events[i];
                if (m->type == 1) {
                    uint8_t status = m->midiData[0] & 0xF0;
                    uint8_t note = m->midiData[1];
                    uint8_t vel = m->midiData[2];
                    if (status == 0x90 && vel > 0) note_on(s, note);
                    else if (status == 0x80 || (status == 0x90 && vel == 0)) note_off(s, note);
                }
            }
            return 1;
        }
        case 29:
            if (index >= 0 && index < NUM_PRESETS && ptr) {
                strcpy((char*)ptr, s->program_names[index]);
                return 1;
            }
            return 0;
        case 35: return 8;
        case 45: strcpy((char*)ptr, "Junjoe 62"); return 1;
        case 47: strcpy((char*)ptr, "OpenAudio"); return 1;
        case 48: strcpy((char*)ptr, "Junjoe 62 Juno-60/106"); return 1;
        default: return 0;
    }
}

__declspec(dllexport) AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
    AEffect* effect = (AEffect*)calloc(1, sizeof(AEffect));
    Junjoe62Synth* synth = (Junjoe62Synth*)calloc(1, sizeof(Junjoe62Synth));

    synth->effect = effect;
    synth->host = audioMaster;
    synth->sample_rate = 44100.0f;
    synth->rng_state = 123456789u;
    synth->drag_param = -1;
    synth->rect.top = 0; synth->rect.left = 0;
    synth->rect.bottom = 405; synth->rect.right = 520;

    static const float c_slop[6] = { 0.985f, 1.012f, 0.992f, 1.018f, 0.989f, 1.006f };
    static const float e_slop[6] = { 1.020f, 0.980f, 1.010f, 0.990f, 1.015f, 0.985f };
    for (int v = 0; v < NUM_VOICES; v++) {
        synth->voices[v].cutoff_slop = c_slop[v];
        synth->voices[v].env_slop = e_slop[v];
    }

    for (int p = 0; p < NUM_PRESETS; p++) {
        strncpy(synth->program_names[p], factory_presets[p].name, 23);
        for (int i = 0; i < NUM_PARAMS; i++) {
            synth->preset_bank[p][i] = factory_presets[p].p[i];
        }
    }
    load_program(synth, 0);

    effect->magic = 0x56737450;
    effect->dispatcher = dispatcher;
    effect->setParameter = set_parameter;
    effect->getParameter = get_parameter;
    effect->processReplacing = process_replacing;
    effect->numPrograms = NUM_PRESETS;
    effect->numParams = NUM_PARAMS;
    effect->numInputs = 0;
    effect->numOutputs = 2;
    effect->flags = (1 << 0) | (1 << 4) | (1 << 8);
    effect->uniqueID = 106162;
    effect->version = 1800;
    effect->user = synth;
    return effect;
}
