#ifndef QCPU_DEVICE_WIRE_H
#define QCPU_DEVICE_WIRE_H

#include "qcpu_device_protocol.h"

#include <stddef.h>
#include <stdint.h>

#define QCPU_DEVICE_REQUEST_WIRE_SIZE 24U
#define QCPU_DEVICE_RESPONSE_WIRE_SIZE 56U

QCPUDeviceStatus qcpu_device_encode_request(
    uint8_t output[QCPU_DEVICE_REQUEST_WIRE_SIZE],
    const QCPUDeviceRequest *request
);

QCPUDeviceStatus qcpu_device_decode_request(
    QCPUDeviceRequest *request,
    const uint8_t *input,
    size_t input_size
);

QCPUDeviceStatus qcpu_device_encode_response(
    uint8_t output[QCPU_DEVICE_RESPONSE_WIRE_SIZE],
    const QCPUDeviceResponse *response
);

QCPUDeviceStatus qcpu_device_decode_response(
    QCPUDeviceResponse *response,
    const uint8_t *input,
    size_t input_size
);

#endif
