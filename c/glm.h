/* glm.h -- GLM-5.2 MoE engine for Sawyer SuperC
 *
 * Based on Colibri by Vincenzo / JustVugg
 * https://github.com/JustVugg/colibri
 * Licensed under Apache 2.0
 *
 * Adapted for Sawyer SuperC by InFill Systems, LLC
 * - Added Sawyer network routing for remote expert fetch
 * - Added token accounting (sawyer_token_budget, sawyer_provider_earn)
 * - Removed Python dependencies, pure C runtime
 */

#ifndef GLM_H
#define GLM_H

#include <stdint.h>
#include <stddef.h>

/* Model configuration */
typedef struct {
    int n_layers;          /* 75 MoE layers + 3 dense */
    int n_heads;           /* 128 */
    int n_kv_heads;        /* 128 (no GQA) */
    int head_dim;          /* 128 */
    int n_routed_experts;  /* 256 per layer */
    int n_shared_experts;  /* 1 per layer */
    int n_active_experts;  /* 8 per token */
    int vocab_size;        /* 151552 */
    int mtp_layers;        /* 1 (layer 78) */
    int d_model;           /* 18432 */
    int mlp_mid;           /* 1024 per expert */
    float router_alpha;    /* sigmoid router slope */
    float routed_scale;   /* DeepSeek-V3-style routed scaling */
} GlmConfig;

/* Expert cache entry */
typedef struct {
    int layer;             /* MoE layer index */
    int expert_id;         /* Expert index within layer */
    void *weights;         /* int4 packed weights */
    float *scales;         /* per-row dequant scales */
    int ref_count;         /* LRU tracking */
    int local;             /* 1 if from disk, 0 if from network */
} ExpertEntry;

/* KV cache (MLA compressed) */
typedef struct {
    float *kv_buffer;      /* compressed MLA KV: 576 floats/token */
    int max_seq_len;
    int n_tokens;
    int n_layers;
} KvCache;

/* Sawyer network client */
typedef struct {
    char router_url[256];
    char api_key[64];
    int connected;
    int64_t token_balance;  /* remaining tokens */
    int64_t tokens_served;  /* tokens served to network */
    float earnings;         /* USD earned */
} SawyerNet;

/* Engine state */
typedef struct {
    GlmConfig config;
    KvCache kv;
    SawyerNet net;

    /* Dense layers (resident in RAM) */
    void *embed_weight;     /* embedding + shared expert dense */
    void *attn_qkv_proj;   /* MLA q/k/v projections */
    void *attn_out_proj;   /* output projection */

    /* Expert cache */
    ExpertEntry *expert_cache;
    int cache_capacity;
    int cache_size;

    /* Runtime */
    int dsa_enabled;       /* DSA sparse attention */
    int mtp_enabled;       /* MTP speculative decoding */
    int draft_enabled;     /* grammar-forced drafts */
    int network_enabled;   /* fetch experts from Sawyer network */
    int serve_enabled;     /* serve experts to network */

    /* Stats */
    int64_t tokens_generated;
    int64_t experts_loaded_local;
    int64_t experts_loaded_network;
    double avg_tok_per_sec;
} GlmEngine;

/* Lifecycle */
GlmEngine *glm_engine_create(const char *model_path, const char *config_json);
void glm_engine_destroy(GlmEngine *engine);

/* Inference */
int glm_chat(GlmEngine *engine, const char *prompt, char *output, size_t output_len);
int glm_generate(GlmEngine *engine, const int *tokens, int n_tokens,
                 int *output, int max_output);

/* Network */
int glm_net_connect(GlmEngine *engine, const char *router_url, const char *api_key);
int glm_net_serve(GlmEngine *engine);  /* blocking -- serves experts to network */

/* Stats */
void glm_stats(GlmEngine *engine, int64_t *tokens_out, int64_t *served_out,
               double *tok_per_sec_out, int64_t *earnings_out);

/* Config overrides (env vars) */
/* COLI_MODEL=path          -- model directory */
/* COLI_RAM=bytes           -- RAM budget (0=auto) */
/* DRAFT=0|1                -- MTP speculative decoding */
/* GRAMMAR=file.gbnf        -- grammar-forced drafts */
/* DSA=0|1                  -- DSA sparse attention */
/* SAWYER_ROUTER=url        -- network router */
/* SAWYER_API_KEY=key       -- network auth */
/* SAWYER_SERVE=0|1         -- serve experts to network */

#endif /* GLM_H */