import { writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

// Fixed coordinates keep exported diagrams independent of layout libraries.
const esc = (s) => String(s).replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;');
class Diagram {
  constructor(title, subtitle, height) {
    this.height = height;
    this.parts = [`<svg xmlns="http://www.w3.org/2000/svg" width="1680" height="${height}" viewBox="0 0 1680 ${height}" role="img" aria-labelledby="title desc">
<title id="title">${esc(title)}</title><desc id="desc">${esc(subtitle)}</desc>
<defs><marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse"><path d="M0 0L10 5L0 10Z" fill="#52616b"/></marker></defs>
<style>
text {font-family:'Microsoft YaHei','Noto Sans CJK SC',sans-serif;fill:#202b33;letter-spacing:0}
.title {font-size:34px;font-weight:700}.sub {font-size:18px;fill:#52616b}
.heading {font-size:23px;font-weight:700}.body {font-size:19px}.small {font-size:17px;fill:#52616b}
.edge {fill:none;stroke:#52616b;stroke-width:2;marker-end:url(#arrow)}
.label {font-size:17px;paint-order:stroke;stroke:#fff;stroke-width:7;stroke-linejoin:round}
.box {stroke-width:1.7;rx:6}.state {fill:#edf6fd;stroke:#327bb0}
.action {fill:#f1f8f4;stroke:#43856a;stroke-dasharray:7 4}
.check {fill:#fff9df;stroke:#a88a21}.fault {fill:#fff0f1;stroke:#bf5365}
.neutral {fill:#f5f7f8;stroke:#89969e}.info {fill:#fff;stroke:#bac5cb}
</style><rect width="1680" height="${height}" fill="#fff"/>`];
    this.text(44, 58, title, 'title');
    this.text(44, 94, subtitle, 'sub');
    this.parts.push('<path d="M44 118H1636" stroke="#cdd5da"/>');
  }
  text(x, y, value, cls = 'body', anchor = 'start') {
    this.parts.push(`<text x="${x}" y="${y}" class="${cls}" text-anchor="${anchor}">${esc(value)}</text>`);
  }
  box(x, y, w, h, title, lines = [], cls = 'info') {
    this.parts.push(`<g data-box="${esc(title)}"><rect class="box ${cls}" x="${x}" y="${y}" width="${w}" height="${h}"/>`);
    this.text(x + 20, y + 34, title, 'heading');
    lines.forEach((line, i) => this.text(x + 20, y + 66 + i * 26, line));
    this.parts.push('</g>');
  }
  arrow(points, label, x, y) {
    this.parts.push(`<path class="edge" d="M${points.map(p => p.join(' ')).join('L')}"/>`);
    if (label) this.text(x, y, label, 'label');
  }
  down(x, y1, y2, label = '') { this.arrow([[x, y1], [x, y2]], label, x + 14, (y1 + y2) / 2 + 5); }
  section(x, y, label) { this.text(x, y, label, 'heading'); }
  save(name, source) {
    this.text(44, this.height - 50, source, 'small');
    this.text(44, this.height - 22, '依据工作区源码绘制 · 2026-09-15 · 条件以实际代码为准；时间为软件阈值，包含轮询与调度误差。', 'small');
    this.parts.push('</svg>');
    writeFileSync(fileURLToPath(new URL(name, import.meta.url)), this.parts.join('\n'), 'utf8');
  }
}

{
  const d = new Diagram('01  PinProbe A1 主状态机', '蓝色：主状态　绿色虚线：动作投递　黄色：条件 / 时序　红色：异常；箭头表示条件满足后的流转。', 2060);
  const x = 560, w = 500, c = 810;
  d.box(x, 160, w, 90, 'INIT  ·  6', ['首次进入有效 IO 处理轮次'], 'state');
  d.down(c, 250, 300, '直接转入');
  d.box(x, 300, w, 110, 'LOCK  ·  0', ['电源锁输出关闭：OUT_POWER = 0', '电源按钮按住 300 ms 可投递解锁'], 'state');
  d.down(c, 410, 490, 'IO 回读：OUT_POWER = 1');
  d.box(x, 490, w, 140, 'IDLE  ·  1', ['空闲 / 观察门位置', '门开到位时可自动纠偏 USB 为拔出', '任意门按钮满 200 ms → 投递黄灯'], 'state');
  d.down(c, 630, 710, '黄灯已亮 +（门上限位或位置确认）');
  d.box(x, 710, w, 140, 'READY  ·  2', ['准备关门；确认条件满足后检查 DUT', 'USB:AUTO ON：插入到位后投递关门', 'USB:AUTO OFF：直接投递关门'], 'state');
  d.down(c, 850, 940, '关门输出有效 + 门尚未关到底');
  d.box(x, 940, w, 140, 'RUNNING  ·  3', ['关门进行中；由输出沿记录起始时间', '观察门下限位 / 风险模式完成条件', '超过预计行程 2/3 后启用激光触发窗口'], 'state');
  d.down(c, 1080, 1170, '关门输出 +（下限位或风险模式条件）');
  d.box(x, 1170, w, 168, 'COMPLETE  ·  5', ['关门完成；正常投递绿灯', '记录耗时，符合条件则学习全行程时间', '单门按钮满 200 ms → 投递开门', '开门过程中仍保持 COMPLETE'], 'state');
  d.arrow([[560, 1270], [520, 1270], [520, 558], [560, 558]]);
  d.box(44, 1165, 435, 170, 'COMPLETE → IDLE', ['开门输出有效 + 门上限位确认', '投递关灯、记录 OPEN_DONE', '清理本周期位置与关门完成计时', '随后满足条件才自动拔出 USB'], 'action');
  d.box(44, 490, 435, 170, 'IO 自动纠偏', ['非 EMERGENCY：电源锁输出为 0', '→ 转入 LOCK', 'IDLE 位置分支读到门下限位', '→ 可直接转入 COMPLETE'], 'info');
  d.box(44, 690, 435, 170, 'READY 返回条件', ['黄灯回读不再为黄色 → IDLE', '本轮仍可能继续检测关门输出', 'SCPI 关门输出上升沿也可使', 'IDLE / READY / COMPLETE → RUNNING'], 'info');
  d.box(44, 885, 435, 224, '关门完成条件', ['正常：关门输出有效且下限位有效', '风险：RISK 开启 + 关门输出有效', '+ IN_LASER1 = 1 + 有关门起始时间', '+ 耗时 > 预计关门时间', '预计时间初值 2500 ms；符合条件学习', '当前没有通用“关门超时即故障”分支'], 'check');
  d.box(1150, 160, 486, 196, '安全触发入口（IO 可信时）', ['急停：按配置解析 NC / NO 有效态', '检测到急停有效沿 → EMERGENCY', '激光：关门计时中、门未到底，', '且耗时 > 预计关门时间 × 2/3', '并且 IS_ANY_LASER 非零 → EMERGENCY'], 'fault');
  d.down(1393, 356, 405);
  d.box(1150, 405, 486, 196, 'EMERGENCY  ·  4', ['安全优先级投递：红灯 + 锁定', '急停时门不在上限位 → 投递开门', '激光触发 → 投递开门、清关门计时', '恢复：急停无效且 IS_ANY_LASER = 0', '→ LOCK；随后按实际电源输出纠偏'], 'fault');
  d.arrow([[1150, 440], [1105, 440], [1105, 355], [1060, 355]], '恢复', 1109, 389);
  d.box(1150, 660, 486, 224, 'USB 底层异常', ['AUTO 开启时：超时 / 传感器冲突', '/ 插拔双输出冲突 → 对应失败日志', '无急停 / 激光紧急时退回 IDLE', '清关门待执行意图，等待按钮释放', '红灯 3 次快闪（每 125 ms 翻转）', '插入失败且门开到位：投递 USB 回退'], 'fault');
  d.box(1150, 940, 486, 168, 'IO 不可信 / 恢复中', ['本轮提前返回，两个气缸镜像置 ERR', '清 USB 计时与待执行关门意图', 'FAULT 连续 10 个状态轮次 → 黄灯', '主状态未在该提前返回分支重新发布'], 'fault');
  d.box(1150, 1165, 486, 170, '关门完成时 USB 未到位', ['完成分支可产生黄色闪烁诊断', 'USB 恢复到位时恢复绿灯', '这不屏蔽后续轮次的底层超时判断', '底层异常仍可能使主状态退回 IDLE'], 'check');
  d.section(44, 1430, '主状态与气缸状态是两套独立镜像');
  d.box(44, 1460, 766, 168, '气缸 1：门', ['OPENING / OPENED：开门输出，结合门上限位', 'CLOSING / CLOSED：关门输出，结合门下限位', '上下限位同时有效或双向输出同时有效 → ERR', '例：主状态 COMPLETE，门气缸仍可显示 OPENING'], 'info');
  d.box(850, 1460, 786, 168, '气缸 2：USB', ['CLOSE / CLOSED 对外表示插入；OPEN / OPENED 表示拔出', '插入到位：USB 上位 = 1 且下位 = 0；拔出到位反之', '流程到位检查还要求无双输出、无反向输出', '超时与冲突会影响 USB 气缸状态及流程告警'], 'info');
  d.section(44, 1700, '入口与周期条件');
  d.box(44, 1730, 766, 194, '控制模式', ['MIXED：物理门按钮与 SCPI 气缸动作均可用', 'LOCAL：拒绝 SCPI 气缸动作；REMOTE：禁用物理门动作', '电源按钮、急停，以及 SCPI 锁 / LED 有各自处理路径', '模式切换清普通气缸待执行命令与关门确认流程', '已生效的硬件输出不会因清命令槽而自动撤销'], 'info');
  d.box(850, 1730, 786, 194, '时间基准与实现细节', ['StateVector：25 ms 节拍；ModBus：每 2 个节拍，名义 50 ms', '门限位 / 门按钮：高电平连续 3 轮确认，低电平立即清零', 'READY 的 500 ms 从任意门按钮按下起算，到期检查双按', '门按钮全部松开满 200 ms 后释放下一次操作限制', '激光宏包含 IN_LASER1（气压位）；按源码记录，需结合接线解读'], 'check');
  d.save('01-state-machine.svg', '源码：Hardware/RamVector/Src/state_vector.c · App/Src/app_tasks.c · Hardware/RamVector/Inc/ram_vector.h');
}

{
  const d = new Diagram('02  门与 USB 自动动作时序', '正常自动周期；USB:AUTO 开启。动作框表示投递或输出观测，不能把“命令已发出”视为“机械到位”。', 2110);
  d.section(60, 164, 'A  关门：确认 → DUT → 插入 USB → 关门');
  d.section(880, 164, 'B  开门：开门到位 → 拔出 USB');
  const l = 70, r = 890, w = 610, lc = 375, rc = 1195;
  d.box(l, 200, w, 114, '关门请求', ['物理：IDLE 单按 200 ms → 黄灯回读进入 READY', 'READY 任意按钮计时满 500 ms，触发时须双按'], 'action');
  d.down(lc, 314, 362, '或：SCPI 门 CLOSE 意图，经执行器转入流程');
  d.box(l, 362, w, 110, '检查 DUT / 自动流程开关', ['USB:AUTO 与 DUT:AUTO 均开启才检查 DUT', 'DUT 未到位：记录事件、取消本次关门准备'], 'check');
  d.down(lc, 472, 522, 'DUT 条件通过');
  d.box(l, 522, w, 112, 'USB 是否需要插入？', ['AUTO OFF 或 USB 已插到位 → 直接进入 A5', '否则：无 USB 故障、无插入输出且气缸槽空闲'], 'check');
  d.down(lc, 634, 684, '需要插入');
  d.box(l, 684, w, 110, 'A3  投递插入；等待输出回读', ['VCMD_CYLINDER2_CLOSE → 执行 IO3=1 / IO4=0', '观测插入输出有效 → usb_move_start_tick = now'], 'action');
  d.down(lc, 794, 844, '从输出观测开始计时');
  d.box(l, 844, w, 112, 'A4  等待 USB 插入到位', ['USB 上位=1、下位=0；流程还检查反向输出', '未到位且 AUTO 开启：超过 2000 ms → F1'], 'check');
  d.down(lc, 956, 1006, '到位即继续；待关门时再次检查 DUT');
  d.box(l, 1006, w, 110, 'A5  投递门关门', ['VCMD_CYLINDER_CLOSE；设关门待生效保护标志', '该标志最多保持 1000 ms，防止 Idle 自动拔出干扰'], 'action');
  d.down(lc, 1116, 1166, '门关门输出上升沿回读');
  d.box(l, 1166, w, 112, 'A6  RUNNING：门下降', ['记录关门起始时间；等待下限位 / 风险条件', 'RUNNING 期间跳过 USB 插入的 2 秒超时分支'], 'state');
  d.down(lc, 1278, 1328, '满足关门完成条件');
  d.box(l, 1328, w, 138, 'A7  COMPLETE', ['正常：绿灯；记录 CLOSE_DONE、学习合格行程', '完成瞬间 USB 未插到位：可置黄色闪烁诊断', '后续仍受 USB 底层故障判断影响（参见 F1）'], 'state');

  d.box(r, 200, w, 114, '开门请求', ['物理：COMPLETE 且门在下限位', '任意门按钮确认 200 ms；或接收 SCPI 门 OPEN'], 'action');
  d.down(rc, 314, 362);
  d.box(r, 362, w, 110, 'B2  投递门开门', ['VCMD_CYLINDER_OPEN', '此时尚未启动自动 USB 拔出'], 'action');
  d.down(rc, 472, 522, 'ModBus 执行后，下一次 IO 回读');
  d.box(r, 522, w, 112, 'B3  门开门输出有效', ['观测输出上升沿，记录 door_open_start_tick', '门上升中，主状态仍为 COMPLETE'], 'action');
  d.down(rc, 634, 684, '持续观察门上限位');
  d.box(r, 684, w, 110, 'B4  开门到位 → IDLE', ['开门输出有效 + 消抖后的门上限位有效', '记录 OPEN_DONE，投递关灯，清周期计时'], 'state');
  d.down(rc, 794, 844, '后续状态轮次进入自动 USB 纠偏');
  d.box(r, 844, w, 140, 'B5  检查自动拔出条件', ['AUTO 开启、已解锁、无 USB 故障 / 急停', 'IDLE、门上限位有效、无关门准备意图', 'USB 未拔到位、未在拔出、气缸命令槽空闲'], 'check');
  d.down(rc, 984, 1034, '条件满足，直接投递；没有额外 2 秒等待');
  d.box(r, 1034, w, 110, 'B6  投递 USB 拔出', ['VCMD_CYLINDER2_OPEN → 执行 IO3=0 / IO4=1', '投递与实际写 IO、回读反馈分属不同步骤'], 'action');
  d.down(rc, 1144, 1194, '回读 USB 拔出输出有效');
  d.box(r, 1194, w, 112, 'B7  此处开始 2 秒超时计时', ['usb_move_start_tick = now（输出沿 / 计时补启）', '未拔到位且 elapsed > 2000 ms → F2'], 'check');
  d.down(rc, 1306, 1356, 'USB 下位=1 且上位=0');
  d.box(r, 1356, w, 110, 'B8  拔出到位', ['气缸 2 = OPENED；清动作计时与故障标志', '主状态保持 IDLE；无需等满 2 秒'], 'state');

  d.arrow([[680, 900], [795, 900], [795, 1508], [400, 1508], [400, 1550]], '异常', 700, 889);
  d.arrow([[1500, 1250], [1600, 1250], [1600, 1508], [1210, 1508], [1210, 1550]], '异常', 1518, 1239);
  d.box(44, 1550, 766, 194, 'F1  USB 插入异常', ['AUTO 开启时：超过 2 秒且非 RUNNING，或传感 / 输出冲突', '记录 USB_INSERT_FAIL；USB 气缸置 ERR', '门在上限位时投递安全优先级 USB 回退', '非急停 / 激光紧急时：退 IDLE、清关门意图、红灯快闪 3 次', '注意：COMPLETE 不在插入超时的豁免范围内'], 'fault');
  d.box(850, 1550, 786, 194, 'F2  USB 拔出异常', ['AUTO 开启时：超过 2 秒，或传感 / 输出冲突', '记录 USB_RETRACT_FAIL；USB 气缸置 ERR', '非急停 / 激光紧急时：退 IDLE、清关门意图、红灯快闪 3 次', '此失败分支没有自动反向重试或关闭拔出输出的命令', '到位观测可清故障；通信不可信则走独立恢复流程'], 'fault');
  d.section(44, 1810, '2 秒的准确位置');
  d.box(44, 1840, 1592, 148, '门开到位 → 条件检查 → 投递 USB 拔出 → 写 IO → 回读拔出输出 → 最多约 2 秒等待到位', ['USB_MOVE_TIMEOUT_MS = 2000；实际比较为 >，由 25 ms 状态轮次执行，反馈来自名义 50 ms IO 轮询。', 'USB 输出首次有效 / 换向时启动计时；有输出但计时为 0 时也会补启。它不是“开门后先延迟 2 秒再拔出”。', 'SCPI 直接控制气缸 2 走独立命令入口，不经过这里的自动开门到位条件编排。'], 'check');
  d.save('02-door-usb-sequence.svg', '源码：state_vector.c:627 / 691 / 720 / 775 / 876 / 1032 / 1093 / 1148 · cmd_exec.c:43 · BsmRelay.c:88');
}

{
  const d = new Diagram('03  动作投递、执行与 IO 更新', '控制闭环：输入观测 → 状态决策 → 命令仲裁 → RS485 写输出 → 机械运动 → IO 回读 → 状态确认。', 2130);
  d.box(44, 160, 480, 140, 'SysTimerCallback · 25 ms 节拍', ['每次置 StateVector 线程标志', '每 2 次置 ModBus 线程标志', '线程标志不是逐拍累积的任务队列'], 'neutral');
  d.box(594, 160, 480, 140, 'StateVectorTask', ['启动先 RamVector_Init(0)，等待 700 ms', '等待线程标志 → StateVector_Input()', '读取 IO 缓存；状态逻辑不直接访问 RS485'], 'state');
  d.box(1144, 160, 492, 140, 'ModBusTask', ['启动等待 500 ms，再等待线程标志', '名义 50 ms 一轮；通信耗时可能延长周期', '先读输入 / 输出，再取命令执行'], 'action');
  d.arrow([[524, 218], [594, 218]]);
  d.arrow([[280, 300], [280, 332], [1390, 332], [1390, 300]]);
  d.section(44, 395, 'A  StateVector_Input() 每轮顺序');
  d.section(900, 395, 'B  ModBusTask 每轮顺序');
  const l = 44, r = 900, w = 660, lc = 374, rc = 1230;
  const ys = [430, 610, 790, 970, 1150];
  d.box(l, ys[0], w, 130, '1  读 RAM IO 快照 / 处理控制模式切换', ['读取原始输入、输出、LED、链路状态与 TIM1 毫秒时钟', 'FAULT / RECOVERING：置气缸 ERR、清 USB 意图并提前返回', 'FAULT 连续 10 个状态轮次触发黄灯告警'], 'check');
  d.box(l, ys[1], w, 130, '2  输入消抖 / IO 自动纠偏', ['门上下限位、两个门按钮：连续 3 次高电平确认', '根据电源、门限位、LED 更新主状态；观测门输出沿计时', '依据输出与限位推导两个气缸状态'], 'state');
  d.box(l, ys[2], w, 130, '3  USB 监测 / 自动流程 / 完成判定', ['处理 USB 超时与冲突，满足条件时自动纠偏拔出', '处理 RUNNING 完成、COMPLETE 开门到位、紧急恢复', '这里只投递命令，执行发生在 ModBusTask'], 'state');
  d.box(l, ys[3], w, 130, '4  安全事件 → 普通事件', ['急停有效沿、激光窗口触发：投递安全优先级动作', '本轮无急停 / 激光紧急时处理电源按钮、SCPI 关门意图', '再处理物理门按钮、DUT / USB 编排与气压诊断'], 'action');
  d.box(l, ys[4], w, 130, '5  按钮释放 / 灯效 / 状态发布', ['推进按钮释放计时、黄色 / 红色闪烁、USB 失败退 IDLE', '记录状态日志；RamVector_SetState() 发布主状态', '更新心跳（当前 node_id=0，单机模式不递增）'], 'state');
  for (let i = 0; i < 4; i++) d.down(lc, ys[i] + 130, ys[i+1]);

  d.box(r, ys[0], w, 130, '1  RS485 读取输入，再读取输出', ['IO_Read(5, 2, in_buf)；IO_Read(5, 1, out_buf)', '读失败 → FAULT；全零帧 → RECOVERING', '首次启动 / 故障后需连续 10 组完全相同非零读数'], 'check');
  d.down(rc, 560, 610, '稳定恢复后才置 OK');
  d.box(r, ys[1], w, 130, '2  更新本机 IO 镜像', ['原始 IN / OUT + 门、锁、LED 等解码 + rs485_ok', 'RamVector_UpdateLocalIO() 在临界区更新快照', '保留 StateVector 写入的两个气缸状态字段'], 'state');
  d.down(rc, 740, 790);
  d.box(r, ys[2], w, 130, '3  CmdExec_ExecuteAll() 取命令', ['RamVector_TakeCmds() 原子快照并清空三个命令槽', '再检查链路：非 OK → 本批命令丢弃、直接返回', 'OK → 依次执行 Lock → Cylinder → LED'], 'action');
  d.down(rc, 920, 970);
  d.box(r, ys[3], w, 130, '4  将命令转换为实际 IO 写操作', ['CmdExec → Lock_Write / Cylinder_Write / LED_Write', '气缸换向：先关闭反向输出，再开启目标方向输出', '写失败记录 IO_WRITE_FAIL；此层不自动重新入槽'], 'action');
  d.down(rc, 1100, 1150);
  d.box(r, ys[4], w, 130, '5  硬件动作；下一轮重新回读', ['继电器 / 电磁阀 → 门或 USB 机械运动 → 限位变化', '本轮写操作不会立即刷新 RamVector 的输出镜像', '下一次读回输出和传感器后，状态机才观察到变化'], 'neutral');
  d.arrow([[1560, 1215], [1615, 1215], [1615, 495], [1560, 495]]);
  d.arrow([[900, 675], [772, 675], [772, 495], [704, 495]], 'IO 快照', 783, 653);
  d.section(44, 1360, 'C  命令入口与仲裁');
  d.box(44, 1395, 470, 196, '命令来源', ['状态机：按钮 / 自动流程 / 安全事件', 'SCPI：门、USB、锁、LED', '门 CLOSE → VCMD_DOOR_CLOSE_REQ', '执行器将该意图交回 StateVector', 'SCPI 门 OPEN / USB 动作直接入气缸槽'], 'action');
  d.box(574, 1395, 520, 196, 'RamVector：三个独立单槽', ['Lock  ·  Cylinder  ·  LED', '安全 2 > 用户 1 > 观测 0', '新命令优先级 ≥ 槽内优先级时覆盖', '同优先级后写覆盖先写；它不是 FIFO', '门与 USB 共用同一个 Cylinder 槽'], 'check');
  d.box(1154, 1395, 482, 196, '执行结果如何确认', ['命令已接收 ≠ 已写入 IO ≠ 已到位', '写 IO 结果用于失败日志', '回读输出沿用于动作计时 / 状态观测', '回读到位传感器用于确认机械完成', 'SCPI 查询读取 RAM 镜像'], 'state');
  d.arrow([[514, 1493], [574, 1493]]);
  d.arrow([[1094, 1493], [1154, 1493]]);
  d.arrow([[834, 1395], [834, 855], [900, 855]], '取槽', 845, 1020);
  d.section(44, 1670, 'D  非正常轮次与时间含义');
  d.box(44, 1705, 766, 252, '链路故障 / 恢复', ['FAULT：读失败，立即停止本轮状态逻辑；气缸镜像置 ERR', '故障告警计数按 25 ms 状态轮次增加，非 10 次独立 RS485 请求', 'RECOVERING：全零帧或尚未取得 10 组稳定非零 IO', '恢复阶段约需 0.5 秒量级；IO 一旦变化，稳定计数重新开始', '执行器在 FAULT / RECOVERING 中取走并丢弃命令', '恢复 OK 后继续依据 IO 纠偏；不会重放已丢弃命令', '这是命令软件处理规则，不能据此推断硬件输出已自动清零'], 'fault');
  d.box(850, 1705, 786, 252, '关键时间与观测边界', ['25 ms：逻辑节拍；50 ms：名义 IO 轮询周期', '200 / 500 ms：门按钮确认阈值；300 ms：电源按钮阈值', '1000 ms：电源切换冷却；关门待生效保护也使用 1000 ms', '2000 ms：USB 行程超时，从输出回读观测开始计算', '关门完成后 3000 ms 起，每 1500 ms 检测低气压，仅记录诊断', '3 次逻辑消抖采样可能读到同一帧 IO，不等同于 3 次独立回读', '实际响应还包含线程调度、读写通信与传感器消抖耗时'], 'check');
  d.save('03-action-update.svg', '源码：App/Src/app_tasks.c:59 / 162 / 175 · ram_vector.c:41 / 120 / 155 · cmd_exec.c:117 · state_vector.c:250');
}
