/*
 * ota_manager.h
 *
 * App-side OTA receive manager.
 */

#ifndef APP_OTA_INC_OTA_MANAGER_H_
#define APP_OTA_INC_OTA_MANAGER_H_

#include "ota_manifest.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t state;
    uint32_t image_size;
    uint32_t received_size;
    uint32_t received_blocks;
    uint32_t total_blocks;
    uint32_t image_crc32;
    uint32_t image_version;
    uint32_t image_id;
    uint32_t last_error;
    uint32_t next_offset;
} OTA_Status_t;

void OTA_ManagerInit(void);
OTA_Error_t OTA_Begin(uint32_t size, uint32_t crc32, uint32_t version, uint32_t image_id);
OTA_Error_t OTA_WriteData(uint32_t offset, const uint8_t *data, size_t length, uint32_t *next_offset);
OTA_Error_t OTA_End(void);
OTA_Error_t OTA_Verify(void);
OTA_Error_t OTA_Commit(void);
OTA_Error_t OTA_ConfirmRunningApp(void);
OTA_Error_t OTA_Abort(void);
OTA_Error_t OTA_GetStatus(OTA_Status_t *status);
const OTA_Manifest_t *OTA_GetManifest(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_OTA_INC_OTA_MANAGER_H_ */
