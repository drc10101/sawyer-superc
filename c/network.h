/* network.h -- Sawyer network client for SuperC
 *
 * Connects the local Colibri-derived engine to the Sawyer
 * distributed MoE routing network for expert offloading
 * and token earning.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "glm.h"

/* Expert request (sent to router when local cache misses) */
typedef struct {
    int layer;
    int expert_id;
    float *input;          /* input activations for this expert */
    int input_len;
} ExpertRequest;

/* Expert response (received from network node) */
typedef struct {
    int layer;
    int expert_id;
    float *output;         /* output activations from remote expert */
    int output_len;
    int latency_ms;        /* round-trip time */
} ExpertResponse;

/* Provider registration */
typedef struct {
    char node_name[64];
    int gpu_vram_mb;
    char gpu_model[64];
    int tier;              /* 1-4 based on VRAM */
    int n_experts_hosted;
} ProviderInfo;

/* Connect to Sawyer router, returns 0 on success */
int sawyer_connect(SawyerNet *net, const char *router_url, const char *api_key);

/* Disconnect from router */
void sawyer_disconnect(SawyerNet *net);

/* Request a remote expert (blocks until response or timeout) */
int sawyer_fetch_expert(SawyerNet *net, ExpertRequest *req, ExpertResponse *resp, int timeout_ms);

/* Register as a provider node */
int sawyer_register_provider(SawyerNet *net, ProviderInfo *info);

/* Serve experts to the network (blocking loop) */
int sawyer_serve_loop(GlmEngine *engine);

/* Check token balance */
int64_t sawyer_token_balance(SawyerNet *net);

/* Get provider earnings */
float sawyer_provider_earnings(SawyerNet *net);

#endif /* NETWORK_H */