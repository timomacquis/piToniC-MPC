// πToniC 1.1.0 — pont C vers les effets NEW, repris tels quels de πMorpH 0.2.0 (src/fx_tidal.h, src/fx_dub.h, src/fx/).
// Reverb : Tidal Verb (Fullban, MIT) à LEVEL 100 % (sortie = effet seul) ; TAILS ON, SWIRL 60, SWIRL RATE 40, AIR 45
// comme dans πMorpH. Delay : ligne A de Dub Station (nachtaktiv303, MIT) à LEVEL 100 %, échos seuls (sortie - entrée).
// Chaque effet ne calcule que s'il reçoit du son, puis jusqu'à 2 s de silence en entrée et en sortie.
#include <new>
#include <cmath>
#include "fx_dub.h"
#include "fx_tidal.h"
#include "fx_new.h"

namespace {
constexpr float kSilence = 3e-5f;   // un quart du plus petit pas int16 en sortie (gain final 0,25)
constexpr long kQuiet = 88200;   // 2 s à 44,1 kHz
inline float divTime(int d) { return 10.f + ((float)d + .5f) * 1990.f / 7.f; }   // milieu de la plage de chaque division (divIndexFor)
}

struct FxNew {
    dubfx::DubDelay delay;
    tidalfx::TidalVerb reverb;
    float p[FXN_COUNT] = {1, 55, 60, 35, 1, 3, 375, 45, 0};
    bool rvOn = false, dlOn = false;
    long rvQuiet = 0, dlQuiet = 0;
    void apply() {
        reverb.set(tidalfx::Type, p[FXN_RTYPE]); reverb.set(tidalfx::Decay, p[FXN_RDECAY]);
        reverb.set(tidalfx::Liveliness, p[FXN_RLIVE]); reverb.set(tidalfx::Warmth, p[FXN_RWARMTH]);
        delay.sync = p[FXN_DSYNC] > .5f;
        delay.time = delay.sync ? divTime((int)std::lround(p[FXN_DDIV])) : p[FXN_DMS];
        delay.feedback = p[FXN_DFEEDBACK]; delay.filter = p[FXN_DFILTER]; delay.level = 100.f;
        delay.update();
    }
};

extern "C" {
FxNew *fxn_create(void) {
    FxNew *f = new (std::nothrow) FxNew();
    if (!f) return nullptr;
    f->delay.level = 100.f;
    f->delay.prepare(44100.0);
    f->reverb.setSampleRate(44100.0);
    f->reverb.set(tidalfx::Tails, 1); f->reverb.set(tidalfx::Swirl, 60); f->reverb.set(tidalfx::SwirlRate, 40);
    f->reverb.set(tidalfx::Air, 45); f->reverb.set(tidalfx::Level, 100);
    f->apply();
    f->reverb.prepare(44100.0, 128); f->reverb.reset();
    return f;
}
void fxn_destroy(FxNew *f) { delete f; }
void fxn_set(FxNew *f, int id, float value) { if (!f || id < 0 || id >= FXN_COUNT) return; f->p[id] = value; f->apply(); }
void fxn_bpm(FxNew *f, float bpm) { if (!f) return; f->delay.bpm = bpm; f->delay.update(); }
void fxn_reset(FxNew *f) {
    if (!f) return;
    f->reverb.reset(); f->delay.prepare(44100.0); f->apply();
    f->rvOn = f->dlOn = false; f->rvQuiet = f->dlQuiet = 0;
}
int fxn_busy(const FxNew *f) { return f && (f->rvOn || f->dlOn); }
void fxn_process(FxNew *f, const float *rev, const float *del, float *wet_rev, float *wet_del, int n) {
    for (int i = 0; i < 2 * n; i++) wet_rev[i] = wet_del[i] = 0;
    if (!f || n <= 0 || n > 128) return;
    float inR = 0, inD = 0;
    for (int i = 0; i < 2 * n; i++) { inR = std::fmax(inR, std::fabs(rev[i])); inD = std::fmax(inD, std::fabs(del[i])); }
    float l[128], r[128];
    // Reverb (Tidal Verb, 100 % effet).
    if (inR > kSilence && !f->rvOn) { f->reverb.reset(); f->rvOn = true; f->rvQuiet = 0; }
    if (f->rvOn) {
        for (int i = 0; i < n; i++) { l[i] = rev[2 * i]; r[i] = rev[2 * i + 1]; }
        const float *in[2] = {l, r}; float *out[2] = {l, r};
        mpc::ProcessContext ctx; ctx.inputs = in; ctx.outputs = out; ctx.numFrames = n; ctx.sampleRate = 44100.0;
        f->reverb.process(ctx);
        float peak = 0;
        for (int i = 0; i < n; i++) { wet_rev[2 * i] = l[i]; wet_rev[2 * i + 1] = r[i]; peak = std::fmax(peak, std::fmax(std::fabs(l[i]), std::fabs(r[i]))); }
        if (inR > kSilence || peak > kSilence) f->rvQuiet = 0; else if ((f->rvQuiet += n) > kQuiet) f->rvOn = false;
    }
    // Delay (Dub Station, ligne A, échos seuls).
    if (inD > kSilence && !f->dlOn) { f->dlOn = true; f->dlQuiet = 0; }
    if (f->dlOn) {
        for (int i = 0; i < n; i++) { l[i] = del[2 * i]; r[i] = del[2 * i + 1]; }
        f->delay.process(l, r, n);
        float peak = 0;
        for (int i = 0; i < n; i++) {
            const float el = l[i] - del[2 * i], er = r[i] - del[2 * i + 1];
            wet_del[2 * i] = el; wet_del[2 * i + 1] = er; peak = std::fmax(peak, std::fmax(std::fabs(el), std::fabs(er)));
        }
        if (inD > kSilence || peak > kSilence) f->dlQuiet = 0; else if ((f->dlQuiet += n) > kQuiet) f->dlOn = false;
    }
}
}
