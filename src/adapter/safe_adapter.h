#ifndef SAFE_ADAPTER_H
#define SAFE_ADAPTER_H

typedef enum {
    ADAPTER_OK = 0,
    ADAPTER_BLOCKED = 1
} AdapterResult;

AdapterResult adapter_dispatch_safe(const char *action);

#endif
