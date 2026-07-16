#include "qcpu_device_wire.h"

#include <float.h>
#include <string.h>

_Static_assert(
    sizeof(double) == sizeof(uint64_t),
    "64-bit double required"
);

_Static_assert(
    DBL_MANT_DIG == 53 &&
    DBL_MAX_EXP == 1024,
    "IEEE-754 binary64 required"
);

static void put_u16_le(uint8_t *output, uint16_t value) {
    output[0] = (uint8_t)(value & UINT16_C(0xff));
    output[1] = (uint8_t)(value >> 8);
}

static void put_u32_le(uint8_t *output, uint32_t value) {
    output[0] = (uint8_t)(value & UINT32_C(0xff));
    output[1] = (uint8_t)((value >> 8) & UINT32_C(0xff));
    output[2] = (uint8_t)((value >> 16) & UINT32_C(0xff));
    output[3] = (uint8_t)(value >> 24);
}

static void put_u64_le(uint8_t *output, uint64_t value) {
    size_t index;

    for (index = 0U; index < 8U; ++index) {
        output[index] = (uint8_t)(value >> (index * 8U));
    }
}

static uint16_t get_u16_le(const uint8_t *input) {
    return
        (uint16_t)input[0] |
        ((uint16_t)input[1] << 8);
}

static uint32_t get_u32_le(const uint8_t *input) {
    return
        (uint32_t)input[0] |
        ((uint32_t)input[1] << 8) |
        ((uint32_t)input[2] << 16) |
        ((uint32_t)input[3] << 24);
}

static uint64_t get_u64_le(const uint8_t *input) {
    uint64_t value = 0U;
    size_t index;

    for (index = 0U; index < 8U; ++index) {
        value |= (uint64_t)input[index] << (index * 8U);
    }

    return value;
}

static uint64_t double_to_u64(double value) {
    uint64_t bits;

    memcpy(&bits, &value, sizeof(bits));

    return bits;
}

static double u64_to_double(uint64_t bits) {
    double value;

    memcpy(&value, &bits, sizeof(value));

    return value;
}

QCPUDeviceStatus qcpu_device_encode_request(
    uint8_t output[QCPU_DEVICE_REQUEST_WIRE_SIZE],
    const QCPUDeviceRequest *request
) {
    if (output == NULL || request == NULL) {
        return QCPU_DEVICE_STATUS_IO;
    }

    put_u32_le(output + 0U, request->magic);
    put_u16_le(output + 4U, request->version);
    put_u16_le(output + 6U, request->command);
    put_u32_le(output + 8U, request->qubits);
    put_u32_le(output + 12U, request->shots);
    put_u64_le(output + 16U, request->seed);

    return QCPU_DEVICE_STATUS_OK;
}

QCPUDeviceStatus qcpu_device_decode_request(
    QCPUDeviceRequest *request,
    const uint8_t *input,
    size_t input_size
) {
    if (
        request == NULL ||
        input == NULL ||
        input_size != QCPU_DEVICE_REQUEST_WIRE_SIZE
    ) {
        return QCPU_DEVICE_STATUS_IO;
    }

    memset(request, 0, sizeof(*request));

    request->magic = get_u32_le(input + 0U);
    request->version = get_u16_le(input + 4U);
    request->command = get_u16_le(input + 6U);
    request->qubits = get_u32_le(input + 8U);
    request->shots = get_u32_le(input + 12U);
    request->seed = get_u64_le(input + 16U);

    if (request->magic != QCPU_DEVICE_REQUEST_MAGIC) {
        return QCPU_DEVICE_STATUS_BAD_MAGIC;
    }

    if (request->version != QCPU_DEVICE_PROTOCOL_VERSION) {
        return QCPU_DEVICE_STATUS_BAD_VERSION;
    }

    return QCPU_DEVICE_STATUS_OK;
}

QCPUDeviceStatus qcpu_device_encode_response(
    uint8_t output[QCPU_DEVICE_RESPONSE_WIRE_SIZE],
    const QCPUDeviceResponse *response
) {
    if (output == NULL || response == NULL) {
        return QCPU_DEVICE_STATUS_IO;
    }

    put_u32_le(output + 0U, response->magic);
    put_u16_le(output + 4U, response->version);
    put_u16_le(output + 6U, response->status);
    put_u32_le(output + 8U, response->flags);
    put_u32_le(output + 12U, response->qubits);
    put_u64_le(output + 16U, response->basis_states);
    put_u64_le(output + 24U, response->shots);
    put_u64_le(output + 32U, response->measured_state);
    put_u64_le(output + 40U, response->invalid_results);
    put_u64_le(output + 48U, double_to_u64(response->norm));

    return QCPU_DEVICE_STATUS_OK;
}

QCPUDeviceStatus qcpu_device_decode_response(
    QCPUDeviceResponse *response,
    const uint8_t *input,
    size_t input_size
) {
    if (
        response == NULL ||
        input == NULL ||
        input_size != QCPU_DEVICE_RESPONSE_WIRE_SIZE
    ) {
        return QCPU_DEVICE_STATUS_IO;
    }

    memset(response, 0, sizeof(*response));

    response->magic = get_u32_le(input + 0U);
    response->version = get_u16_le(input + 4U);
    response->status = get_u16_le(input + 6U);
    response->flags = get_u32_le(input + 8U);
    response->qubits = get_u32_le(input + 12U);
    response->basis_states = get_u64_le(input + 16U);
    response->shots = get_u64_le(input + 24U);
    response->measured_state = get_u64_le(input + 32U);
    response->invalid_results = get_u64_le(input + 40U);
    response->norm = u64_to_double(get_u64_le(input + 48U));

    if (response->magic != QCPU_DEVICE_RESPONSE_MAGIC) {
        return QCPU_DEVICE_STATUS_BAD_MAGIC;
    }

    if (response->version != QCPU_DEVICE_PROTOCOL_VERSION) {
        return QCPU_DEVICE_STATUS_BAD_VERSION;
    }

    return QCPU_DEVICE_STATUS_OK;
}
