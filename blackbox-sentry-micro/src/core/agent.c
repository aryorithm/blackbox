#include "sentry.h"
#include "hal/time.h"
#include "transport/client_tcp.h"
#include "collectors/sys_stats.h"

// Configuration
#define REPORT_INTERVAL_MS 5000

void sentry_run() {
    uint64_t last_report = hal_get_time_ms();
    
    // Connect to Core
    // In C, we pass structs as contexts instead of 'this'
    transport_ctx_t transport;
    if (transport_connect(&transport, CONFIG_SERVER_IP, CONFIG_SERVER_PORT) != 0) {
        LOG_ERR("Failed to connect to server");
        return;
    }

    while (1) {
        uint64_t now = hal_get_time_ms();

        // 1. Periodic Collection (Heartbeat)
        if (now - last_report > REPORT_INTERVAL_MS) {
            
            // Create ProtoBuf struct on Stack (Zero Alloc)
            Packet packet = Packet_init_zero;
            
            // Fill ID
            strncpy(packet.device_id, CONFIG_DEVICE_ID, sizeof(packet.device_id));

            // Collect CPU Temp
            Metric m1 = Metric_init_zero;
            m1.timestamp = (uint32_t)(now / 1000);
            strncpy(m1.key, "cpu_usage", 10);
            m1.value_num = collector_get_cpu_usage();
            
            // Add to packet (NanoPB uses callbacks for arrays, simplified here)
            // ... packing logic ...

            // Serialize & Send
            uint8_t buffer[128];
            size_t len = proto_serialize(&packet, buffer, sizeof(buffer));
            
            transport_send(&transport, buffer, len);
            
            last_report = now;
        }

        // 2. Event Loop Sleep (Yield CPU)
        hal_sleep_ms(100); 
    }
}