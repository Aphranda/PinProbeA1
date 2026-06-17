/*
 * ota_manifest.h
 *
 * OTA image metadata, flash layout, and boot flag definitions.
 */

#ifndef APP_OTA_INC_OTA_MANIFEST_H_
#define APP_OTA_INC_OTA_MANIFEST_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal Flash layout for OTA phase. */
#define OTA_APP_BASE_ADDR              0x08006000UL
#define OTA_APP_MAX_SIZE               (230UL * 1024UL)
#define OTA_CONFIG_BASE_ADDR           0x0803F800UL
#define OTA_CONFIG_SIZE                0x800UL

/* W25Q128 global layout shared with RamVector snapshots and logs. */
#define OTA_EXT_BOOT_INFO_ADDR         0x000000UL
#define OTA_EXT_BOOT_INFO_SIZE         0x001000UL
#define OTA_EXT_SNAPSHOT_ACTIVE_ADDR   0x001000UL
#define OTA_EXT_SNAPSHOT_ACTIVE_SIZE   0x002000UL
#define OTA_EXT_SNAPSHOT_BACKUP_ADDR   0x003000UL
#define OTA_EXT_SNAPSHOT_BACKUP_SIZE   0x002000UL
#define OTA_EXT_EVENT_LOG_ADDR         0x005000UL
#define OTA_EXT_EVENT_LOG_SIZE         0x010000UL
#define OTA_EXT_AREA_ADDR              0x015000UL
#define OTA_EXT_AREA_SIZE              0x400000UL

#define OTA_METADATA_A_ADDR            (OTA_EXT_AREA_ADDR + 0x0000UL)
#define OTA_METADATA_B_ADDR            (OTA_EXT_AREA_ADDR + 0x1000UL)
#define OTA_BOOT_FLAGS_ADDR            (OTA_EXT_AREA_ADDR + 0x2000UL)
#define OTA_CONTROL_LOG_ADDR           (OTA_EXT_AREA_ADDR + 0x3000UL)
#define OTA_SLOT_A_ADDR                (OTA_EXT_AREA_ADDR + 0x4000UL)
#define OTA_SLOT_B_ADDR                (OTA_SLOT_A_ADDR + OTA_SLOT_SIZE)
#define OTA_SLOT_SIZE                  0x80000UL

#define OTA_SCPI_CHUNK_SIZE            128U
#define OTA_MAX_NODES                  8U

#define OTA_MANIFEST_MAGIC             0x504F5441UL  /* "POTA" */
#define OTA_MANIFEST_VERSION           0x00010000UL
#define OTA_BOOT_FLAGS_MAGIC           0x504F5446UL  /* "POTF" */
#define OTA_PRODUCT_ID_PINPROBE_A1     0x50415031UL  /* "PAP1" */
#define OTA_IMAGE_TYPE_APP             1UL

typedef enum {
    OTA_OK = 0,
    OTA_ERR_BUSY = 1,
    OTA_ERR_BAD_STATE = 2,
    OTA_ERR_BAD_PARAM = 3,
    OTA_ERR_FLASH_FAIL = 4,
    OTA_ERR_SIZE_OVERFLOW = 5,
    OTA_ERR_CRC_FAIL = 6,
    OTA_ERR_CAN_TIMEOUT = 7,
    OTA_ERR_CAN_NACK = 8,
    OTA_ERR_NODE_NOT_READY = 9,
    OTA_ERR_BOOT_FAIL = 10,
    OTA_ERR_APP_NOT_CONFIRMED = 11,
    OTA_ERR_BLOCK_MISSING = 12,
    OTA_ERR_BAD_MANIFEST = 13,
    OTA_ERR_UNSUPPORTED_IMAGE = 14,
} OTA_Error_t;

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_RECEIVING,
    OTA_STATE_VERIFYING,
    OTA_STATE_READY,
    OTA_STATE_DISTRIBUTING,
    OTA_STATE_WAIT_ALL_READY,
    OTA_STATE_COMMIT_PENDING,
    OTA_STATE_BOOT_UPDATING,
    OTA_STATE_CONFIRMED,
    OTA_STATE_ABORTED,
    OTA_STATE_ERROR,
} OTA_State_t;

typedef enum {
    OTA_BOOT_STATE_IDLE = 0,
    OTA_BOOT_STATE_PENDING,
    OTA_BOOT_STATE_WRITING,
    OTA_BOOT_STATE_WRITTEN,
    OTA_BOOT_STATE_CONFIRMED,
    OTA_BOOT_STATE_ROLLBACK_REQUIRED,
    OTA_BOOT_STATE_FAILED,
} OTA_BootState_t;

typedef enum {
    OTA_SLOT_NONE = 0,
    OTA_SLOT_A = 1,
    OTA_SLOT_B = 2,
} OTA_SlotId_t;

typedef struct {
    uint32_t magic;
    uint32_t manifest_version;
    uint32_t sequence;

    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t image_version;
    uint32_t image_build_id;

    uint32_t product_id;
    uint32_t hw_rev_mask;
    uint32_t image_type;
    uint32_t bootloader_min_version;

    uint32_t app_base_addr;
    uint32_t app_max_size;
    uint32_t slot_addr;
    uint32_t slot_size;

    uint8_t  source_node;
    uint8_t  target_mask;
    uint8_t  ready_mask;
    uint8_t  error_node;

    uint8_t  state;
    uint8_t  flags;
    uint16_t block_size;

    uint32_t received_size;
    uint32_t verified_size;
    uint32_t last_error;

    uint8_t  reserved[64];
    uint32_t manifest_crc32;
} OTA_Manifest_t;

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint8_t  pending_slot;
    uint8_t  previous_slot;
    uint8_t  update_state;
    uint8_t  max_attempts;
    uint8_t  attempt_count;
    uint8_t  reserved0[3];
    uint32_t last_error;
    uint32_t active_manifest_sequence;
    uint32_t reserved[8];
    uint32_t flags_crc32;
} OTA_BootFlags_t;

uint32_t OTA_Crc32(const void *data, size_t length);
void OTA_ManifestInit(OTA_Manifest_t *manifest);
bool OTA_ManifestFinalize(OTA_Manifest_t *manifest);
bool OTA_ManifestValidate(const OTA_Manifest_t *manifest);
void OTA_BootFlagsInit(OTA_BootFlags_t *flags);
bool OTA_BootFlagsFinalize(OTA_BootFlags_t *flags);
bool OTA_BootFlagsValidate(const OTA_BootFlags_t *flags);
uint32_t OTA_SlotAddress(uint8_t slot_id);
const char *OTA_ErrorName(uint32_t error_code);
const char *OTA_StateName(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* APP_OTA_INC_OTA_MANIFEST_H_ */
