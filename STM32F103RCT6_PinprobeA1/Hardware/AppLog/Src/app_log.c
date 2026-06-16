/*
 * app_log.c - local ring-buffer diagnostics for state-machine events.
 *
 * The log is intentionally not stored in RamVector. RamVector remains a fast
 * reflected state table; AppLog is a best-effort diagnostic stream.
 */

#include "app_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "tim.h"
#include <stdio.h>

static AppLog_Table_t log_table __attribute__((section("AppLog"), aligned(4)));
static AppLog_Sink_t log_sink;

static uint16_t clamp_u16(uint32_t value)
{
    return (value > 0xFFFFU) ? 0xFFFFU : (uint16_t)value;
}

static const char *state_name(uint8_t state)
{
    switch (state) {
    case 0: return "LOCK";
    case 1: return "IDLE";
    case 2: return "READY";
    case 3: return "RUNNING";
    case 4: return "EMERGENCY";
    case 5: return "COMPLETE";
    case 6: return "INIT";
    case 0xFF: return "START";
    default: return "STATE?";
    }
}

static const char *action_name(uint8_t action)
{
    switch (action) {
    case APPLOG_ACT_LOCK: return "LOCK";
    case APPLOG_ACT_UNLOCK: return "UNLOCK";
    case APPLOG_ACT_CLOSE_START: return "CLOSE_START";
    case APPLOG_ACT_CLOSE_DONE: return "CLOSE_DONE";
    case APPLOG_ACT_OPEN_START: return "OPEN_START";
    case APPLOG_ACT_OPEN_DONE: return "OPEN_DONE";
    case APPLOG_ACT_CLOSE_PHASE: return "CLOSE_PHASE";
    case APPLOG_ACT_CLOSE_MARK: return "CLOSE_MARK";
    case APPLOG_ACT_CYLINDER_OPEN: return "CYLINDER_OPEN";
    case APPLOG_ACT_CYLINDER_CLOSE: return "CYLINDER_CLOSE";
    case APPLOG_ACT_LED_OFF: return "LED_OFF";
    case APPLOG_ACT_LED_GREEN: return "LED_GREEN";
    case APPLOG_ACT_LED_RED: return "LED_RED";
    case APPLOG_ACT_LED_YELLOW: return "LED_YELLOW";
    default: return "ACTION?";
    }
}

static const char *event_name(uint8_t event)
{
    switch (event) {
    case APPLOG_EVT_RS485_FAULT: return "RS485_FAULT";
    case APPLOG_EVT_RS485_RECOVERED: return "RS485_RECOVERED";
    case APPLOG_EVT_ESTOP: return "ESTOP";
    case APPLOG_EVT_LASER: return "LASER";
    case APPLOG_EVT_RISK_PRESSURE: return "RISK_PRESSURE";
    case APPLOG_EVT_AIR_LOW: return "AIR_LOW";
    case APPLOG_EVT_POWER_BTN: return "POWER_BTN";
    case APPLOG_EVT_DOOR_BTN: return "DOOR_BTN";
    case APPLOG_EVT_SCPI_LOCK: return "SCPI_LOCK";
    case APPLOG_EVT_SCPI_CYLINDER: return "SCPI_CYLINDER";
    case APPLOG_EVT_SCPI_LED: return "SCPI_LED";
    default: return "EVENT?";
    }
}

static const char *level_name(uint8_t level)
{
    switch (level) {
    case APPLOG_LEVEL_INFO: return "INFO";
    case APPLOG_LEVEL_WARN: return "WARN";
    case APPLOG_LEVEL_ERROR: return "ERROR";
    default: return "LVL?";
    }
}

static void push_record(uint8_t level, uint8_t module, uint8_t event_id,
                        uint16_t arg0, uint16_t arg1)
{
    AppLog_Record_t record;

    record.tick = GetTim1Ms();
    record.node_id = log_table.node_id;
    record.level = level;
    record.module = module;
    record.event_id = event_id;
    record.arg0 = arg0;
    record.arg1 = arg1;
    record.reserved = 0;

    taskENTER_CRITICAL();
    record.seq = ++log_table.next_seq;
    log_table.records[log_table.write_index] = record;
    log_table.write_index = (uint16_t)((log_table.write_index + 1U) % APPLOG_RECORD_CAPACITY);
    if (log_table.read_count == APPLOG_RECORD_CAPACITY) {
        log_table.read_index = (uint16_t)((log_table.read_index + 1U) % APPLOG_RECORD_CAPACITY);
        log_table.drop_count++;
    } else {
        log_table.read_count++;
    }

    if (log_table.realtime_enabled) {
        if (log_table.realtime_count == APPLOG_RECORD_CAPACITY) {
            log_table.realtime_index = (uint16_t)((log_table.realtime_index + 1U) % APPLOG_RECORD_CAPACITY);
            log_table.realtime_drop_count++;
        } else {
            log_table.realtime_count++;
        }
    }
    taskEXIT_CRITICAL();
}

void AppLog_Init(uint8_t node_id)
{
    log_table.magic = APPLOG_MAGIC;
    log_table.version = APPLOG_VERSION;
    log_table.capacity = APPLOG_RECORD_CAPACITY;
    log_table.write_index = 0;
    log_table.read_index = 0;
    log_table.read_count = 0;
    log_table.realtime_index = 0;
    log_table.realtime_count = 0;
    log_table.next_seq = 0;
    log_table.node_id = node_id;
    log_table.realtime_enabled = 0;
    log_table.reserved[0] = 0;
    log_table.reserved[1] = 0;
    log_table.drop_count = 0;
    log_table.realtime_drop_count = 0;
    log_table.end_magic = APPLOG_END_MAGIC;
    log_sink = NULL;
}

AppLog_Table_t *AppLog_GetTable(void)
{
    return &log_table;
}

void AppLog_SetNodeId(uint8_t node_id)
{
    log_table.node_id = node_id;
}

void AppLog_Clear(void)
{
    taskENTER_CRITICAL();
    log_table.magic = APPLOG_MAGIC;
    log_table.version = APPLOG_VERSION;
    log_table.capacity = APPLOG_RECORD_CAPACITY;
    log_table.write_index = 0;
    log_table.read_index = 0;
    log_table.read_count = 0;
    log_table.realtime_index = 0;
    log_table.realtime_count = 0;
    log_table.reserved[0] = 0;
    log_table.reserved[1] = 0;
    log_table.drop_count = 0;
    log_table.realtime_drop_count = 0;
    log_table.end_magic = APPLOG_END_MAGIC;
    taskEXIT_CRITICAL();
}

void AppLog_SetRealtime(bool enabled)
{
    taskENTER_CRITICAL();
    log_table.realtime_enabled = enabled ? 1U : 0U;
    log_table.realtime_index = log_table.write_index;
    log_table.realtime_count = 0;
    taskEXIT_CRITICAL();
}

bool AppLog_IsRealtimeEnabled(void)
{
    return (log_table.realtime_enabled != 0U);
}

void AppLog_SetSink(AppLog_Sink_t sink)
{
    log_sink = sink;
}

uint16_t AppLog_PumpRealtime(uint16_t max_records)
{
    uint16_t sent = 0;

    while (sent < max_records) {
        AppLog_Record_t record;

        if (log_table.realtime_enabled == 0U || log_sink == NULL) {
            break;
        }

        taskENTER_CRITICAL();
        if (log_table.realtime_count == 0U) {
            taskEXIT_CRITICAL();
            break;
        }
        record = log_table.records[log_table.realtime_index];
        taskEXIT_CRITICAL();

        if (!log_sink(&record)) {
            break;
        }

        taskENTER_CRITICAL();
        if (log_table.realtime_count > 0U) {
            log_table.realtime_index = (uint16_t)((log_table.realtime_index + 1U) % APPLOG_RECORD_CAPACITY);
            log_table.realtime_count--;
        }
        taskEXIT_CRITICAL();
        sent++;
    }

    return sent;
}

void AppLog_State(uint8_t old_state, uint8_t new_state, uint32_t elapsed_ms)
{
    push_record(APPLOG_LEVEL_INFO, APPLOG_MODULE_STATE, new_state,
                old_state, clamp_u16(elapsed_ms));
}

void AppLog_Action(uint8_t action_id, uint32_t elapsed_ms, uint16_t arg)
{
    push_record(APPLOG_LEVEL_INFO, APPLOG_MODULE_ACTION, action_id,
                clamp_u16(elapsed_ms), arg);
}

void AppLog_Event(uint8_t event_id, uint32_t arg0, uint32_t arg1)
{
    uint8_t level = (event_id == APPLOG_EVT_RS485_RECOVERED) ?
                    APPLOG_LEVEL_INFO : APPLOG_LEVEL_WARN;
    push_record(level, APPLOG_MODULE_EVENT, event_id,
                clamp_u16(arg0), clamp_u16(arg1));
}

void AppLog_IO(uint8_t in_lo, uint8_t in_hi, uint8_t out_lo, uint8_t out_hi)
{
    push_record(APPLOG_LEVEL_INFO, APPLOG_MODULE_IO, 0,
                (uint16_t)(((uint16_t)in_lo << 8) | in_hi),
                (uint16_t)(((uint16_t)out_lo << 8) | out_hi));
}

bool AppLog_Read(AppLog_Record_t *out)
{
    if (out == NULL) {
        return false;
    }

    taskENTER_CRITICAL();
    if (log_table.read_count == 0U) {
        taskEXIT_CRITICAL();
        return false;
    }

    *out = log_table.records[log_table.read_index];
    log_table.read_index = (uint16_t)((log_table.read_index + 1U) % APPLOG_RECORD_CAPACITY);
    log_table.read_count--;
    taskEXIT_CRITICAL();
    return true;
}

void AppLog_GetStatus(AppLog_Status_t *out)
{
    if (out == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    out->pending = log_table.read_count;
    out->capacity = log_table.capacity;
    out->next_seq = log_table.next_seq;
    out->drop_count = log_table.drop_count;
    out->realtime_drop_count = log_table.realtime_drop_count;
    out->realtime_enabled = (log_table.realtime_enabled != 0U);
    taskEXIT_CRITICAL();
}

uint32_t AppLog_GetDropCount(void)
{
    uint32_t count;

    taskENTER_CRITICAL();
    count = log_table.drop_count;
    taskEXIT_CRITICAL();
    return count;
}

size_t AppLog_Format(const AppLog_Record_t *record, char *buffer, size_t buffer_size)
{
    int len;
    unsigned long tick_s;
    unsigned long tick_ms;

    if (record == NULL || buffer == NULL || buffer_size == 0U) {
        return 0U;
    }

    tick_s = (unsigned long)(record->tick / 1000UL);
    tick_ms = (unsigned long)(record->tick % 1000UL);

    switch (record->module) {
    case APPLOG_MODULE_STATE:
        len = snprintf(buffer, buffer_size,
                       "[T+%lu.%03lus][N%u][%s][STATE] %s -> %s %u ms",
                       tick_s,
                       tick_ms,
                       record->node_id,
                       level_name(record->level),
                       state_name((uint8_t)record->arg0),
                       state_name(record->event_id),
                       record->arg1);
        break;
    case APPLOG_MODULE_ACTION:
        switch (record->event_id) {
        case APPLOG_ACT_CYLINDER_OPEN:
        case APPLOG_ACT_CYLINDER_CLOSE:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][ACTION] %s cyl=%u",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           action_name(record->event_id),
                           record->arg1);
            break;
        case APPLOG_ACT_LOCK:
        case APPLOG_ACT_UNLOCK:
        case APPLOG_ACT_LED_OFF:
        case APPLOG_ACT_LED_GREEN:
        case APPLOG_ACT_LED_RED:
        case APPLOG_ACT_LED_YELLOW:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][ACTION] %s",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           action_name(record->event_id));
            break;
        case APPLOG_ACT_CLOSE_PHASE:
        case APPLOG_ACT_CLOSE_MARK:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][ACTION] %s elapsed=%ums mark=%u",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           action_name(record->event_id),
                           record->arg0,
                           record->arg1);
            break;
        default:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][ACTION] %s elapsed=%ums",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           action_name(record->event_id),
                           record->arg0);
            break;
        }
        break;
    case APPLOG_MODULE_EVENT:
        switch (record->event_id) {
        case APPLOG_EVT_SCPI_CYLINDER:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][EVENT] %s cyl=%u cmd=%s",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           event_name(record->event_id),
                           record->arg0,
                           (record->arg1 == 0U) ? "CLOSE" : "OPEN");
            break;
        case APPLOG_EVT_SCPI_LOCK:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][EVENT] %s cmd=%s",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           event_name(record->event_id),
                           (record->arg0 == 0U) ? "UNLOCK" : "LOCKED");
            break;
        case APPLOG_EVT_SCPI_LED:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][EVENT] %s cmd=%u",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           event_name(record->event_id),
                           record->arg0);
            break;
        case APPLOG_EVT_POWER_BTN:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][EVENT] %s pressed",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           event_name(record->event_id));
            break;
        case APPLOG_EVT_DOOR_BTN:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][EVENT] %s mask=0x%02X state=%s",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           event_name(record->event_id),
                           record->arg0,
                           state_name((uint8_t)record->arg1));
            break;
        default:
            len = snprintf(buffer, buffer_size,
                           "[T+%lu.%03lus][N%u][%s][EVENT] %s arg0=%u arg1=%u",
                           tick_s,
                           tick_ms,
                           record->node_id,
                           level_name(record->level),
                           event_name(record->event_id),
                           record->arg0,
                           record->arg1);
            break;
        }
        break;
    case APPLOG_MODULE_IO:
        len = snprintf(buffer, buffer_size,
                       "[T+%lu.%03lus][N%u][%s][IO] IN:0x%02X,0x%02X OUT:0x%02X,0x%02X",
                       tick_s,
                       tick_ms,
                       record->node_id,
                       level_name(record->level),
                       (unsigned int)(record->arg0 >> 8),
                       (unsigned int)(record->arg0 & 0xFFU),
                       (unsigned int)(record->arg1 >> 8),
                       (unsigned int)(record->arg1 & 0xFFU));
        break;
    default:
        len = snprintf(buffer, buffer_size,
                       "[T+%lu.%03lus][N%u][%s][?] module=%u event=%u arg0=%u arg1=%u",
                       tick_s,
                       tick_ms,
                       record->node_id,
                       level_name(record->level),
                       record->module,
                       record->event_id,
                       record->arg0,
                       record->arg1);
        break;
    }

    if (len < 0) {
        buffer[0] = '\0';
        return 0U;
    }
    if ((size_t)len >= buffer_size) {
        return buffer_size - 1U;
    }
    return (size_t)len;
}
