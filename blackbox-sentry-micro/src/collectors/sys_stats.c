// src/collectors/sys_stats.c
#include "collectors/sys_stats.h"
#include <stdio.h>
#include <stdlib.h>

float collector_get_cpu_usage(void) {
    // Reading /proc/stat accurately requires state (prev_idle, prev_total).
    // For this lightweight example, we use getloadavg as a proxy for CPU load.

    double load[1];
    if (getloadavg(load, 1) != -1) {
        // Return load average (approximate)
        return (float)(load[0] * 100.0);
    }
    return 0.0f;
}