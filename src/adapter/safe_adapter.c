#include <stdio.h>
#include <string.h>
#include "safe_adapter.h"

AdapterResult adapter_dispatch_safe(const char *action) {
    if (strcmp(action, "led_on") == 0) {
        printf("[ADAPTER_SIM] LED_ON allowed\n");
        return ADAPTER_OK;
    }

    if (strcmp(action, "led_off") == 0) {
        printf("[ADAPTER_SIM] LED_OFF allowed\n");
        return ADAPTER_OK;
    }

    if (strcmp(action, "servo_up") == 0) {
        printf("[ADAPTER_SIM] SERVO_UP allowed\n");
        return ADAPTER_OK;
    }

    if (strcmp(action, "servo_down") == 0) {
        printf("[ADAPTER_SIM] SERVO_DOWN allowed\n");
        return ADAPTER_OK;
    }

    if (strcmp(action, "buzzer_beep") == 0) {
        printf("[ADAPTER_SIM] BUZZER_BEEP allowed\n");
        return ADAPTER_OK;
    }

    printf("[ADAPTER_BLOCKED] Unknown or unsafe action blocked: %s\n", action);
    return ADAPTER_BLOCKED;
}
