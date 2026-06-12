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
from datetime import datetime
from collections import deque

# ══════════════════════════════════════════════════════════════════════
# 常量
# ══════════════════════════════════════════════════════════════════════
APP_TITLE = "PinProbeA1 调试工具 v1.0"
DEFAULT_BAUD = 115200
SCPI_TERMINATOR = "\r\n"
SERIAL_TIMEOUT = 0.5
MAX_LOG_LINES = 2000
STATS_HISTORY = 1000

BAUD_RATES = [9600, 19200, 38400, 57600, 115200, 230400, 460800]

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
    ],
    "门/气缸": [
        ("开门", "CONFigure:CYLInder1 OPEN"),
        ("关门", "CONFigure:CYLInder1 CLOSE"),
        ("读门状态", "READ:CYLInder1:STATe?"),
        ("USB 插入", "CONFigure:CYLInder2 OPEN"),
        ("USB 拔出", "CONFigure:CYLInder2 CLOSE"),
        ("读 USB 状态", "READ:CYLInder2:STATe?"),
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
    ],
    "系统状态": [
        ("读系统状态", "READ:SYSTem:STATe?"),
        ("读全部IO", "READ:IO:ALL?"),
    ],
    "急停": [
        ("常闭 NC (默认)", "CONFigure:ESTOP:TYPE NC"),
        ("常开 NO", "CONFigure:ESTOP:TYPE NO"),
        ("读急停类型", "CONFigure:ESTOP:TYPE?"),
        ("风险模式 ON", "CONFigure:RISK:MODE ON"),
        ("风险模式 OFF", "CONFigure:RISK:MODE OFF"),
        ("读风险模式", "CONFigure:RISK:MODE?"),
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
        ("动作耗时 ON", "CONFigure:DEBUg:ACTion ON"),
        ("动作耗时 OFF", "CONFigure:DEBUg:ACTion OFF"),
        ("事件打印 ON", "CONFigure:DEBUg:EVENt ON"),
        ("事件打印 OFF", "CONFigure:DEBUg:EVENt OFF"),
        ("IO刷屏 ON", "CONFigure:DEBUg:IO ON"),
        ("IO刷屏 OFF", "CONFigure:DEBUg:IO OFF"),
    ],
}

# 自动轮询的状态查询命令组
AUTO_POLL_COMMANDS = [
    ("系统状态", "READ:SYSTem:STATe?"),
    ("全部IO", "READ:IO:ALL?"),
    ("门状态", "READ:CYLInder1:STATe?"),
    ("USB状态", "READ:CYLInder2:STATe?"),
    ("锁状态", "READ:LOCK:STATe?"),
    ("LED状态", "READ:LED:STATe?"),
]

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
    # 输出 OUT[0]
    ("OUT", 0, 0x01): "开门(open)",
    ("OUT", 0, 0x02): "关门(close)",
    ("OUT", 0, 0x04): "USB进气",
    ("OUT", 0, 0x08): "USB出气",
    ("OUT", 0, 0x10): "绿灯(G)",
    ("OUT", 0, 0x20): "红灯(R)",
    ("OUT", 0, 0x40): "黄灯(Y)",
    ("OUT", 0, 0x80): "电源输出(power)",
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

def format_io_status(response: str) -> list[str]:
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
                lines.append(f"  {state}  {name}")
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
            "READ:LOCK:STATe?",
            "CONFigure:LED GREEN",
            "CONFigure:LED RED",
            "CONFigure:LED YELLOW",
            "CONFigure:LED OFF",
        ],
        "interval_ms": 150,
        "description": "混合读写命令, 模拟真实负载"
    },
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
        self.cmd_queue.put((cmd, expect_response))

    def _worker_loop(self) -> None:
        """后台工作循环: 发送 SCPI 命令 → 读取响应 (过滤固件调试输出)"""
        while self._running:
            try:
                cmd, expect_response = self.cmd_queue.get(timeout=0.05)
            except queue.Empty:
                continue

            try:
                with self._lock:
                    if not self.is_connected:
                        self.rx_queue.put(("ERROR", cmd, "串口未连接"))
                        continue

                    full_cmd = cmd.strip() + SCPI_TERMINATOR
                    send_time = time.perf_counter()
                    self.serial_port.write(full_cmd.encode("utf-8"))
                    time.sleep(0.03)

                if expect_response:
                    deadline = time.time() + SERIAL_TIMEOUT
                    scpi_resp = ""
                    while time.time() < deadline:
                        line = self.serial_port.read_until(b'\n', size=512)
                        if not line:
                            break
                        decoded = line.decode("utf-8", errors="replace").strip()
                        if not decoded:
                            continue
                        # 跳过固件调试输出 (与 state_vector.c 中 printf 前缀对齐)
                        if (decoded.startswith("[STATE]") or
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
                            decoded.startswith("Intake air")):
                            self.rx_queue.put(("debug", "", decoded, 0))
                            continue
                        if decoded.startswith("**ERROR") or decoded.startswith("**SRQ"):
                            scpi_resp = decoded
                            break
                        scpi_resp = decoded
                        break

                    elapsed_us = int((time.perf_counter() - send_time) * 1_000_000)
                    resp_text = scpi_resp if scpi_resp else "(无响应)"
                    self.rx_queue.put(("response", cmd, resp_text, elapsed_us))
                else:
                    elapsed_us = int((time.perf_counter() - send_time) * 1_000_000)
                    self.rx_queue.put(("sent", cmd, "(无需响应)", elapsed_us))

            except serial.SerialException as e:
                self.rx_queue.put(("ERROR", cmd, str(e)))
            except Exception as e:
                self.rx_queue.put(("ERROR", cmd, f"未知错误: {e}"))


# ══════════════════════════════════════════════════════════════════════
# GUI 主窗口
# ══════════════════════════════════════════════════════════════════════
class PinProbeApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title(APP_TITLE)
        self.root.geometry("1300x850")
        self.root.minsize(1100, 700)

        # 串口工作线程
        self.serial_worker = SerialWorker()

        # 状态变量
        self.polling_enabled = tk.BooleanVar(value=False)
        self.poll_interval_ms = tk.IntVar(value=500)
        self.auto_scroll = tk.BooleanVar(value=True)
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

        # 构建 UI
        self._setup_styles()
        self._build_ui()

        # 定时器: 处理串口响应
        self._process_responses()
        # 定时器: 自动轮询
        self._poll_timer()
        # 定时器: 刷新串口列表
        self._refresh_com_list()

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
        # 主容器: 上下分
        main_pw = ttk.PanedWindow(self.root, orient=tk.VERTICAL)
        main_pw.pack(fill=tk.BOTH, expand=True)

        # ---- 上半部分 ----
        top_frame = ttk.Frame(main_pw)
        main_pw.add(top_frame, weight=60)

        # ---- 下半部分: 日志 ----
        bottom_frame = ttk.LabelFrame(main_pw, text="命令日志 / 响应输出")
        main_pw.add(bottom_frame, weight=40)

        # 顶部区域分为左右
        top_pw = ttk.PanedWindow(top_frame, orient=tk.HORIZONTAL)
        top_pw.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        # 左侧: 串口设置 + 命令面板
        left_frame = ttk.Frame(top_pw)
        top_pw.add(left_frame, weight=50)

        # 右侧: 状态监控 + 压力测试
        right_frame = ttk.Frame(top_pw)
        top_pw.add(right_frame, weight=50)

        # ---- 左侧内容 ----
        self._build_serial_panel(left_frame)
        self._build_command_panel(left_frame)

        # ---- 右侧内容 ----
        right_nb = ttk.Notebook(right_frame)
        right_nb.pack(fill=tk.BOTH, expand=True)

        monitor_tab = ttk.Frame(right_nb)
        right_nb.add(monitor_tab, text="状态监控")

        pressure_tab = ttk.Frame(right_nb)
        right_nb.add(pressure_tab, text="压力测试")

        custom_tab = ttk.Frame(right_nb)
        right_nb.add(custom_tab, text="自定义命令")

        self._build_monitor_panel(monitor_tab)
        self._build_pressure_panel(pressure_tab)
        self._build_custom_cmd_panel(custom_tab)

        # ---- 日志区域 ----
        self._build_log_panel(bottom_frame)

        # ---- 状态栏 ----
        status_frame = ttk.Frame(self.root)
        status_frame.pack(fill=tk.X, side=tk.BOTTOM, padx=2, pady=1)
        self.status_label = ttk.Label(status_frame, text="就绪 - 请选择串口并连接",
                                      relief=tk.SUNKEN, anchor=tk.W)
        self.status_label.pack(fill=tk.X)
        self.conn_indicator = ttk.Label(status_frame, text="● 未连接",
                                        foreground="gray", font=("", 10))
        self.conn_indicator.pack(side=tk.RIGHT, padx=5)

    def _build_serial_panel(self, parent: ttk.Frame):
        """串口设置面板"""
        frame = ttk.LabelFrame(parent, text="串口设置")
        frame.pack(fill=tk.X, padx=3, pady=3)

        row1 = ttk.Frame(frame)
        row1.pack(fill=tk.X, padx=5, pady=3)

        ttk.Label(row1, text="端口:").pack(side=tk.LEFT)
        self.com_combo = ttk.Combobox(row1, width=12, state="readonly")
        self.com_combo.pack(side=tk.LEFT, padx=(2, 10))

        ttk.Label(row1, text="波特率:").pack(side=tk.LEFT)
        self.baud_combo = ttk.Combobox(row1, width=10, state="readonly",
                                       values=[str(b) for b in BAUD_RATES])
        self.baud_combo.set(str(DEFAULT_BAUD))
        self.baud_combo.pack(side=tk.LEFT, padx=(2, 10))

        self.btn_refresh = ttk.Button(row1, text="刷新", style="Secondary.TButton",
                                      command=self._refresh_com_list)
        self.btn_refresh.pack(side=tk.LEFT, padx=2)

        row2 = ttk.Frame(frame)
        row2.pack(fill=tk.X, padx=5, pady=3)

        self.btn_connect = ttk.Button(row2, text="连接", style="Success.TButton",
                                      command=self._toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=2)

        self.btn_disconnect = ttk.Button(row2, text="断开", style="Danger.TButton",
                                         command=self._disconnect, state=tk.DISABLED)
        self.btn_disconnect.pack(side=tk.LEFT, padx=2)

        self.conn_detail = ttk.Label(row2, text="未连接", foreground="gray")
        self.conn_detail.pack(side=tk.LEFT, padx=10)

        # 设备信息展示
        row3 = ttk.Frame(frame)
        row3.pack(fill=tk.X, padx=5, pady=3)
        self.device_info_label = ttk.Label(row3, text="设备: ---", foreground="gray")
        self.device_info_label.pack(side=tk.LEFT)

        self.btn_idn = ttk.Button(row3, text="查询设备", style="Secondary.TButton",
                                  command=self._query_device_id, state=tk.DISABLED)
        self.btn_idn.pack(side=tk.RIGHT, padx=2)

    def _build_command_panel(self, parent: ttk.Frame):
        """SCPI 命令快捷面板 (分类折叠)"""
        frame = ttk.LabelFrame(parent, text="SCPI 命令面板")
        frame.pack(fill=tk.BOTH, expand=True, padx=3, pady=3)

        # 搜索框
        search_row = ttk.Frame(frame)
        search_row.pack(fill=tk.X, padx=5, pady=3)
        ttk.Label(search_row, text="搜索:").pack(side=tk.LEFT)
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
        self.cmd_canvas.create_window((0, 0), window=self.cmd_scroll_frame, anchor="nw")
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
                                       text=f"  {category}  ")
            cat_frame.pack(fill=tk.X, padx=3, pady=2)
            self.cmd_button_frames[category] = cat_frame

            # 流式布局按钮
            btn_row = ttk.Frame(cat_frame)
            btn_row.pack(fill=tk.X, padx=3, pady=2)

            col = 0
            for label, cmd in commands:
                btn = ttk.Button(btn_row, text=label, width=18,
                                 command=lambda c=cmd: self._send_scpi(c))
                btn.grid(row=0, column=col, padx=2, pady=2, sticky="w")
                btn.tooltip = cmd  # 附加命令文本用于搜索
                btn.category = category
                self.all_cmd_buttons.append(btn)
                col += 1
                if col >= 3:  # 每行3个
                    btn_row = ttk.Frame(cat_frame)
                    btn_row.pack(fill=tk.X, padx=3, pady=2)
                    col = 0

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

        self.poll_cb = ttk.Checkbutton(ctrl_frame, text="自动轮询",
                                       variable=self.polling_enabled)
        self.poll_cb.pack(side=tk.LEFT)

        ttk.Label(ctrl_frame, text="间隔(ms):").pack(side=tk.LEFT, padx=(10, 2))
        self.poll_interval_spin = ttk.Spinbox(ctrl_frame, from_=100, to=10000,
                                              increment=100, width=7,
                                              textvariable=self.poll_interval_ms)
        self.poll_interval_spin.pack(side=tk.LEFT)

        ttk.Button(ctrl_frame, text="立即刷新", command=self._manual_poll).pack(
            side=tk.LEFT, padx=10)

        self.poll_status_label = ttk.Label(ctrl_frame, text="轮询已停止",
                                           foreground="gray")
        self.poll_status_label.pack(side=tk.RIGHT, padx=5)

        # 状态显示表格
        table_frame = ttk.Frame(parent)
        table_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=2)

        columns = ("name", "value", "raw_cmd", "timestamp")
        self.monitor_tree = ttk.Treeview(table_frame, columns=columns,
                                         show="headings", height=8)
        self.monitor_tree.heading("name", text="状态项")
        self.monitor_tree.heading("value", text="当前值")
        self.monitor_tree.heading("raw_cmd", text="查询命令")
        self.monitor_tree.heading("timestamp", text="更新时间")
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
            self.monitor_tree.insert("", tk.END, values=(name, "---", cmd, ""))

        # 颜色标签
        self.monitor_tree.tag_configure("updated", foreground="#007700")
        self.monitor_tree.tag_configure("ERROR", foreground="#cc0000")
        self.monitor_tree.tag_configure("stale", foreground="#999999")

        # ---- LED 指示灯面板 ----
        self._build_led_panel(parent)

    def _build_led_panel(self, parent: ttk.Frame):
        """IO状态 LED 指示灯 (两排: 输入/输出)"""
        led_frame = ttk.LabelFrame(parent, text="IO 指示灯")
        led_frame.pack(fill=tk.X, padx=5, pady=3)

        # 标题行 + 刷新按钮
        title_row = ttk.Frame(led_frame)
        title_row.pack(fill=tk.X, padx=5, pady=(3, 0))
        ttk.Button(title_row, text="刷新IO", width=10,
                   command=lambda: self._send_scpi("READ:IO:ALL?")).pack(side=tk.RIGHT)

        # LED 定义: (类型, 字节索引, 位掩码, 标签名) — 全部16位
        self.led_defs_input = [
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
            ("IN", 1, 0x20, "IN14"),
            ("IN", 1, 0x40, "IN15"),
            ("IN", 1, 0x80, "IN16"),
        ]
        self.led_defs_output = [
            ("OUT", 0, 0x01, "开门"),
            ("OUT", 0, 0x02, "关门"),
            ("OUT", 0, 0x04, "USB进气"),
            ("OUT", 0, 0x08, "USB出气"),
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

        LED_R = 10           # 灯半径
        LED_GAP = 20          # 灯间距
        LABEL_H = 18          # 标签高度
        ROW_H = LED_R * 2 + LABEL_H + 8

        # 输入行
        in_label = ttk.Label(led_frame, text="输入", font=("", 9, "bold"))
        in_label.pack(anchor=tk.W, padx=5, pady=(3, 0))
        in_count = len(self.led_defs_input)
        in_width = in_count * (LED_R * 2 + LED_GAP) + LED_GAP
        self.canvas_in = tk.Canvas(led_frame, width=in_width, height=ROW_H,
                                    bg="#ffffff", highlightthickness=0)
        self.canvas_in.pack(padx=5, pady=2)

        # 输出行
        out_label = ttk.Label(led_frame, text="输出", font=("", 9, "bold"))
        out_label.pack(anchor=tk.W, padx=5, pady=(5, 0))
        out_count = len(self.led_defs_output)
        out_width = out_count * (LED_R * 2 + LED_GAP) + LED_GAP
        self.canvas_out = tk.Canvas(led_frame, width=out_width, height=ROW_H,
                                     bg="#ffffff", highlightthickness=0)
        self.canvas_out.pack(padx=5, pady=2)

        # 绘制 LED 灯
        self._draw_leds(self.canvas_in, self.led_defs_input, LED_R, LED_GAP, LABEL_H)
        self._draw_leds(self.canvas_out, self.led_defs_output, LED_R, LED_GAP, LABEL_H)

        # IO 原始值显示
        self.io_raw_label = ttk.Label(led_frame, text="---",
                                       font=("Cascadia Code", 10, "bold"),
                                       foreground="#2c3e50")
        self.io_raw_label.pack(pady=(6, 3))

    def _draw_leds(self, canvas: tk.Canvas, defs: list, r: int, gap: int, lh: int):
        """在 Canvas 上绘制 LED 灯（圆+标签），返回 circle ID 列表"""
        ids = []
        x = gap + r
        y_center = r + 4
        y_label = y_center + r + 2

        for io_type, byte_idx, mask, label in defs:
            # LED 圆（初始灰色=off）
            cid = canvas.create_oval(x - r, y_center - r, x + r, y_center + r,
                                     fill="#cccccc", outline="#999999", width=1)
            # 标签
            tid = canvas.create_text(x, y_label, text=label,
                                     font=("Microsoft YaHei UI", 7), fill="#666666")
            ids.append((cid, tid, io_type, byte_idx, mask))
            x += r * 2 + gap

        # 保存 ID 引用
        if not hasattr(self, '_led_canvas_ids'):
            self._led_canvas_ids = {}
        self._led_canvas_ids[canvas] = ids

    def _update_leds_from_response(self, response: str):
        """根据 READ:IO:ALL? 响应更新所有 LED 指示灯和原始值显示"""
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
            for cid, tid, io_type, byte_idx, mask in ids:
                val = data.get((io_type, byte_idx), 0)
                active = (val & mask) != 0
                color = ON_COLOR if active else OFF_COLOR
                outline = "#00AA00" if active else "#999999"
                canvas.itemconfigure(cid, fill=color, outline=outline)

    def _build_pressure_panel(self, parent: ttk.Frame):
        """压力测试面板"""
        # ---- 预设选择 ----
        preset_frame = ttk.LabelFrame(parent, text="测试预设")
        preset_frame.pack(fill=tk.X, padx=5, pady=3)

        preset_row = ttk.Frame(preset_frame)
        preset_row.pack(fill=tk.X, padx=5, pady=3)

        ttk.Label(preset_row, text="预设:").pack(side=tk.LEFT)
        self.preset_var = tk.StringVar()
        self.preset_combo = ttk.Combobox(preset_row, textvariable=self.preset_var,
                                         state="readonly", width=25)
        self.preset_combo["values"] = list(PRESSURE_PRESETS.keys())
        if PRESSURE_PRESETS:
            self.preset_combo.current(0)
        self.preset_combo.pack(side=tk.LEFT, padx=5)
        self.preset_combo.bind("<<ComboboxSelected>>", self._on_preset_selected)

        ttk.Button(preset_row, text="加载预设", command=self._load_preset).pack(
            side=tk.LEFT, padx=5)

        self.preset_desc_label = ttk.Label(preset_row, text="", foreground="gray")
        self.preset_desc_label.pack(side=tk.LEFT, padx=10)

        # ---- 命令列表 ----
        cmdlist_frame = ttk.LabelFrame(parent, text="测试命令序列 (顺序循环)")
        cmdlist_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=3)

        list_toolbar = ttk.Frame(cmdlist_frame)
        list_toolbar.pack(fill=tk.X, padx=3, pady=2)

        ttk.Button(list_toolbar, text="添加", command=self._pressure_add_cmd).pack(
            side=tk.LEFT, padx=2)
        ttk.Button(list_toolbar, text="删除选中", command=self._pressure_del_cmd).pack(
            side=tk.LEFT, padx=2)
        ttk.Button(list_toolbar, text="清空", command=self._pressure_clear_cmds).pack(
            side=tk.LEFT, padx=2)

        self.pressure_cmd_text = tk.Text(cmdlist_frame, height=5, wrap=tk.WORD)
        pressure_cmd_scroll = ttk.Scrollbar(cmdlist_frame, orient=tk.VERTICAL,
                                            command=self.pressure_cmd_text.yview)
        self.pressure_cmd_text.configure(yscrollcommand=pressure_cmd_scroll.set)
        self.pressure_cmd_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=3, pady=2)
        pressure_cmd_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # ---- 参数设置 ----
        param_frame = ttk.LabelFrame(parent, text="测试参数")
        param_frame.pack(fill=tk.X, padx=5, pady=3)

        p_row = ttk.Frame(param_frame)
        p_row.pack(fill=tk.X, padx=5, pady=3)

        ttk.Label(p_row, text="命令间隔(ms):").pack(side=tk.LEFT)
        self.pressure_interval = tk.IntVar(value=200)
        ttk.Spinbox(p_row, from_=10, to=60000, increment=10, width=8,
                    textvariable=self.pressure_interval).pack(side=tk.LEFT, padx=2)

        ttk.Label(p_row, text="循环次数 (0=无限):").pack(side=tk.LEFT, padx=(15, 2))
        self.pressure_loops = tk.IntVar(value=0)
        ttk.Spinbox(p_row, from_=0, to=1000000, increment=100, width=8,
                    textvariable=self.pressure_loops).pack(side=tk.LEFT, padx=2)

        ttk.Label(p_row, text="超时(ms):").pack(side=tk.LEFT, padx=(15, 2))
        self.pressure_timeout = tk.IntVar(value=2000)
        ttk.Spinbox(p_row, from_=50, to=30000, increment=100, width=8,
                    textvariable=self.pressure_timeout).pack(side=tk.LEFT, padx=2)

        # ---- 控制按钮 ----
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)

        self.btn_pressure_start = ttk.Button(btn_frame, text="▶ 开始压力测试",
                                             command=self._toggle_pressure)
        self.btn_pressure_start.pack(side=tk.LEFT, padx=2)

        self.btn_pressure_stop = ttk.Button(btn_frame, text="■ 停止", style="Danger.TButton",
                                            command=self._stop_pressure,
                                            state=tk.DISABLED)
        self.btn_pressure_stop.pack(side=tk.LEFT, padx=2)

        self.pressure_status_label = ttk.Label(btn_frame, text="就绪",
                                               foreground="gray")
        self.pressure_status_label.pack(side=tk.LEFT, padx=15)

        # ---- 统计信息 ----
        stats_frame = ttk.LabelFrame(parent, text="实时统计")
        stats_frame.pack(fill=tk.X, padx=5, pady=3)

        self.stats_text = tk.Text(stats_frame, height=5, state=tk.DISABLED,
                                  font=("Consolas", 9))
        self.stats_text.pack(fill=tk.BOTH, expand=True, padx=3, pady=2)

    def _build_custom_cmd_panel(self, parent: ttk.Frame):
        """自定义命令面板"""
        # 命令输入
        input_frame = ttk.LabelFrame(parent, text="手动命令输入 (按 Enter 发送)")
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

        ttk.Button(input_row, text="发送", command=self._send_custom_cmd).pack(
            side=tk.LEFT, padx=5)

        # 历史记录
        hist_frame = ttk.LabelFrame(parent, text="命令历史 (点击重用)")
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

        ttk.Button(parent, text="清空历史", command=self._clear_history).pack(
            side=tk.RIGHT, padx=5, pady=2)

    def _build_log_panel(self, parent: ttk.LabelFrame):
        """日志输出面板"""
        toolbar = ttk.Frame(parent)
        toolbar.pack(fill=tk.X, padx=3, pady=2)

        ttk.Button(toolbar, text="清空日志", command=self._clear_log).pack(side=tk.LEFT)
        ttk.Checkbutton(toolbar, text="自动滚动", variable=self.auto_scroll).pack(
            side=tk.LEFT, padx=10)
        ttk.Button(toolbar, text="导出日志", command=self._export_log).pack(
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
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.com_combo["values"] = ports
        if ports and not self.com_combo.get():
            self.com_combo.current(0)
        self._log(f"检测到 {len(ports)} 个串口: {', '.join(ports) if ports else '无'}",
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
            messagebox.showwarning("警告", "请先选择串口")
            return
        try:
            baud = int(self.baud_combo.get())
        except ValueError:
            baud = DEFAULT_BAUD

        try:
            self.serial_worker.connect(port, baud)
            self.connected.set(True)
            self.conn_indicator.configure(text="● 已连接", foreground="#00AA00")
            self.conn_detail.configure(text=f"{port} @ {baud} bps", foreground="#00AA00")
            self.btn_connect.configure(state=tk.DISABLED)
            self.btn_disconnect.configure(state=tk.NORMAL)
            self.btn_idn.configure(state=tk.NORMAL)
            self.status_label.configure(text=f"已连接 {port} @ {baud} bps")
            self._log(f"串口已连接: {port} @ {baud} bps", "INFO")
            # 连接后自动查询设备
            self.root.after(300, self._query_device_id)
        except Exception as e:
            messagebox.showerror("连接失败", str(e))
            self._log(f"连接失败: {e}", "ERROR")

    def _disconnect(self):
        """断开串口"""
        self._stop_polling()
        self._stop_pressure()
        self.serial_worker.disconnect()
        self.connected.set(False)
        self.conn_indicator.configure(text="● 未连接", foreground="gray")
        self.conn_detail.configure(text="未连接", foreground="gray")
        self.btn_connect.configure(state=tk.NORMAL)
        self.btn_disconnect.configure(state=tk.DISABLED)
        self.btn_idn.configure(state=tk.DISABLED)
        self.device_info_label.configure(text="设备: ---", foreground="gray")
        self.status_label.configure(text="已断开连接")
        self._log("串口已断开", "INFO")

    # ── 日志系统 ──────────────────────────────────────────────────────
    def _log(self, message: str, tag: str = "INFO"):
        """写入日志 (VS Code 风格着色)"""
        ts = datetime.now().strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{ts}] ", "TIME")
        self.log_text.insert(tk.END, f"{message}\n", tag)
        line_count = int(self.log_text.index("end-1c").split(".")[0])
        if line_count > MAX_LOG_LINES:
            self.log_text.delete("1.0", f"{line_count - MAX_LOG_LINES}.0")
        if self.auto_scroll.get():
            self.log_text.see(tk.END)

    def _clear_log(self):
        self.log_text.delete("1.0", tk.END)

    def _export_log(self):
        """导出日志到文件"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".log",
            filetypes=[("日志文件", "*.log"), ("文本文件", "*.txt"), ("所有文件", "*.*")],
            initialfile=f"pinprobe_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"
        )
        if filename:
            with open(filename, "w", encoding="utf-8") as f:
                f.write(self.log_text.get("1.0", tk.END))
            self._log(f"日志已导出到: {filename}", "INFO")

    # ── SCPI 命令发送 ────────────────────────────────────────────────
    # IDN 设置命令映射
    _IDN_SET_MAP = {
        "__IDN1_SET__": ("厂商名", "SYSTem:IDN1"),
        "__IDN2_SET__": ("产品型号", "SYSTem:IDN2"),
        "__IDN3_SET__": ("序列号/日期", "SYSTem:IDN3"),
        "__IDN4_SET__": ("固件版本", "SYSTem:IDN4"),
    }

    def _send_scpi(self, cmd: str):
        """发送 SCPI 命令"""
        if not self.serial_worker.is_connected:
            messagebox.showwarning("未连接", "请先连接串口")
            return

        # IDN 设置：弹窗输入值
        if cmd in self._IDN_SET_MAP:
            label, scpi_cmd = self._IDN_SET_MAP[cmd]
            from tkinter import simpledialog
            value = simpledialog.askstring(f"设置 {label}",
                                           f"请输入新的{label}：",
                                           parent=self.root)
            if not value:
                return
            cmd = f'{scpi_cmd} "{value}"'

        self._log(f">>> {cmd}", "CMD")
        self.serial_worker.send_command(cmd, expect_response=cmd.endswith("?"))
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
        self._send_scpi("*IDN?")

    # ── 响应处理 ──────────────────────────────────────────────────────
    def _process_responses(self):
        """定时处理串口响应队列"""
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
                    if cmd.strip() == "*IDN?":
                        self.device_info_label.configure(
                            text=f"设备: {resp}", foreground="#4FC1FF")
                    # IO状态形象化显示
                    if cmd.strip() == "READ:IO:ALL?":
                        self._update_leds_from_response(resp)
                        io_lines = format_io_status(resp)
                        for line in io_lines:
                            self._log(line, "IO")

                elif msg_type == "debug":
                    _, _, decoded, _ = msg
                    tag = decoded.split()[0].strip("[]")
                    # 复合动作标签映射: CLOSE_START/DONE → CLOSE, OPEN_START/DONE → OPEN
                    if tag.startswith("CLOSE"):   tag = "CLOSE"
                    elif tag.startswith("OPEN"):  tag = "OPEN"
                    self._log(decoded, tag)

                elif msg_type == "sent":
                    _, cmd, _, elapsed_us = msg
                    self._log(f"→ [{elapsed_us/1000:.1f}ms] {cmd}", "CMD")

                elif msg_type == "ERROR":
                    _, cmd, err = msg
                    self._log(f"✗ [{cmd}] {err}", "ERROR")
                    self._update_monitor_row(cmd, f"ERROR: {err}")

        except queue.Empty:
            pass

        # 继续定时检查
        self.root.after(20, self._process_responses)

    def _update_monitor_row(self, cmd: str, response: str):
        """更新状态监控表格"""
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
        if self.polling_enabled.get() and self.serial_worker.is_connected:
            self._do_poll_round()
        # 调度下一次
        interval = max(100, self.poll_interval_ms.get())
        self._poll_job_id = self.root.after(interval, self._poll_timer)

    def _do_poll_round(self):
        """执行一轮状态轮询"""
        for name, cmd in AUTO_POLL_COMMANDS:
            self.serial_worker.send_command(cmd, expect_response=True)

    def _manual_poll(self):
        """手动触发一次轮询"""
        if not self.serial_worker.is_connected:
            messagebox.showwarning("未连接", "请先连接串口")
            return
        self._do_poll_round()
        self.poll_status_label.configure(text="已刷新", foreground="#007700")
        self.root.after(1000, lambda: self.poll_status_label.configure(
            text="轮询已停止" if not self.polling_enabled.get() else "轮询运行中",
            foreground="gray" if not self.polling_enabled.get() else "#007700"))

    def _stop_polling(self):
        self.polling_enabled.set(False)
        self.poll_status_label.configure(text="轮询已停止", foreground="gray")

    # ── 压力测试 ──────────────────────────────────────────────────────
    def _on_preset_selected(self, event=None):
        """预设选择时显示描述"""
        preset_name = self.preset_var.get()
        if preset_name in PRESSURE_PRESETS:
            self.preset_desc_label.configure(
                text=PRESSURE_PRESETS[preset_name].get("description", ""))

    def _load_preset(self):
        """加载选中的预设"""
        preset_name = self.preset_var.get()
        if preset_name not in PRESSURE_PRESETS:
            return
        preset = PRESSURE_PRESETS[preset_name]
        # 填充命令列表
        self.pressure_cmd_text.delete("1.0", tk.END)
        self.pressure_cmd_text.insert("1.0", "\n".join(preset["commands"]))
        # 设置间隔
        self.pressure_interval.set(preset.get("interval_ms", 200))
        self._log(f"已加载预设: {preset_name}", "INFO")

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
            messagebox.showwarning("未连接", "请先连接串口")
            return

        commands = self._get_pressure_commands()
        if not commands:
            messagebox.showwarning("无命令", "请添加至少一条测试命令")
            return

        # 重置统计
        self.pressure_stats = {
            "total": 0, "success": 0, "fail": 0, "timeout": 0,
            "latencies": deque(maxlen=STATS_HISTORY),
            "errors": deque(maxlen=100),
        }

        self.pressure_running = True
        self.btn_pressure_start.configure(state=tk.DISABLED)
        self.btn_pressure_stop.configure(state=tk.NORMAL)
        self.pressure_status_label.configure(text="运行中...", foreground="#007700")

        # 启动后台线程
        self.pressure_thread = threading.Thread(
            target=self._pressure_loop,
            args=(commands,),
            daemon=True
        )
        self.pressure_thread.start()

        self._log(f"压力测试开始: {len(commands)} 条命令, "
                  f"间隔 {self.pressure_interval.get()}ms, "
                  f"循环 {self.pressure_loops.get() if self.pressure_loops.get() > 0 else '无限'} 次",
                  "WARN")

        # 定时更新统计显示
        self._update_pressure_stats()

    def _stop_pressure(self):
        """停止压力测试"""
        self.pressure_running = False
        self.btn_pressure_start.configure(state=tk.NORMAL)
        self.btn_pressure_stop.configure(state=tk.DISABLED)
        self.pressure_status_label.configure(text="已停止", foreground="#cc0000")
        self._log("压力测试已停止", "WARN")

    def _pressure_loop(self, commands: list[str]):
        """压力测试工作循环 (后台线程, 直接操作串口以降低延迟)"""
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
                self.serial_worker.serial_port.write(full_cmd.encode("utf-8"))
                time.sleep(0.03)

                if cmd.endswith("?"):
                    deadline = time.time() + timeout_s
                    scpi_resp = ""
                    while time.time() < deadline:
                        line = self.serial_worker.serial_port.read_until(b'\n', size=512)
                        if not line:
                            break
                        decoded = line.decode("utf-8", errors="replace").strip()
                        if not decoded:
                            continue
                        if (decoded.startswith("[STATE]") or
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
                            decoded.startswith("Intake air")):
                            continue
                        scpi_resp = decoded
                        break

                    elapsed_us = int((time.perf_counter() - send_time) * 1_000_000)
                    if scpi_resp:
                        self.pressure_stats["success"] += 1
                    else:
                        self.pressure_stats["timeout"] += 1
                    self.pressure_stats["latencies"].append(elapsed_us)
                else:
                    elapsed_us = int((time.perf_counter() - send_time) * 1_000_000)
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
        if not self.pressure_running:
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
            f"总发送: {total}    成功: {success}    失败: {fail}    超时: {timeout}",
            f"成功率: {success_rate:.2f}%",
            f"延迟 (μs):  平均={avg_us:.0f}  最小={min_us:.0f}  最大={max_us:.0f}",
            f"延迟 (ms):  平均={avg_us/1000:.2f}  最小={min_us/1000:.2f}  最大={max_us/1000:.2f}",
            f"P50={p50:.0f}μs  P95={p95:.0f}μs  P99={p99:.0f}μs",
        ]

        self.stats_text.configure(state=tk.NORMAL)
        self.stats_text.delete("1.0", tk.END)
        self.stats_text.insert("1.0", "\n".join(lines))
        self.stats_text.configure(state=tk.DISABLED)

        # 更新状态标签
        self.pressure_status_label.configure(
            text=f"运行中 | 发送:{total} 成功率:{success_rate:.1f}% 平均延迟:{avg_us/1000:.2f}ms",
            foreground="#007700")

        # 继续定时更新
        self.root.after(200, self._update_pressure_stats)

    # ── 应用退出 ──────────────────────────────────────────────────────
    def on_close(self):
        """窗口关闭处理"""
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

    # 启动时自动刷新串口列表
    root.after(100, app._refresh_com_list)
    # 加载默认预设
    root.after(200, app._load_preset)

    root.mainloop()


if __name__ == "__main__":
    main()
