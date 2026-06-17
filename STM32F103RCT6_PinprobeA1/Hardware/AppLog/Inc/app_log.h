/*
 * app_log.h - structured diagnostic log, kept outside RamVector.
 */

#ifndef APP_LOG_H_
#define APP_LOG_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ===== LOG RAM table layout: 0x2000A400..0x2000A8FF, independent of RamVector ===== */
#define APPLOG_BASE_ADDR       0x2000A400UL
#define APPLOG_SIZE            0x00000500U
#define APPLOG_RECORD_CAPACITY 76U
#define APPLOG_MAGIC           0x504C4F47UL  /* "PLOG" */
#define APPLOG_END_MAGIC       0x454E4400UL  /* "END\0" */
#define APPLOG_VERSION         1U

typedef enum {
    APPLOG_LEVEL_INFO  = 1,
    APPLOG_LEVEL_WARN  = 2,
    APPLOG_LEVEL_ERROR = 3,
} AppLog_Level_t;

typedef enum {
    APPLOG_MODULE_STATE  = 1,
    APPLOG_MODULE_ACTION = 2,
    APPLOG_MODULE_EVENT  = 3,
    APPLOG_MODULE_IO     = 4,
} AppLog_Module_t;

typedef enum {
    APPLOG_ACT_LOCK = 1,
    APPLOG_ACT_UNLOCK,
    APPLOG_ACT_CLOSE_START,
    APPLOG_ACT_CLOSE_DONE,
    APPLOG_ACT_OPEN_START,
    APPLOG_ACT_OPEN_DONE,
    APPLOG_ACT_CLOSE_PHASE,
    APPLOG_ACT_CLOSE_MARK,
    APPLOG_ACT_CYLINDER_OPEN,
    APPLOG_ACT_CYLINDER_CLOSE,
    APPLOG_ACT_LED_OFF,
    APPLOG_ACT_LED_GREEN,
    APPLOG_ACT_LED_RED,
    APPLOG_ACT_LED_YELLOW,
} AppLog_ActionId_t;

typedef enum {
    APPLOG_EVT_RS485_FAULT = 1,
    APPLOG_EVT_RS485_RECOVERED,
    APPLOG_EVT_ESTOP,
    APPLOG_EVT_LASER,
    APPLOG_EVT_RISK_PRESSURE,
    APPLOG_EVT_AIR_LOW,
    APPLOG_EVT_POWER_BTN,
    APPLOG_EVT_DOOR_BTN,
    APPLOG_EVT_SCPI_LOCK,
    APPLOG_EVT_SCPI_CYLINDER,
    APPLOG_EVT_SCPI_LED,
    APPLOG_EVT_IO_WRITE_FAIL,
} AppLog_EventId_t;

typedef struct {
    uint32_t tick;
    uint16_t seq;
    uint8_t  node_id;
    uint8_t  level;
    uint8_t  module;
    uint8_t  event_id;
    uint16_t arg0;
    uint16_t arg1;
    uint16_t reserved;
} AppLog_Record_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t capacity;
    uint16_t write_index;
    uint16_t read_index;
    uint16_t read_count;
    uint16_t realtime_index;
    uint16_t realtime_count;
    uint16_t next_seq;
    uint8_t  node_id;
    uint8_t  realtime_enabled;
    uint8_t  reserved[2];
    uint32_t drop_count;
    uint32_t realtime_drop_count;
    AppLog_Record_t records[APPLOG_RECORD_CAPACITY];
    uint32_t end_magic;
} AppLog_Table_t;

typedef char AppLog_Record_Size_Check[(sizeof(AppLog_Record_t) == 16U) ? 1 : -1];
typedef char AppLog_Table_Size_Check[(sizeof(AppLog_Table_t) <= APPLOG_SIZE) ? 1 : -1];

typedef struct {
    uint16_t pending;
    uint16_t capacity;
    uint16_t next_seq;
    uint32_t drop_count;
    uint32_t realtime_drop_count;
    bool realtime_enabled;
} AppLog_Status_t;

typedef bool (*AppLog_Sink_t)(const AppLog_Record_t *record);

void AppLog_Init(uint8_t node_id);
AppLog_Table_t *AppLog_GetTable(void);
void AppLog_SetNodeId(uint8_t node_id);
void AppLog_Clear(void);
void AppLog_SetRealtime(bool enabled);
bool AppLog_IsRealtimeEnabled(void);
void AppLog_SetSink(AppLog_Sink_t sink);
uint16_t AppLog_PumpRealtime(uint16_t max_records);

void AppLog_State(uint8_t old_state, uint8_t new_state, uint32_t elapsed_ms);
void AppLog_Action(uint8_t action_id, uint32_t elapsed_ms, uint16_t arg);
void AppLog_Event(uint8_t event_id, uint32_t arg0, uint32_t arg1);
void AppLog_TimedEvent(uint8_t event_id, uint32_t elapsed_ms, uint16_t arg);
void AppLog_IO(uint8_t in_lo, uint8_t in_hi, uint8_t out_lo, uint8_t out_hi);

bool AppLog_Read(AppLog_Record_t *out);
void AppLog_GetStatus(AppLog_Status_t *out);
uint32_t AppLog_GetDropCount(void);

size_t AppLog_Format(const AppLog_Record_t *record, char *buffer, size_t buffer_size);

bool AppLog_UartSink(const AppLog_Record_t *record);

#endif /* APP_LOG_H_ */
