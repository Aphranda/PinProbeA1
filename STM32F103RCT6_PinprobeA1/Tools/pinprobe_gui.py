#!/usr/bin/env python3
"""
PinProbeA1 ─ 单机调试/压力测试 GUI 工具
=========================================
串口直连 → SCPI 命令 → 控制 PinProbeA1 设备
支持状态监控、循环压力测试、延迟统计

依赖: pyserial
  pip install pyserial
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import serial
import serial.tools.list_ports
import threading
import time
import queue
import zlib
import traceback
from datetime import datetime
from collections import deque
from pathlib import Path

# ══════════════════════════════════════════════════════════════════════
# 常量
# ══════════════════════════════════════════════════════════════════════
APP_TITLE = "PinProbeA1 Box Debug Tool"
DEFAULT_LANGUAGE = "en"
LANGUAGE_LABELS = {
    "en": "English",
    "zh": "中文",
}

UI_TEXT = {
    "en": {
        "app_title": "PinProbeA1 Box Debug Tool",
        "menu_language": "Language",
        "language": "Language:",
        "log_frame": "Command Log / Response Output",
        "tab_monitor": "Status Monitor",
        "tab_pressure": "Stress Test",
        "tab_ota": "Firmware Upgrade",
        "tab_custom": "Custom Command",
        "status_ready": "Ready - select a serial port and connect",
        "connected_dot": "● Connected",
        "disconnected_dot": "● Disconnected",
        "connected": "Connected",
        "disconnected": "Disconnected",
        "device_empty": "Device: ---",
        "device_value": "Device: {resp}",
        "serial_settings": "Serial Settings",
        "port": "Port:",
        "baud": "Baud:",
        "refresh": "Refresh",
        "connect": "Connect",
        "disconnect": "Disconnect",
        "query_device": "Query Device",
        "scpi_panel": "SCPI Command Panel",
        "search": "Search:",
        "auto_poll": "Auto Poll",
        "interval_ms": "Interval (ms):",
        "refresh_now": "Refresh Now",
        "poll_stopped": "Polling stopped",
        "poll_running": "Polling running",
        "refreshed": "Refreshed",
        "poll_stopped_by_pressure": "Stress test running; polling stopped",
        "col_status_item": "Status Item",
        "col_current_value": "Current Value",
        "col_query_cmd": "Query Command",
        "col_updated": "Updated",
        "io_indicators": "IO Indicators",
        "refresh_io": "Refresh IO",
        "input": "Input",
        "output": "Output",
        "test_presets": "Test Presets",
        "preset": "Preset:",
        "load_preset": "Load Preset",
        "test_sequence": "Test Command Sequence (loop order)",
        "add": "Add",
        "delete_selected": "Delete Selected",
        "clear": "Clear",
        "test_params": "Test Parameters",
        "cmd_interval": "Command interval (ms):",
        "loop_count": "Loop count (0=infinite):",
        "timeout_ms": "Timeout (ms):",
        "start_pressure": "▶ Start Stress Test",
        "stop": "■ Stop",
        "ready": "Ready",
        "realtime_stats": "Real-time Statistics",
        "firmware_file": "Firmware File",
        "select_file": "Select...",
        "version": "Version:",
        "upload_control": "Upload Control",
        "query_status": "Query Status",
        "query_boot": "Query Boot",
        "start_upload": "Start Upload",
        "full_verify": "Full Verification",
        "abort": "Abort",
        "commit_upgrade": "Commit Upgrade",
        "manual_cmd_input": "Manual Command Input (press Enter to send)",
        "send": "Send",
        "cmd_history": "Command History (click to reuse)",
        "clear_history": "Clear History",
        "clear_log": "Clear Window",
        "clear_device_log": "Clear Device Log",
        "auto_scroll": "Auto Scroll",
        "live_listen": "Real-time Listen",
        "export_log": "Export Log",
        "ota_no_file": "No firmware selected",
        "filetypes_log": "Log files",
        "filetypes_text": "Text files",
        "filetypes_all": "All files",
        "filetypes_bin": "Firmware binary",
        "select_firmware_title": "Select Firmware File",
        "warn": "Warning",
        "no_port": "Select a serial port first",
        "connect_failed": "Connection Failed",
        "read_failed": "Read Failed",
        "not_connected_title": "Not Connected",
        "not_connected_msg": "Connect the serial port first",
        "ota_busy_title": "OTA Busy",
        "ota_busy_msg": "Firmware upload is running. Wait for completion or abort it first.",
        "pressure_busy_title": "Stress Test Running",
        "pressure_busy_msg": "Stress test is running. Stop it first.",
        "no_command_title": "No Command",
        "no_command_msg": "Add at least one test command",
        "no_firmware_title": "No Firmware Selected",
        "no_firmware_msg": "Select a firmware file first",
        "firmware_error_title": "Firmware File Error",
        "param_error_title": "Parameter Error",
        "param_error_msg": "Version and Image ID support decimal or 0x-prefixed hexadecimal",
        "confirm_commit_title": "Confirm Commit",
        "confirm_commit_msg": "Commit upgrade now? The device will write the OTA commit flag, then Bootloader will move the image.",
        "reboot_title": "Reboot Device",
        "reboot_msg": "OTA has been committed. Reboot now to enter Bootloader?",
        "set_title": "Set {label}",
        "set_prompt": "Enter new {label}:",
        "idn_mfg": "Manufacturer",
        "idn_model": "Product Model",
        "idn_serial": "Serial/Date",
        "idn_fw": "Firmware Version",
        "ports_found": "Found {count} serial port(s): {ports}",
        "none": "none",
        "serial_connected": "Serial connected: {port} @ {baud} bps",
        "serial_disconnected": "Serial disconnected",
        "live_listen_state": "Real-time listen {state}",
        "enabled": "enabled",
        "disabled": "disabled",
        "log_exported": "Log exported to: {filename}",
        "firmware_selected": "Firmware selected",
        "firmware_vector_invalid": "Invalid firmware vector table",
        "aborting": "Aborting...",
        "preparing_upload": "Preparing upload...",
        "preparing_verify": "Preparing full verification...",
        "no_response": "(no response)",
        "no_response_needed": "(no response required)",
        "unknown_error": "Unknown error: {err}",
        "serial_not_connected": "Serial port not connected",
        "serial_ota_locked": "OTA upload in progress; serial port is locked",
        "live_listen_cmd": "Real-time Listen",
        "ota_committed": "OTA committed, waiting for reboot",
        "ota_uploading": "Uploading {offset}/{size} bytes ({progress:.1f}%, {speed:.1f} KiB/s)",
        "ota_status_uploading": "OTA uploading {progress:.1f}%",
        "ota_ready": "Firmware uploaded and verified",
        "ota_status_ready": "OTA firmware ready",
        "ota_done": "OTA done: {resp}",
        "ota_failed": "OTA failed: {err}",
        "loaded_preset": "Loaded preset: {name}",
        "pressure_running": "Running...",
        "pressure_stopped": "Stopped",
        "pressure_stopped_log": "Stress test stopped",
        "pressure_start_log": "Stress test started: {count} command(s), interval {interval}ms, loops {loops}",
        "infinite": "infinite",
        "stats_total": "Total: {total}    Success: {success}    Fail: {fail}    Timeout: {timeout}",
        "stats_success": "Success rate: {rate:.2f}%",
        "stats_latency_us": "Latency (us):  avg={avg:.0f}  min={min:.0f}  max={max:.0f}",
        "stats_latency_ms": "Latency (ms):  avg={avg:.2f}  min={min:.2f}  max={max:.2f}",
        "stats_percentiles": "P50={p50:.0f}us  P95={p95:.0f}us  P99={p99:.0f}us",
        "pressure_status": "Running | sent:{total} success:{rate:.1f}% avg latency:{avg:.2f}ms",
        "file_too_small": "File too small; vector table missing",
        "file_too_large": "File exceeds APP maximum size {size} bytes",
        "bad_stack": "Invalid initial stack pointer: 0x{stack:08X}",
        "bad_reset": "Invalid Reset vector: 0x{reset:08X}; confirm this is APP bin, not Factory/Bootloader image",
        "app_vector_ok": "APP vector OK SP=0x{stack:08X} Reset=0x{reset:08X}",
        "user_abort": "User aborted",
        "stage_no_response": "{stage} no response",
        "stage_failed": "{stage} failed: {resp}",
        "bootloader_failed": "Bootloader failed at {stages}: {reasons}",
        "bootloader_missing": "Bootloader stage missing: {stages}",
        "post_missing": "POST pass not received",
        "full_verify_passed": "Full verification passed: {resp}",
        "timeout": "timeout",
        "ota_begin_busy": "BEGIN busy; aborting current runtime state and retrying",
    },
    "zh": {
        "app_title": "PinProbeA1 Box Debug Tool",
        "menu_language": "语言",
        "language": "语言:",
        "log_frame": "命令日志 / 响应输出",
        "tab_monitor": "状态监控",
        "tab_pressure": "压力测试",
        "tab_ota": "固件升级",
        "tab_custom": "自定义命令",
        "status_ready": "就绪 - 请选择串口并连接",
        "connected_dot": "● 已连接",
        "disconnected_dot": "● 未连接",
        "connected": "已连接",
        "disconnected": "未连接",
        "device_empty": "设备: ---",
        "device_value": "设备: {resp}",
        "serial_settings": "串口设置",
        "port": "端口:",
        "baud": "波特率:",
        "refresh": "刷新",
        "connect": "连接",
        "disconnect": "断开",
        "query_device": "查询设备",
        "scpi_panel": "SCPI 命令面板",
        "search": "搜索:",
        "auto_poll": "自动轮询",
        "interval_ms": "间隔(ms):",
        "refresh_now": "立即刷新",
        "poll_stopped": "轮询已停止",
        "poll_running": "轮询运行中",
        "refreshed": "已刷新",
        "poll_stopped_by_pressure": "压力测试中，轮询已停止",
        "col_status_item": "状态项",
        "col_current_value": "当前值",
        "col_query_cmd": "查询命令",
        "col_updated": "更新时间",
        "io_indicators": "IO 指示灯",
        "refresh_io": "刷新IO",
        "input": "输入",
        "output": "输出",
        "test_presets": "测试预设",
        "preset": "预设:",
        "load_preset": "加载预设",
        "test_sequence": "测试命令序列 (顺序循环)",
        "add": "添加",
        "delete_selected": "删除选中",
        "clear": "清空",
        "test_params": "测试参数",
        "cmd_interval": "命令间隔(ms):",
        "loop_count": "循环次数 (0=无限):",
        "timeout_ms": "超时(ms):",
        "start_pressure": "▶ 开始压力测试",
        "stop": "■ 停止",
        "ready": "就绪",
        "realtime_stats": "实时统计",
        "firmware_file": "固件文件",
        "select_file": "选择...",
        "version": "版本:",
        "upload_control": "上传控制",
        "query_status": "查询状态",
        "query_boot": "查询Boot",
        "start_upload": "开始上传",
        "full_verify": "完整验证",
        "abort": "中止",
        "commit_upgrade": "提交升级",
        "manual_cmd_input": "手动命令输入 (按 Enter 发送)",
        "send": "发送",
        "cmd_history": "命令历史 (点击重用)",
        "clear_history": "清空历史",
        "clear_log": "清空窗口",
        "clear_device_log": "清设备日志",
        "auto_scroll": "自动滚动",
        "live_listen": "实时旁听",
        "export_log": "导出日志",
        "ota_no_file": "未选择固件",
        "filetypes_log": "日志文件",
        "filetypes_text": "文本文件",
        "filetypes_all": "所有文件",
        "filetypes_bin": "固件二进制",
        "select_firmware_title": "选择固件文件",
        "warn": "警告",
        "no_port": "请先选择串口",
        "connect_failed": "连接失败",
        "read_failed": "读取失败",
        "not_connected_title": "未连接",
        "not_connected_msg": "请先连接串口",
        "ota_busy_title": "OTA忙",
        "ota_busy_msg": "固件上传正在进行，请等待完成或先中止。",
        "pressure_busy_title": "压力测试中",
        "pressure_busy_msg": "压力测试正在运行，请先停止测试。",
        "no_command_title": "无命令",
        "no_command_msg": "请添加至少一条测试命令",
        "no_firmware_title": "未选择固件",
        "no_firmware_msg": "请先选择固件文件",
        "firmware_error_title": "固件文件错误",
        "param_error_title": "参数错误",
        "param_error_msg": "版本和 Image ID 支持十进制或 0x 前缀十六进制",
        "confirm_commit_title": "确认提交",
        "confirm_commit_msg": "确认提交升级? 设备将写入 OTA 提交标志，下一步由 Bootloader 执行搬运。",
        "reboot_title": "重启设备",
        "reboot_msg": "OTA 已提交。是否立即重启设备进入 Bootloader?",
        "set_title": "设置 {label}",
        "set_prompt": "请输入新的{label}:",
        "idn_mfg": "厂商名",
        "idn_model": "产品型号",
        "idn_serial": "序列号/日期",
        "idn_fw": "固件版本",
        "ports_found": "检测到 {count} 个串口: {ports}",
        "none": "无",
        "serial_connected": "串口已连接: {port} @ {baud} bps",
        "serial_disconnected": "串口已断开",
        "live_listen_state": "实时旁听已{state}",
        "enabled": "开启",
        "disabled": "关闭",
        "log_exported": "日志已导出到: {filename}",
        "firmware_selected": "固件已选择",
        "firmware_vector_invalid": "固件向量表非法",
        "aborting": "正在中止...",
        "preparing_upload": "准备上传...",
        "preparing_verify": "准备完整验证...",
        "no_response": "(无响应)",
        "no_response_needed": "(无需响应)",
        "unknown_error": "未知错误: {err}",
        "serial_not_connected": "串口未连接",
        "serial_ota_locked": "OTA 上传中, 串口已独占",
        "live_listen_cmd": "实时旁听",
        "ota_committed": "OTA 已提交，等待重启",
        "ota_uploading": "上传中 {offset}/{size} bytes ({progress:.1f}%, {speed:.1f} KiB/s)",
        "ota_status_uploading": "OTA 上传中 {progress:.1f}%",
        "ota_ready": "固件已上传并校验通过",
        "ota_status_ready": "OTA 固件已就绪",
        "ota_done": "OTA 完成: {resp}",
        "ota_failed": "OTA 失败: {err}",
        "loaded_preset": "已加载预设: {name}",
        "pressure_running": "运行中...",
        "pressure_stopped": "已停止",
        "pressure_stopped_log": "压力测试已停止",
        "pressure_start_log": "压力测试开始: {count} 条命令, 间隔 {interval}ms, 循环 {loops} 次",
        "infinite": "无限",
        "stats_total": "总发送: {total}    成功: {success}    失败: {fail}    超时: {timeout}",
        "stats_success": "成功率: {rate:.2f}%",
        "stats_latency_us": "延迟 (μs):  平均={avg:.0f}  最小={min:.0f}  最大={max:.0f}",
        "stats_latency_ms": "延迟 (ms):  平均={avg:.2f}  最小={min:.2f}  最大={max:.2f}",
        "stats_percentiles": "P50={p50:.0f}μs  P95={p95:.0f}μs  P99={p99:.0f}μs",
        "pressure_status": "运行中 | 发送:{total} 成功率:{rate:.1f}% 平均延迟:{avg:.2f}ms",
        "file_too_small": "文件太小, 缺少向量表",
        "file_too_large": "文件超过APP最大尺寸 {size} bytes",
        "bad_stack": "首字栈指针非法: 0x{stack:08X}",
        "bad_reset": "Reset向量非法: 0x{reset:08X}; 请确认选择的是APP bin, 不是Factory/Bootloader镜像",
        "app_vector_ok": "APP向量 OK SP=0x{stack:08X} Reset=0x{reset:08X}",
        "user_abort": "用户中止",
        "stage_no_response": "{stage} 无响应",
        "stage_failed": "{stage} failed: {resp}",
        "bootloader_failed": "Bootloader失败阶段 {stages}: {reasons}",
        "bootloader_missing": "Bootloader阶段缺失: {stages}",
        "post_missing": "未收到POST通过",
        "full_verify_passed": "完整验证通过: {resp}",
        "timeout": "超时",
        "ota_begin_busy": "BEGIN忙，先中止当前运行状态并重试",
    },
}
DEFAULT_BAUD = 115200
SCPI_TERMINATOR = "\r\n"
SERIAL_TIMEOUT = 0.5
MIN_POLL_INTERVAL_MS = 500
OTA_CHUNK_SIZE = 128
OTA_ACK_TIMEOUT = 1.0
OTA_MAX_RETRIES = 3
OTA_POST_TIMEOUT = 45.0
MAX_LOG_LINES = 2000
STATS_HISTORY = 1000

OTA_APP_BASE_ADDR = 0x08006000
OTA_APP_MAX_SIZE = 230 * 1024
OTA_SRAM_BASE = 0x20000000
OTA_SRAM_SIZE = 0x0000C000

OTA_BOOT_MARKERS = ("R", "S", "I", "F", "C", "P", "D", "J")
OTA_BOOT_FAIL_MARKERS = ("w", "W", "X", "V", "Q", "N", "s", "i")
OTA_BOOT_FAIL_REASONS = {
    "w": "BootFlags WRITING 写入失败, 未擦APP",
    "W": "APP已写入, 但BootFlags WRITTEN写入失败",
    "X": "Bootloader搬运失败",
    "V": "固件向量表非法, 可能选择了Factory/Bootloader镜像而不是APP bin",
    "Q": "OTA sequence不匹配",
    "N": "BootFlags/Manifest无效",
    "s": "Bootloader SPI初始化失败",
    "i": "W25Q128识别失败",
}
OTA_BOOT_FAIL_REASONS_EN = {
    "w": "BootFlags WRITING write failed; APP not erased",
    "W": "APP written, but BootFlags WRITTEN write failed",
    "X": "Bootloader image move failed",
    "V": "Invalid firmware vector table; APP bin may be mixed with Factory/Bootloader image",
    "Q": "OTA sequence mismatch",
    "N": "Invalid BootFlags/Manifest",
    "s": "Bootloader SPI initialization failed",
    "i": "W25Q128 identification failed",
}

BAUD_RATES = [115200]
SCPI_NO_RESPONSE_COMMANDS = {"*CLS", "*RST", "*WAI"}
SCPI_MULTILINE_COMMANDS = {"READ:LOG:ALL?"}
SCPI_LOG_READ_COMMANDS = {"READ:LOG:NEXT?", "READ:LOG:ALL?"}

# ── SCPI 命令面板定义 ────────────────────────────────────────────────
SCPI_COMMANDS = {
    "系统": [
        ("*IDN?", "*IDN?"),
        ("*RST 复位", "*RST"),
        ("*CLS 清状态", "*CLS"),
        ("*WAI", "*WAI"),
        ("*STB?", "*STB?"),
        ("*OPC?", "*OPC?"),
        ("读错误", "SYSTem:ERRor:NEXT?"),
        ("错误计数", "SYSTem:ERRor:COUNt?"),
        ("固件版本", "SYSTem:VERSion?"),
        ("运行时间", "SYSTem:UPTime?"),
        ("系统重启", "SYSTem:REBoot"),
        ("Flash ID", "SYSTem:FLASH:ID?"),
        ("OTA 状态", "SYSTem:OTA:STATus?"),
        ("OTA Boot状态", "SYSTem:OTA:BOOT?"),
        ("OTA 校验", "SYSTem:OTA:VERify?"),
        ("OTA 中止", "SYSTem:OTA:ABORt"),
        ("BSM波特率115200", "CONFigure:BAUDrate 115200"),
        ("Boot诊断 ON", "CONFigure:BOOT:DIAG ON"),
        ("Boot诊断 OFF", "CONFigure:BOOT:DIAG OFF"),
        ("读Boot诊断", "READ:BOOT:DIAG?"),
    ],
    "门/气缸": [
        ("开门", "CONFigure:CYLInder1 OPEN"),
        ("关门", "CONFigure:CYLInder1 CLOSE"),
        ("读门状态", "READ:CYLInder1:STATe?"),
        ("USB 插入", "CONFigure:CYLInder2 CLOSE"),
        ("USB 拔出", "CONFigure:CYLInder2 OPEN"),
        ("读 USB 状态", "READ:CYLInder2:STATe?"),
        ("USB自动 ON", "CONFigure:USB:AUTO ON"),
        ("USB自动 OFF", "CONFigure:USB:AUTO OFF"),
        ("读USB自动(出厂)", "READ:USB:AUTO?"),
        ("DUT检测 ON", "CONFigure:DUT:AUTO ON"),
        ("DUT检测 OFF", "CONFigure:DUT:AUTO OFF"),
        ("读DUT检测", "READ:DUT:AUTO?"),
        ("读DUT状态", "READ:DUT:STATe?"),
    ],
    "门锁": [
        ("🔓 解锁", "CONFigure:LOCK UNLOCK"),
        ("🔒 锁定", "CONFigure:LOCK LOCKED"),
        ("读锁状态", "READ:LOCK:STATe?"),
    ],
    "LED": [
        ("🟢 绿灯", "CONFigure:LED GREEN"),
        ("🔴 红灯", "CONFigure:LED RED"),
        ("🟡 黄灯", "CONFigure:LED YELLOW"),
        ("⚫ 关灯", "CONFigure:LED OFF"),
        ("读 LED 状态", "READ:LED:STATe?"),
        ("LED映射 G,R,Y", "CONFigure:LED:MAP G,R,Y"),
        ("LED映射 R,G,Y", "CONFigure:LED:MAP R,G,Y"),
        ("LED映射 Y,R,G", "CONFigure:LED:MAP Y,R,G"),
        ("读 LED 映射", "READ:LED:MAP?"),
    ],
    "系统状态": [
        ("读系统状态", "READ:SYSTem:STATe?"),
        ("读全部IO", "READ:IO:ALL?"),
        ("读DUT状态", "READ:DUT:STATe?"),
    ],
    "急停": [
        ("常闭 NC (默认)", "CONFigure:ESTOP:TYPE NC"),
        ("常开 NO", "CONFigure:ESTOP:TYPE NO"),
        ("读急停类型", "READ:ESTOP:TYPE?"),
        ("风险模式 ON", "CONFigure:RISK:MODE ON"),
        ("风险模式 OFF", "CONFigure:RISK:MODE OFF"),
        ("读风险模式", "READ:RISK:MODE?"),
    ],
    "IDN配置": [
        ("🔍 读*IDN?", "*IDN?"),
        ("读厂商", "SYSTem:IDN1?"),
        ("读型号", "SYSTem:IDN2?"),
        ("读序列号", "SYSTem:IDN3?"),
        ("读固件版本", "SYSTem:IDN4?"),
        ("✏ 设厂商...", "__IDN1_SET__"),
        ("✏ 设型号...", "__IDN2_SET__"),
        ("✏ 设序列号...", "__IDN3_SET__"),
        ("✏ 设固件版本...", "__IDN4_SET__"),
    ],
    "调试": [
        ("状态跟踪 ON", "CONFigure:DEBUg:STATe ON"),
        ("状态跟踪 OFF", "CONFigure:DEBUg:STATe OFF"),
        ("读状态跟踪", "READ:DEBUg:STATe?"),
        ("动作耗时 ON", "CONFigure:DEBUg:ACTion ON"),
        ("动作耗时 OFF", "CONFigure:DEBUg:ACTion OFF"),
        ("读动作耗时", "READ:DEBUg:ACTion?"),
        ("事件打印 ON", "CONFigure:DEBUg:EVENt ON"),
        ("事件打印 OFF", "CONFigure:DEBUg:EVENt OFF"),
        ("读事件打印", "READ:DEBUg:EVENt?"),
        ("IO刷屏 ON", "CONFigure:DEBUg:IO ON"),
        ("IO刷屏 OFF", "CONFigure:DEBUg:IO OFF"),
        ("读IO刷屏", "READ:DEBUg:IO?"),
        ("波特率 115200", "CONFigure:BAUDrate 115200"),
    ],
    "日志": [
        ("实时日志 ON", "CONFigure:LOG:UART ON"),
        ("实时日志 OFF", "CONFigure:LOG:UART OFF"),
        ("读实时日志开关", "READ:LOG:UART?"),
        ("读日志状态", "READ:LOG:STATus?"),
        ("读下一条日志", "READ:LOG:NEXT?"),
        ("读全部日志", "READ:LOG:ALL?"),
        ("清空日志", "CONFigure:LOG:CLEar"),
    ],
}

SCPI_CATEGORY_EN = {
    "系统": "System",
    "门/气缸": "Door / Cylinder",
    "门锁": "Lock",
    "LED": "LED",
    "系统状态": "System Status",
    "急停": "E-stop",
    "IDN配置": "IDN Config",
    "调试": "Debug",
    "日志": "Log",
}

SCPI_LABEL_EN = {
    "*RST 复位": "*RST Reset",
    "*CLS 清状态": "*CLS Clear",
    "读错误": "Read Error",
    "错误计数": "Error Count",
    "固件版本": "Firmware Ver.",
    "运行时间": "Uptime",
    "系统重启": "Reboot",
    "OTA 状态": "OTA Status",
    "OTA Boot状态": "OTA Boot Status",
    "OTA 校验": "OTA Verify",
    "OTA 中止": "OTA Abort",
    "BSM波特率115200": "BSM Baud 115200",
    "Boot诊断 ON": "Boot Diag ON",
    "Boot诊断 OFF": "Boot Diag OFF",
    "读Boot诊断": "Read Boot Diag",
    "开门": "Open Door",
    "关门": "Close Door",
    "读门状态": "Read Door State",
    "USB 插入": "USB Insert",
    "USB 拔出": "USB Unplug",
    "读 USB 状态": "Read USB State",
    "USB自动 ON": "USB Auto ON",
    "USB自动 OFF": "USB Auto OFF",
    "读USB自动(出厂)": "Read USB Auto (Factory)",
    "DUT检测 ON": "DUT Check ON",
    "DUT检测 OFF": "DUT Check OFF",
    "读DUT检测": "Read DUT Check",
    "读DUT状态": "Read DUT State",
    "🔓 解锁": "Unlock",
    "🔒 锁定": "Lock",
    "读锁状态": "Read Lock State",
    "🟢 绿灯": "Green",
    "🔴 红灯": "Red",
    "🟡 黄灯": "Yellow",
    "⚫ 关灯": "Off",
    "读 LED 状态": "Read LED State",
    "LED映射 G,R,Y": "LED Map G,R,Y",
    "LED映射 R,G,Y": "LED Map R,G,Y",
    "LED映射 Y,R,G": "LED Map Y,R,G",
    "读 LED 映射": "Read LED Map",
    "读系统状态": "Read System State",
    "读全部IO": "Read All IO",
    "常闭 NC (默认)": "NC (Default)",
    "常开 NO": "NO",
    "读急停类型": "Read E-stop Type",
    "风险模式 ON": "Risk Mode ON",
    "风险模式 OFF": "Risk Mode OFF",
    "读风险模式": "Read Risk Mode",
    "🔍 读*IDN?": "Read *IDN?",
    "读厂商": "Read Manufacturer",
    "读型号": "Read Model",
    "读序列号": "Read Serial",
    "读固件版本": "Read FW Version",
    "✏ 设厂商...": "Set Manufacturer...",
    "✏ 设型号...": "Set Model...",
    "✏ 设序列号...": "Set Serial...",
    "✏ 设固件版本...": "Set FW Version...",
    "状态跟踪 ON": "State Trace ON",
    "状态跟踪 OFF": "State Trace OFF",
    "读状态跟踪": "Read State Trace",
    "动作耗时 ON": "Action Time ON",
    "动作耗时 OFF": "Action Time OFF",
    "读动作耗时": "Read Action Time",
    "事件打印 ON": "Event Print ON",
    "事件打印 OFF": "Event Print OFF",
    "读事件打印": "Read Event Print",
    "IO刷屏 ON": "IO Trace ON",
    "IO刷屏 OFF": "IO Trace OFF",
    "读IO刷屏": "Read IO Trace",
    "波特率 115200": "Baud 115200",
    "实时日志 ON": "Live Log ON",
    "实时日志 OFF": "Live Log OFF",
    "读实时日志开关": "Read Live Log",
    "读日志状态": "Read Log Status",
    "读下一条日志": "Read Next Log",
    "读全部日志": "Read All Logs",
    "清空日志": "Clear Logs",
}

# 自动轮询的状态查询命令组
AUTO_POLL_COMMANDS = [
    ("系统状态", "READ:SYSTem:STATe?"),
    ("全部IO", "READ:IO:ALL?"),
    ("门状态", "READ:CYLInder1:STATe?"),
    ("USB状态", "READ:CYLInder2:STATe?"),
    ("USB自动", "READ:USB:AUTO?"),
    ("DUT检测", "READ:DUT:AUTO?"),
    ("DUT状态", "READ:DUT:STATe?"),
    ("锁状态", "READ:LOCK:STATe?"),
    ("LED状态", "READ:LED:STATe?"),
    ("日志状态", "READ:LOG:STATus?"),
]

AUTO_POLL_LABEL_EN = {
    "系统状态": "System State",
    "全部IO": "All IO",
    "门状态": "Door State",
    "USB状态": "USB State",
    "USB自动": "USB Auto",
    "DUT检测": "DUT Check",
    "DUT状态": "DUT State",
    "锁状态": "Lock State",
    "LED状态": "LED State",
    "日志状态": "Log Status",
}

# IO信号位定义（用于 READ:IO:ALL? 响应解析）
IO_BIT_MAP = {
    # 输入 IN[0]
    ("IN", 0, 0x01): "门上限位(up)",
    ("IN", 0, 0x02): "门下限位(down)",
    ("IN", 0, 0x04): "门中位(mid)",
    ("IN", 0, 0x08): "USB上位",
    ("IN", 0, 0x10): "USB下位",
    ("IN", 0, 0x20): "气压",
    ("IN", 0, 0x40): "激光2",
    ("IN", 0, 0x80): "激光3",
    # 输入 IN[1]
    ("IN", 1, 0x01): "激光4",
    ("IN", 1, 0x02): "关门按钮1",
    ("IN", 1, 0x04): "关门按钮2",
    ("IN", 1, 0x08): "急停按钮(stop)",
    ("IN", 1, 0x10): "电源按钮(power)",
    ("IN", 1, 0x20): "DUT到位",
    # 输出 OUT[0]
    ("OUT", 0, 0x01): "开门(open)",
    ("OUT", 0, 0x02): "关门(close)",
    ("OUT", 0, 0x04): "USB插入",
    ("OUT", 0, 0x08): "USB拔出",
    ("OUT", 0, 0x10): "绿灯(G)",
    ("OUT", 0, 0x20): "红灯(R)",
    ("OUT", 0, 0x40): "黄灯(Y)",
    ("OUT", 0, 0x80): "电源输出(power)",
}

IO_LABEL_EN = {
    "门上限位(up)": "Door upper (up)",
    "门下限位(down)": "Door lower (down)",
    "门中位(mid)": "Door middle (mid)",
    "USB上位": "USB upper",
    "USB下位": "USB lower",
    "气压": "Air pressure",
    "激光2": "Laser 2",
    "激光3": "Laser 3",
    "激光4": "Laser 4",
    "关门按钮1": "Close button 1",
    "关门按钮2": "Close button 2",
    "急停按钮(stop)": "E-stop button",
    "电源按钮(power)": "Power button",
    "DUT到位": "DUT in-position",
    "开门(open)": "Open door",
    "关门(close)": "Close door",
    "USB插入": "USB insert",
    "USB拔出": "USB unplug",
    "绿灯(G)": "Green light (G)",
    "红灯(R)": "Red light (R)",
    "黄灯(Y)": "Yellow light (Y)",
    "电源输出(power)": "Power output",
}

def parse_io_response(response: str) -> dict:
    """解析 READ:IO:ALL? 响应，返回 {('IN',0): val, ...} 和原始字节"""
    result = {}
    try:
        # 格式: "IN:0xHH,0xHH OUT:0xHH,0xHH"
        parts = response.split()
        for part in parts:
            if ':' not in part:
                continue
            label, hexpair = part.split(':', 1)
            bytes_str = hexpair.split(',')
            for i, bs in enumerate(bytes_str):
                if bs.startswith('0x') or bs.startswith('0X'):
                    val = int(bs, 16)
                else:
                    val = int(bs, 16) if all(c in '0123456789ABCDEFabcdef' for c in bs) else 0
                result[(label, i)] = val
    except (ValueError, IndexError):
        pass
    return result

def format_io_status(response: str, lang: str = DEFAULT_LANGUAGE) -> list[str]:
    """将 READ:IO:ALL? 响应格式化为形象化文本行列表"""
    data = parse_io_response(response)
    if not data:
        return [response]  # 解析失败返回原始文本

    lines = []
    for (io_type, byte_idx), val in sorted(data.items()):
        lines.append(f"══ {io_type}[{byte_idx}] = 0x{val:02X} ══")
        for (t, b, mask), name in sorted(IO_BIT_MAP.items(), key=lambda x: x[0][2], reverse=True):
            if t == io_type and b == byte_idx:
                state = "● ON " if (val & mask) else "○ off"
                display_name = IO_LABEL_EN.get(name, name) if lang == "en" else name
                lines.append(f"  {state}  {display_name}")
    return lines

# 压力测试预设
PRESSURE_PRESETS = {
    "系统状态轮询": {
        "commands": ["READ:SYSTem:STATe?"],
        "interval_ms": 100,
        "description": "高频轮询系统状态, 测试 SCPI 响应稳定性"
    },
    "全状态轮询": {
        "commands": [
            "READ:SYSTem:STATe?",
            "READ:CYLInder1:STATe?",
            "READ:CYLInder2:STATe?",
            "READ:DUT:AUTO?",
            "READ:DUT:STATe?",
            "READ:LOCK:STATe?",
            "READ:LED:STATe?",
        ],
        "interval_ms": 200,
        "description": "轮询全部状态, 测试多命令序列"
    },
    "LED 闪烁": {
        "commands": [
            "CONFigure:LED GREEN",
            "CONFigure:LED RED",
            "CONFigure:LED YELLOW",
            "CONFigure:LED OFF",
        ],
        "interval_ms": 500,
        "description": "循环切换 LED 颜色, 测试写命令"
    },
    "开关门循环": {
        "commands": [
            "CONFigure:CYLInder1 OPEN",
            "CONFigure:CYLInder1 CLOSE",
        ],
        "interval_ms": 1000,
        "description": "循环开关门, 测试气缸控制 (注意安全!)"
    },
    "随机混合": {
        "commands": [
            "READ:SYSTem:STATe?",
            "READ:CYLInder1:STATe?",
            "READ:CYLInder2:STATe?",
            "READ:DUT:STATe?",
            "READ:LOCK:STATe?",
            "CONFigure:LED GREEN",
            "CONFigure:LED RED",
            "CONFigure:LED YELLOW",
            "CONFigure:LED OFF",
        ],
        "interval_ms": 150,
        "description": "混合读写命令, 模拟真实负载"
    },
    "日志链路": {
        "commands": [
            "READ:LOG:STATus?",
            "READ:LOG:NEXT?",
            "READ:DEBUg:STATe?",
            "READ:DEBUg:ACTion?",
            "READ:DEBUg:EVENt?",
            "READ:DEBUg:IO?",
        ],
        "interval_ms": 250,
        "description": "轮询日志与调试开关, 测试新增诊断命令"
    },
}

PRESSURE_PRESET_EN = {
    "系统状态轮询": {
        "name": "System State Poll",
        "description": "High-rate system-state polling; tests SCPI response stability",
    },
    "全状态轮询": {
        "name": "Full State Poll",
        "description": "Poll all states; tests multi-command sequence",
    },
    "LED 闪烁": {
        "name": "LED Blink",
        "description": "Cycle LED colors; tests write commands",
    },
    "开关门循环": {
        "name": "Door Open/Close Loop",
        "description": "Loop door open/close; tests cylinder control (watch safety)",
    },
    "随机混合": {
        "name": "Mixed Commands",
        "description": "Mixed read/write commands; simulates normal load",
    },
    "日志链路": {
        "name": "Log Link",
        "description": "Poll log and debug switches; tests diagnostic commands",
    },
}

LED_DEF_LABEL_EN = {
    "门上限": "Door Up",
    "门下限": "Door Dn",
    "门中位": "Door Mid",
    "USB上位": "USB Up",
    "USB下位": "USB Dn",
    "气压": "Air",
    "激光2": "Laser 2",
    "激光3": "Laser 3",
    "激光4": "Laser 4",
    "关门钮1": "Close 1",
    "关门钮2": "Close 2",
    "急停": "E-stop",
    "电源钮": "Pwr Btn",
    "DUT到位": "DUT In",
    "IN15": "IN15",
    "IN16": "IN16",
    "开门": "Open",
    "关门": "Close",
    "USB插入": "USB In",
    "USB拔出": "USB Out",
    "绿灯": "Green",
    "红灯": "Red",
    "黄灯": "Yellow",
    "电源": "Power",
    "启动LED": "Start",
    "OUT10": "OUT10",
    "OUT11": "OUT11",
    "OUT12": "OUT12",
    "OUT13": "OUT13",
    "OUT14": "OUT14",
    "OUT15": "OUT15",
    "OUT16": "OUT16",
}

# ══════════════════════════════════════════════════════════════════════
# 串口通信层
# ══════════════════════════════════════════════════════════════════════

class SerialWorker:
    """后台串口通信 — 发送 SCPI 命令并读取响应"""

    def __init__(self):
        self.serial_port: serial.Serial | None = None
        self.rx_queue = queue.Queue()       # 响应消息队列 -> GUI
        self.cmd_queue = queue.Queue()      # 命令队列 <- GUI
        self._running = False
        self._thread: threading.Thread | None = None
        self._lock = threading.Lock()
        self._ota_active = False
        self._live_listen = False
        self._response_event = threading.Event()

    @property
    def is_connected(self) -> bool:
        return self.serial_port is not None and self.serial_port.is_open

    def connect(self, port: str, baud: int) -> None:
        """打开串口"""
        with self._lock:
            if self.is_connected:
                self.disconnect()
            self.serial_port = serial.Serial(
                port=port,
                baudrate=baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=SERIAL_TIMEOUT,
            )
            # RS485 半双工: 拉低 RTS 使能接收, 否则收不到响应
            self.serial_port.rts = False
            self.serial_port.dtr = False
            self._running = True
            self._thread = threading.Thread(target=self._worker_loop, daemon=True)
            self._thread.start()

    def disconnect(self) -> None:
        """关闭串口"""
        self._running = False
        if self._thread and self._thread.is_alive():
            self._thread.join(timeout=2)
        with self._lock:
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
            self.serial_port = None

    def send_command(self, cmd: str, expect_response: bool = True) -> None:
        """发送 SCPI 命令 (线程安全)"""
        if self._ota_active:
            self.rx_queue.put(("ERROR", cmd, UI_TEXT[DEFAULT_LANGUAGE]["serial_ota_locked"]))
            return
        self.cmd_queue.put((cmd, expect_response))

    def ota_set_active(self, active: bool) -> None:
        """标记 OTA 上传独占串口。"""
        self._ota_active = active

    def set_live_listen(self, enabled: bool) -> None:
        """开启/关闭空闲串口旁听。"""
        self._live_listen = enabled

    @staticmethod
    def _is_debug_line(decoded: str) -> bool:
        return (decoded.startswith("[T+") or
                decoded.startswith("[STATE]") or
                decoded.startswith("[CLOSE]") or
                decoded.startswith("[EVENT]") or
                decoded.startswith("[RISK]") or
                decoded.startswith("[IO]") or
                decoded.startswith("[RS485]") or
                decoded.startswith("[LOCK]") or
                decoded.startswith("[UNLOCK]") or
                decoded.startswith("[CLOSE_START]") or
                decoded.startswith("[CLOSE_DONE]") or
                decoded.startswith("[OPEN_START]") or
                decoded.startswith("[OPEN_DONE]") or
                decoded.startswith("E-STOP") or
                decoded.startswith("Door_Emerge") or
                decoded.startswith("Intake air"))

    @staticmethod
    def _expects_response(cmd: str) -> bool:
        return cmd.strip() not in SCPI_NO_RESPONSE_COMMANDS

    @staticmethod
    def _is_multiline_command(cmd: str) -> bool:
        return cmd.strip().upper() in SCPI_MULTILINE_COMMANDS

    @staticmethod
    def _accepts_log_line_response(cmd: str) -> bool:
        return cmd.strip().upper() in SCPI_LOG_READ_COMMANDS

    def _read_scpi_response_locked(
        self,
        timeout: float,
        multiline: bool = False,
        accept_log_lines: bool = False,
    ) -> str:
        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self.serial_port.read_until(b'\n', size=512)
            if not line:
                continue
            decoded = line.decode("utf-8", errors="replace").strip()
            if not decoded:
                continue
            is_log_line_response = accept_log_lines and decoded.startswith("[T+")
            if self._is_debug_line(decoded) and not is_log_line_response:
                self.rx_queue.put(("debug", "", decoded, 0))
                continue
            if multiline:
                lines = [decoded]
                while self.serial_port.in_waiting:
                    extra = self.serial_port.read_until(b'\n', size=512)
                    extra_decoded = extra.decode("utf-8", errors="replace").strip()
                    if not extra_decoded:
                        continue
                    is_extra_log_line = accept_log_lines and extra_decoded.startswith("[T+")
                    if self._is_debug_line(extra_decoded) and not is_extra_log_line:
                        self.rx_queue.put(("debug", "", extra_decoded, 0))
                        continue
                    lines.append(extra_decoded)
                return "\n".join(lines)
            return decoded
        return ""

    def _clear_stale_input_locked(self) -> None:
        if self.serial_port and self.serial_port.is_open:
            if self._live_listen:
                self._poll_live_lines_locked(max_lines=64)
            else:
                self.serial_port.reset_input_buffer()

    def _poll_live_lines_locked(self, max_lines: int = 16) -> None:
        """空闲时读取下位机自发输出, 不参与 SCPI 响应等待。"""
        if (not self._live_listen or self._ota_active or
                not self.serial_port or not self.serial_port.is_open):
            return

        old_timeout = self.serial_port.timeout
        self.serial_port.timeout = 0.02
        try:
            count = 0
            while self.serial_port.in_waiting and count < max_lines:
                raw = self.serial_port.read_until(b'\n', size=512)
                if not raw:
                    break
                decoded = raw.decode("utf-8", errors="replace").strip()
                if decoded:
                    self.rx_queue.put(("live", decoded))
                    count += 1
        finally:
            self.serial_port.timeout = old_timeout

    def transact_raw(self, payload: bytes, timeout: float = OTA_ACK_TIMEOUT) -> str:
        """直接发送 bytes 并读取一行 SCPI 响应, OTA 上传线程使用。"""
        with self._lock:
            if not self.is_connected:
                raise serial.SerialException(UI_TEXT[DEFAULT_LANGUAGE]["serial_not_connected"])
            self._clear_stale_input_locked()
            self.serial_port.write(payload)
            self.serial_port.flush()
            return self._read_scpi_response_locked(timeout)

    def read_raw_lines(self, timeout: float, stop_text: str | None = None) -> list[str]:
        """读取原始串口行, OTA 提交后收集 Bootloader/POST 输出。"""
        lines: list[str] = []
        with self._lock:
            if not self.is_connected:
                raise serial.SerialException(UI_TEXT[DEFAULT_LANGUAGE]["serial_not_connected"])
            deadline = time.time() + timeout
            while time.time() < deadline:
                raw = self.serial_port.read_until(b'\n', size=512)
                if not raw:
                    continue
                decoded = raw.decode("utf-8", errors="replace").strip()
                if not decoded:
                    continue
                lines.append(decoded)
                if stop_text and stop_text in decoded:
                    break
        return lines

    def _worker_loop(self) -> None:
        """后台工作循环: 发送 SCPI 命令 → 读取响应 (过滤固件调试输出)"""
        while self._running:
            try:
                cmd, expect_response = self.cmd_queue.get(timeout=0.05)
            except queue.Empty:
                try:
                    with self._lock:
                        self._poll_live_lines_locked()
                except serial.SerialException as e:
                    self.rx_queue.put(("ERROR", UI_TEXT[DEFAULT_LANGUAGE]["live_listen_cmd"], str(e)))
                continue

            try:
                with self._lock:
                    if not self.is_connected:
                        self.rx_queue.put(("ERROR", cmd, UI_TEXT[DEFAULT_LANGUAGE]["serial_not_connected"]))
                        continue

                    full_cmd = cmd.strip() + SCPI_TERMINATOR
                    send_time = time.perf_counter()
                    self._clear_stale_input_locked()
                    self.serial_port.write(full_cmd.encode("utf-8"))
                    time.sleep(0.03)

                    if expect_response:
                        scpi_resp = self._read_scpi_response_locked(
                            SERIAL_TIMEOUT,
                            multiline=self._is_multiline_command(cmd),
                            accept_log_lines=self._accepts_log_line_response(cmd),
                        )
                        elapsed_us = int((time.perf_counter() - send_time) * 1_000_000)
                        resp_text = scpi_resp if scpi_resp else UI_TEXT[DEFAULT_LANGUAGE]["no_response"]
                        self.rx_queue.put(("response", cmd, resp_text, elapsed_us))
                    else:
                        elapsed_us = int((time.perf_counter() - send_time) * 1_000_000)
                        self.rx_queue.put(("sent", cmd, UI_TEXT[DEFAULT_LANGUAGE]["no_response_needed"], elapsed_us))

            except serial.SerialException as e:
                self.rx_queue.put(("ERROR", cmd, str(e)))
            except Exception as e:
                self.rx_queue.put(("ERROR", cmd, UI_TEXT[DEFAULT_LANGUAGE]["unknown_error"].format(err=e)))


# ══════════════════════════════════════════════════════════════════════
# GUI 主窗口
# ══════════════════════════════════════════════════════════════════════
class PinProbeApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.language = tk.StringVar(value=DEFAULT_LANGUAGE)
        self.language_display = tk.StringVar(value=LANGUAGE_LABELS[DEFAULT_LANGUAGE])
        self.root.report_callback_exception = self._report_callback_exception
        self.root.title(APP_TITLE)
        self.root.geometry("1400x850")
        self.root.minsize(1200, 760)
        self._response_job_id = None
        self._poll_job_id = None
        self._poll_status_reset_job_id = None
        self._stats_job_id = None
        self._query_device_job_id = None
        self._ota_reboot_prompt_job_id = None
        self._language_change_job_id = None
        self._ui_rebuilding = False

        # 串口工作线程
        self.serial_worker = SerialWorker()

        # 状态变量
        self.polling_enabled = tk.BooleanVar(value=False)
        self.poll_interval_ms = tk.IntVar(value=500)
        self.auto_scroll = tk.BooleanVar(value=True)
        self.live_listen = tk.BooleanVar(value=False)
        self.connected = tk.BooleanVar(value=False)

        # 压力测试状态
        self.pressure_running = False
        self.pressure_thread: threading.Thread | None = None
        self.pressure_stats = {
            "total": 0, "success": 0, "fail": 0, "timeout": 0,
            "latencies": deque(maxlen=STATS_HISTORY),
            "errors": deque(maxlen=100),
        }

        # 命令历史
        self.cmd_history: list[str] = []

        # OTA 状态
        self.ota_file_path: Path | None = None
        self.ota_running = False
        self.ota_thread: threading.Thread | None = None
        self.ota_abort_requested = False
        self.ota_progress = tk.DoubleVar(value=0.0)
        self.ota_status_var = tk.StringVar(value=self._tr("ota_no_file"))
        self.ota_file_var = tk.StringVar(value="")
        self.ota_version_var = tk.StringVar(value="0x00010000")
        self.ota_image_id_var = tk.StringVar(value="1")

        # 构建 UI
        self._setup_styles()
        self._build_ui()

        # 定时器: 处理串口响应
        self._process_responses()
        # 定时器: 自动轮询
        self._poll_timer()
        # 定时器: 刷新串口列表
        self._refresh_com_list()
        if hasattr(self, "preset_combo"):
            self._load_preset(log_loaded=False)

    def _report_callback_exception(self, exc_type, exc_value, exc_tb):
        log_path = Path(__file__).with_name("pinprobe_gui_error.log")
        text = "".join(traceback.format_exception(exc_type, exc_value, exc_tb))
        try:
            with log_path.open("a", encoding="utf-8") as f:
                f.write(f"\n[{datetime.now():%Y-%m-%d %H:%M:%S}]\n{text}")
        except OSError:
            pass
        try:
            messagebox.showerror("PinProbe GUI Error", f"{exc_value}\n\nSaved to:\n{log_path}")
        except tk.TclError:
            pass

    def _tr(self, key: str, **kwargs) -> str:
        lang = self.language.get() if hasattr(self, "language") else DEFAULT_LANGUAGE
        text = UI_TEXT.get(lang, UI_TEXT["en"]).get(key, UI_TEXT["en"].get(key, key))
        return text.format(**kwargs) if kwargs else text

    def _current_lang(self) -> str:
        return self.language.get() if hasattr(self, "language") else DEFAULT_LANGUAGE

    def _widget_exists(self, name: str) -> bool:
        widget = getattr(self, name, None)
        if widget is None:
            return False
        try:
            return bool(widget.winfo_exists())
        except tk.TclError:
            return False

    def _cancel_after_job(self, attr_name: str):
        job_id = getattr(self, attr_name, None)
        if job_id is None:
            return
        try:
            self.root.after_cancel(job_id)
        except tk.TclError:
            pass
        setattr(self, attr_name, None)

    def _cancel_ui_jobs(self):
        for attr_name in (
            "_response_job_id",
            "_poll_job_id",
            "_poll_status_reset_job_id",
            "_stats_job_id",
            "_query_device_job_id",
            "_ota_reboot_prompt_job_id",
            "_language_change_job_id",
        ):
            self._cancel_after_job(attr_name)

    def _run_ui_safe(self, callback, *args):
        if self._ui_rebuilding:
            return
        try:
            callback(*args)
        except tk.TclError:
            return

    def _command_category_label(self, category: str) -> str:
        return SCPI_CATEGORY_EN.get(category, category) if self._current_lang() == "en" else category

    def _command_label(self, label: str) -> str:
        return SCPI_LABEL_EN.get(label, label) if self._current_lang() == "en" else label

    def _auto_poll_label(self, label: str) -> str:
        return AUTO_POLL_LABEL_EN.get(label, label) if self._current_lang() == "en" else label

    def _led_label(self, label: str) -> str:
        return LED_DEF_LABEL_EN.get(label, label) if self._current_lang() == "en" else label

    def _preset_label(self, preset_name: str) -> str:
        if self._current_lang() == "en":
            return PRESSURE_PRESET_EN.get(preset_name, {}).get("name", preset_name)
        return preset_name

    def _preset_description(self, preset_name: str) -> str:
        if self._current_lang() == "en":
            return PRESSURE_PRESET_EN.get(preset_name, {}).get(
                "description", PRESSURE_PRESETS[preset_name].get("description", ""))
        return PRESSURE_PRESETS[preset_name].get("description", "")

    def _on_language_changed(self, event=None):
        label = self.language_display.get()
        lang = next((code for code, name in LANGUAGE_LABELS.items() if name == label), DEFAULT_LANGUAGE)
        if lang == self.language.get():
            return
        self._cancel_after_job("_language_change_job_id")
        self._language_change_job_id = self.root.after(
            50,
            lambda target_lang=lang: self._apply_language_changed(target_lang))

    def _apply_language_changed(self, lang: str):
        self._language_change_job_id = None
        if lang == self.language.get():
            return
        self._ui_rebuilding = True
        self._cancel_ui_jobs()
        selected_port = self.com_var.get() if hasattr(self, "com_var") else ""
        pressure_commands = ""
        if hasattr(self, "pressure_cmd_text"):
            pressure_commands = self.pressure_cmd_text.get("1.0", tk.END).strip()
        preset_key = None
        if hasattr(self, "preset_var") and hasattr(self, "preset_display_to_key"):
            preset_key = self.preset_display_to_key.get(self.preset_var.get(), self.preset_var.get())
        log_content = ""
        if hasattr(self, "log_text"):
            log_content = self.log_text.get("1.0", tk.END).rstrip()
        self.language.set(lang)
        self.root.title(self._tr("app_title"))
        for child in self.root.winfo_children():
            child.destroy()
        self._setup_styles()
        self._build_ui()
        if selected_port and hasattr(self, "com_var"):
            self.com_var.set(selected_port)
        if preset_key in PRESSURE_PRESETS and hasattr(self, "preset_combo"):
            preset_label = self._preset_label(preset_key)
            self.preset_var.set(preset_label)
            self._on_preset_selected()
        if pressure_commands and hasattr(self, "pressure_cmd_text"):
            self.pressure_cmd_text.delete("1.0", tk.END)
            self.pressure_cmd_text.insert("1.0", pressure_commands)
        if log_content and hasattr(self, "log_text"):
            self.log_text.configure(state=tk.NORMAL)
            self.log_text.insert("1.0", log_content + "\n")
            self.log_text.configure(state=tk.DISABLED)
        self._refresh_history_listbox()
        if self.connected.get() and self.serial_worker.is_connected:
            self.conn_indicator.configure(text=self._tr("connected_dot"), foreground="#00AA00")
            self.conn_detail.configure(foreground="#00AA00")
            self.btn_connect.configure(state=tk.DISABLED)
            self.btn_disconnect.configure(state=tk.NORMAL)
            self.btn_idn.configure(state=tk.NORMAL)
        self._refresh_com_list()
        if hasattr(self, "preset_combo") and not pressure_commands:
            self._load_preset(log_loaded=False)
        self._ui_rebuilding = False
        self._process_responses()
        self._poll_timer()

    # ── 样式设置 ──────────────────────────────────────────────────────
    def _setup_styles(self):
        """配置扁平清新风格"""
        style = ttk.Style()
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass

        # ---- 调色板 ----
        BG_MAIN      = "#f0f2f5"
        BG_CARD      = "#ffffff"
        BG_INPUT     = "#f8f9fa"
        FG_PRIMARY   = "#2c3e50"
        FG_SECONDARY = "#6c757d"
        ACCENT       = "#4a90d9"
        ACCENT_HOVER = "#3a7bc8"
        ACCENT_LIGHT = "#e8f0fe"
        DANGER       = "#e74c3c"
        SUCCESS      = "#27ae60"
        WARNING      = "#f39c12"
        BORDER       = "#dee2e6"
        BORDER_FOCUS = "#4a90d9"

        self.root.configure(bg=BG_MAIN)

        # ---- 全局字体 ----
        default_font = ("Microsoft YaHei UI", 9)
        heading_font = ("Microsoft YaHei UI", 10, "bold")
        mono_font    = ("Cascadia Code", 9)

        style.configure(".", font=default_font, background=BG_MAIN, foreground=FG_PRIMARY)

        # ---- TFrame ----
        style.configure("TFrame", background=BG_MAIN)
        style.configure("Card.TFrame", background=BG_CARD)

        # ---- TLabel ----
        style.configure("TLabel", background=BG_MAIN, foreground=FG_PRIMARY)
        style.configure("Card.TLabel", background=BG_CARD)
        style.configure("Heading.TLabel", font=heading_font, foreground=FG_PRIMARY)
        style.configure("Secondary.TLabel", foreground=FG_SECONDARY)
        style.configure("Accent.TLabel", foreground=ACCENT)
        style.configure("Success.TLabel", foreground=SUCCESS)
        style.configure("Danger.TLabel", foreground=DANGER)

        # ---- TLabelframe (卡片式) ----
        style.configure("TLabelframe", background=BG_CARD, relief="solid",
                        borderwidth=1, bordercolor=BORDER)
        style.configure("TLabelframe.Label", background=BG_CARD, foreground=FG_PRIMARY,
                        font=heading_font, borderwidth=0)

        # ---- TButton (扁平) ----
        style.configure("TButton",
                        background=ACCENT, foreground="#ffffff",
                        borderwidth=0, relief="flat", padding=(14, 6),
                        font=default_font, focuscolor="none")
        style.map("TButton",
                  background=[("active", ACCENT_HOVER),
                              ("pressed", "#2e6eb5"),
                              ("disabled", "#b0c4de")],
                  foreground=[("disabled", "#ffffff")],
                  relief=[("pressed", "flat")])

        # 次要按钮
        style.configure("Secondary.TButton",
                        background=BG_INPUT, foreground=FG_PRIMARY,
                        borderwidth=1, bordercolor=BORDER,
                        relief="flat", padding=(14, 6))
        style.map("Secondary.TButton",
                  background=[("active", "#e9ecef"), ("pressed", "#dee2e6")],
                  bordercolor=[("active", ACCENT)])

        # 危险按钮
        style.configure("Danger.TButton",
                        background=DANGER, foreground="#ffffff",
                        borderwidth=0, relief="flat", padding=(14, 6))
        style.map("Danger.TButton",
                  background=[("active", "#c0392b"), ("pressed", "#a93226")])

        # 出厂配置警示按钮
        style.configure("FactoryWarn.TButton",
                        background="#fde2e2", foreground="#8a1f1f",
                        borderwidth=0,
                        relief="flat", padding=(14, 6))
        style.map("FactoryWarn.TButton",
                  background=[("active", "#fbd0d0"), ("pressed", "#f6bcbc")],
                  foreground=[("disabled", "#8a1f1f")])

        # 成功按钮 (连接状态)
        style.configure("Success.TButton",
                        background=SUCCESS, foreground="#ffffff",
                        borderwidth=0, relief="flat", padding=(14, 6))
        style.map("Success.TButton",
                  background=[("active", "#219a52"), ("pressed", "#1e8449")])

        # ---- TCombobox ----
        style.configure("TCombobox",
                        background=BG_INPUT, fieldbackground=BG_INPUT,
                        borderwidth=1, bordercolor=BORDER,
                        relief="flat", arrowcolor=FG_PRIMARY, padding=(8, 4))
        style.map("TCombobox",
                  bordercolor=[("focus", BORDER_FOCUS), ("hover", ACCENT)])

        # ---- TSpinbox ----
        style.configure("TSpinbox",
                        background=BG_INPUT, fieldbackground=BG_INPUT,
                        borderwidth=1, bordercolor=BORDER,
                        relief="flat", arrowcolor=FG_PRIMARY, padding=(8, 4))
        style.map("TSpinbox",
                  bordercolor=[("focus", BORDER_FOCUS), ("hover", ACCENT)])

        # ---- TEntry ----
        style.configure("TEntry",
                        fieldbackground=BG_INPUT, borderwidth=1,
                        bordercolor=BORDER, relief="flat", padding=(8, 4))
        style.map("TEntry",
                  bordercolor=[("focus", BORDER_FOCUS), ("hover", ACCENT)])

        # ---- TNotebook ----
        style.configure("TNotebook", background=BG_MAIN, borderwidth=0)
        style.configure("TNotebook.Tab",
                        background=BG_MAIN, foreground=FG_SECONDARY,
                        borderwidth=0, padding=(18, 8), font=default_font)
        style.map("TNotebook.Tab",
                  background=[("selected", BG_CARD)],
                  foreground=[("selected", ACCENT)],
                  expand=[("selected", [0, 0, 0, 0])])

        # ---- TCheckbutton ----
        style.configure("TCheckbutton", background=BG_MAIN)
        style.map("TCheckbutton",
                  indicatorcolor=[("selected", ACCENT), ("hover", ACCENT_HOVER)])

        # ---- Treeview (状态监控表) ----
        style.configure("Treeview",
                        background=BG_CARD, fieldbackground=BG_CARD,
                        foreground=FG_PRIMARY, borderwidth=1, bordercolor=BORDER,
                        rowheight=28, relief="flat")
        style.configure("Treeview.Heading",
                        background=BG_INPUT, foreground=FG_PRIMARY,
                        relief="flat", borderwidth=0, font=heading_font, padding=(8, 4))
        style.map("Treeview",
                  background=[("selected", ACCENT_LIGHT)],
                  foreground=[("selected", FG_PRIMARY)])
        style.map("Treeview.Heading",
                  background=[("active", "#e9ecef")])

        # ---- TScrollbar ----
        style.configure("TScrollbar",
                        background=BG_MAIN, troughcolor=BG_INPUT,
                        borderwidth=0, relief="flat", arrowcolor=FG_SECONDARY)

        # ---- TPanedwindow ----
        style.configure("TPanedwindow", background=BORDER, sashrelief="flat")

        # ---- TProgressbar (备用于压力测试进度) ----
        style.configure("TProgressbar",
                        background=ACCENT, troughcolor=BG_INPUT,
                        borderwidth=0, relief="flat")

        # 保存颜色引用供其他方法使用
        self._colors = {
            "bg_main": BG_MAIN, "bg_card": BG_CARD, "fg_primary": FG_PRIMARY,
            "fg_secondary": FG_SECONDARY, "accent": ACCENT,
            "danger": DANGER, "success": SUCCESS, "warning": WARNING,
        }
    def _build_ui(self):
        """构建完整 GUI"""
        self._build_menu_bar()

        # 主容器: 上下分
        main_pw = ttk.PanedWindow(self.root, orient=tk.VERTICAL)
        main_pw.pack(fill=tk.BOTH, expand=True)

        # ---- 上半部分 ----
        top_frame = ttk.Frame(main_pw)
        main_pw.add(top_frame, weight=60)

        # ---- 下半部分: 日志 ----
        bottom_frame = ttk.LabelFrame(main_pw, text=self._tr("log_frame"))
        main_pw.add(bottom_frame, weight=40)

        # 顶部区域分为固定左栏 + 自适应右栏，避免 PanedWindow 把 SCPI 面板压到 0。
        top_frame.columnconfigure(0, minsize=620, weight=0)
        top_frame.columnconfigure(1, weight=1)
        top_frame.rowconfigure(0, weight=1)

        # 左侧: 串口设置 + 命令面板
        left_frame = ttk.Frame(top_frame, width=620)
        left_frame.grid(row=0, column=0, sticky="nsew", padx=(2, 1), pady=2)
        left_frame.grid_propagate(False)

        # 右侧: 状态监控 + 压力测试
        right_frame = ttk.Frame(top_frame)
        right_frame.grid(row=0, column=1, sticky="nsew", padx=(1, 2), pady=2)

        # ---- 左侧内容 ----
        self._build_serial_panel(left_frame)
        self._build_command_panel(left_frame)

        # ---- 右侧内容 ----
        self.right_nb = ttk.Notebook(right_frame)
        self.right_nb.pack(fill=tk.BOTH, expand=True)

        monitor_tab = ttk.Frame(self.right_nb)
        self.right_nb.add(monitor_tab, text=self._tr("tab_monitor"))

        pressure_tab = ttk.Frame(self.right_nb)
        self.right_nb.add(pressure_tab, text=self._tr("tab_pressure"))

        ota_tab = ttk.Frame(self.right_nb)
        self.right_nb.add(ota_tab, text=self._tr("tab_ota"))

        custom_tab = ttk.Frame(self.right_nb)
        self.right_nb.add(custom_tab, text=self._tr("tab_custom"))

        self._build_monitor_panel(monitor_tab)
        self._build_pressure_panel(pressure_tab)
        self._build_ota_panel(ota_tab)
        self._build_custom_cmd_panel(custom_tab)

        # ---- 日志区域 ----
        self._build_log_panel(bottom_frame)

        # ---- 状态栏 ----
        status_frame = ttk.Frame(self.root)
        status_frame.pack(fill=tk.X, side=tk.BOTTOM, padx=2, pady=1)
        self.status_label = ttk.Label(status_frame, text=self._tr("status_ready"),
                                      relief=tk.SUNKEN, anchor=tk.W)
        self.status_label.pack(fill=tk.X)
        self.conn_indicator = ttk.Label(status_frame, text=self._tr("disconnected_dot"),
                                        foreground="gray", font=("", 10))
        self.conn_indicator.pack(side=tk.RIGHT, padx=5)

    def _build_menu_bar(self):
        """构建窗口菜单栏"""
        menu_bar = tk.Menu(self.root)
        language_menu = tk.Menu(menu_bar, tearoff=False)
        self.language_display.set(LANGUAGE_LABELS[self.language.get()])
        for code in ("en", "zh"):
            language_menu.add_radiobutton(
                label=LANGUAGE_LABELS[code],
                variable=self.language_display,
                value=LANGUAGE_LABELS[code],
                command=self._on_language_changed,
            )
        menu_bar.add_cascade(label=self._tr("menu_language"), menu=language_menu)
        self.root.config(menu=menu_bar)

    def _build_serial_panel(self, parent: ttk.Frame):
        """串口设置面板"""
        frame = ttk.LabelFrame(parent, text=self._tr("serial_settings"))
        frame.pack(fill=tk.X, padx=3, pady=3)

        row1 = ttk.Frame(frame)
        row1.pack(fill=tk.X, padx=5, pady=3)

        ttk.Label(row1, text=self._tr("port")).pack(side=tk.LEFT)
        self.com_combo = ttk.Combobox(row1, width=12, state="readonly")
        self.com_combo.pack(side=tk.LEFT, padx=(2, 10))

        ttk.Label(row1, text=self._tr("baud")).pack(side=tk.LEFT)
        self.baud_combo = ttk.Combobox(row1, width=10, state="readonly",
                                       values=[str(b) for b in BAUD_RATES])
        self.baud_combo.set(str(DEFAULT_BAUD))
        self.baud_combo.pack(side=tk.LEFT, padx=(2, 10))

        self.btn_refresh = ttk.Button(row1, text=self._tr("refresh"), style="Secondary.TButton",
                                      command=self._refresh_com_list)
        self.btn_refresh.pack(side=tk.LEFT, padx=2)

        row2 = ttk.Frame(frame)
        row2.pack(fill=tk.X, padx=5, pady=3)

        self.btn_connect = ttk.Button(row2, text=self._tr("connect"), style="Success.TButton",
                                      command=self._toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=2)

        self.btn_disconnect = ttk.Button(row2, text=self._tr("disconnect"), style="Danger.TButton",
                                         command=self._disconnect, state=tk.DISABLED)
        self.btn_disconnect.pack(side=tk.LEFT, padx=2)

        self.conn_detail = ttk.Label(row2, text=self._tr("disconnected"), foreground="gray")
        self.conn_detail.pack(side=tk.LEFT, padx=10)

        # 设备信息展示
        row3 = ttk.Frame(frame)
        row3.pack(fill=tk.X, padx=5, pady=3)
        self.device_info_label = ttk.Label(row3, text=self._tr("device_empty"), foreground="gray")
        self.device_info_label.pack(side=tk.LEFT)

        self.btn_idn = ttk.Button(row3, text=self._tr("query_device"), style="Secondary.TButton",
                                  command=self._query_device_id, state=tk.DISABLED)
        self.btn_idn.pack(side=tk.RIGHT, padx=2)

    def _build_command_panel(self, parent: ttk.Frame):
        """SCPI 命令快捷面板 (分类折叠)"""
        frame = ttk.LabelFrame(parent, text=self._tr("scpi_panel"))
        frame.pack(fill=tk.BOTH, expand=True, padx=3, pady=3)

        # 搜索框
        search_row = ttk.Frame(frame)
        search_row.pack(fill=tk.X, padx=5, pady=3)
        ttk.Label(search_row, text=self._tr("search")).pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add("write", lambda *a: self._filter_commands())
        search_entry = ttk.Entry(search_row, textvariable=self.search_var)
        search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)

        # 可滚动的命令区域
        cmd_canvas_frame = ttk.Frame(frame)
        cmd_canvas_frame.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        self.cmd_canvas = tk.Canvas(cmd_canvas_frame, highlightthickness=0)
        scrollbar = ttk.Scrollbar(cmd_canvas_frame, orient=tk.VERTICAL,
                                  command=self.cmd_canvas.yview)
        self.cmd_scroll_frame = ttk.Frame(self.cmd_canvas)

        self.cmd_scroll_frame.bind("<Configure>",
                                   lambda e: self.cmd_canvas.configure(
                                       scrollregion=self.cmd_canvas.bbox("all")))
        self.cmd_window_id = self.cmd_canvas.create_window(
            (0, 0), window=self.cmd_scroll_frame, anchor="nw")
        self.cmd_canvas.bind("<Configure>", self._resize_command_canvas_window)
        self.cmd_canvas.configure(yscrollcommand=scrollbar.set)

        self.cmd_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # 鼠标滚轮支持
        def _on_mousewheel(event):
            self.cmd_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        self.cmd_canvas.bind_all("<MouseWheel>", _on_mousewheel)

        # 填充命令按钮
        self._populate_command_buttons()

    def _populate_command_buttons(self):
        """填充分类命令按钮"""
        self.cmd_button_frames: dict[str, ttk.LabelFrame] = {}
        self.all_cmd_buttons: list[ttk.Button] = []

        for category, commands in SCPI_COMMANDS.items():
            cat_frame = ttk.LabelFrame(self.cmd_scroll_frame,
                                       text=f"  {self._command_category_label(category)}  ")
            cat_frame.pack(fill=tk.X, padx=3, pady=2)
            self.cmd_button_frames[category] = cat_frame

            btn_grid = ttk.Frame(cat_frame)
            btn_grid.pack(fill=tk.X, padx=3, pady=2)
            for col_idx in range(3):
                btn_grid.columnconfigure(col_idx, weight=1, uniform="cmd_btn")

            for idx, (label, cmd) in enumerate(commands):
                row = idx // 3
                col = idx % 3
                style_name = "FactoryWarn.TButton" if cmd.startswith("CONFigure:USB:AUTO ") else "TButton"
                btn = ttk.Button(btn_grid, text=self._command_label(label), width=14,
                                 style=style_name,
                                 command=lambda c=cmd: self._send_scpi(c))
                btn.grid(row=row, column=col, padx=2, pady=2, sticky="ew")
                btn.tooltip = cmd  # 附加命令文本用于搜索
                btn.category = category
                btn.raw_label = label
                self.all_cmd_buttons.append(btn)

    def _resize_command_canvas_window(self, event=None):
        if not self._widget_exists("cmd_canvas") or not hasattr(self, "cmd_window_id"):
            return
        try:
            self.cmd_canvas.itemconfigure(self.cmd_window_id, width=self.cmd_canvas.winfo_width())
        except tk.TclError:
            pass

    def _filter_commands(self):
        """根据搜索词过滤命令按钮"""
        query = self.search_var.get().strip().lower()
        for btn in self.all_cmd_buttons:
            if not query:
                btn.configure(state=tk.NORMAL)
            elif (query in btn.cget("text").lower() or
                  query in (btn.tooltip or "").lower()):
                btn.configure(state=tk.NORMAL)
            else:
                btn.configure(state=tk.DISABLED)

    def _build_monitor_panel(self, parent: ttk.Frame):
        """状态监控面板"""
        # 控制行
        ctrl_frame = ttk.Frame(parent)
        ctrl_frame.pack(fill=tk.X, padx=5, pady=5)

        self.poll_cb = ttk.Checkbutton(ctrl_frame, text=self._tr("auto_poll"),
                                       variable=self.polling_enabled)
        self.poll_cb.pack(side=tk.LEFT)

        ttk.Label(ctrl_frame, text=self._tr("interval_ms")).pack(side=tk.LEFT, padx=(10, 2))
        self.poll_interval_spin = ttk.Spinbox(ctrl_frame, from_=MIN_POLL_INTERVAL_MS, to=10000,
                                              increment=100, width=7,
                                              textvariable=self.poll_interval_ms)
        self.poll_interval_spin.pack(side=tk.LEFT)

        ttk.Button(ctrl_frame, text=self._tr("refresh_now"), command=self._manual_poll).pack(
            side=tk.LEFT, padx=10)

        self.poll_status_label = ttk.Label(ctrl_frame, text=self._tr("poll_stopped"),
                                           foreground="gray")
        self.poll_status_label.pack(side=tk.RIGHT, padx=5)

        # 状态显示表格
        table_frame = ttk.Frame(parent)
        table_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=2)

        columns = ("name", "value", "raw_cmd", "timestamp")
        self.monitor_tree = ttk.Treeview(table_frame, columns=columns,
                                         show="headings", height=8)
        self.monitor_tree.heading("name", text=self._tr("col_status_item"))
        self.monitor_tree.heading("value", text=self._tr("col_current_value"))
        self.monitor_tree.heading("raw_cmd", text=self._tr("col_query_cmd"))
        self.monitor_tree.heading("timestamp", text=self._tr("col_updated"))
        self.monitor_tree.column("name", width=120)
        self.monitor_tree.column("value", width=150)
        self.monitor_tree.column("raw_cmd", width=250)
        self.monitor_tree.column("timestamp", width=160)

        monitor_scroll = ttk.Scrollbar(table_frame, orient=tk.VERTICAL,
                                       command=self.monitor_tree.yview)
        self.monitor_tree.configure(yscrollcommand=monitor_scroll.set)

        self.monitor_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        monitor_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # 初始化表格行
        for name, cmd in AUTO_POLL_COMMANDS:
            self.monitor_tree.insert("", tk.END, values=(self._auto_poll_label(name), "---", cmd, ""))

        # 颜色标签
        self.monitor_tree.tag_configure("updated", foreground="#007700")
        self.monitor_tree.tag_configure("ERROR", foreground="#cc0000")
        self.monitor_tree.tag_configure("stale", foreground="#999999")

        # ---- LED 指示灯面板 ----
        self._build_led_panel(parent)

    def _build_led_panel(self, parent: ttk.Frame):
        """IO状态 LED 指示灯 (两排: 输入/输出)"""
        led_frame = ttk.LabelFrame(parent, text=self._tr("io_indicators"))
        led_frame.pack(fill=tk.X, padx=5, pady=3)
        self._led_canvas_ids = {}

        # 标题行 + 刷新按钮
        title_row = ttk.Frame(led_frame)
        title_row.pack(fill=tk.X, padx=5, pady=(3, 0))
        ttk.Button(title_row, text=self._tr("refresh_io"), width=10,
                   command=lambda: self._send_scpi("READ:IO:ALL?")).pack(side=tk.RIGHT)

        # LED 定义: (类型, 字节索引, 位掩码, 标签名) — 全部16位
        led_defs_input = [
            ("IN", 0, 0x01, "门上限"),
            ("IN", 0, 0x02, "门下限"),
            ("IN", 0, 0x04, "门中位"),
            ("IN", 0, 0x08, "USB上位"),
            ("IN", 0, 0x10, "USB下位"),
            ("IN", 0, 0x20, "气压"),
            ("IN", 0, 0x40, "激光2"),
            ("IN", 0, 0x80, "激光3"),
            ("IN", 1, 0x01, "激光4"),
            ("IN", 1, 0x02, "关门钮1"),
            ("IN", 1, 0x04, "关门钮2"),
            ("IN", 1, 0x08, "急停"),
            ("IN", 1, 0x10, "电源钮"),
            ("IN", 1, 0x20, "DUT到位"),
            ("IN", 1, 0x40, "IN15"),
            ("IN", 1, 0x80, "IN16"),
        ]
        led_defs_output = [
            ("OUT", 0, 0x01, "开门"),
            ("OUT", 0, 0x02, "关门"),
            ("OUT", 0, 0x04, "USB插入"),
            ("OUT", 0, 0x08, "USB拔出"),
            ("OUT", 0, 0x10, "绿灯"),
            ("OUT", 0, 0x20, "红灯"),
            ("OUT", 0, 0x40, "黄灯"),
            ("OUT", 0, 0x80, "电源"),
            ("OUT", 1, 0x01, "启动LED"),
            ("OUT", 1, 0x02, "OUT10"),
            ("OUT", 1, 0x04, "OUT11"),
            ("OUT", 1, 0x08, "OUT12"),
            ("OUT", 1, 0x10, "OUT13"),
            ("OUT", 1, 0x20, "OUT14"),
            ("OUT", 1, 0x40, "OUT15"),
            ("OUT", 1, 0x80, "OUT16"),
        ]
        led_defs_input = [
            (io_type, byte_idx, mask, self._led_label(label))
            for io_type, byte_idx, mask, label in led_defs_input
        ]
        led_defs_output = [
            (io_type, byte_idx, mask, self._led_label(label))
            for io_type, byte_idx, mask, label in led_defs_output
        ]

        LED_R = 10           # 灯半径
        CELL_W = 56           # 每个 IO 点的固定宽度，避免英文标签重叠
        LABEL_H = 28          # 标签高度
        ROW_H = LED_R * 2 + LABEL_H + 8

        # 输入行
        in_label = ttk.Label(led_frame, text=self._tr("input"), font=("", 9, "bold"))
        in_label.pack(anchor=tk.W, padx=5, pady=(3, 0))
        in_count = len(led_defs_input)
        in_width = in_count * CELL_W
        self.canvas_in = tk.Canvas(led_frame, width=in_width, height=ROW_H,
                                    bg="#ffffff", highlightthickness=0)
        self.canvas_in.pack(padx=5, pady=2)

        # 输出行
        out_label = ttk.Label(led_frame, text=self._tr("output"), font=("", 9, "bold"))
        out_label.pack(anchor=tk.W, padx=5, pady=(5, 0))
        out_count = len(led_defs_output)
        out_width = out_count * CELL_W
        self.canvas_out = tk.Canvas(led_frame, width=out_width, height=ROW_H,
                                     bg="#ffffff", highlightthickness=0)
        self.canvas_out.pack(padx=5, pady=2)

        # 绘制 LED 灯
        self._draw_leds(self.canvas_in, led_defs_input, LED_R, CELL_W, LABEL_H)
        self._draw_leds(self.canvas_out, led_defs_output, LED_R, CELL_W, LABEL_H)

        # IO 原始值显示
        self.io_raw_label = ttk.Label(led_frame, text="---",
                                       font=("Cascadia Code", 10, "bold"),
                                       foreground="#2c3e50")
        self.io_raw_label.pack(pady=(6, 3))

    def _draw_leds(self, canvas: tk.Canvas, defs: list, r: int, cell_w: int, lh: int):
        """在 Canvas 上绘制 LED 灯（圆+标签），返回 circle ID 列表"""
        ids = []
        y_center = r + 4
        y_label = y_center + r + 4

        for idx, (io_type, byte_idx, mask, label) in enumerate(defs):
            x = idx * cell_w + cell_w // 2
            # LED 圆（初始灰色=off）
            cid = canvas.create_oval(x - r, y_center - r, x + r, y_center + r,
                                     fill="#cccccc", outline="#999999", width=1)
            # 标签
            tid = canvas.create_text(x, y_label, text=label,
                                     font=("Microsoft YaHei UI", 7), fill="#666666",
                                     anchor=tk.N, justify=tk.CENTER, width=cell_w - 4)
            ids.append((cid, tid, io_type, byte_idx, mask))

        # 保存 ID 引用
        if not hasattr(self, '_led_canvas_ids'):
            self._led_canvas_ids = {}
        self._led_canvas_ids[canvas] = ids

    def _update_leds_from_response(self, response: str):
        """根据 READ:IO:ALL? 响应更新所有 LED 指示灯和原始值显示"""
        if self._ui_rebuilding or not self._widget_exists("io_raw_label"):
            return
        data = parse_io_response(response)
        if not data:
            return

        # 更新原始值标签
        in0 = data.get(("IN", 0), 0)
        in1 = data.get(("IN", 1), 0)
        out0 = data.get(("OUT", 0), 0)
        out1 = data.get(("OUT", 1), 0)
        self.io_raw_label.configure(
            text=f"IN: 0x{in0:02X}, 0x{in1:02X}    OUT: 0x{out0:02X}, 0x{out1:02X}")

        ON_COLOR = "#00CC00"   # 绿色=激活
        OFF_COLOR = "#cccccc"  # 灰色=未激活

        for canvas, ids in getattr(self, '_led_canvas_ids', {}).items():
            try:
                if not canvas.winfo_exists():
                    continue
            except tk.TclError:
                continue
            for cid, tid, io_type, byte_idx, mask in ids:
                val = data.get((io_type, byte_idx), 0)
                active = (val & mask) != 0
                color = ON_COLOR if active else OFF_COLOR
                outline = "#00AA00" if active else "#999999"
                canvas.itemconfigure(cid, fill=color, outline=outline)

    def _build_pressure_panel(self, parent: ttk.Frame):
        """压力测试面板"""
        # ---- 预设选择 ----
        preset_frame = ttk.LabelFrame(parent, text=self._tr("test_presets"))
        preset_frame.pack(fill=tk.X, padx=5, pady=3)

        preset_row = ttk.Frame(preset_frame)
        preset_row.pack(fill=tk.X, padx=5, pady=3)

        ttk.Label(preset_row, text=self._tr("preset")).pack(side=tk.LEFT)
        self.preset_var = tk.StringVar()
        self.preset_display_to_key = {self._preset_label(name): name for name in PRESSURE_PRESETS}
        self.preset_combo = ttk.Combobox(preset_row, textvariable=self.preset_var,
                                         state="readonly", width=25)
        self.preset_combo["values"] = list(self.preset_display_to_key.keys())
        if PRESSURE_PRESETS:
            self.preset_combo.current(0)
        self.preset_combo.pack(side=tk.LEFT, padx=5)
        self.preset_combo.bind("<<ComboboxSelected>>", self._on_preset_selected)

        ttk.Button(preset_row, text=self._tr("load_preset"), command=self._load_preset).pack(
            side=tk.LEFT, padx=5)

        self.preset_desc_label = ttk.Label(preset_row, text="", foreground="gray")
        self.preset_desc_label.pack(side=tk.LEFT, padx=10)

        # ---- 命令列表 ----
        cmdlist_frame = ttk.LabelFrame(parent, text=self._tr("test_sequence"))
        cmdlist_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=3)

        list_toolbar = ttk.Frame(cmdlist_frame)
        list_toolbar.pack(fill=tk.X, padx=3, pady=2)

        ttk.Button(list_toolbar, text=self._tr("add"), command=self._pressure_add_cmd).pack(
            side=tk.LEFT, padx=2)
        ttk.Button(list_toolbar, text=self._tr("delete_selected"), command=self._pressure_del_cmd).pack(
            side=tk.LEFT, padx=2)
        ttk.Button(list_toolbar, text=self._tr("clear"), command=self._pressure_clear_cmds).pack(
            side=tk.LEFT, padx=2)

        self.pressure_cmd_text = tk.Text(cmdlist_frame, height=5, wrap=tk.WORD)
        pressure_cmd_scroll = ttk.Scrollbar(cmdlist_frame, orient=tk.VERTICAL,
                                            command=self.pressure_cmd_text.yview)
        self.pressure_cmd_text.configure(yscrollcommand=pressure_cmd_scroll.set)
        self.pressure_cmd_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=3, pady=2)
        pressure_cmd_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # ---- 参数设置 ----
        param_frame = ttk.LabelFrame(parent, text=self._tr("test_params"))
        param_frame.pack(fill=tk.X, padx=5, pady=3)

        p_row = ttk.Frame(param_frame)
        p_row.pack(fill=tk.X, padx=5, pady=3)

        ttk.Label(p_row, text=self._tr("cmd_interval")).pack(side=tk.LEFT)
        self.pressure_interval = tk.IntVar(value=200)
        ttk.Spinbox(p_row, from_=10, to=60000, increment=10, width=8,
                    textvariable=self.pressure_interval).pack(side=tk.LEFT, padx=2)

        ttk.Label(p_row, text=self._tr("loop_count")).pack(side=tk.LEFT, padx=(15, 2))
        self.pressure_loops = tk.IntVar(value=0)
        ttk.Spinbox(p_row, from_=0, to=1000000, increment=100, width=8,
                    textvariable=self.pressure_loops).pack(side=tk.LEFT, padx=2)

        ttk.Label(p_row, text=self._tr("timeout_ms")).pack(side=tk.LEFT, padx=(15, 2))
        self.pressure_timeout = tk.IntVar(value=2000)
        ttk.Spinbox(p_row, from_=50, to=30000, increment=100, width=8,
                    textvariable=self.pressure_timeout).pack(side=tk.LEFT, padx=2)

        # ---- 控制按钮 ----
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)

        self.btn_pressure_start = ttk.Button(btn_frame, text=self._tr("start_pressure"),
                                             command=self._toggle_pressure)
        self.btn_pressure_start.pack(side=tk.LEFT, padx=2)

        self.btn_pressure_stop = ttk.Button(btn_frame, text=self._tr("stop"), style="Danger.TButton",
                                            command=self._stop_pressure,
                                            state=tk.DISABLED)
        self.btn_pressure_stop.pack(side=tk.LEFT, padx=2)

        self.pressure_status_label = ttk.Label(btn_frame, text=self._tr("ready"),
                                               foreground="gray")
        self.pressure_status_label.pack(side=tk.LEFT, padx=15)

        # ---- 统计信息 ----
        stats_frame = ttk.LabelFrame(parent, text=self._tr("realtime_stats"))
        stats_frame.pack(fill=tk.X, padx=5, pady=3)

        self.stats_text = tk.Text(stats_frame, height=5, state=tk.DISABLED,
                                  font=("Consolas", 9))
        self.stats_text.pack(fill=tk.BOTH, expand=True, padx=3, pady=2)

    def _build_ota_panel(self, parent: ttk.Frame):
        """固件升级 OTA 面板"""
        file_frame = ttk.LabelFrame(parent, text=self._tr("firmware_file"))
        file_frame.pack(fill=tk.X, padx=5, pady=5)

        file_row = ttk.Frame(file_frame)
        file_row.pack(fill=tk.X, padx=5, pady=5)
        ttk.Entry(file_row, textvariable=self.ota_file_var).pack(
            side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(file_row, text=self._tr("select_file"), command=self._ota_select_file).pack(
            side=tk.LEFT, padx=(6, 0))

        meta_row = ttk.Frame(file_frame)
        meta_row.pack(fill=tk.X, padx=5, pady=(0, 5))
        ttk.Label(meta_row, text=self._tr("version")).pack(side=tk.LEFT)
        ttk.Entry(meta_row, textvariable=self.ota_version_var, width=14).pack(
            side=tk.LEFT, padx=(4, 12))
        ttk.Label(meta_row, text="Image ID:").pack(side=tk.LEFT)
        ttk.Entry(meta_row, textvariable=self.ota_image_id_var, width=8).pack(
            side=tk.LEFT, padx=(4, 12))
        self.ota_info_label = ttk.Label(meta_row, text="Size: --  CRC32: --", foreground="gray")
        self.ota_info_label.pack(side=tk.LEFT, fill=tk.X, expand=True)

        action_frame = ttk.LabelFrame(parent, text=self._tr("upload_control"))
        action_frame.pack(fill=tk.X, padx=5, pady=5)

        btn_row = ttk.Frame(action_frame)
        btn_row.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(btn_row, text=self._tr("query_status"), command=self._ota_query_status).pack(side=tk.LEFT)
        ttk.Button(btn_row, text=self._tr("query_boot"), command=self._ota_query_boot).pack(side=tk.LEFT, padx=(5, 0))
        self.btn_ota_start = ttk.Button(btn_row, text=self._tr("start_upload"), command=self._ota_start_upload)
        self.btn_ota_start.pack(side=tk.LEFT, padx=5)
        self.btn_ota_verify_flow = ttk.Button(btn_row, text=self._tr("full_verify"), command=self._ota_start_verify_flow)
        self.btn_ota_verify_flow.pack(side=tk.LEFT, padx=5)
        self.btn_ota_abort = ttk.Button(btn_row, text=self._tr("abort"), command=self._ota_abort,
                                        state=tk.DISABLED)
        self.btn_ota_abort.pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_row, text=self._tr("commit_upgrade"), command=self._ota_commit).pack(side=tk.LEFT, padx=5)

        self.ota_progressbar = ttk.Progressbar(action_frame, variable=self.ota_progress,
                                               maximum=100.0)
        self.ota_progressbar.pack(fill=tk.X, padx=5, pady=(0, 5))
        ttk.Label(action_frame, textvariable=self.ota_status_var).pack(
            fill=tk.X, padx=5, pady=(0, 5))

    def _build_custom_cmd_panel(self, parent: ttk.Frame):
        """自定义命令面板"""
        # 命令输入
        input_frame = ttk.LabelFrame(parent, text=self._tr("manual_cmd_input"))
        input_frame.pack(fill=tk.X, padx=5, pady=5)

        input_row = ttk.Frame(input_frame)
        input_row.pack(fill=tk.X, padx=5, pady=5)

        self.custom_cmd_var = tk.StringVar()
        self.custom_cmd_entry = ttk.Entry(input_row, textvariable=self.custom_cmd_var,
                                          font=("Consolas", 11))
        self.custom_cmd_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
        self.custom_cmd_entry.bind("<Return>", lambda e: self._send_custom_cmd())
        self.custom_cmd_entry.bind("<Up>", self._history_up)
        self.custom_cmd_entry.bind("<Down>", self._history_down)
        self._history_idx = -1

        ttk.Button(input_row, text=self._tr("send"), command=self._send_custom_cmd).pack(
            side=tk.LEFT, padx=5)

        # 历史记录
        hist_frame = ttk.LabelFrame(parent, text=self._tr("cmd_history"))
        hist_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=3)

        self.history_listbox = tk.Listbox(hist_frame, height=8,
                                          font=("Consolas", 9))
        hist_scroll = ttk.Scrollbar(hist_frame, orient=tk.VERTICAL,
                                    command=self.history_listbox.yview)
        self.history_listbox.configure(yscrollcommand=hist_scroll.set)
        self.history_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=3, pady=2)
        hist_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.history_listbox.bind("<Double-Button-1>", self._history_reuse)
        self.history_listbox.bind("<Return>", self._history_reuse)

        ttk.Button(parent, text=self._tr("clear_history"), command=self._clear_history).pack(
            side=tk.RIGHT, padx=5, pady=2)

    def _build_log_panel(self, parent: ttk.LabelFrame):
        """日志输出面板"""
        toolbar = ttk.Frame(parent)
        toolbar.pack(fill=tk.X, padx=3, pady=2)

        ttk.Button(toolbar, text=self._tr("clear_log"), command=self._clear_log).pack(side=tk.LEFT)
        ttk.Button(toolbar, text=self._tr("clear_device_log"), command=self._clear_device_log).pack(
            side=tk.LEFT, padx=(5, 0))
        ttk.Checkbutton(toolbar, text=self._tr("auto_scroll"), variable=self.auto_scroll).pack(
            side=tk.LEFT, padx=10)
        ttk.Checkbutton(toolbar, text=self._tr("live_listen"), variable=self.live_listen,
                        command=self._toggle_live_listen).pack(side=tk.LEFT, padx=4)
        ttk.Button(toolbar, text=self._tr("export_log"), command=self._export_log).pack(
            side=tk.RIGHT, padx=2)

        self.log_text = scrolledtext.ScrolledText(
            parent, height=12, wrap=tk.WORD,
            font=("Cascadia Code", 9), bg="#1E1E1E", fg="#D4D4D4",
            insertbackground="#ffffff",
        )
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=3, pady=2)

        # VS Code 风格日志着色
        self.log_text.tag_config("TIME", foreground="#808080")
        self.log_text.tag_config("CMD", foreground="#569CD6")
        self.log_text.tag_config("OK", foreground="#6A9955")
        self.log_text.tag_config("ERROR", foreground="#F44747")
        self.log_text.tag_config("WARN", foreground="#CE9178")
        self.log_text.tag_config("INFO", foreground="#4FC1FF")
        self.log_text.tag_config("LIVE", foreground="#B5CEA8")
        self.log_text.tag_config("STATE", foreground="#CE9178")
        self.log_text.tag_config("DOOR", foreground="#DCDCAA")
        self.log_text.tag_config("IO", foreground="#569CD6")
        self.log_text.tag_config("RAMVEC", foreground="#C586C0")
        self.log_text.tag_config("RS485", foreground="#FF6B6B")      # RS485通讯异常
        self.log_text.tag_config("EMERGENCY", foreground="#FF4444")  # 急停/紧急状态
        self.log_text.tag_config("E-STOP", foreground="#FF4444")     # 急停按钮
        self.log_text.tag_config("CLOSE", foreground="#DCDCAA")      # 关门过程
        self.log_text.tag_config("EVENT", foreground="#CE9178")      # 事件 (ESTOP/LASER)
        self.log_text.tag_config("RISK", foreground="#FF6B6B")       # 风险模式
        self.log_text.tag_config("LOCK", foreground="#4FC1FF")       # 锁动作
        self.log_text.tag_config("UNLOCK", foreground="#4FC1FF")     # 解锁动作
        self.log_text.tag_config("OPEN", foreground="#569CD6")       # 开门动作

    # ── 串口连接管理 ──────────────────────────────────────────────────
    def _refresh_com_list(self):
        """刷新可用串口列表"""
        if self._ui_rebuilding or not self._widget_exists("com_combo"):
            return
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.com_combo["values"] = ports
        if ports and not self.com_combo.get():
            self.com_combo.current(0)
        self._log(self._tr("ports_found", count=len(ports),
                           ports=", ".join(ports) if ports else self._tr("none")),
                  "INFO")

    def _toggle_connection(self):
        """切换连接状态"""
        if self.serial_worker.is_connected:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        """连接串口"""
        port = self.com_combo.get()
        if not port:
            messagebox.showwarning(self._tr("warn"), self._tr("no_port"))
            return
        try:
            baud = int(self.baud_combo.get())
        except ValueError:
            baud = DEFAULT_BAUD

        try:
            self.serial_worker.connect(port, baud)
            self.serial_worker.set_live_listen(self.live_listen.get())
            self.connected.set(True)
            self.conn_indicator.configure(text=self._tr("connected_dot"), foreground="#00AA00")
            self.conn_detail.configure(text=f"{port} @ {baud} bps", foreground="#00AA00")
            self.btn_connect.configure(state=tk.DISABLED)
            self.btn_disconnect.configure(state=tk.NORMAL)
            self.btn_idn.configure(state=tk.NORMAL)
            self.status_label.configure(text=f"{self._tr('connected')} {port} @ {baud} bps")
            self._log(self._tr("serial_connected", port=port, baud=baud), "INFO")
            # 连接后自动查询设备
            self._cancel_after_job("_query_device_job_id")
            self._query_device_job_id = self.root.after(
                300, lambda: self._run_ui_safe(self._query_device_id))
        except Exception as e:
            messagebox.showerror(self._tr("connect_failed"), str(e))
            self._log(f"{self._tr('connect_failed')}: {e}", "ERROR")

    def _disconnect(self):
        """断开串口"""
        self._stop_polling()
        self._stop_pressure()
        self.serial_worker.disconnect()
        self.connected.set(False)
        self.conn_indicator.configure(text=self._tr("disconnected_dot"), foreground="gray")
        self.conn_detail.configure(text=self._tr("disconnected"), foreground="gray")
        self.btn_connect.configure(state=tk.NORMAL)
        self.btn_disconnect.configure(state=tk.DISABLED)
        self.btn_idn.configure(state=tk.DISABLED)
        self.device_info_label.configure(text=self._tr("device_empty"), foreground="gray")
        self.status_label.configure(text=self._tr("disconnected"))
        self._log(self._tr("serial_disconnected"), "INFO")

    def _toggle_live_listen(self):
        enabled = self.live_listen.get()
        self.serial_worker.set_live_listen(enabled)
        state = self._tr("enabled") if enabled else self._tr("disabled")
        self._log(self._tr("live_listen_state", state=state), "INFO")

    # ── 日志系统 ──────────────────────────────────────────────────────
    def _log(self, message: str, tag: str = "INFO"):
        """写入日志 (VS Code 风格着色)"""
        if not self._widget_exists("log_text"):
            return
        old_state = str(self.log_text.cget("state"))
        if old_state == tk.DISABLED:
            self.log_text.configure(state=tk.NORMAL)
        ts = datetime.now().strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{ts}] ", "TIME")
        self.log_text.insert(tk.END, f"{message}\n", tag)
        line_count = int(self.log_text.index("end-1c").split(".")[0])
        if line_count > MAX_LOG_LINES:
            self.log_text.delete("1.0", f"{line_count - MAX_LOG_LINES}.0")
        if self.auto_scroll.get():
            self.log_text.see(tk.END)
        if old_state == tk.DISABLED:
            self.log_text.configure(state=tk.DISABLED)

    def _clear_log(self):
        if not self._widget_exists("log_text"):
            return
        old_state = str(self.log_text.cget("state"))
        if old_state == tk.DISABLED:
            self.log_text.configure(state=tk.NORMAL)
        self.log_text.delete("1.0", tk.END)
        if old_state == tk.DISABLED:
            self.log_text.configure(state=tk.DISABLED)

    def _clear_device_log(self):
        self._send_scpi("CONFigure:LOG:CLEar")

    def _export_log(self):
        """导出日志到文件"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".log",
            filetypes=[(self._tr("filetypes_log"), "*.log"),
                       (self._tr("filetypes_text"), "*.txt"),
                       (self._tr("filetypes_all"), "*.*")],
            initialfile=f"pinprobe_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"
        )
        if filename:
            with open(filename, "w", encoding="utf-8") as f:
                f.write(self.log_text.get("1.0", tk.END))
            self._log(self._tr("log_exported", filename=filename), "INFO")

    # ── OTA 固件上传 ──────────────────────────────────────────────────
    def _ota_select_file(self):
        path = filedialog.askopenfilename(
            title=self._tr("select_firmware_title"),
            filetypes=[(self._tr("filetypes_bin"), "*.bin"), (self._tr("filetypes_all"), "*.*")]
        )
        if not path:
            return

        self.ota_file_path = Path(path)
        self.ota_file_var.set(str(self.ota_file_path))
        try:
            data = self.ota_file_path.read_bytes()
        except OSError as exc:
            messagebox.showerror(self._tr("read_failed"), str(exc))
            self.ota_file_path = None
            return

        crc = zlib.crc32(data) & 0xFFFFFFFF
        image_ok, image_msg = self._ota_validate_app_image(data)
        self.ota_info_label.configure(
            text=f"Size: {len(data)} bytes  CRC32: 0x{crc:08X}  {image_msg}",
            foreground="#4FC1FF" if image_ok else "#F44747")
        self.ota_status_var.set(self._tr("firmware_selected") if image_ok else self._tr("firmware_vector_invalid"))

    @staticmethod
    def _ota_parse_u32(text: str) -> int:
        return int(text.strip(), 0)

    def _ota_validate_app_image(self, data: bytes) -> tuple[bool, str]:
        if len(data) < 8:
            return False, self._tr("file_too_small")
        if len(data) > OTA_APP_MAX_SIZE:
            return False, self._tr("file_too_large", size=OTA_APP_MAX_SIZE)

        stack = int.from_bytes(data[0:4], "little")
        reset = int.from_bytes(data[4:8], "little")
        sram_end = OTA_SRAM_BASE + OTA_SRAM_SIZE
        app_end = OTA_APP_BASE_ADDR + OTA_APP_MAX_SIZE

        if not (OTA_SRAM_BASE <= stack <= sram_end):
            return False, self._tr("bad_stack", stack=stack)
        if not (OTA_APP_BASE_ADDR <= reset < app_end) or (reset & 1) == 0:
            return False, self._tr("bad_reset", reset=reset)
        return True, self._tr("app_vector_ok", stack=stack, reset=reset)

    def _ota_selected_file_is_valid(self) -> bool:
        if self.ota_file_path is None:
            messagebox.showwarning(self._tr("no_firmware_title"), self._tr("no_firmware_msg"))
            return False
        try:
            data = self.ota_file_path.read_bytes()
        except OSError as exc:
            messagebox.showerror(self._tr("read_failed"), str(exc))
            return False

        image_ok, image_msg = self._ota_validate_app_image(data)
        if not image_ok:
            messagebox.showerror(self._tr("firmware_error_title"), image_msg)
            self.ota_status_var.set(self._tr("firmware_vector_invalid"))
            return False
        return True

    def _ota_query_status(self):
        self._send_scpi("SYSTem:OTA:STATus?")

    def _ota_query_boot(self):
        self._send_scpi("SYSTem:OTA:BOOT?")

    def _ota_commit(self):
        if not self.serial_worker.is_connected:
            messagebox.showwarning(self._tr("not_connected_title"), self._tr("not_connected_msg"))
            return
        if not messagebox.askyesno(self._tr("confirm_commit_title"), self._tr("confirm_commit_msg")):
            return
        self._send_scpi("SYSTem:OTA:COMMit")

    def _ota_prompt_reboot(self):
        if not self.serial_worker.is_connected:
            return
        if messagebox.askyesno(self._tr("reboot_title"), self._tr("reboot_msg")):
            self._send_scpi("SYSTem:REBoot")

    def _ota_abort(self):
        self.ota_abort_requested = True
        self.ota_status_var.set(self._tr("aborting"))

    def _set_ota_controls_idle(self):
        if self._widget_exists("btn_ota_start"):
            self.btn_ota_start.configure(state=tk.NORMAL)
        if self._widget_exists("btn_ota_verify_flow"):
            self.btn_ota_verify_flow.configure(state=tk.NORMAL)
        if self._widget_exists("btn_ota_abort"):
            self.btn_ota_abort.configure(state=tk.DISABLED)

    def _ota_start_upload(self):
        if self.ota_running:
            return
        if not self.serial_worker.is_connected:
            messagebox.showwarning(self._tr("not_connected_title"), self._tr("not_connected_msg"))
            return
        if self.ota_file_path is None:
            messagebox.showwarning(self._tr("no_firmware_title"), self._tr("no_firmware_msg"))
            return
        if not self._ota_selected_file_is_valid():
            return

        try:
            version = self._ota_parse_u32(self.ota_version_var.get())
            image_id = self._ota_parse_u32(self.ota_image_id_var.get())
        except ValueError:
            messagebox.showerror(self._tr("param_error_title"), self._tr("param_error_msg"))
            return

        self._stop_polling()
        self._stop_pressure()
        self.ota_abort_requested = False
        self.ota_running = True
        self.btn_ota_start.configure(state=tk.DISABLED)
        self.btn_ota_abort.configure(state=tk.NORMAL)
        self.ota_progress.set(0.0)
        self.ota_status_var.set(self._tr("preparing_upload"))
        self.ota_thread = threading.Thread(
            target=self._ota_upload_thread,
            args=(self.ota_file_path, version, image_id),
            daemon=True)
        self.ota_thread.start()

    def _ota_start_verify_flow(self):
        if self.ota_running:
            return
        if not self.serial_worker.is_connected:
            messagebox.showwarning(self._tr("not_connected_title"), self._tr("not_connected_msg"))
            return
        if self.ota_file_path is None:
            messagebox.showwarning(self._tr("no_firmware_title"), self._tr("no_firmware_msg"))
            return
        if not self._ota_selected_file_is_valid():
            return

        try:
            version = self._ota_parse_u32(self.ota_version_var.get())
            image_id = self._ota_parse_u32(self.ota_image_id_var.get())
        except ValueError:
            messagebox.showerror(self._tr("param_error_title"), self._tr("param_error_msg"))
            return

        self._stop_polling()
        self._stop_pressure()
        self.ota_abort_requested = False
        self.ota_running = True
        self.btn_ota_start.configure(state=tk.DISABLED)
        self.btn_ota_verify_flow.configure(state=tk.DISABLED)
        self.btn_ota_abort.configure(state=tk.NORMAL)
        self.ota_progress.set(0.0)
        self.ota_status_var.set(self._tr("preparing_verify"))
        self.ota_thread = threading.Thread(
            target=self._ota_verify_flow_thread,
            args=(self.ota_file_path, version, image_id),
            daemon=True)
        self.ota_thread.start()

    def _ota_upload_thread(self, path: Path, version: int, image_id: int):
        self.serial_worker.ota_set_active(True)
        start_time = time.time()
        try:
            data = path.read_bytes()
            image_ok, image_msg = self._ota_validate_app_image(data)
            if not image_ok:
                raise RuntimeError(image_msg)
            size = len(data)
            crc = zlib.crc32(data) & 0xFFFFFFFF
            self.serial_worker.rx_queue.put(("ota", f"BEGIN size={size} crc=0x{crc:08X}"))

            begin = f"SYSTem:OTA:BEGIN {size},{crc},{version},{image_id}\r\n".encode("ascii")
            resp = self._ota_begin_with_busy_recovery(begin, timeout=5.0)
            if not self._ota_response_ok(resp):
                raise RuntimeError(resp or self._tr("stage_no_response", stage="BEGIN"))

            offset = 0
            while offset < size:
                if self.ota_abort_requested:
                    try:
                        self.serial_worker.transact_raw(b"SYSTem:OTA:ABORt\r\n", timeout=OTA_ACK_TIMEOUT)
                    finally:
                        raise RuntimeError(self._tr("user_abort"))

                chunk = data[offset:offset + OTA_CHUNK_SIZE]
                header = f"SYSTem:OTA:DATA {offset},#3{len(chunk):03d}".encode("ascii")
                payload = header + chunk + SCPI_TERMINATOR.encode("ascii")

                resp = ""
                for _ in range(OTA_MAX_RETRIES):
                    resp = self.serial_worker.transact_raw(payload, timeout=OTA_ACK_TIMEOUT)
                    if self._ota_response_ok(resp):
                        break
                    time.sleep(0.05)
                if not self._ota_response_ok(resp):
                    raise RuntimeError(resp or self._tr("stage_no_response", stage=f"DATA offset {offset}"))

                offset += len(chunk)
                progress = (offset * 100.0) / size if size else 100.0
                elapsed = max(time.time() - start_time, 0.001)
                speed = offset / elapsed
                self.serial_worker.rx_queue.put(("ota_progress", progress, offset, size, speed))

            resp = self.serial_worker.transact_raw(b"SYSTem:OTA:END\r\n", timeout=5.0)
            if not self._ota_response_ok(resp):
                raise RuntimeError(resp or self._tr("stage_no_response", stage="END"))

            resp = self.serial_worker.transact_raw(b"SYSTem:OTA:VERify?\r\n", timeout=10.0)
            if "STATE:READY" not in resp:
                raise RuntimeError(resp or self._tr("stage_no_response", stage="VERIFY"))

            self.serial_worker.rx_queue.put(("ota_done", resp))
        except Exception as exc:
            self.serial_worker.rx_queue.put(("ota_error", str(exc)))
        finally:
            self.serial_worker.ota_set_active(False)
            self.ota_running = False
            self.root.after(0, lambda: self._run_ui_safe(self._set_ota_controls_idle))

    @staticmethod
    def _ota_response_ok(resp: str) -> bool:
        return resp.strip().strip('"').startswith("OK")

    def _ota_transact_checked(self, payload: bytes, label: str, timeout: float) -> str:
        resp = self.serial_worker.transact_raw(payload, timeout=timeout)
        timeout_text = self._tr("timeout")
        self.serial_worker.rx_queue.put(("ota", f"{label} => {resp or '<{timeout_text}>'}"))
        if not self._ota_response_ok(resp):
            raise RuntimeError(self._tr("stage_failed", stage=label, resp=resp or timeout_text))
        return resp

    def _ota_begin_with_busy_recovery(self, begin_payload: bytes, timeout: float) -> str:
        resp = self.serial_worker.transact_raw(begin_payload, timeout=timeout)
        timeout_text = self._tr("timeout")
        self.serial_worker.rx_queue.put(("ota", f"BEGIN => {resp or '<' + timeout_text + '>'}"))
        if "ERR,1,BUSY" not in resp:
            return resp

        self.serial_worker.rx_queue.put(("ota", self._tr("ota_begin_busy")))
        abort_resp = self.serial_worker.transact_raw(b"SYSTem:OTA:ABORt\r\n", timeout=3.0)
        self.serial_worker.rx_queue.put(("ota", f"ABORT => {abort_resp or '<' + timeout_text + '>'}"))
        if not self._ota_response_ok(abort_resp):
            return resp

        resp = self.serial_worker.transact_raw(begin_payload, timeout=timeout)
        self.serial_worker.rx_queue.put(("ota", f"BEGIN retry => {resp or '<' + timeout_text + '>'}"))
        return resp

    def _ota_upload_image_raw(self, data: bytes, version: int, image_id: int, start_time: float) -> str:
        size = len(data)
        crc = zlib.crc32(data) & 0xFFFFFFFF
        self.serial_worker.rx_queue.put(("ota", f"BEGIN size={size} crc=0x{crc:08X}"))

        begin = f"SYSTem:OTA:BEGIN {size},{crc},{version},{image_id}\r\n".encode("ascii")
        begin_resp = self._ota_begin_with_busy_recovery(begin, timeout=10.0)
        if not self._ota_response_ok(begin_resp):
            raise RuntimeError(self._tr("stage_failed", stage="BEGIN", resp=begin_resp or self._tr("timeout")))

        offset = 0
        while offset < size:
            if self.ota_abort_requested:
                try:
                    self.serial_worker.transact_raw(b"SYSTem:OTA:ABORt\r\n", timeout=OTA_ACK_TIMEOUT)
                finally:
                    raise RuntimeError(self._tr("user_abort"))

            chunk = data[offset:offset + OTA_CHUNK_SIZE]
            header = f"SYSTem:OTA:DATA {offset},#3{len(chunk):03d}".encode("ascii")
            payload = header + chunk + SCPI_TERMINATOR.encode("ascii")

            resp = ""
            for _ in range(OTA_MAX_RETRIES):
                resp = self.serial_worker.transact_raw(payload, timeout=OTA_ACK_TIMEOUT)
                if self._ota_response_ok(resp):
                    break
                time.sleep(0.05)
            if not self._ota_response_ok(resp):
                raise RuntimeError(resp or self._tr("stage_no_response", stage=f"DATA offset {offset}"))

            offset += len(chunk)
            progress = (offset * 100.0) / size if size else 100.0
            elapsed = max(time.time() - start_time, 0.001)
            speed = offset / elapsed
            self.serial_worker.rx_queue.put(("ota_progress", progress, offset, size, speed))

        self._ota_transact_checked(b"SYSTem:OTA:END\r\n", "END", 8.0)
        verify_resp = self.serial_worker.transact_raw(b"SYSTem:OTA:VERify?\r\n", timeout=15.0)
        self.serial_worker.rx_queue.put(("ota", f"VERIFY => {verify_resp or '<' + self._tr('timeout') + '>'}"))
        if "STATE:READY" not in verify_resp:
            raise RuntimeError(verify_resp or self._tr("stage_no_response", stage="VERIFY"))
        return verify_resp

    def _ota_verify_flow_thread(self, path: Path, version: int, image_id: int):
        self.serial_worker.ota_set_active(True)
        start_time = time.time()
        try:
            data = path.read_bytes()
            image_ok, image_msg = self._ota_validate_app_image(data)
            if not image_ok:
                raise RuntimeError(image_msg)

            for raw in (b"*IDN?\r\n",
                        b"SYSTem:FLASH:ID?\r\n",
                        b"CONFigure:BOOT:DIAG ON\r\n",
                        b"READ:BOOT:DIAG?\r\n"):
                label = raw.decode("ascii").strip()
                resp = self.serial_worker.transact_raw(raw, timeout=5.0)
                self.serial_worker.rx_queue.put(("ota", f"{label} => {resp or '<timeout>'}"))
                if not resp:
                    raise RuntimeError(self._tr("stage_no_response", stage=label))

            verify_resp = self._ota_upload_image_raw(data, version, image_id, start_time)
            boot_before = self.serial_worker.transact_raw(b"SYSTem:OTA:BOOT?\r\n", timeout=5.0)
            self.serial_worker.rx_queue.put(("ota", f"BOOT? before commit => {boot_before or '<timeout>'}"))

            commit_resp = self.serial_worker.transact_raw(b"SYSTem:OTA:COMMit\r\n", timeout=3.0)
            self.serial_worker.rx_queue.put(("ota", f"COMMIT => {commit_resp or '<timeout>'}"))
            if not self._ota_response_ok(commit_resp):
                raise RuntimeError(commit_resp or self._tr("stage_no_response", stage="COMMIT"))

            lines = self.serial_worker.read_raw_lines(OTA_POST_TIMEOUT, stop_text="[POST] === PASSED")
            for line in lines:
                self.serial_worker.rx_queue.put(("ota_line", line))
            line_set = set(lines)
            missing = [marker for marker in OTA_BOOT_MARKERS if marker not in line_set]
            failures = [marker for marker in OTA_BOOT_FAIL_MARKERS if marker in line_set]
            if failures:
                reason_map = OTA_BOOT_FAIL_REASONS_EN if self._current_lang() == "en" else OTA_BOOT_FAIL_REASONS
                reasons = [reason_map.get(marker, marker) for marker in failures]
                raise RuntimeError(self._tr(
                    "bootloader_failed", stages=",".join(failures), reasons="; ".join(reasons)))
            if missing:
                raise RuntimeError(self._tr("bootloader_missing", stages=",".join(missing)))
            if not any("[POST] === PASSED" in line for line in lines):
                raise RuntimeError(self._tr("post_missing"))

            for raw in (b"*IDN?\r\n",
                        b"SYSTem:OTA:BOOT?\r\n",
                        b"CONFigure:BOOT:DIAG OFF\r\n",
                        b"READ:BOOT:DIAG?\r\n"):
                label = raw.decode("ascii").strip()
                resp = self.serial_worker.transact_raw(raw, timeout=5.0)
                self.serial_worker.rx_queue.put(("ota", f"{label} => {resp or '<timeout>'}"))
                if not resp:
                    raise RuntimeError(self._tr("stage_no_response", stage=label))

            self.serial_worker.rx_queue.put(("ota_done", self._tr("full_verify_passed", resp=verify_resp)))
        except Exception as exc:
            self.serial_worker.rx_queue.put(("ota_error", str(exc)))
            try:
                self.serial_worker.transact_raw(b"CONFigure:BOOT:DIAG OFF\r\n", timeout=3.0)
            except Exception:
                pass
        finally:
            self.serial_worker.ota_set_active(False)
            self.ota_running = False
            self.root.after(0, lambda: self._run_ui_safe(self._set_ota_controls_idle))

    # ── SCPI 命令发送 ────────────────────────────────────────────────
    # IDN 设置命令映射
    _IDN_SET_MAP = {
        "__IDN1_SET__": ("idn_mfg", "SYSTem:IDN1"),
        "__IDN2_SET__": ("idn_model", "SYSTem:IDN2"),
        "__IDN3_SET__": ("idn_serial", "SYSTem:IDN3"),
        "__IDN4_SET__": ("idn_fw", "SYSTem:IDN4"),
    }

    def _send_scpi(self, cmd: str):
        """发送 SCPI 命令"""
        if not self.serial_worker.is_connected:
            messagebox.showwarning(self._tr("not_connected_title"), self._tr("not_connected_msg"))
            return
        if self.ota_running:
            messagebox.showwarning(self._tr("ota_busy_title"), self._tr("ota_busy_msg"))
            return
        if self.pressure_running:
            messagebox.showwarning(self._tr("pressure_busy_title"), self._tr("pressure_busy_msg"))
            return

        # IDN 设置：弹窗输入值
        if cmd in self._IDN_SET_MAP:
            label_key, scpi_cmd = self._IDN_SET_MAP[cmd]
            label = self._tr(label_key)
            from tkinter import simpledialog
            value = simpledialog.askstring(self._tr("set_title", label=label),
                                           self._tr("set_prompt", label=label),
                                           parent=self.root)
            if not value:
                return
            cmd = f'{scpi_cmd} "{value}"'

        self._log(f">>> {cmd}", "CMD")
        expect_response = self.serial_worker._expects_response(cmd)
        self.serial_worker.send_command(cmd, expect_response=expect_response)
        self._add_to_history(cmd)

    def _send_custom_cmd(self):
        """发送自定义命令"""
        cmd = self.custom_cmd_var.get().strip()
        if not cmd:
            return
        self._send_scpi(cmd)
        self.custom_cmd_var.set("")
        self._history_idx = -1

    def _add_to_history(self, cmd: str):
        """添加到命令历史"""
        if cmd in self.cmd_history:
            self.cmd_history.remove(cmd)
        self.cmd_history.insert(0, cmd)
        if len(self.cmd_history) > 200:
            self.cmd_history = self.cmd_history[:200]
        self._refresh_history_listbox()

    def _refresh_history_listbox(self):
        if not self._widget_exists("history_listbox"):
            return
        self.history_listbox.delete(0, tk.END)
        for cmd in self.cmd_history[:50]:
            self.history_listbox.insert(tk.END, cmd)

    def _history_up(self, event):
        if not self.cmd_history:
            return
        self._history_idx = min(self._history_idx + 1, len(self.cmd_history) - 1)
        self.custom_cmd_var.set(self.cmd_history[self._history_idx])
        self.custom_cmd_entry.icursor(tk.END)

    def _history_down(self, event):
        if self._history_idx <= 0:
            self._history_idx = -1
            self.custom_cmd_var.set("")
            return
        self._history_idx -= 1
        self.custom_cmd_var.set(self.cmd_history[self._history_idx])
        self.custom_cmd_entry.icursor(tk.END)

    def _history_reuse(self, event=None):
        sel = self.history_listbox.curselection()
        if sel:
            cmd = self.history_listbox.get(sel[0])
            self._send_scpi(cmd)

    def _clear_history(self):
        self.cmd_history.clear()
        self._refresh_history_listbox()

    # ── 设备查询 ──────────────────────────────────────────────────────
    def _query_device_id(self):
        """查询设备 ID"""
        self._query_device_job_id = None
        self._send_scpi("*IDN?")

    # ── 响应处理 ──────────────────────────────────────────────────────
    def _process_responses(self):
        """定时处理串口响应队列"""
        if self._ui_rebuilding or not self._widget_exists("log_text"):
            return
        try:
            while True:
                msg = self.serial_worker.rx_queue.get_nowait()
                msg_type = msg[0]

                if msg_type == "response":
                    _, cmd, resp, elapsed_us = msg
                    elapsed_ms = elapsed_us / 1000
                    tag = "OK" if "ERR" not in resp.upper() else "ERROR"
                    self._log(f"← [{elapsed_ms:.1f}ms] {resp}", tag)
                    self._update_monitor_row(cmd, resp)
                    if cmd.strip() == "SYSTem:OTA:COMMit" and resp.strip().startswith("OK"):
                        self.ota_status_var.set(self._tr("ota_committed"))
                        self._cancel_after_job("_ota_reboot_prompt_job_id")
                        self._ota_reboot_prompt_job_id = self.root.after(
                            100, lambda: self._run_ui_safe(self._ota_prompt_reboot))
                    if cmd.strip() == "*IDN?":
                        if self._widget_exists("device_info_label"):
                            self.device_info_label.configure(
                                text=self._tr("device_value", resp=resp), foreground="#4FC1FF")
                    # IO状态形象化显示
                    if cmd.strip() == "READ:IO:ALL?":
                        self._update_leds_from_response(resp)
                        io_lines = format_io_status(resp, self._current_lang())
                        for line in io_lines:
                            self._log(line, "IO")

                elif msg_type == "debug":
                    _, _, decoded, _ = msg
                    tag = decoded.split()[0].strip("[]")
                    # 复合动作标签映射: CLOSE_START/DONE → CLOSE, OPEN_START/DONE → OPEN
                    if tag.startswith("CLOSE"):   tag = "CLOSE"
                    elif tag.startswith("OPEN"):  tag = "OPEN"
                    self._log(decoded, tag)

                elif msg_type == "live":
                    _, decoded = msg
                    tag = decoded.split()[0].strip("[]") if decoded else "LIVE"
                    if tag.startswith("T+"):
                        tag = "LIVE"
                    elif tag.startswith("CLOSE"):
                        tag = "CLOSE"
                    elif tag.startswith("OPEN"):
                        tag = "OPEN"
                    self._log(decoded, tag)

                elif msg_type == "sent":
                    _, cmd, _, elapsed_us = msg
                    self._log(f"→ [{elapsed_us/1000:.1f}ms] {cmd}", "CMD")

                elif msg_type == "ERROR":
                    _, cmd, err = msg
                    self._log(f"✗ [{cmd}] {err}", "ERROR")
                    self._update_monitor_row(cmd, f"ERROR: {err}")

                elif msg_type == "ota":
                    _, text = msg
                    self._log(f"OTA {text}", "INFO")

                elif msg_type == "ota_line":
                    _, text = msg
                    tag = "OK" if "[POST] === PASSED" in text else "INFO"
                    self._log(f"OTA {text}", tag)

                elif msg_type == "ota_progress":
                    _, progress, offset, size, speed = msg
                    self.ota_progress.set(progress)
                    speed_kib = speed / 1024.0
                    self.ota_status_var.set(
                        self._tr("ota_uploading", offset=offset, size=size,
                                 progress=progress, speed=speed_kib))
                    self.status_label.configure(text=self._tr("ota_status_uploading", progress=progress))

                elif msg_type == "ota_done":
                    _, resp = msg
                    self.ota_progress.set(100.0)
                    self.ota_status_var.set(self._tr("ota_ready"))
                    self.status_label.configure(text=self._tr("ota_status_ready"))
                    self._log(self._tr("ota_done", resp=resp), "OK")

                elif msg_type == "ota_error":
                    _, err = msg
                    self.ota_status_var.set(self._tr("ota_failed", err=err))
                    self.status_label.configure(text=self._tr("ota_failed", err=""))
                    self._log(self._tr("ota_failed", err=err), "ERROR")

        except queue.Empty:
            pass

        except tk.TclError:
            pass

        # 继续定时检查
        self._response_job_id = self.root.after(20, self._process_responses)

    def _update_monitor_row(self, cmd: str, response: str):
        """更新状态监控表格"""
        if not self._widget_exists("monitor_tree"):
            return
        cmd_stripped = cmd.strip()
        for item_id in self.monitor_tree.get_children():
            values = self.monitor_tree.item(item_id, "values")
            if values[2].strip() == cmd_stripped:
                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                tag = "ERROR" if "ERROR" in response or "ERR" in response else "updated"
                self.monitor_tree.item(item_id, values=(values[0], response, values[2], ts),
                                       tags=(tag,))
                break

    # ── 自动轮询 ──────────────────────────────────────────────────────
    _poll_job_id = None

    def _poll_timer(self):
        """自动轮询定时器"""
        if self._ui_rebuilding:
            return
        if self.polling_enabled.get() and self.serial_worker.is_connected:
            self._do_poll_round()
        # 调度下一次
        interval = max(MIN_POLL_INTERVAL_MS, self.poll_interval_ms.get())
        self._poll_job_id = self.root.after(interval, self._poll_timer)

    def _do_poll_round(self):
        """执行一轮状态轮询"""
        if self.pressure_running:
            return
        for name, cmd in AUTO_POLL_COMMANDS:
            self.serial_worker.send_command(cmd, expect_response=True)

    def _manual_poll(self):
        """手动触发一次轮询"""
        if not self.serial_worker.is_connected:
            messagebox.showwarning(self._tr("not_connected_title"), self._tr("not_connected_msg"))
            return
        self._do_poll_round()
        self.poll_status_label.configure(text=self._tr("refreshed"), foreground="#007700")
        self._cancel_after_job("_poll_status_reset_job_id")
        self._poll_status_reset_job_id = self.root.after(1000, self._reset_poll_status_label)

    def _reset_poll_status_label(self):
        self._poll_status_reset_job_id = None
        if not self._widget_exists("poll_status_label"):
            return
        self.poll_status_label.configure(
            text=self._tr("poll_stopped") if not self.polling_enabled.get() else self._tr("poll_running"),
            foreground="gray" if not self.polling_enabled.get() else "#007700")

    def _stop_polling(self):
        self.polling_enabled.set(False)
        if hasattr(self, "poll_status_label"):
            self.poll_status_label.configure(text=self._tr("poll_stopped"), foreground="gray")

    # ── 压力测试 ──────────────────────────────────────────────────────
    def _on_preset_selected(self, event=None):
        """预设选择时显示描述"""
        preset_name = self.preset_display_to_key.get(self.preset_var.get(), self.preset_var.get())
        if preset_name in PRESSURE_PRESETS:
            self.preset_desc_label.configure(
                text=self._preset_description(preset_name))

    def _load_preset(self, log_loaded: bool = True):
        """加载选中的预设"""
        preset_name = self.preset_display_to_key.get(self.preset_var.get(), self.preset_var.get())
        if preset_name not in PRESSURE_PRESETS:
            return
        preset = PRESSURE_PRESETS[preset_name]
        # 填充命令列表
        self.pressure_cmd_text.delete("1.0", tk.END)
        self.pressure_cmd_text.insert("1.0", "\n".join(preset["commands"]))
        # 设置间隔
        self.pressure_interval.set(preset.get("interval_ms", 200))
        self._on_preset_selected()
        if log_loaded:
            self._log(self._tr("loaded_preset", name=self._preset_label(preset_name)), "INFO")

    def _pressure_add_cmd(self):
        """手动添加压力测试命令"""
        cmd = self.custom_cmd_var.get().strip()
        if not cmd:
            cmd = "*IDN?"
        current = self.pressure_cmd_text.get("1.0", tk.END).strip()
        if current:
            self.pressure_cmd_text.insert(tk.END, "\n" + cmd)
        else:
            self.pressure_cmd_text.insert("1.0", cmd)

    def _pressure_del_cmd(self):
        """删除选中的命令"""
        try:
            sel = self.pressure_cmd_text.tag_ranges(tk.SEL)
            if sel:
                self.pressure_cmd_text.delete(sel[0], sel[1])
        except tk.TclError:
            pass

    def _pressure_clear_cmds(self):
        self.pressure_cmd_text.delete("1.0", tk.END)

    def _get_pressure_commands(self) -> list[str]:
        """获取压力测试命令列表"""
        text = self.pressure_cmd_text.get("1.0", tk.END).strip()
        if not text:
            return ["*IDN?"]
        return [line.strip() for line in text.split("\n") if line.strip()]

    def _toggle_pressure(self):
        if self.pressure_running:
            self._stop_pressure()
        else:
            self._start_pressure()

    def _start_pressure(self):
        """开始压力测试"""
        if not self.serial_worker.is_connected:
            messagebox.showwarning(self._tr("not_connected_title"), self._tr("not_connected_msg"))
            return

        commands = self._get_pressure_commands()
        if not commands:
            messagebox.showwarning(self._tr("no_command_title"), self._tr("no_command_msg"))
            return

        if self.polling_enabled.get():
            self.polling_enabled.set(False)
            self.poll_status_label.configure(text=self._tr("poll_stopped_by_pressure"), foreground="#cc0000")

        # 重置统计
        self.pressure_stats = {
            "total": 0, "success": 0, "fail": 0, "timeout": 0,
            "latencies": deque(maxlen=STATS_HISTORY),
            "errors": deque(maxlen=100),
        }

        self.pressure_running = True
        self.btn_pressure_start.configure(state=tk.DISABLED)
        self.btn_pressure_stop.configure(state=tk.NORMAL)
        self.pressure_status_label.configure(text=self._tr("pressure_running"), foreground="#007700")

        # 启动后台线程
        self.pressure_thread = threading.Thread(
            target=self._pressure_loop,
            args=(commands,),
            daemon=True
        )
        self.pressure_thread.start()

        loops = self.pressure_loops.get() if self.pressure_loops.get() > 0 else self._tr("infinite")
        self._log(self._tr("pressure_start_log", count=len(commands),
                           interval=self.pressure_interval.get(), loops=loops), "WARN")

        # 定时更新统计显示
        self._update_pressure_stats()

    def _stop_pressure(self):
        """停止压力测试"""
        self.pressure_running = False
        self._cancel_after_job("_stats_job_id")
        if self._widget_exists("btn_pressure_start"):
            self.btn_pressure_start.configure(state=tk.NORMAL)
        if self._widget_exists("btn_pressure_stop"):
            self.btn_pressure_stop.configure(state=tk.DISABLED)
        if self._widget_exists("pressure_status_label"):
            self.pressure_status_label.configure(text=self._tr("pressure_stopped"), foreground="#cc0000")
        self._log(self._tr("pressure_stopped_log"), "WARN")

    def _pressure_loop(self, commands: list[str]):
        """压力测试工作循环 (后台线程, 独占一次 SCPI 事务后释放串口)"""
        interval_s = self.pressure_interval.get() / 1000.0
        max_loops = self.pressure_loops.get()
        timeout_s = self.pressure_timeout.get() / 1000.0
        loop_count = 0
        cmd_idx = 0

        while self.pressure_running:
            cmd = commands[cmd_idx]
            send_time = time.perf_counter()

            try:
                full_cmd = cmd.strip() + SCPI_TERMINATOR
                expect_response = self.serial_worker._expects_response(cmd)

                with self.serial_worker._lock:
                    if not self.serial_worker.is_connected:
                        raise serial.SerialException(self._tr("serial_not_connected"))

                    self.serial_worker._clear_stale_input_locked()
                    self.serial_worker.serial_port.write(full_cmd.encode("utf-8"))
                    time.sleep(0.03)
                    scpi_resp = ""
                    if expect_response:
                        scpi_resp = self.serial_worker._read_scpi_response_locked(timeout_s)

                elapsed_us = int((time.perf_counter() - send_time) * 1_000_000)
                if expect_response and not scpi_resp:
                    self.pressure_stats["timeout"] += 1
                else:
                    self.pressure_stats["success"] += 1
                self.pressure_stats["latencies"].append(elapsed_us)

            except Exception as e:
                self.pressure_stats["fail"] += 1
                self.pressure_stats["errors"].append(str(e))

            self.pressure_stats["total"] += 1

            # 下一个命令
            cmd_idx = (cmd_idx + 1) % len(commands)
            if cmd_idx == 0:
                loop_count += 1
                if max_loops > 0 and loop_count >= max_loops:
                    # 完成指定循环次数
                    self.root.after(0, self._stop_pressure)
                    break

            # 间隔等待
            time.sleep(interval_s)

    def _update_pressure_stats(self):
        """更新压力测试统计显示 (主线程)"""
        self._stats_job_id = None
        if (not self.pressure_running or self._ui_rebuilding
                or not self._widget_exists("stats_text")
                or not self._widget_exists("pressure_status_label")):
            return

        stats = self.pressure_stats
        latencies = list(stats["latencies"])

        avg_us = sum(latencies) / len(latencies) if latencies else 0
        max_us = max(latencies) if latencies else 0
        min_us = min(latencies) if latencies else 0

        # 计算 P50/P95/P99
        sorted_lat = sorted(latencies)
        p50 = sorted_lat[len(sorted_lat) // 2] if sorted_lat else 0
        p95 = sorted_lat[int(len(sorted_lat) * 0.95)] if len(sorted_lat) >= 20 else 0
        p99 = sorted_lat[int(len(sorted_lat) * 0.99)] if len(sorted_lat) >= 100 else 0

        total = stats["total"]
        success = stats["success"]
        fail = stats["fail"]
        timeout = stats["timeout"]
        success_rate = (success / total * 100) if total > 0 else 0

        # 显示统计
        lines = [
            self._tr("stats_total", total=total, success=success, fail=fail, timeout=timeout),
            self._tr("stats_success", rate=success_rate),
            self._tr("stats_latency_us", avg=avg_us, min=min_us, max=max_us),
            self._tr("stats_latency_ms", avg=avg_us/1000, min=min_us/1000, max=max_us/1000),
            self._tr("stats_percentiles", p50=p50, p95=p95, p99=p99),
        ]

        self.stats_text.configure(state=tk.NORMAL)
        self.stats_text.delete("1.0", tk.END)
        self.stats_text.insert("1.0", "\n".join(lines))
        self.stats_text.configure(state=tk.DISABLED)

        # 更新状态标签
        self.pressure_status_label.configure(
            text=self._tr("pressure_status", total=total, rate=success_rate, avg=avg_us/1000),
            foreground="#007700")

        # 继续定时更新
        self._stats_job_id = self.root.after(200, self._update_pressure_stats)

    # ── 应用退出 ──────────────────────────────────────────────────────
    def on_close(self):
        """窗口关闭处理"""
        self._cancel_ui_jobs()
        if self.ota_running:
            self.ota_abort_requested = True
            if self.ota_thread and self.ota_thread.is_alive():
                self.ota_thread.join(timeout=1.5)
        self._stop_pressure()
        self._stop_polling()
        if self.serial_worker.is_connected:
            self.serial_worker.disconnect()
        self.root.destroy()


# ══════════════════════════════════════════════════════════════════════
# 入口
# ══════════════════════════════════════════════════════════════════════
def main():
    root = tk.Tk()
    app = PinProbeApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)

    root.mainloop()


if __name__ == "__main__":
    main()
