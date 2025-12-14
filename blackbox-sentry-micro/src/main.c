/**
 * @file main.c
 * @brief Entry point for the IoT Agent.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "sentry.h"

// Forward declaration from src/core/agent.c
void sentry_run(void);

// Global flag for shutdown
volatile int g_running = 1;

void handle_signal(int sig) {
    (void)sig;
    LOG_INFO("Shutdown signal received. Exiting...");
    g_running = 0;
    exit(0); // For the micro version, immediate exit is often acceptable
}

int main(int argc, char** argv) {
    // 1. Setup Signal Handlers (Ctrl+C)
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // 2. Banner
    printf("\n");
    printf("   [ BLACKBOX SENTRY MICRO ]\n");
    printf("   [ Target: %s:%d ]\n", CONFIG_SERVER_IP, CONFIG_SERVER_PORT);
    printf("   [ ID:     %s ]\n\n", CONFIG_DEVICE_ID);

    // 3. System Init (HAL)
    // In a real RTOS, you would initialize UART/Network/Clock here.
    LOG_INFO("System Initialized.");

    // 4. Enter Main Loop
    sentry_run();

    return 0;
}