/*
 * ota_boot_request.h
 *
 * SRAM handoff marker used by the application to request OTA processing
 * on the next bootloader entry.
 */

#ifndef HARDWARE_OTA_INC_OTA_BOOT_REQUEST_H_
#define HARDWARE_OTA_INC_OTA_BOOT_REQUEST_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_BOOT_REQUEST_MAGIC      0x504F5442UL  /* "POTB" */
#define OTA_BOOT_REQUEST_MAGIC_INV  0xAFB0ABBDUL
#define OTA_BOOT_REQUEST_SECTION    "BootRequest"

typedef struct {
    uint32_t magic;
    uint32_t magic_inv;
    uint32_t sequence;
    uint32_t reserved;
} OTA_BootRequest_t;

static inline volatile OTA_BootRequest_t *OTA_BootRequestPtr(void)
{
    extern volatile OTA_BootRequest_t g_ota_boot_request;
    return &g_ota_boot_request;
}

static inline void OTA_BootRequestSet(uint32_t sequence)
{
    volatile OTA_BootRequest_t *request = OTA_BootRequestPtr();
    request->sequence = sequence;
    request->reserved = 0UL;
    request->magic_inv = OTA_BOOT_REQUEST_MAGIC_INV;
    request->magic = OTA_BOOT_REQUEST_MAGIC;
}

static inline void OTA_BootRequestClear(void)
{
    volatile OTA_BootRequest_t *request = OTA_BootRequestPtr();
    request->magic = 0UL;
    request->magic_inv = 0UL;
    request->sequence = 0UL;
    request->reserved = 0UL;
}

static inline uint32_t OTA_BootRequestGet(void)
{
    volatile OTA_BootRequest_t *request = OTA_BootRequestPtr();
    if (request->magic == OTA_BOOT_REQUEST_MAGIC &&
        request->magic_inv == OTA_BOOT_REQUEST_MAGIC_INV) {
        return request->sequence;
    }
    return 0UL;
}

static inline uint32_t OTA_BootRequestConsume(void)
{
    uint32_t sequence = OTA_BootRequestGet();
    OTA_BootRequestClear();
    return sequence;
}

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_OTA_INC_OTA_BOOT_REQUEST_H_ */
