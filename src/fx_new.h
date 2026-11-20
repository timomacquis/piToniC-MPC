/* πToniC 1.1.0 — effets NEW (Tidal Verb + delay A de Dub Station), API C du pont C++ (src/fx_new.cpp).
 * Bus de send en entrée, sortie 100 % effet. Les deux effets sont en parallèle. Aucune allocation hors fxn_create. */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FxNew FxNew;
enum {FXN_RTYPE,FXN_RDECAY,FXN_RLIVE,FXN_RWARMTH,FXN_DSYNC,FXN_DDIV,FXN_DMS,FXN_DFEEDBACK,FXN_DFILTER,FXN_COUNT};
FxNew *fxn_create(void);
void fxn_destroy(FxNew *f);
void fxn_set(FxNew *f, int id, float value);
void fxn_bpm(FxNew *f, float bpm);
void fxn_reset(FxNew *f);
/* rev, del : bus stéréo entrelacés ; wet_rev, wet_del : sorties entrelacées (écrites) ; n <= 128 trames. */
void fxn_process(FxNew *f, const float *rev, const float *del, float *wet_rev, float *wet_del, int n);
int fxn_busy(const FxNew *f);
#ifdef __cplusplus
}
#endif
