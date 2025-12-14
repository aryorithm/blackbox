// include/transport/client_tcp.h
#ifndef TRANSPORT_CLIENT_TCP_H
#define TRANSPORT_CLIENT_TCP_H

#include <stdint.h>
#include <stddef.h>
#include "hal/net.h" // Assumes hal_net_connect/send defined previously

typedef struct {
    net_socket_t sock;
    int is_connected;
} transport_ctx_t;

// Connect to the Blackbox Core
int transport_connect(transport_ctx_t* ctx, const char* ip, uint16_t port);

// Send data buffer
int transport_send(transport_ctx_t* ctx, const uint8_t* data, size_t len);

// Close connection
void transport_close(transport_ctx_t* ctx);

#endif