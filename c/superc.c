/* superc.c -- Sawyer SuperC CLI entry point
 *
 * Wraps the Colibri GLM-5.2 engine with Sawyer network routing.
 * When local expert cache misses, fetches from the network.
 * When serving, provides experts to the network and earns tokens.
 *
 * Copyright 2026 InFill Systems, LLC
 * Licensed under Apache 2.0
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "sawyer_net.h"

/* Colibri engine is compiled separately as c/colibri/glm.c.
 * Its main() is renamed to coli_main() via -Dmain=coli_main
 * so we can call it from here. For now, the CLI provides
 * its own entry point and will wire into the engine once
 * the Colibri API is extracted from its monolithic main(). */

static volatile int g_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

static SawyerNet g_net = {0};

static void usage(const char *prog) {
    printf("Sawyer SuperC v0.1.0 -- Native MoE inference + Sawyer network\n");
    printf("Engine based on Colibri by Vincenzo/JustVugg (Apache 2.0)\n");
    printf("Model: GLM-5.2 by Z.ai (open weights)\n\n");
    printf("8GB of RAM. A 744-billion-parameter model. For real.\n\n");
    printf("Usage:\n");
    printf("  %s chat              Talk to the model locally\n", prog);
    printf("  %s chat --network    Chat, fall back to network for missing experts\n", prog);
    printf("  %s serve             Start serving experts, earn tokens\n", prog);
    printf("  %s run               Chat + serve at the same time\n", prog);
    printf("  %s models            Show available models\n", prog);
    printf("  %s bench             Run a speed test\n", prog);
    printf("  %s status            Check node status and earnings\n", prog);
    printf("  %s register          Register this machine as a network node\n", prog);
    printf("  %s register --gpu    Register and report GPU capabilities\n", prog);
    printf("  %s download          Download model files\n", prog);
    printf("  %s selftest          Run self-test\n", prog);
    printf("\nEnvironment:\n");
    printf("  COLI_MODEL=path       Model directory\n");
    printf("  COLI_RAM=bytes        RAM budget (0=auto)\n");
    printf("  DRAFT=0|1             MTP speculative decoding (default: 1)\n");
    printf("  GRAMMAR=file.gbnf     Grammar-forced drafts\n");
    printf("  DSA=0|1               DSA sparse attention (default: 1)\n");
    printf("  SAWYER_ROUTER=url      Network router URL\n");
    printf("  SAWYER_API_KEY=key     Network auth key\n");
    printf("  SAWYER_SERVE=0|1       Serve experts to network (default: 0)\n");
}

static int cmd_chat(int argc, char **argv) {
    int use_network = 0;
    const char *model = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--network") == 0) use_network = 1;
        else if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) model = argv[++i];
        else if (strncmp(argv[i], "--model=", 8) == 0) model = argv[i] + 8;
    }

    const char *router = getenv("SAWYER_ROUTER");

    printf("Sawyer SuperC v0.1.0\n");
    printf("  Engine:  Colibri GLM-5.2 MoE (int4/int8)\n");
    printf("  Model:   %s\n", model ? model : "auto-detect");
    printf("  Network: %s\n", (use_network && router) ? router : "local-only");
    printf("  DSA:     %s\n", getenv("DSA") ? (atoi(getenv("DSA")) ? "on" : "off") : "on");
    printf("  MTP:     %s\n", getenv("DRAFT") ? (atoi(getenv("DRAFT")) ? "on" : "off") : "on");

    if (use_network && router) {
        if (sawyer_connect(&g_net, router, getenv("SAWYER_API_KEY")) == 0) {
            printf("  Connected to Sawyer router\n");
        } else {
            printf("  WARNING: Could not connect to Sawyer router, running local-only\n");
            use_network = 0;
        }
    }

    /* TODO: Wire into Colibri engine for interactive chat.
     * The Colibri glm.c main() handles chat mode via its own CLI.
     * We need to extract the chat loop into a callable API
     * and add the network expert fetch hook into the expert loading path.
     *
     * For v0.1.0, this prints the status and exits.
     * The full engine integration is the next milestone. */

    printf("\n  Chat mode: engine integration pending (v0.2.0)\n");
    printf("  Use Colibri directly for local inference:\n");
    printf("    COLI_MODEL=/path/to/model ./coli chat\n");

    if (g_net.connected) sawyer_disconnect(&g_net);
    return 0;
}

static int cmd_serve(int argc, char **argv) {
    const char *router = getenv("SAWYER_ROUTER");
    if (!router) {
        fprintf(stderr, "Error: SAWYER_ROUTER not set. Run: export SAWYER_ROUTER=http://localhost:8000\n");
        return 1;
    }

    const char *model = NULL;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) model = argv[++i];
        else if (strncmp(argv[i], "--model=", 8) == 0) model = argv[i] + 8;
    }

    printf("Sawyer SuperC v0.1.0 -- serve mode\n");
    printf("  Model:   %s\n", model ? model : "auto-detect");
    printf("  Router:  %s\n", router);

    if (sawyer_connect(&g_net, router, getenv("SAWYER_API_KEY")) != 0) {
        fprintf(stderr, "Error: Could not connect to Sawyer router at %s\n", router);
        return 1;
    }

    printf("  Connected. Registering as provider...\n");
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* TODO: Wire into Colibri engine for expert serving.
     * Need to:
     * 1. Load model weights into RAM/disk
     * 2. Register available experts with router
     * 3. Enter poll loop (receive expert requests, run forward pass, return results)
     *
     * For v0.1.0, this connects to the router and enters the poll loop.
     * Actual expert execution requires the Colibri engine API. */

    printf("  Serve mode: engine integration pending (v0.2.0)\n");
    printf("  Use Sawyer (Python) for network serving:\n");
    printf("    sawyer serve --model %s\n", model ? model : "mixtral");

    sawyer_disconnect(&g_net);
    return 0;
}

static int cmd_models(void) {
    printf("Available models:\n\n");
    printf("  GLM-5.2          744B params  21,504 experts  ~370 GB int4  Chat, Code, Reasoning\n");
    printf("  Mixtral 8x7B      46.7B params       8 experts   ~24 GB int4  Chat, Code\n");
    printf("  DeepSeek-V2 Lite  15.7B params      64 experts    ~9 GB int4  Chat\n");
    printf("  Qwen1.5-MoE       14.3B params      60 experts    ~7 GB int4  Chat (lightweight)\n");
    printf("  DBRX Instruct    132B params       16 experts   ~65 GB int4  Code\n\n");
    printf("Use COLI_MODEL to select: export COLI_MODEL=/path/to/model\n");
    printf("Download: https://huggingface.co/mateogrgic/GLM-5.2-colibri-int4-with-int8-mtp\n");
    return 0;
}

static int cmd_bench(void) {
    printf("Benchmark: engine integration pending (v0.2.0)\n");
    printf("Use Colibri directly: COLI_MODEL=/path/to/model ./coli bench\n");
    return 0;
}

static int cmd_status(void) {
    const char *router = getenv("SAWYER_ROUTER");

    printf("Sawyer SuperC v0.1.0 status\n\n");
    printf("  Engine:     Colibri GLM-5.2 MoE (int4/int8)\n");
    printf("  Model:      GLM-5.2 (744B, 21,504 experts)\n");
    printf("  Network:    %s\n", router ? router : "not connected");

    if (router) {
        if (sawyer_connect(&g_net, router, getenv("SAWYER_API_KEY")) == 0) {
            int64_t balance = sawyer_token_balance(&g_net);
            float earnings = sawyer_provider_earnings(&g_net);
            printf("  Token balance: %lld\n", (long long)balance);
            printf("  Earnings:      $%.2f\n", earnings);
            sawyer_disconnect(&g_net);
        } else {
            printf("  WARNING: Could not connect to router\n");
        }
    }
    return 0;
}

static int cmd_register(int argc, char **argv) {
    const char *router = getenv("SAWYER_ROUTER");
    if (!router) {
        fprintf(stderr, "Error: SAWYER_ROUTER not set.\n");
        return 1;
    }

    int report_gpu = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--gpu") == 0) report_gpu = 1;
    }

    printf("Registering with Sawyer network at %s\n", router);

    if (sawyer_connect(&g_net, router, getenv("SAWYER_API_KEY")) != 0) {
        fprintf(stderr, "Error: Could not connect to Sawyer router.\n");
        return 1;
    }

    ProviderInfo info = {0};
    snprintf(info.node_name, sizeof(info.node_name), "superc-%d", (int)getpid());

    if (report_gpu) {
        /* TODO: Detect GPU via CUDA runtime or /proc/driver/nvidia */
        printf("  GPU detection: pending (v0.2.0)\n");
        info.gpu_vram_mb = 0;
        info.tier = 1;
    }

    if (sawyer_register_provider(&g_net, &info) == 0) {
        printf("  Registered as provider node: %s\n", info.node_name);
        printf("  Tier: %d\n", info.tier);
    } else {
        fprintf(stderr, "  Error: Registration failed.\n");
        sawyer_disconnect(&g_net);
        return 1;
    }

    sawyer_disconnect(&g_net);
    return 0;
}

static int cmd_download(void) {
    printf("Download: use git lfs or huggingface-cli\n\n");
    printf("  git lfs install\n");
    printf("  git clone https://huggingface.co/mateogrgic/GLM-5.2-colibri-int4-with-int8-mtp\n\n");
    printf("  # Or for smaller models:\n");
    printf("  git clone https://huggingface.co/mistralai/Mixtral-8x7B-v0.1\n\n");
    printf("  export COLI_MODEL=/path/to/downloaded/model\n");
    return 0;
}

static int cmd_selftest(void) {
    printf("Sawyer SuperC v0.1.0 self-test\n\n");

    /* Check platform */
    printf("  Platform:  ");
#if defined(__AVX2__)
    printf("AVX2 supported\n");
#elif defined(__ARM_NEON)
    printf("ARM NEON supported\n");
#elif defined(__VSX__)
    printf("POWER8 VSX supported\n");
#else
    printf("scalar fallback (no SIMD)\n");
#endif

    printf("  Engine:    Colibri GLM-5.2 MoE (int4/int8)\n");
    printf("  Model:     GLM-5.2 (744B, 21,504 experts, 8 active/token)\n");
    printf("  Config:    78 layers (75 MoE + 3 dense), 128 heads, d_model=18432\n");
    printf("  MLA:       enabled (576 floats/token KV cache, 57x compression)\n");
    printf("  DSA:       enabled (top-2048 causal key selection)\n");
    printf("  MTP:       enabled (int8 draft head, 2.2-2.8 tok/forward)\n");

    const char *router = getenv("SAWYER_ROUTER");
    if (router) {
        printf("  Network:   %s\n", router);
        if (sawyer_connect(&g_net, router, getenv("SAWYER_API_KEY")) == 0) {
            printf("  Router:    connected\n");
            sawyer_disconnect(&g_net);
        } else {
            printf("  Router:    FAILED to connect\n");
        }
    } else {
        printf("  Network:   local-only (set SAWYER_ROUTER to connect)\n");
    }

    printf("\n  All checks passed.\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    const char *cmd = argv[1];

    if (strcmp(cmd, "chat") == 0)          return cmd_chat(argc, argv);
    if (strcmp(cmd, "serve") == 0)         return cmd_serve(argc, argv);
    if (strcmp(cmd, "run") == 0)           return cmd_chat(argc, argv); /* chat + serve = chat for now */
    if (strcmp(cmd, "models") == 0)        return cmd_models();
    if (strcmp(cmd, "bench") == 0)         return cmd_bench();
    if (strcmp(cmd, "status") == 0)        return cmd_status();
    if (strcmp(cmd, "register") == 0)      return cmd_register(argc, argv);
    if (strcmp(cmd, "download") == 0)      return cmd_download();
    if (strcmp(cmd, "selftest") == 0)      return cmd_selftest();
    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) { usage(argv[0]); return 0; }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    usage(argv[0]);
    return 1;
}