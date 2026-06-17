/*
 * ota_manifest.c
 *
 * OTA manifest and boot flag helpers.
 */

#include "ota_manifest.h"
#include "ota_boot_request.h"

#include <string.h>

#define OTA_CRC32_INIT      0xFFFFFFFFUL
#define OTA_CRC32_XOROUT    0xFFFFFFFFUL
#define OTA_CRC32_POLY_REV  0xEDB88320UL

volatile OTA_BootRequest_t g_ota_boot_request __attribute__((section(OTA_BOOT_REQUEST_SECTION), zero_init));

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t length)
{
    while (length-- > 0U) {
        crc ^= *data++;
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            if ((crc & 1UL) != 0UL) {
                crc = (crc >> 1) ^ OTA_CRC32_POLY_REV;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint32_t OTA_Crc32(const void *data, size_t length)
{
    if (data == NULL && length > 0U) {
        return 0U;
    }

    uint32_t crc = OTA_CRC32_INIT;
    crc = crc32_update(crc, (const uint8_t *)data, length);
    return crc ^ OTA_CRC32_XOROUT;
}

void OTA_ManifestInit(OTA_Manifest_t *manifest)
{
    if (manifest == NULL) {
        return;
    }

    memset(manifest, 0, sizeof(*manifest));
    manifest->magic = OTA_MANIFEST_MAGIC;
    manifest->manifest_version = OTA_MANIFEST_VERSION;
    manifest->product_id = OTA_PRODUCT_ID_PINPROBE_A1;
    manifest->image_type = OTA_IMAGE_TYPE_APP;
    manifest->app_base_addr = OTA_APP_BASE_ADDR;
    manifest->app_max_size = OTA_APP_MAX_SIZE;
    manifest->slot_addr = OTA_SLOT_A_ADDR;
    manifest->slot_size = OTA_SLOT_SIZE;
    manifest->block_size = OTA_SCPI_CHUNK_SIZE;
    manifest->state = (uint8_t)OTA_STATE_IDLE;
}

bool OTA_ManifestFinalize(OTA_Manifest_t *manifest)
{
    if (manifest == NULL) {
        return false;
    }

    manifest->manifest_crc32 = 0U;
    manifest->manifest_crc32 = OTA_Crc32(manifest, sizeof(*manifest));
    return true;
}

bool OTA_ManifestValidate(const OTA_Manifest_t *manifest)
{
    if (manifest == NULL) {
        return false;
    }

    if (manifest->magic != OTA_MANIFEST_MAGIC) {
        return false;
    }
    if ((manifest->manifest_version >> 16) != (OTA_MANIFEST_VERSION >> 16)) {
        return false;
    }
    if (manifest->image_size == 0UL ||
        manifest->image_size > OTA_APP_MAX_SIZE ||
        manifest->image_size > manifest->slot_size) {
        return false;
    }
    if (manifest->product_id != OTA_PRODUCT_ID_PINPROBE_A1) {
        return false;
    }
    if (manifest->image_type != OTA_IMAGE_TYPE_APP) {
        return false;
    }
    if (manifest->app_base_addr != OTA_APP_BASE_ADDR) {
        return false;
    }
    if (manifest->app_max_size > OTA_APP_MAX_SIZE) {
        return false;
    }
    if (!((manifest->slot_addr == OTA_SLOT_A_ADDR) ||
          (manifest->slot_addr == OTA_SLOT_B_ADDR))) {
        return false;
    }
    if (manifest->slot_size > OTA_SLOT_SIZE) {
        return false;
    }
    if (manifest->block_size == 0U || manifest->block_size > OTA_SCPI_CHUNK_SIZE) {
        return false;
    }

    OTA_Manifest_t copy = *manifest;
    uint32_t expected_crc = copy.manifest_crc32;
    copy.manifest_crc32 = 0U;
    return OTA_Crc32(&copy, sizeof(copy)) == expected_crc;
}

void OTA_BootFlagsInit(OTA_BootFlags_t *flags)
{
    if (flags == NULL) {
        return;
    }

    memset(flags, 0, sizeof(*flags));
    flags->magic = OTA_BOOT_FLAGS_MAGIC;
    flags->pending_slot = (uint8_t)OTA_SLOT_NONE;
    flags->previous_slot = (uint8_t)OTA_SLOT_NONE;
    flags->update_state = (uint8_t)OTA_BOOT_STATE_IDLE;
    flags->max_attempts = 1U;
}

bool OTA_BootFlagsFinalize(OTA_BootFlags_t *flags)
{
    if (flags == NULL) {
        return false;
    }

    flags->flags_crc32 = 0U;
    flags->flags_crc32 = OTA_Crc32(flags, sizeof(*flags));
    return true;
}

bool OTA_BootFlagsValidate(const OTA_BootFlags_t *flags)
{
    if (flags == NULL) {
        return false;
    }

    if (flags->magic != OTA_BOOT_FLAGS_MAGIC) {
        return false;
    }
    if (flags->pending_slot > (uint8_t)OTA_SLOT_B ||
        flags->previous_slot > (uint8_t)OTA_SLOT_B) {
        return false;
    }
    if (flags->update_state > (uint8_t)OTA_BOOT_STATE_FAILED) {
        return false;
    }
    if (flags->max_attempts == 0U) {
        return false;
    }

    OTA_BootFlags_t copy = *flags;
    uint32_t expected_crc = copy.flags_crc32;
    copy.flags_crc32 = 0U;
    return OTA_Crc32(&copy, sizeof(copy)) == expected_crc;
}

uint32_t OTA_SlotAddress(uint8_t slot_id)
{
    switch ((OTA_SlotId_t)slot_id) {
    case OTA_SLOT_A:
        return OTA_SLOT_A_ADDR;
    case OTA_SLOT_B:
        return OTA_SLOT_B_ADDR;
    default:
        return 0UL;
    }
}

const char *OTA_ErrorName(uint32_t error_code)
{
    switch ((OTA_Error_t)error_code) {
    case OTA_OK: return "OK";
    case OTA_ERR_BUSY: return "BUSY";
    case OTA_ERR_BAD_STATE: return "BAD_STATE";
    case OTA_ERR_BAD_PARAM: return "BAD_PARAM";
    case OTA_ERR_FLASH_FAIL: return "FLASH_FAIL";
    case OTA_ERR_SIZE_OVERFLOW: return "SIZE_OVERFLOW";
    case OTA_ERR_CRC_FAIL: return "CRC_FAIL";
    case OTA_ERR_CAN_TIMEOUT: return "CAN_TIMEOUT";
    case OTA_ERR_CAN_NACK: return "CAN_NACK";
    case OTA_ERR_NODE_NOT_READY: return "NODE_NOT_READY";
    case OTA_ERR_BOOT_FAIL: return "BOOT_FAIL";
    case OTA_ERR_APP_NOT_CONFIRMED: return "APP_NOT_CONFIRMED";
    case OTA_ERR_BLOCK_MISSING: return "BLOCK_MISSING";
    case OTA_ERR_BAD_MANIFEST: return "BAD_MANIFEST";
    case OTA_ERR_UNSUPPORTED_IMAGE: return "UNSUPPORTED_IMAGE";
    default: return "UNKNOWN";
    }
}

const char *OTA_StateName(uint8_t state)
{
    switch ((OTA_State_t)state) {
    case OTA_STATE_IDLE: return "IDLE";
    case OTA_STATE_RECEIVING: return "RECEIVING";
    case OTA_STATE_VERIFYING: return "VERIFYING";
    case OTA_STATE_READY: return "READY";
    case OTA_STATE_DISTRIBUTING: return "DISTRIBUTING";
    case OTA_STATE_WAIT_ALL_READY: return "WAIT_ALL_READY";
    case OTA_STATE_COMMIT_PENDING: return "COMMIT_PENDING";
    case OTA_STATE_BOOT_UPDATING: return "BOOT_UPDATING";
    case OTA_STATE_CONFIRMED: return "CONFIRMED";
    case OTA_STATE_ABORTED: return "ABORTED";
    case OTA_STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}
