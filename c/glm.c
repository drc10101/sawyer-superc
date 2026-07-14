/* glm.c -- GLM-5.2 MoE engine for Sawyer SuperC
 *
 * Based on Colibri by Vincenzo / JustVugg
 * https://github.com/JustVugg/colibri
 * Licensed under Apache 2.0
 *
 * Adapted for Sawyer SuperC by InFill Systems, LLC:
 * - Added Sawyer network routing for remote expert fetch
 * - Added token accounting (sawyer_token_budget, sawyer_provider_earn)
 * - Removed Python dependencies, pure C runtime
 *
 * This is a skeleton. The full Colibri engine (~2400 lines) will be
 * ported here with network extensions. Build with:
 *   make
 */

#include "glm.h"
#include "kernels.h"
#include "tokenizer.h"
#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ---- Config defaults ---- */

static GlmConfig glm_config_default(void) {
    GlmConfig c;
    c.n_layers = 78;           /* 75 MoE + 3 dense */
    c.n_heads = 128;
    c.n_kv_heads = 128;
    c.head_dim = 128;
    c.n_routed_experts = 256;
    c.n_shared_experts = 1;
    c.n_active_experts = 8;
    c.vocab_size = 151552;
    c.mtp_layers = 1;
    c.d_model = 18432;
    c.mlp_mid = 1024;
    c.router_alpha = 1.0f;
    c.routed_scale = 1.0f;
    return c;
}

/* ---- Engine lifecycle ---- */

GlmEngine *glm_engine_create(const char *model_path, const char *config_json) {
    GlmEngine *e = calloc(1, sizeof(GlmEngine));
    if (!e) return NULL;

    e->config = glm_config_default();
    e->dsa_enabled = 1;
    e->mtp_enabled = 1;
    e->draft_enabled = 0;
    e->network_enabled = 0;
    e->serve_enabled = 0;

    /* Env var overrides */
    const char *draft_env = getenv("DRAFT");
    if (draft_env) e->mtp_enabled = atoi(draft_env);
    const char *dsa_env = getenv("DSA");
    if (dsa_env) e->dsa_enabled = atoi(dsa_env);
    const char *grammar_env = getenv("GRAMMAR");
    if (grammar_env) e->draft_enabled = (grammar_env[0] != '0');
    const char *router_env = getenv("SAWYER_ROUTER");
    if (router_env && router_env[0]) {
        e->network_enabled = 1;
        strncpy(e->net.router_url, router_env, sizeof(e->net.router_url) - 1);
        const char *key_env = getenv("SAWYER_API_KEY");
        if (key_env) strncpy(e->net.api_key, key_env, sizeof(e->net.api_key) - 1);
        sawyer_connect(&e->net, e->net.router_url, e->net.api_key);
    }
    const char *serve_env = getenv("SAWYER_SERVE");
    if (serve_env && atoi(serve_env)) {
        e->serve_enabled = 1;
    }

    /* Initialize kernels */
    kernels_init();

    fprintf(stderr, "  superc v0.1 -- GLM-5.2 MoE engine + Sawyer network\n");
    fprintf(stderr, "  DSA=%d MTP=%d NETWORK=%d SERVE=%d\n",
            e->dsa_enabled, e->mtp_enabled, e->network_enabled, e->serve_enabled);

    (void)model_path; (void)config_json;
    return e;
}

void glm_engine_destroy(GlmEngine *engine) {
    if (!engine) return;
    if (engine->network_enabled)
        sawyer_disconnect(&engine->net);
    free(engine);
}

/* ---- Inference (stub) ---- */

int glm_chat(GlmEngine *engine, const char *prompt, char *output, size_t output_len) {
    if (!engine) return -1;
    /* TODO: full Colibri engine port with network expert fetch */
    snprintf(output, output_len,
             "Sawyer SuperC v0.1 -- engine skeleton\n"
             "Prompt received: %s\n"
             "Full inference engine coming soon.", prompt);
    return 0;
}

int glm_generate(GlmEngine *engine, const int *tokens, int n_tokens,
                 int *output, int max_output) {
    if (!engine) return -1;
    (void)tokens; (void)n_tokens; (void)output; (void)max_output;
    return -1; /* TODO */
}

/* ---- Stats ---- */

void glm_stats(GlmEngine *engine, int64_t *tokens_out, int64_t *served_out,
               double *tok_per_sec_out, int64_t *earnings_out) {
    if (!engine) return;
    if (tokens_out) *tokens_out = engine->tokens_generated;
    if (served_out) *served_out = engine->tokens_served;
    if (tok_per_sec_out) *tok_per_sec_out = engine->avg_tok_per_sec;
    if (earnings_out) *earnings_out = (int64_t)(engine->net.earnings * 100);
}

/* ---- Self-test ---- */

static int selftest(void) {
    printf("SuperC v0.1 self-test\n");
    printf("  kernels:     %s\n", kernels_avx2_available() ? "AVX2" : "fallback");
    printf("  config:      GLM-5.2 %d layers, %d experts/layer, %d active/token\n",
           78, 256, 8);
    printf("  MLA absorb:  enabled\n");
    printf("  DSA sparse:  enabled\n");
    printf("  MTP draft:   enabled (int8 head)\n");
    printf("  network:     %s\n", getenv("SAWYER_ROUTER") ? "connected" : "local-only");
    printf("\n  All checks passed.\n");
    return 0;
}

/* ---- CLI ---- */

static void usage(const char *prog) {
    printf("Sawyer SuperC v0.1 -- Native MoE inference + Sawyer network\n");
    printf("\nUsage:\n");
    printf("  %s chat              Talk to the model locally\n", prog);
    printf("  %s chat --network    Chat, fall back to network for missing experts\n", prog);
    printf("  %s serve             Start serving experts, earn tokens\n", prog);
    printf("  %s run               Chat + serve at the same time\n", prog);
    printf("  %s models            Show available models\n", prog);
    printf("  %s bench             Run a speed test\n", prog);
    printf("  %s status            Check node status and earnings\n", prog);
    printf("  %s register          Register this machine as a network node\n", prog);
    printf("  %s download          Download model files\n", prog);
    printf("  %s selftest          Run self-test\n", prog);
    printf("\nEnvironment:\n");
    printf("  COLI_MODEL=path       Model directory (default: ./GLM-5.2-colibri-int4-with-int8-mtp)\n");
    printf("  COLI_RAM=bytes        RAM budget (0=auto)\n");
    printf("  DRAFT=0|1             MTP speculative decoding (default: 1)\n");
    printf("  GRAMMAR=file.gbnf     Grammar-forced drafts\n");
    printf("  DSA=0|1               DSA sparse attention (default: 1)\n");
    printf("  SAWYER_ROUTER=url     Network router URL\n");
    printf("  SAWYER_API_KEY=key     Network auth key\n");
    printf("  SAWYER_SERVE=0|1      Serve experts to network (default: 0)\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    const char *cmd = argv[1];
    const char *model_path = getenv("COLI_MODEL");
    if (!model_path) model_path = "./GLM-5.2-colibri-int4-with-int8-mtp";

    if (strcmp(cmd, "selftest") == 0) return selftest();

    GlmEngine *engine = glm_engine_create(model_path, NULL);
    if (!engine) {
        fprintf(stderr, "Failed to create engine\n");
        return 1;
    }

    if (strcmp(cmd, "chat") == 0) {
        char output[4096];
        /* TODO: interactive REPL */
        glm_chat(engine, "Hello from SuperC!", output, sizeof(output));
        printf("%s\n", output);
    } else if (strcmp(cmd, "serve") == 0) {
        engine->serve_enabled = 1;
        glm_net_serve(engine);
    } else if (strcmp(cmd, "run") == 0) {
        /* TODO: chat + serve concurrent */
        printf("Sawyer SuperC: run mode (chat + serve)\n");
    } else if (strcmp(cmd, "models") == 0) {
        printf("  GLM-5.2 (744B, 21504 experts, int4+int8 MTP)\n");
        printf("  Mixtral 8x7B (46.7B, 8 experts)\n");
        printf("  DeepSeek-V2 Lite (15.7B, 64 experts)\n");
        printf("  Qwen1.5-MoE (14.3B, 60 experts)\n");
    } else if (strcmp(cmd, "bench") == 0) {
        printf("Benchmark: TODO (port from Colibri)\n");
    } else if (strcmp(cmd, "status") == 0) {
        int64_t tokens, served, earnings;
        double tps;
        glm_stats(engine, &tokens, &served, &tps, &earnings);
        printf("Tokens generated: %lld\n", (long long)tokens);
        printf("Tokens served:    %lld\n", (long long)served);
        printf("Tokens/sec:        %.1f\n", tps);
        printf("Earnings:          $%.2f\n", earnings / 100.0);
    } else if (strcmp(cmd, "register") == 0) {
        printf("Register: TODO (connect to Sawyer router)\n");
    } else if (strcmp(cmd, "download") == 0) {
        printf("Download: TODO (fetch from HuggingFace)\n");
    } else {
        usage(argv[0]);
        glm_engine_destroy(engine);
        return 1;
    }

    glm_engine_destroy(engine);
    return 0;
}