import { readFileSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const local = (name) => fileURLToPath(new URL(name, import.meta.url));
const reference = readFileSync(local('../PinProbe A1 箱体控制 SCPI 指令说明20260912.html'), 'utf8');
// Reuse the exact reference stylesheet and embedded brand asset.
const style = reference.match(/<style>([\s\S]*?)<\/style>/)?.[1];
const logo = reference.match(/<svg class="svg-defs"[\s\S]*?<\/svg>/)?.[0];
if (!style || !logo) throw new Error('Reference stylesheet or embedded GTS logo missing');
const esc = (s) => String(s).replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;');
let serial = 0;
class Flow {
  constructor(title, height) {
    this.id = `f${++serial}`;
    this.parts = [`<svg class="flow" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 704 ${height}" role="img" aria-labelledby="${this.id}-title"><title id="${this.id}-title">${esc(title)}</title><defs><marker id="${this.id}-arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M0 0L10 5L0 10Z" fill="#687381"/></marker></defs>`];
  }
  text(x, y, value, cls = 'f-body') { this.parts.push(`<text x="${x}" y="${y}" class="${cls}">${esc(value)}</text>`); }
  node(x, y, w, h, title, lines, kind = 'state') {
    this.parts.push(`<g data-node="${esc(title)}"><rect x="${x}" y="${y}" width="${w}" height="${h}" rx="3" class="f-node f-${kind}"/>`);
    this.text(x + 10, y + 20, title, 'f-title');
    lines.forEach((line, i) => this.text(x + 10, y + 39 + 16 * i, line));
    this.parts.push('</g>');
  }
  arrow(points, label = '', x = 0, y = 0) {
    this.parts.push(`<path d="M${points.map(p => p.join(' ')).join('L')}" class="f-edge" marker-end="url(#${this.id}-arrow)"/>`);
    if (label) this.text(x, y, label, 'f-label');
  }
  down(x, y1, y2, label = '') { this.arrow([[x,y1],[x,y2]], label, x+9, (y1+y2)/2+4); }
  finish(filename) {
    const svg = this.parts.join('\n') + '</svg>';
    const standalone = svg.replace('<defs>', `<style>${svgStyle}</style><defs>`);
    writeFileSync(local(filename), standalone, 'utf8');
    return svg;
  }
}
const svgStyle = `
.flow {display:block;width:100%;height:auto;margin:7px 0 8px;overflow:visible}
.flow text {font-family:"Microsoft YaHei","SimSun","DengXian",Arial,sans-serif;letter-spacing:0;fill:#1f2328}
.flow .f-title {font-size:13px;font-weight:700;fill:#1f3864}
.flow .f-body {font-size:11.4px}.flow .f-label {font-size:10.8px;fill:#5b6573;paint-order:stroke;stroke:#fff;stroke-width:4px;stroke-linejoin:round}
.flow .f-node {stroke-width:1.2}.flow .f-state {fill:#f5f8fc;stroke:#1f3864}
.flow .f-action {fill:#f7fbf5;stroke:#2e7d32;stroke-dasharray:4 2}
.flow .f-check {fill:#fffaf0;stroke:#b18a39}.flow .f-fault {fill:#fff5f5;stroke:#c1121f}
.flow .f-edge {fill:none;stroke:#687381;stroke-width:1.3}
`;
const table = (headers, rows) => `<table><thead><tr>${headers.map(s=>`<th>${s}</th>`).join('')}</tr></thead><tbody>${rows.map(row=>`<tr>${row.map(s=>`<td>${s}</td>`).join('')}</tr>`).join('')}</tbody></table>`;
const section = (title) => `<div class="section-title">${title}</div>`;
const pages = [];

{
  const f = new Flow('主状态机：正常循环与安全分支', 450);
  f.node(0, 12, 190, 88, 'INIT · 6', ['上电初始状态', '首次有效 IO 处理轮次', '立即进入 LOCK']);
  f.node(240, 12, 190, 88, 'LOCK · 0', ['锁定；观察电源输出', '电源按钮按住 300 ms', '可投递锁定 / 解锁动作']);
  f.node(480, 12, 190, 88, 'IDLE · 1', ['空闲；观察门位置', '任意门按钮满 200 ms', '投递黄灯，等待回读']);
  f.arrow([[190,56],[240,56]], '初始', 199,45);
  f.arrow([[430,56],[480,56]], '已解锁', 435,45);
  f.down(575,100,174);
  f.text(438,139,'黄灯 + 门上位 / 位置确认','f-label');
  f.node(480, 174, 190, 104, 'READY · 2', ['门按钮确认条件满足', '检查 DUT；AUTO 开启时', 'USB 插入到位后投递关门', '黄灯清除 → 返回 IDLE']);
  f.node(240, 174, 190, 104, 'RUNNING · 3', ['关门输出有效，门未到底', '记录关门起点；观察限位', '正常下限位 / 风险条件', '满足即进入 COMPLETE']);
  f.node(0, 174, 190, 104, 'COMPLETE · 5', ['关门完成，正常亮绿灯', '单按钮满 200 ms 投递开门', '开门期间仍保持此主状态', '门开到位后返回 IDLE']);
  f.arrow([[480,226],[430,226]], '关门', 439,215);
  f.arrow([[240,226],[190,226]], '完成', 199,215);
  f.arrow([[95,278],[95,320],[694,320],[694,56],[670,56]], '开门输出 + 上限位 → IDLE；投递关灯，随后检查自动 USB 拔出', 106,310);
  f.node(0, 355, 190, 80, '全局安全触发', ['急停有效沿，或关门中', '进入激光窗口后检测触发'], 'fault');
  f.node(240, 355, 190, 80, 'EMERGENCY · 4', ['投递红灯 + 锁定 + 开门', '急停时已开到位则不发开门'], 'fault');
  f.node(480, 355, 190, 80, '恢复 → LOCK', ['急停无效且激光宏为 0', '随后仍按实际 IO 纠偏']);
  f.arrow([[190,395],[240,395]]);
  f.arrow([[430,395],[480,395]]);
  const svg = f.finish('a4-01-state-machine.svg');
  pages.push({title:'主状态机与异常流转',body:section('01 / 主状态与正常运行循环')+
    '<p class="legend">蓝框：主状态　绿虚框：动作　黄框：条件 / 计时　红框：异常；动作完成以 IO 回读为准。</p>'+svg+
    section('状态纠偏与异常条件')+table(['条件 / 分支','当前实现'],[
      ['电源与门位置','非紧急状态下电源输出为 0 → LOCK；解锁 → IDLE。IDLE 位置分支可按下限位进入 COMPLETE。'],
      ['关门输出沿','IDLE / READY / COMPLETE 观测到关门输出上升沿且门未到底，可直接进入 RUNNING。'],
      ['关门完成 / 激光窗口','正常：关门输出 + 下限位。风险：RISK 开启、关门输出、气压位为 1、耗时超过预计时间。预计时间初值 2500 ms；激光窗口在其 2/3 之后。'],
      ['USB 底层异常','AUTO 开启时超时 / 冲突可记失败、退 IDLE、清关门意图并红灯快闪 3 次；插入失败且门开到位时投递回退。'],
      ['通信不可信','本轮逻辑提前返回，两个气缸镜像置 ERR；连续 FAULT 告警。此分支不重新发布主状态，详见第 3 页。'],
    ])+section('主状态与动作状态分开观察')+table(['对象','状态含义'],[
      ['门 / USB 气缸','分别维护 OPENING、OPENED、CLOSING、CLOSED、ERR。USB 的 CLOSE 表示插入，OPEN 表示拔出。'],
      ['控制模式','MIXED：物理门动作和 SCPI 气缸动作；LOCAL：拒绝 SCPI 气缸动作；REMOTE：禁用物理门动作。电源按钮、锁、LED 和安全事件有独立处理路径。'],
    ])+'<div class="note-box">实现边界：激光宏包含气压位 IN_LASER1；需结合接线解读。当前没有通用“关门超时即故障”分支。图按源码实际条件绘制。</div>',source:'state_vector.c:491 / 627 / 796 / 910 / 1234 · ram_vector.h'});
}

{
  const f = new Flow('关门与开门的 USB 自动动作时序', 596);
  f.text(2,15,'A  关门流程', 'f-title');f.text(370,15,'B  开门与 USB 拔出', 'f-title');
  const left = [
    ['关门请求',['物理：单按 200 ms 进入 READY，再确认关门','或 SCPI 门 CLOSE 意图，交状态机编排'],'action'],
    ['DUT / USB 自动开关检查',['USB:AUTO 与 DUT:AUTO 均开启才检查 DUT','DUT 未到位取消本次；AUTO OFF 直达 A5'],'check'],
    ['A3  投递 USB 插入',['USB 已插到位可直达 A5；否则等气缸槽空闲','投递 CYLINDER2_CLOSE，经执行器写输出'],'action'],
    ['A4  回读插入输出 → 开始 2 秒计时',['超 2 秒未到位 → 故障（RUNNING 豁免）','USB 上位 = 1 且下位 = 0，到位即继续'],'check'],
    ['A5  条件通过 → 投递门关门',['再次检查 DUT；流程到位检查还排除反向输出','USB 自动关闭时，可直接投递门关门'],'action'],
    ['A6  关门输出沿 → RUNNING',['门未关到底时记录关门起始时间','等待下限位或风险模式完成条件'],'state'],
    ['A7  COMPLETE',['正常投递绿灯，记录 CLOSE_DONE / 行程学习','完成时 USB 未到位可产生黄色闪烁诊断'],'state'],
  ];
  const right = [
    ['开门请求 → 投递门开门',['COMPLETE 且门关到位：单按钮确认 200 ms','或 SCPI 门 OPEN → CYLINDER_OPEN'],'action'],
    ['B2  回读开门输出沿',['记录 door_open_start_tick；门上升','主状态仍为 COMPLETE，此时不自动拔出 USB'],'state'],
    ['B3  门开到位 → IDLE',['开门输出有效 + 消抖后的门上限位有效','投递关灯、记录 OPEN_DONE、清周期计时'],'state'],
    ['B4  自动拔出条件满足',['已解锁、AUTO 开启、无 USB 故障 / 急停','IDLE、门开到位、无关门准备意图、命令槽空闲'],'check'],
    ['B5  投递 USB 拔出',['USB 未拔到位且未在拔出；满足条件直接投递','CYLINDER2_OPEN → IO3=0 / IO4=1'],'action'],
    ['B6  回读拔出输出 → 此处开始 2 秒',['usb_move_start_tick = now','超过 2000 ms 仍未到位 → USB 拔出故障'],'check'],
    ['B7  USB 拔出到位',['USB 下位 = 1 且上位 = 0 → 气缸 OPENED','清计时 / 故障；主状态 IDLE，无须等满 2 秒'],'state'],
  ];
  for(let i=0;i<7;i++) {
    const y=32+i*80;
    f.node(0,y,326,64,left[i][0],left[i][1],left[i][2]);
    f.node(370,y,326,64,right[i][0],right[i][1],right[i][2]);
    if(i<6){f.down(163,y+64,y+80);f.down(533,y+64,y+80);}
  }
  const svg=f.finish('a4-02-door-usb.svg');
  pages.push({title:'门与 USB 自动动作时序',body:section('02 / 正常周期：先插入再关门，开到位后拔出')+svg+
    '<div class="note-box"><strong>2 秒的位置：</strong>开门到位 → 条件检查 → 投递拔出 → 写 IO → <strong>回读拔出输出时开始计时</strong>。当前没有“开门后固定等待 2 秒再拔出”的延时。</div>'+
    table(['关键条件','当前实现'],[
      ['关门确认 500 ms','READY 中从任意门按钮按下起算，到期检查双按钮同时有效；不是连续双按 500 ms。门按钮全部释放 200 ms 后才解除下一次操作限制。'],
      ['USB 2 秒超时','AUTO 开启才执行；插入超时豁免 RUNNING，拔出不设该豁免。到位优先结束等待；超时判断是 elapsed > 2000 ms。'],
      ['异常与 COMPLETE','完成时的 USB 黄闪不屏蔽后续底层超时，仍可能退 IDLE。拔出失败分支没有自动反向重试或关闭拔出输出命令。'],
      ['自动与直接命令','SCPI 直接操作 USB 不经过自动开门到位编排。自动回退在插入输出仍有效时还要求插入已到位；关门后待生效保护标志最多保持 1000 ms。'],
    ]),source:'state_vector.c:627 / 691 / 720 / 775 / 876 / 1032 / 1093 · cmd_exec.c:43'});
}

{
  const f=new Flow('状态决策与 ModBus IO 更新闭环',536);
  f.text(0,15,'StateVectorTask · 名义 25 ms','f-title');f.text(370,15,'ModBusTask · 名义 50 ms','f-title');
  const left=[
    ['1  读取 RamVector IO 快照',['读取输入 / 输出、LED、链路与毫秒时钟','FAULT / RECOVERING → 本轮提前返回'],'check'],
    ['2  消抖 / 状态纠偏 / 气缸观测',['门限位和门按钮高电平连续 3 轮确认','根据电源、限位、LED、输出沿更新状态'],'state'],
    ['3  USB 监测 / 自动动作 / 完成判定',['检查 USB 超时与冲突，编排自动拔出','关门完成、开门到位、紧急恢复判断'],'state'],
    ['4  安全事件 → 普通事件',['先处理急停有效沿与激光；条件允许时','处理电源、SCPI 关门意图、门按钮 / DUT'],'action'],
    ['5  释放计时 / 灯效 / 状态发布',['USB 故障退 IDLE、推进红 / 黄闪烁','记录日志 → SetState 发布主状态'],'state'],
  ];
  const right=[
    ['1  先读输入，再读输出',['IO_Read(5, 2) → IO_Read(5, 1)','失败 → FAULT；全零 / 未稳定 → RECOVERING'],'check'],
    ['2  发布 IO 镜像',['UpdateLocalIO 更新原始 IO 和解码结果','保留状态机维护的两个气缸状态字段'],'state'],
    ['3  原子取出并清空命令槽',['TakeCmds 取快照 → 再检查链路','非 OK：丢弃；OK：Lock → Cylinder → LED'],'action'],
    ['4  将命令转换成 RS485 写操作',['CmdExec → BsmRelay → WriteIO','气缸先关反向再开目标；失败记日志'],'action'],
    ['5  机械动作 / 等下一轮回读',['继电器、电磁阀 → 门 / USB 移动 → 限位变化','写输出不立即刷新 RAM；下轮读回再确认'],'action'],
  ];
  for(let i=0;i<5;i++){
    const y=32+i*80;
    f.node(0,y,326,64,left[i][0],left[i][1],left[i][2]);
    f.node(370,y,326,64,right[i][0],right[i][1],right[i][2]);
    if(i<4){f.down(163,y+64,y+80);f.down(533,y+64,y+80);}
  }
  f.arrow([[370,144],[348,144],[348,64],[326,64]]);
  f.text(329,105,'IO','f-label');
  f.node(0,462,696,64,'命令来源 → 三个独立单槽 → 执行器',['状态机按钮 / 自动 / 安全动作；SCPI 命令 → Lock、Cylinder、LED 槽；门与 USB 共用 Cylinder 槽。','安全 2 > 用户 1 > 观测 0；新命令优先级 ≥ 槽内时覆盖，同优先级后写覆盖先写，不是 FIFO 队列。'],'check');
  f.arrow([[163,416],[163,462]]);
  f.arrow([[696,490],[703,490],[703,224],[696,224]]);
  const svg=f.finish('a4-03-action-update.svg');
  pages.push({title:'动作执行与 IO 更新闭环',body:section('03 / 决策、投递、写输出、读反馈的异步链路')+svg+
    section('时序、恢复与执行边界')+table(['项目','当前实现'],[
      ['任务启动 / 节拍','ModBus 启动等待 500 ms；StateVector 初始化向量表后等待 700 ms。SysTimer 每 25 ms 唤醒状态任务，每 2 次唤醒 ModBus。线程标志不累积完整逐拍队列。'],
      ['IO 故障 / 恢复','读失败后进入 FAULT；恢复需连续 10 组完全相同的非零 IO。全零帧或尚未稳定时为 RECOVERING，输入输出变化会重计。'],
      ['告警与丢弃','FAULT 连续 10 个状态轮次触发黄灯，不是 10 次独立 RS485 请求。FAULT / RECOVERING 中执行器取走并丢弃命令，恢复后不重放。'],
      ['命令到位确认','命令接收不等于硬件到位；输出回读沿用于计时，传感器回读用于确认完成。SCPI 门 CLOSE 先转关门意图，再由状态机编排 DUT / USB。'],
      ['时间与诊断','名义 25 / 50 ms 之外还有通信、调度和消抖耗时。3 轮消抖可能读取同一 IO 帧。关门完成 3 s 后每 1.5 s 检测低气压，仅记录诊断。'],
    ])+'<div class="note-box">模式切换清普通气缸待执行命令及关门确认流程，保留安全命令。清命令槽或暂停状态逻辑不代表硬件输出已关闭。当前为 node_id=0 本机流程，未将预留 CAN 同步画成已接入路径。</div>',source:'App/Src/app_tasks.c:59 / 162 / 175 · ram_vector.c:41 / 120 / 155 · cmd_exec.c:117'});
}

const html=`<!DOCTYPE html>
<html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>PinProbe A1 状态机与动作控制流程图</title>
<style>${style}\n${svgStyle}
.legend {font-size:10px;color:#5b6573}
.source {font-size:8.6px;color:#687381;margin-top:7px;overflow-wrap:anywhere}
.main {min-height:0}.page-header,.page-footer {flex-shrink:0}
@media print {.page:last-child {break-after:auto;page-break-after:auto}}
</style></head><body>${logo}
${pages.map((p,i)=>`<div class="page" id="page-${i+1}"><div class="page-header"><div><h1>${p.title}</h1><div class="subtitle">PinProbe A1 状态机与动作控制流程图</div></div><svg class="logo" viewBox="4400 6500 18750 3520" role="img" aria-label="GTS"><use href="#gts-logo"></use></svg><div class="doc-meta"><strong>2026-09-15</strong>Control Flow<br>PinProbe A1</div></div><div class="main">${p.body}<p class="source">源码定位：${p.source}<br>依据当前工作区实现整理；时间为软件阈值，未替代实机验证。</p></div><div class="page-footer"><span>Accurate · Simple · Fast · Agile | General Test Systems Inc.</span><span>PinProbe A1 控制流程 | ${i+1} / 3</span></div></div>`).join('\n')}
</body></html>`;
writeFileSync(local('../PinProbe A1 状态机与动作控制流程图20260915.html'),html,'utf8');
