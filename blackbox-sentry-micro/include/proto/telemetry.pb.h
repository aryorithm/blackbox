#ifndef TELEMETRY_PB_H
#define TELEMETRY_PB_H
#include "pb.h" // NanoPB core

typedef struct _Metric {
    uint32_t timestamp;
    char key[16];
    float value_num;
    char value_str[16];
} Metric;

typedef struct _Packet {
    char device_id[32];
    pb_callback_t metrics; // Callback for repeated fields
} Packet;

#define Packet_init_zero { {0}, {{0}} }
#define Metric_init_zero { 0, {0}, 0, {0} }

// ... extern definitions for pb_encode ...
#endif