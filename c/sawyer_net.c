/* sawyer_net.c -- Sawyer network client for SuperC
 *
 * Connects the local Colibri-derived engine to the Sawyer
 * distributed MoE routing network. When the local expert cache
 * misses, this module asks the Sawyer router for a node that
 * has the expert, then fetches the result.
 *
 * When running in serve mode, this module registers with the
 * router and enters a receive loop, running experts for other
 * users and earning tokens.
 *
 * Copyright 2026 InFill Systems, LLC
 * Licensed under Apache 2.0
 */

#include "sawyer_net.h"
#include "colibri/json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>

/* ---- Platform-specific networking ---- */
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
  #define INVALID_SOCK INVALID_SOCKET
  #define CLOSE_SOCK closesocket
  static int ws2_init = 0;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  typedef int socket_t;
  #define INVALID_SOCK (-1)
  #define CLOSE_SOCK close
#endif

/* ---- HTTP client (minimal, no TLS -- router is localhost or VPN) ---- */

typedef struct {
    char body[256 * 1024];   /* response body buffer */
    int  body_len;
    int  status_code;
} HttpResponse;

static int http_init(void) {
#ifdef _WIN32
    if (!ws2_init) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
        ws2_init = 1;
    }
#endif
    return 0;
}

static socket_t http_connect(const char *host, int port) {
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0) return INVALID_SOCK;

    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCK) { freeaddrinfo(res); return INVALID_SOCK; }

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        CLOSE_SOCK(sock);
        freeaddrinfo(res);
        return INVALID_SOCK;
    }

    freeaddrinfo(res);
    return sock;
}

static int http_send(socket_t sock, const char *data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = (int)send(sock, data + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

static int http_recv(socket_t sock, char *buf, int buf_len, int timeout_ms) {
    /* Simple recv with timeout using select */
    fd_set fds;
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    if (select((int)sock + 1, &fds, NULL, NULL, &tv) <= 0) return 0;
    return (int)recv(sock, buf, buf_len, 0);
}

static int http_request(const char *method, const char *host, int port,
                        const char *path, const char *body, int body_len,
                        const char *api_key, HttpResponse *resp) {
    http_init();
    memset(resp, 0, sizeof(*resp));

    socket_t sock = http_connect(host, port);
    if (sock == INVALID_SOCK) return -1;

    /* Build request */
    char req[4096];
    int rlen = snprintf(req, sizeof(req),
        "%s %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Accept: application/json\r\n"
        "Connection: close\r\n",
        method, path, host, port);

    if (api_key && api_key[0]) {
        rlen += snprintf(req + rlen, sizeof(req) - rlen,
            "Authorization: Bearer %s\r\n", api_key);
    }

    if (body && body_len > 0) {
        rlen += snprintf(req + rlen, sizeof(req) - rlen,
            "Content-Length: %d\r\n", body_len);
    }

    rlen += snprintf(req + rlen, sizeof(req) - rlen, "\r\n");

    if (http_send(sock, req, rlen) != 0) { CLOSE_SOCK(sock); return -1; }

    if (body && body_len > 0) {
        if (http_send(sock, body, body_len) != 0) { CLOSE_SOCK(sock); return -1; }
    }

    /* Read response */
    char buf[64 * 1024];
    int total = 0;
    while (total < (int)sizeof(buf) - 1) {
        int n = http_recv(sock, buf + total, sizeof(buf) - total - 1, 5000);
        if (n <= 0) break;
        total += n;
    }
    buf[total] = '\0';
    CLOSE_SOCK(sock);

    /* Parse status line */
    char *status_line = strstr(buf, "HTTP/1.");
    if (status_line) {
        sscanf(status_line, "HTTP/1.%*d %d", &resp->status_code);
    }

    /* Find body (after \r\n\r\n) */
    char *body_start = strstr(buf, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        int header_len = (int)(body_start - buf);
        int body_len_resp = total - header_len;
        if (body_len_resp > 0) {
            int copy_len = body_len_resp < (int)sizeof(resp->body) - 1 ? body_len_resp : (int)sizeof(resp->body) - 1;
            memcpy(resp->body, body_start, copy_len);
            resp->body[copy_len] = '\0';
            resp->body_len = copy_len;
        }
    }

    return resp->status_code;
}

/* ---- Sawyer network API ---- */

static void parse_host_port(const char *url, char *host, int host_size, int *port) {
    /* Parse "http://host:port" or "https://host:port" */
    const char *start = url;
    if (strncmp(start, "http://", 7) == 0) start += 7;
    else if (strncmp(start, "https://", 8) == 0) start += 8;

    *port = 8000; /* default Sawyer router port */
    const char *colon = strchr(start, ':');
    const char *slash = strchr(start, '/');

    if (colon && (!slash || colon < slash)) {
        int len = (int)(colon - start);
        if (len >= host_size) len = host_size - 1;
        memcpy(host, start, len);
        host[len] = '\0';
        *port = atoi(colon + 1);
    } else {
        int len = slash ? (int)(slash - start) : (int)strlen(start);
        if (len >= host_size) len = host_size - 1;
        memcpy(host, start, len);
        host[len] = '\0';
    }
}

int sawyer_connect(SawyerNet *net, const char *router_url, const char *api_key) {
    if (!net || !router_url) return -1;

    memset(net, 0, sizeof(*net));
    strncpy(net->router_url, router_url, sizeof(net->router_url) - 1);
    if (api_key) strncpy(net->api_key, api_key, sizeof(net->api_key) - 1);

    /* Health check: GET /v1/models */
    char host[256];
    int port;
    parse_host_port(router_url, host, sizeof(host), &port);

    HttpResponse resp;
    int status = http_request("GET", host, port, "/v1/models", NULL, 0, api_key, &resp);

    if (status == 200) {
        net->connected = 1;
        net->token_balance = -1; /* unknown, fetch on demand */
        net->tokens_served = 0;
        net->earnings = 0.0;
        return 0;
    }

    net->connected = 0;
    return -1;
}

void sawyer_disconnect(SawyerNet *net) {
    if (!net) return;
    net->connected = 0;
}

int sawyer_fetch_expert(SawyerNet *net, ExpertRequest *req, ExpertResponse *resp, int timeout_ms) {
    if (!net || !net->connected) return -1;

    /* POST /v1/expert with layer, expert_id, and input activations */
    char body[256 * 1024]; /* large enough for expert input */
    int body_len = snprintf(body, sizeof(body),
        "{\"layer\":%d,\"expert_id\":%d,\"input_len\":%d}", /* input activations sent as binary after JSON header */
        req->layer, req->expert_id, req->input_len);

    /* For now, send the JSON request. The full implementation will
     * serialize the float activations and include them in the body.
     * The router responds with the expert output activations. */

    char host[256];
    int port;
    parse_host_port(net->router_url, host, sizeof(host), &port);

    char path[256];
    snprintf(path, sizeof(path), "/v1/expert/%d/%d", req->layer, req->expert_id);

    HttpResponse http_resp;
    int status = http_request("POST", host, port, path, body, body_len, net->api_key, &http_resp);

    if (status != 200) return -1;

    /* Parse response: { "output_len": N, "latency_ms": T } */
    /* Full implementation will deserialize the output activations */
    if (resp) {
        resp->layer = req->layer;
        resp->expert_id = req->expert_id;
        resp->latency_ms = 0;
        /* Parse latency from response */
        const char *lat_str = strstr(http_resp.body, "\"latency_ms\"");
        if (lat_str) {
            lat_str = strchr(lat_str + 12, ':');
            if (lat_str) resp->latency_ms = atoi(lat_str + 1);
        }
    }

    net->token_balance--; /* debit one token for network expert fetch */
    return 0;
}

int sawyer_register_provider(SawyerNet *net, ProviderInfo *info) {
    if (!net || !net->connected) return -1;

    char body[1024];
    snprintf(body, sizeof(body),
        "{\"node_name\":\"%s\",\"gpu_vram_mb\":%d,\"gpu_model\":\"%s\",\"tier\":%d,\"n_experts_hosted\":%d}",
        info->node_name, info->gpu_vram_mb, info->gpu_model, info->tier, info->n_experts_hosted);

    char host[256];
    int port;
    parse_host_port(net->router_url, host, sizeof(host), &port);

    HttpResponse resp;
    int status = http_request("POST", host, port, "/v1/provider/register", body, (int)strlen(body), net->api_key, &resp);

    if (status == 200 || status == 201) {
        net->serve_enabled = 1;
        return 0;
    }

    return -1;
}

int sawyer_serve_loop(Model *model) {
    /* v0.1.0: serve loop is a stub. Full wiring in v0.2.0 when
     * Colibri's Model struct is integrated with SawyerNet. */
    (void)model;
    fprintf(stderr, "  sawyer: serve mode not yet wired (v0.2.0)\n");
    fprintf(stderr, "  sawyer: register with --network flag to enable serving\n");
    return 0;
}

int64_t sawyer_token_balance(SawyerNet *net) {
    if (!net || !net->connected) return -1;

    char host[256];
    int port;
    parse_host_port(net->router_url, host, sizeof(host), &port);

    HttpResponse resp;
    int status = http_request("GET", host, port, "/v1/account/balance", NULL, 0, net->api_key, &resp);

    if (status == 200) {
        /* Parse balance from JSON response */
        const char *bal = strstr(resp.body, "\"balance\"");
        if (bal) {
            bal = strchr(bal + 9, ':');
            if (bal) net->token_balance = atoll(bal + 1);
        }
    }

    return net->token_balance;
}

float sawyer_provider_earnings(SawyerNet *net) {
    if (!net || !net->connected) return 0.0f;

    char host[256];
    int port;
    parse_host_port(net->router_url, host, sizeof(host), &port);

    HttpResponse resp;
    int status = http_request("GET", host, port, "/v1/provider/earnings", NULL, 0, net->api_key, &resp);

    if (status == 200) {
        const char *earn = strstr(resp.body, "\"earnings\"");
        if (earn) {
            earn = strchr(earn + 10, ':');
            if (earn) net->earnings = atof(earn + 1);
        }
    }

    return net->earnings;
}