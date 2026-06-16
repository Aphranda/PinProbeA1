# PinProbeA1 Git 分支与发布计划

日期: 2026-06-13

## 当前基线

当前 `main` 已替换为单机稳定基线:

- 稳定 tag: `v0.9.0-standalone`
- 预发布 tag: `v0.9.0-standalone-rc.1`
- 维护分支: `release/pinprobea1-standalone-v0.9`
- 旧 main 备份: `archive/main-rough-20260613`

`main` 现在代表客户现场可用的单机稳定版本。后续多机开发不直接在 `main` 上进行。

## 分支职责

### main

稳定主线。

- 只接收已验证版本。
- 可直接用于构建现场固件。
- 每次进入 `main` 的重要版本都应打 tag。
- 不在 `main` 上做实验性开发。

### develop

下一代多机版本集成分支。

- 后续 CAN、多机 RamVector 同步、快照、W25Q128、节点寻址 SCPI 等功能先合入这里。
- `develop` 可以不稳定，但应保持可编译。
- 阶段性集成通过后，再从 `develop` 拉出 `release/*`。

### feature/*

具体功能分支。

建议命名:

- `feature/can-multinode`
- `feature/ramvector-sync`
- `feature/snapshot-w25q128`
- `feature/scpi-multinode`
- `feature/modbus-transaction`

规则:

- 从 `develop` 拉出。
- 功能完成后通过 `--no-ff` 合回 `develop`。
- 不直接合入 `main`。

### release/*

预发布/冻结分支。

建议命名:

- `release/pinprobea1-standalone-v0.9`
- `release/pinprobea1-multinode-v0.9`

规则:

- 从 `develop` 或稳定基线拉出。
- 拉出后冻结新功能，只接受 bugfix、构建修正、文档同步。
- 每个硬件验证批次打 `rc` tag。
- 验证通过后合入 `main` 并打正式 tag。

### hotfix/*

现场紧急修复分支。

规则:

- 从 `main` 拉出。
- 修复完成后合回 `main`。
- 同时合回 `develop`，避免多机版本丢失单机修复。
- 打补丁版本 tag。

示例:

```bash
git switch main
git switch -c hotfix/standalone-rs485-timeout

# 修复并提交
git commit -m "fix: improve standalone rs485 timeout handling"

git switch main
git merge --no-ff hotfix/standalone-rs485-timeout
git tag -a v0.9.1-standalone -m "PinProbeA1 standalone hotfix v0.9.1"

git switch develop
git merge --no-ff hotfix/standalone-rs485-timeout
```

### archive/*

历史封存分支。

- 只用于保留历史状态。
- 不参与日常开发。
- 不再从 archive 分支继续演进功能。

## 推荐版本命名

单机版本:

- `v0.9.0-standalone-rc.1`
- `v0.9.0-standalone`
- `v1.0.0-standalone`

多机版本:

- `v0.9.0-multinode-rc.1`
- `v0.9.0-multinode-rc.2`
- `v1.0.0-multinode`

补丁版本:

- `v0.9.1-standalone`
- `v1.0.1-multinode`

## 多机开发建议流程

建立多机集成分支:

```bash
git switch main
git switch -c develop
git push -u origin develop
```

开发 CAN 多机功能:

```bash
git switch develop
git switch -c feature/can-multinode
```

功能完成后合回:

```bash
git switch develop
git merge --no-ff feature/can-multinode
git push origin develop
```

准备多机预发布:

```bash
git switch develop
git switch -c release/pinprobea1-multinode-v0.9
git tag -a v0.9.0-multinode-rc.1 -m "PinProbeA1 multinode prerelease rc1"
```

多机验证通过后:

```bash
git switch main
git merge --no-ff release/pinprobea1-multinode-v0.9
git tag -a v1.0.0-multinode -m "PinProbeA1 multinode release"
git push origin main develop --tags
```

## 发布产物归档

每个 release tag 应保存对应产物:

- `.hex`
- `.elf`
- `.map`
- 构建日志
- 测试记录
- 对应 commit hash
- 对应 tag 名称

建议产物命名:

```text
PinProbeA1_standalone_v0.9.0.hex
PinProbeA1_standalone_v0.9.0.elf
PinProbeA1_standalone_v0.9.0.map

PinProbeA1_multinode_v0.9.0-rc.1.hex
PinProbeA1_multinode_v0.9.0-rc.1.elf
PinProbeA1_multinode_v0.9.0-rc.1.map
```

## 操作原则

1. `main` 永远代表可交付版本。
2. 多机新功能全部从 `develop` 或 `feature/*` 开始。
3. 单机现场 bug 从 `main` 开 `hotfix/*`，修完后回灌 `develop`。
4. `release/*` 只做收敛，不做大功能。
5. 每次发布必须打 tag，并保存构建产物和 map 文件。
