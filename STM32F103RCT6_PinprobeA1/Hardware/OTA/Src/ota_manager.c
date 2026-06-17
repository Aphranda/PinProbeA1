/*
 * ota_manager.c
 *
 * App-side OTA receive manager.
 */

#include "ota_manager.h"

#include "ota_boot_request.h"
#include "w25q128.h"

#include <string.h>

#define OTA_MAX_BLOCKS       ((OTA_APP_MAX_SIZE + OTA_SCPI_CHUNK_SIZE - 1UL) / OTA_SCPI_CHUNK_SIZE)
#define OTA_BITMAP_BYTES     ((OTA_MAX_BLOCKS + 7UL) / 8UL)

static OTA_Manifest_t ota_manifest;
static uint8_t block_bitmap[OTA_BITMAP_BYTES];
static uint32_t received_blocks;
static bool manager_initialized;

static OTA_Error_t write_and_verify_object(uint32_t address, const void *object, size_t size)
{
    uint8_t verify[sizeof(OTA_Manifest_t)];

    if (object == NULL || size == 0U || size > sizeof(verify)) {
        return OTA_ERR_BAD_PARAM;
    }

    W25Q128_Status_t flash_status = W25Q128_EraseSector(address);
    if (flash_status == W25Q128_OK) {
        flash_status = W25Q128_Write(address, object, size);
    }
    if (flash_status == W25Q128_OK) {
        flash_status = W25Q128_Read(address, verify, size);
    }
    if (flash_status != W25Q128_OK) {
        return OTA_ERR_FLASH_FAIL;
    }
    if (memcmp(verify, object, size) != 0) {
        return OTA_ERR_FLASH_FAIL;
    }

    return OTA_OK;
}

static uint32_t next_commit_sequence(void)
{
    OTA_BootFlags_t old_flags;
    if (W25Q128_Read(OTA_BOOT_FLAGS_ADDR, &old_flags, sizeof(old_flags)) == W25Q128_OK &&
        OTA_BootFlagsValidate(&old_flags)) {
        return old_flags.sequence + 1UL;
    }
    return 1UL;
}

static void set_error(OTA_Error_t error)
{
    ota_manifest.last_error = (uint32_t)error;
    if (error != OTA_OK) {
        ota_manifest.state = (uint8_t)OTA_STATE_ERROR;
    }
}

static bool block_is_set(uint32_t block_index)
{
    if (block_index >= OTA_MAX_BLOCKS) {
        return false;
    }
    return (block_bitmap[block_index / 8UL] & (uint8_t)(1U << (block_index % 8UL))) != 0U;
}

static void block_set(uint32_t block_index)
{
    if (block_index < OTA_MAX_BLOCKS && !block_is_set(block_index)) {
        block_bitmap[block_index / 8UL] |= (uint8_t)(1U << (block_index % 8UL));
        received_blocks++;
    }
}

static uint32_t total_blocks_for_size(uint32_t size)
{
    return (size + OTA_SCPI_CHUNK_SIZE - 1UL) / OTA_SCPI_CHUNK_SIZE;
}

static uint32_t erase_size_for_image(uint32_t image_size)
{
    return (image_size + W25Q128_SECTOR_SIZE - 1UL) & ~(W25Q128_SECTOR_SIZE - 1UL);
}

static bool all_blocks_received(void)
{
    return received_blocks >= total_blocks_for_size(ota_manifest.image_size);
}

static uint32_t find_next_missing_offset(void)
{
    uint32_t total_blocks = total_blocks_for_size(ota_manifest.image_size);
    for (uint32_t i = 0UL; i < total_blocks; ++i) {
        if (!block_is_set(i)) {
            return i * OTA_SCPI_CHUNK_SIZE;
        }
    }
    return ota_manifest.image_size;
}

void OTA_ManagerInit(void)
{
    OTA_ManifestInit(&ota_manifest);
    memset(block_bitmap, 0, sizeof(block_bitmap));
    received_blocks = 0UL;
    manager_initialized = true;
}

OTA_Error_t OTA_Begin(uint32_t size, uint32_t crc32, uint32_t version, uint32_t image_id)
{
    if (!manager_initialized) {
        OTA_ManagerInit();
    }

    if (ota_manifest.state != (uint8_t)OTA_STATE_IDLE &&
        ota_manifest.state != (uint8_t)OTA_STATE_ABORTED &&
        ota_manifest.state != (uint8_t)OTA_STATE_ERROR) {
        return OTA_ERR_BUSY;
    }
    if (size == 0UL || size > OTA_APP_MAX_SIZE || size > OTA_SLOT_SIZE) {
        return OTA_ERR_SIZE_OVERFLOW;
    }

    OTA_ManifestInit(&ota_manifest);
    ota_manifest.image_size = size;
    ota_manifest.image_crc32 = crc32;
    ota_manifest.image_version = version;
    ota_manifest.image_build_id = image_id;
    ota_manifest.slot_addr = OTA_SLOT_A_ADDR;
    ota_manifest.slot_size = OTA_SLOT_SIZE;
    ota_manifest.state = (uint8_t)OTA_STATE_RECEIVING;
    ota_manifest.last_error = OTA_OK;
    memset(block_bitmap, 0, sizeof(block_bitmap));
    received_blocks = 0UL;

    W25Q128_Status_t flash_status = W25Q128_Init();
    if (flash_status == W25Q128_OK) {
        flash_status = W25Q128_EraseRange(OTA_SLOT_A_ADDR, erase_size_for_image(size));
    }
    if (flash_status != W25Q128_OK) {
        set_error(OTA_ERR_FLASH_FAIL);
        return OTA_ERR_FLASH_FAIL;
    }

    return OTA_OK;
}

OTA_Error_t OTA_WriteData(uint32_t offset, const uint8_t *data, size_t length, uint32_t *next_offset)
{
    if (!manager_initialized) {
        OTA_ManagerInit();
    }
    if (ota_manifest.state != (uint8_t)OTA_STATE_RECEIVING) {
        return OTA_ERR_BAD_STATE;
    }
    if (data == NULL || length == 0U || length > OTA_SCPI_CHUNK_SIZE) {
        set_error(OTA_ERR_BAD_PARAM);
        return OTA_ERR_BAD_PARAM;
    }
    if (offset >= ota_manifest.image_size ||
        length > (size_t)(ota_manifest.image_size - offset)) {
        set_error(OTA_ERR_SIZE_OVERFLOW);
        return OTA_ERR_SIZE_OVERFLOW;
    }
    if ((offset % OTA_SCPI_CHUNK_SIZE) != 0UL) {
        set_error(OTA_ERR_BAD_PARAM);
        return OTA_ERR_BAD_PARAM;
    }

    uint32_t block_index = offset / OTA_SCPI_CHUNK_SIZE;
    if (!block_is_set(block_index)) {
        W25Q128_Status_t flash_status = W25Q128_Write(OTA_SLOT_A_ADDR + offset, data, length);
        if (flash_status != W25Q128_OK) {
            set_error(OTA_ERR_FLASH_FAIL);
            return OTA_ERR_FLASH_FAIL;
        }
        block_set(block_index);
        ota_manifest.received_size += (uint32_t)length;
    }

    if (next_offset != NULL) {
        *next_offset = find_next_missing_offset();
    }
    return OTA_OK;
}

OTA_Error_t OTA_End(void)
{
    if (!manager_initialized) {
        OTA_ManagerInit();
    }
    if (ota_manifest.state != (uint8_t)OTA_STATE_RECEIVING) {
        return OTA_ERR_BAD_STATE;
    }
    if (!all_blocks_received()) {
        ota_manifest.last_error = OTA_ERR_BLOCK_MISSING;
        return OTA_ERR_BLOCK_MISSING;
    }

    ota_manifest.state = (uint8_t)OTA_STATE_VERIFYING;
    return OTA_OK;
}

OTA_Error_t OTA_Verify(void)
{
    if (!manager_initialized) {
        OTA_ManagerInit();
    }
    if (ota_manifest.state != (uint8_t)OTA_STATE_VERIFYING &&
        ota_manifest.state != (uint8_t)OTA_STATE_READY) {
        return OTA_ERR_BAD_STATE;
    }

    uint8_t buffer[OTA_SCPI_CHUNK_SIZE];
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t remaining = ota_manifest.image_size;
    uint32_t offset = 0UL;

    while (remaining > 0UL) {
        size_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : (size_t)remaining;
        W25Q128_Status_t flash_status = W25Q128_Read(OTA_SLOT_A_ADDR + offset, buffer, chunk);
        if (flash_status != W25Q128_OK) {
            set_error(OTA_ERR_FLASH_FAIL);
            return OTA_ERR_FLASH_FAIL;
        }

        for (size_t i = 0U; i < chunk; ++i) {
            crc ^= buffer[i];
            for (uint8_t bit = 0U; bit < 8U; ++bit) {
                if ((crc & 1UL) != 0UL) {
                    crc = (crc >> 1) ^ 0xEDB88320UL;
                } else {
                    crc >>= 1;
                }
            }
        }

        offset += (uint32_t)chunk;
        remaining -= (uint32_t)chunk;
    }

    crc ^= 0xFFFFFFFFUL;
    ota_manifest.verified_size = ota_manifest.image_size;
    if (crc != ota_manifest.image_crc32) {
        set_error(OTA_ERR_CRC_FAIL);
        return OTA_ERR_CRC_FAIL;
    }

    ota_manifest.state = (uint8_t)OTA_STATE_READY;
    ota_manifest.last_error = OTA_OK;
    OTA_ManifestFinalize(&ota_manifest);
    return OTA_OK;
}

OTA_Error_t OTA_Commit(void)
{
    if (!manager_initialized) {
        OTA_ManagerInit();
    }
    if (ota_manifest.state != (uint8_t)OTA_STATE_READY) {
        return OTA_ERR_BAD_STATE;
    }

    W25Q128_Status_t flash_status = W25Q128_Init();
    if (flash_status != W25Q128_OK) {
        set_error(OTA_ERR_FLASH_FAIL);
        return OTA_ERR_FLASH_FAIL;
    }

    uint32_t sequence = next_commit_sequence();
    ota_manifest.sequence = sequence;
    ota_manifest.state = (uint8_t)OTA_STATE_COMMIT_PENDING;
    ota_manifest.last_error = OTA_OK;
    OTA_ManifestFinalize(&ota_manifest);
    if (!OTA_ManifestValidate(&ota_manifest)) {
        set_error(OTA_ERR_BAD_MANIFEST);
        return OTA_ERR_BAD_MANIFEST;
    }

    OTA_BootFlags_t boot_flags;
    OTA_BootFlagsInit(&boot_flags);
    boot_flags.sequence = sequence;
    boot_flags.pending_slot = (uint8_t)OTA_SLOT_A;
    boot_flags.previous_slot = (uint8_t)OTA_SLOT_NONE;
    boot_flags.update_state = (uint8_t)OTA_BOOT_STATE_PENDING;
    boot_flags.max_attempts = 2U;
    boot_flags.attempt_count = 0U;
    boot_flags.last_error = OTA_OK;
    boot_flags.active_manifest_sequence = sequence;
    OTA_BootFlagsFinalize(&boot_flags);
    if (!OTA_BootFlagsValidate(&boot_flags)) {
        set_error(OTA_ERR_BAD_MANIFEST);
        return OTA_ERR_BAD_MANIFEST;
    }

    OTA_Error_t err = write_and_verify_object(OTA_METADATA_A_ADDR, &ota_manifest, sizeof(ota_manifest));
    if (err == OTA_OK) {
        err = write_and_verify_object(OTA_BOOT_FLAGS_ADDR, &boot_flags, sizeof(boot_flags));
    }
    if (err != OTA_OK) {
        set_error(err);
        return err;
    }

    OTA_BootRequestSet(sequence);
    return OTA_OK;
}

OTA_Error_t OTA_ConfirmRunningApp(void)
{
    OTA_BootFlags_t boot_flags;
    OTA_Manifest_t manifest;

    W25Q128_Status_t flash_status = W25Q128_Init();
    if (flash_status != W25Q128_OK) {
        return OTA_ERR_FLASH_FAIL;
    }
    if (W25Q128_Read(OTA_BOOT_FLAGS_ADDR, &boot_flags, sizeof(boot_flags)) != W25Q128_OK ||
        W25Q128_Read(OTA_METADATA_A_ADDR, &manifest, sizeof(manifest)) != W25Q128_OK) {
        return OTA_ERR_FLASH_FAIL;
    }
    if (!OTA_BootFlagsValidate(&boot_flags)) {
        return OTA_OK;
    }
    if (boot_flags.update_state != (uint8_t)OTA_BOOT_STATE_WRITTEN ||
        boot_flags.pending_slot == (uint8_t)OTA_SLOT_NONE) {
        return OTA_OK;
    }
    if (!OTA_ManifestValidate(&manifest) ||
        boot_flags.active_manifest_sequence != manifest.sequence) {
        return OTA_ERR_BAD_MANIFEST;
    }

    boot_flags.pending_slot = (uint8_t)OTA_SLOT_NONE;
    boot_flags.previous_slot = (uint8_t)OTA_SLOT_A;
    boot_flags.update_state = (uint8_t)OTA_BOOT_STATE_CONFIRMED;
    boot_flags.attempt_count = 0U;
    boot_flags.last_error = OTA_OK;
    OTA_BootFlagsFinalize(&boot_flags);

    OTA_Error_t err = write_and_verify_object(OTA_BOOT_FLAGS_ADDR, &boot_flags, sizeof(boot_flags));
    if (err != OTA_OK) {
        return err;
    }

    ota_manifest = manifest;
    ota_manifest.state = (uint8_t)OTA_STATE_CONFIRMED;
    ota_manifest.last_error = OTA_OK;
    return OTA_OK;
}

OTA_Error_t OTA_Abort(void)
{
    if (!manager_initialized) {
        OTA_ManagerInit();
    }
    ota_manifest.state = (uint8_t)OTA_STATE_ABORTED;
    ota_manifest.last_error = OTA_OK;
    return OTA_OK;
}

OTA_Error_t OTA_GetStatus(OTA_Status_t *status)
{
    if (status == NULL) {
        return OTA_ERR_BAD_PARAM;
    }
    if (!manager_initialized) {
        OTA_ManagerInit();
    }

    memset(status, 0, sizeof(*status));
    status->state = ota_manifest.state;
    status->image_size = ota_manifest.image_size;
    status->received_size = ota_manifest.received_size;
    status->received_blocks = received_blocks;
    status->total_blocks = total_blocks_for_size(ota_manifest.image_size);
    status->image_crc32 = ota_manifest.image_crc32;
    status->image_version = ota_manifest.image_version;
    status->image_id = ota_manifest.image_build_id;
    status->last_error = ota_manifest.last_error;
    status->next_offset = find_next_missing_offset();
    return OTA_OK;
}

OTA_Error_t OTA_GetBootInfo(OTA_BootInfo_t *info)
{
    OTA_BootFlags_t boot_flags;
    OTA_Manifest_t manifest;

    if (info == NULL) {
        return OTA_ERR_BAD_PARAM;
    }
    memset(info, 0, sizeof(*info));

    W25Q128_Status_t flash_status = W25Q128_Init();
    if (flash_status != W25Q128_OK) {
        info->flash_ok = false;
        return OTA_ERR_FLASH_FAIL;
    }
    info->flash_ok = true;

    if (W25Q128_Read(OTA_BOOT_FLAGS_ADDR, &boot_flags, sizeof(boot_flags)) != W25Q128_OK ||
        W25Q128_Read(OTA_METADATA_A_ADDR, &manifest, sizeof(manifest)) != W25Q128_OK) {
        return OTA_ERR_FLASH_FAIL;
    }

    info->flags_valid = OTA_BootFlagsValidate(&boot_flags);
    info->manifest_valid = OTA_ManifestValidate(&manifest);
    if (info->flags_valid) {
        info->sequence = boot_flags.sequence;
        info->active_manifest_sequence = boot_flags.active_manifest_sequence;
        info->pending_slot = boot_flags.pending_slot;
        info->previous_slot = boot_flags.previous_slot;
        info->update_state = boot_flags.update_state;
        info->max_attempts = boot_flags.max_attempts;
        info->attempt_count = boot_flags.attempt_count;
        info->last_error = boot_flags.last_error;
    }
    if (info->manifest_valid) {
        info->manifest_state = manifest.state;
        info->image_size = manifest.image_size;
        info->image_crc32 = manifest.image_crc32;
        info->image_version = manifest.image_version;
        info->image_id = manifest.image_build_id;
    }

    return OTA_OK;
}

const OTA_Manifest_t *OTA_GetManifest(void)
{
    if (!manager_initialized) {
        OTA_ManagerInit();
    }
    return &ota_manifest;
}
