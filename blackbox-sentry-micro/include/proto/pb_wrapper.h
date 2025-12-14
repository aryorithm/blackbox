// include/proto/pb_wrapper.h
#ifndef PROTO_PB_WRAPPER_H
#define PROTO_PB_WRAPPER_H

#include <stddef.h>
#include <stdint.h>
// Include the Generated NanoPB header
#include "proto/telemetry.pb.h" 

// Helper to serialize a Packet struct into a byte buffer
size_t proto_serialize(const Packet* packet, uint8_t* buffer, size_t buffer_size);

#endif