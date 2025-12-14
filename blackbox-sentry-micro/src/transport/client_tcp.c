// src/transport/client_tcp.c
#include "transport/client_tcp.h"
#include "sentry.h"

int transport_connect(transport_ctx_t* ctx, const char* ip, uint16_t port) {
    if (!ctx) return -1;

    int ret = hal_net_connect(&ctx->sock, ip, port);
    if (ret == 0) {
        ctx->is_connected = 1;
        LOG_INFO("Connected to %s:%d", ip, port);
    } else {
        ctx->is_connected = 0;
        LOG_ERR("Connection failed: error %d", ret);
    }
    return ret;
}

int transport_send(transport_ctx_t* ctx, const uint8_t* data, size_t len) {
    if (!ctx || !ctx->is_connected) return -1;

    int ret = hal_net_send(&ctx->sock, data, len);
    if (ret < 0) {
        LOG_ERR("Send failed. Closing connection.");
        transport_close(ctx);
        return -1;
    }
    return ret;
}

void transport_close(transport_ctx_t* ctx) {
    if (ctx && ctx->is_connected) {
        hal_net_close(&ctx->sock);
        ctx->is_connected = 0;
    }
}