/* sawyer_net.h -- Sawyer network client for SuperC
 *
 * Connects the local Colibri-derived engine to the Sawyer
 * distributed MoE routing network for expert offloading
 * and token earning.
 *
 * Copyright 2026 InFill Systems, LLC
 * Licensed under Apache 2.0
 */

#ifndef SAWYER_NET_H
#define SAWYER_NET_H

#include <stdint.h>
#include <stddef.h>

/* Expert request (sent to router when local cache misses) */
typedef struct {
    int layer;             /* MoE layer index (0-74) */
    int expert_id;         /* Expert index within layer (0-255) */
    float *input;          /* input activations for this expert */
    int input_len;         /* number of floats in input */
} ExpertRequest;

/* Expert response (received from network node) */
typedef struct {
    int layer;             /* MoE layer index */
    int expert_id;         /* Expert index within layer */
    float *output;         /* output activations from remote expert */
    int output_len;        /* number of floats in output */
    int latency_ms;        /* round-trip time in ms */
} ExpertResponse;

/* Provider registration */
typedef struct {
    char node_name[64];
    int gpu_vram_mb;
    char gpu_model[64];
    int tier;              /* 1-4 based on VRAM */
    int n_experts_hosted;
} ProviderInfo;

/* Sawyer network client state */
typedef struct {
    char router_url[256];
    char api_key[64];
    int connected;
    int serve_enabled;
    int64_t token_balance;  /* remaining tokens (-1 = unknown) */
    int64_t tokens_served;  /* tokens served to network */
    float earnings;         /* USD earned */
} SawyerNet;

/* Forward declaration -- engine state defined in colibri/glm.c */
typedef struct Model Model;

/* Connect to Sawyer router, returns 0 on success */
int sawyer_connect(SawyerNet *net, const char *router_url, const char *api_key);

/* Disconnect from router */
void sawyer_disconnect(SawyerNet *net);

/* Request a remote expert (blocks until response or timeout) */
int sawyer_fetch_expert(SawyerNet *net, ExpertRequest *req, ExpertResponse *resp, int timeout_ms);

/* Register as a provider node */
int sawyer_register_provider(SawyerNet *net, ProviderInfo *info);

/* Serve experts to the network (blocking loop) */
int sawyer_serve_loop(Model *model);

/* Check token balance */
int64_t sawyer_token_balance(SawyerNet *net);

/* Get provider earnings */
float sawyer_provider_earnings(SawyerNet *net);

#endif /* SAWYER_NET_H */